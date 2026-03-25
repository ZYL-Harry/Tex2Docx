* 第三版：在第二版基于C++实现Tex文件到Docx文件的基础转换和基于Python实现网络端应用的基础上，基于LangChain框架构建智能体，并通过Ollama模型实现关于Docx文件的校对和优化
  * 未完成功能：表格的尺寸控制，伪代码
  * 进一步：关于Ollama模型在文章校对优化上的训练增强
* 项目结构：
  ```text
  Tex2Docx/
  ├── src/    // 核心源码目录
  │   ├── cpp/    // C++源码子目录
  │   │   ├── convert2exe.cpp          // 生成exe可执行文件
  │   │   ├── main.cpp                 // 测试主函数
  │   │   ├── document_ast.h           // 抽象语法树
  │   │   ├── parse.cpp                // tex文件解析
  │   │   ├── parse.h
  │   │   ├── parse_template.cpp       // tex文章格式解析
  │   │   ├── parse_reference.cpp      // bib参考文献解析
  │   │   ├── tex_attribute.h          // tex文本基本属性
  │   │   ├── generator.cpp            // docx文件生成
  │   │   ├── generator.h
  │   │   ├── document_docx.cpp        // docx文件生成辅助
  │   │   ├── document_docx.h
  │   │   ├── utils_write_xml.h
  │   │   └── 其他依赖
  ├── bin/    // 可执行文件目录
  │   ├── main.py                  // Python Web后端代码
  │   ├── Project1.exe             // VS2022编译后的C++可执行文件
  │   └── pdfium.dll
  │   ├── tool.py                  // 【新增】智能体模型推理辅助
  ├── data/    // 数据目录
  │   ├── default/    // 默认资源子目录
  │   │   ├── document.tex    // 默认tex文件
  │   │   ├── refs.bib        // 默认参考文献bib文件
  │   │   ├── article.cls     // 默认格式cls文件
  │   │   └── figures.zip     // 默认图片压缩包
  │   ├── uploads/    // 上传文件临时存储目录
  │   │   └── temp_xxx/    // 自动生成的临时目录（含上传文件和解压后的图片）
  │   └── outputs/    // 转换生成的docx文件目录
  └── introduction.txt     // 项目思路介绍
* 主要源文件说明: 
  * main.cpp：文件转换的测试主函数，采用默认的测试文件
  * document_ast.h：基于tex语法结构的抽象语法树，封装文档中元素的结构化表示，包括标题、文段、公式、图像、表格等
  * parse.cpp：Tex文件解析器，基于字符串的提取，将文章结构、内容及属性解析并存储于抽象语法树
      * 主要头文件：parse.h
  * parse_template.cpp：cls文件解析器（以article.cls为例），基于字符串的提取，提取文章格式属性并存储于抽象语法树
  * parse_reference.cpp：bib文件解析器，基于字符串的提取，提取参考文献的关键元素并存储于抽象语法树
  * generator.cpp：Docx文件生成器，根据抽象语法树内容生成Docx文件
      * 主要头文件：generator.h
  * document_docx.cpp：基于minidocx进行Docx文件相关功能的拓展，包括公式、图像、表格等
      * 主要头文件：document_docx.h, utils_write_xml.h
  * tex_attribute.h：tex文件中基本元素的属性，共解析与写入配置使用，包含字号、段落、特殊符号等
  * convert2exe.cpp：用于编译生成可执行文件
  * main.py：部署后使用的基于Flask的Web应用文件，用于处理文件上传、调用C++引擎Project1.exe进行TeX到DOCX的转换、以及文件下载、校对优化和智能聊天功能
  * tool.py：用于校对、优化的智能体推理辅助函数，主要包括`get_polish_suggestions`函数
* 主要库：
  * minidocx: 生成Docx文件的基本元素，包括设置对齐、段落、字体、大小、颜色、图像、表格
  * pdfium: 读取PDF图片文件，辅助实现图片由PDF到高清PNG的转换
* introduction.txt：实现的具体思路与流程
* 项目配置属性：
  * 开发平台：Visual Studio 2022
  * 语言：ISO C++20 标准 (/std:c++20)
  * 配置-Release，平台-x64
  * 附加包含目录：
    * ...\minidocx-next\minidocx-next\include\minidocx
    * ...\pugixml-master\src
    * ...\pdfium\include\public
  * 附加库目录：
    * ...\minidocx-next\minidocx-next\build\bin\lib\Release
    * ...\pdfium\include\public
  * 附加依赖项：
    * minidocx.lib
    * pdfium.lib
  * Web部署及功能依赖的主要第三方库：详见`requirements.txt`
* 项目部署：
    * 编译convert2exe.cpp生成项目的exe可执行文件
    * 将后端文件main.py、可执行文件、依赖的动态链接库pdfium.dll、tool.py放置同一目录下
    * 运行main.py即可获得服务监听的网络端口
