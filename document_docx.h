#ifndef DOCUMENT_DOCX
#define DOCUMENT_DOCX

#include<string>
#include"minidocx.hpp"
#include"tex_attribute.h"
#include"pugixml.hpp"
#include<iostream>


// 处理公式上下标的二叉树
struct FormulaNode {
    std::string data;
    std::shared_ptr<FormulaNode> sup;  // 上标
    std::shared_ptr<FormulaNode> sub;  // 下标
};


class DocxDocument : public md::Document 
{
public:
    DocxDocument() {};
    ~DocxDocument() = default;

    void saveAs(const std::string& filename);
    void flush();
    
    void writeOfficeDocument();
    static pugi::xml_node writeBlock(pugi::xml_node w_body, md::Block& block);
    static pugi::xml_node writeParagraph(pugi::xml_node w_body, md::Paragraph& para);
    static void writeRun(pugi::xml_node w_p, md::Run& run);
    static void writeRichText(pugi::xml_node w_p, const md::RichText& rich);
    static void latex2omml(pugi::xml_node m_r, const std::string& latex_math);
    
    static void writeFormula(pugi::xml_node m_r, const std::string& str, const size_t len, const bool whitespace);
    static void addCharacter(pugi::xml_node m_oMath, const std::string& character);
    static void addMathCommand(pugi::xml_node parent_node, const std::string& symbol);
    static void addMathbb(pugi::xml_node parent_node, const std::string& arg);

    static size_t writeComplexFormula(pugi::xml_node m_r, size_t p, const std::string& formula_latex, std::shared_ptr<FormulaNode>& formula_tree);
    static size_t processBracketComplexFormula(pugi::xml_node m_r, std::string& formula_latex_complex);
    static size_t processFracComplexFormula(pugi::xml_node m_r, const std::string& formula_latex_complex);
    static std::shared_ptr<FormulaNode> searchSubTree(const std::string& formula_latex, std::shared_ptr<FormulaNode>& formula_tree);
    static void writeTree(pugi::xml_node m_r, const std::shared_ptr<FormulaNode>& formula_tree);

    static pugi::xml_node writeTable(pugi::xml_node w_body, const md::Table& tbl);
    static void writePicture(pugi::xml_node w_p, const md::Picture& pict);

private:
    md::PartName mainPart_{ MAIN_PART };
};

inline std::vector<std::string>
splitSpecialCharacter(std::string& input_string);

size_t
find_supsub_item_end(const std::string& template_content, size_t& p_start);

inline std::tuple<size_t, std::vector<std::string>>
splitBiggestSupSub(const std::string& text);

inline bool
isComplexBracketBalanced(const std::string& formula, const std::unordered_map<std::string, std::string>& BracketPairs);


inline md::Alignment
mapping_alignStyle_docx(const std::string& align_style_string)
{
	if (align_style_string == CENTER)	return md::Alignment::Centered;
	if (align_style_string == LEFT)	return md::Alignment::Left;
	if (align_style_string == RIGHT)	return md::Alignment::Right;
	if (align_style_string == JUSTIFYING)	return md::Alignment::Justified;
}



inline void 
printNode(const pugi::xml_node& node, int indent = 0)
{
    // 打印缩进
    for (int i = 0; i < indent; ++i) std::cout << "  ";

    // 打印节点名
    std::cout << "<" << node.name();

    // 打印属性
    for (auto& attr : node.attributes())
    {
        std::cout << " " << attr.name() << "=\"" << attr.value() << "\"";
    }
    std::cout << ">";

    // 打印文本
    if (node.text())
    {
        std::cout << node.text().get();
    }

    std::cout << std::endl;

    // 递归子节点
    for (auto& child : node.children())
    {
        printNode(child, indent + 1);
    }

    // 结束标签
    for (int i = 0; i < indent; ++i) std::cout << "  ";
    std::cout << "</" << node.name() << ">" << std::endl;
}


#endif