#include <iostream>
#include <fstream>
#include <string>
#include "document_ast.h"
#include "parser.h"
#include "generator.h"

// 核心转换函数：接收TEX路径、BIB路径、图片目录、输出DOCX路径
bool convertFile(
    const std::string& tex_path,    // 输入TEX文件路径
    const std::string& ref_path,    // BIB引用文件路径
    const std::string& figure_dir,  // 图片目录路径
    const std::string& cls_path,    // 格式文件路径
    const std::string& outputPath   // 输出DOCX文件路径
) {
    // ========== 第一步：前置检查（避免无效执行） ==========
    // 检查TEX文件是否存在
    std::ifstream tex_file(tex_path);
    if (!tex_file.is_open()) {
        std::cerr << "错误：无法打开TEX文件：" << tex_path << std::endl;
        return false;
    }
    tex_file.close(); // 检查完就关闭，交给你的解析器处理

    // 检查BIB文件是否存在（可选，但建议加）
    std::ifstream ref_file(ref_path);
    if (!ref_file.is_open()) {
        std::cerr << "错误：无法打开BIB引用文件：" << ref_path << std::endl;
        return false;
    }
    ref_file.close();

    try {
        // ========== 第二步：调用你的核心转换算法 ==========
        // 初始化解析器
        TexParser tex_parser(tex_path, ref_path, figure_dir, cls_path);

        // 执行解析流程（你的原有逻辑）
        tex_parser.parse_reference();    // 解析BIB引用
        tex_parser.read_tex_file();      // 读取TEX文件
        AST::Document& document_ast = tex_parser.parse();  // 生成AST

        // 生成DOCX文件
        DocxGenerator docx_generator(outputPath, document_ast);
        docx_generator.generate();

        // 生成示例图（如果需要保留）
        // generate_figure_example();

        std::cout << "提示：DOCX文件生成成功！" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        // 捕获所有异常，避免程序崩溃
        std::cerr << "错误：转换失败，原因：" << e.what() << std::endl;
        return false;
    }
    catch (...) {
        // 捕获未知异常
        std::cerr << "错误：转换失败，发生未知错误！" << std::endl;
        return false;
    }
}

// 主函数：接收命令行参数
int main(int argc, char* argv[]) {
    // ========== 第三步：校验命令行参数 ==========
    // 参数说明：程序名 + tex_path + ref_path + figure_dir + cls_path + outputPath = 5个参数
    if (argc != 6) {
        std::cerr << "使用方法：" << std::endl;
        std::cerr << "Windows: convert_tool.exe <TEX文件路径> <BIB文件路径> <图片目录> <CLS文件路径> <输出DOCX路径>" << std::endl;
        std::cerr << "Linux/Mac: ./convert_tool <TEX文件路径> <BIB文件路径> <图片目录> <CLS文件路径> <输出DOCX路径>" << std::endl;
        return 1; // 返回非0表示执行失败
    }

    // 提取命令行参数
    std::string tex_path = argv[1];
    std::string ref_path = argv[2];
    std::string figure_dir = argv[3];
    std::string cls_path = argv[4];
    std::string outputPath = argv[5];

    // ========== 第四步：执行转换并返回结果 ==========
    if (convertFile(tex_path, ref_path, figure_dir, cls_path, outputPath)) {
        std::cout << "转换成功！输出文件：" << outputPath << std::endl;
        return 0; // 返回0表示执行成功
    }
    else {
        std::cerr << "转换失败！" << std::endl;
        return 1;
    }
}