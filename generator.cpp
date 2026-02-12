#include"generator.h"
#include"document_ast.h"
#include<stdexcept>
#include <sstream>
#include <iostream>
#include <filesystem>
#include"minidocx.hpp"
#include"document_docx.h"
#include "word/main/properties/paragraph.hpp"
#include<stack>
#include<regex>
#include"parser.h"


/* 拷贝构造 */
DocxGenerator::DocxGenerator(const DocxGenerator& word_generator) {}


/* 拷贝赋值 */
DocxGenerator&
DocxGenerator::operator=(const DocxGenerator& str) {}


/* 析构函数 */
DocxGenerator::~DocxGenerator() {}


/* 函数：word文件生成 */
void
DocxGenerator::generate()
{
    // 添加文章页面格式
    this->generate_pagestyle();
    // 写入文章基本信息
    this->generate_information();
    // 写入文章摘要
    this->generate_abstract();
    // 写入文章关键词
    this->generate_keywords();
    // 写入文章正文
    this->generate_body();
    // 写入补充信息
    this->generate_supplement();
    // 写入参考文献
    this->generate_reference();
    // 保存docx文件
    this->document_docx.saveAs(this->docx_path);
    std::cout << "Successfully create docx file." << std::endl;
}


/* 函数：添加文章页面格式 */
void 
DocxGenerator::generate_pagestyle()
{
    // 编号
    md::AbstractNumberingDefinition absNumDef;
    absNumDef.type_ = md::NumberingType::HybridMultiLevel;
    // 统计编号字号
    std::vector<int> fontsizes;
    int fontsize_normal;
    for (AST::BlockElement& ast_content : this->document_ast.content)
    {
        if (auto sectionPtr = std::get_if<std::shared_ptr<AST::Header>>(&ast_content))
        {
            std::shared_ptr<AST::Header> section = *sectionPtr;
            std::shared_ptr<AST::Textbold> section_text = std::get<std::shared_ptr<AST::Textbold>>(section->content[0]);
            if (fontsizes.size() == 0 || std::count(fontsizes.begin(), fontsizes.end(), section_text->fontsize) == 0)
            {
                fontsizes.push_back(section_text->fontsize);
            }
        }
        if (auto paragraphPtr = std::get_if<std::shared_ptr<AST::Paragraph>>(&ast_content))
        {
            std::shared_ptr<AST::Paragraph> paragraph = *paragraphPtr;
            if (paragraph->content.size() == 1)
            {
                std::shared_ptr<AST::Text> paragraph_text = std::get<std::shared_ptr<AST::Text>>(paragraph->content[0]);
                fontsize_normal = paragraph_text->fontsize;
            }
        }
    }
    std::sort(fontsizes.begin(), fontsizes.end(), std::greater<int>());
    int i;
    for (i = 0; i < fontsizes.size(); i++)
    {
        // 第i级编号
        absNumDef.levels_[i].numStart_ = 1;
        absNumDef.levels_[i].numStyle_ = md::NumberStyle::Decimal;
        std::string fmt = "%1";
        for (int j = 2; j <= i+1; j++)
        {
            fmt += ".%" + std::to_string(j);
        }
        absNumDef.levels_[i].numFmt_ = fmt;
        absNumDef.levels_[i].numAlign_ = md::Alignment::Left;
        absNumDef.levels_[i].fontSize_ = fontsizes[i];
        absNumDef.levels_[i].font_ = md::RichTextProperties::Font();
        absNumDef.levels_[i].font_->ascii_ = "Times New Roman";
        absNumDef.levels_[i].fontStyle_.bold_ = true;
    }
    this->num_header = fontsizes.size();
    // 小编号
    absNumDef.levels_[this->num_header].numStart_ = 1;
    absNumDef.levels_[this->num_header].numStyle_ = md::NumberStyle::Decimal;
    absNumDef.levels_[this->num_header].numFmt_ = "(%" + std::to_string(this->num_header + 1) + ")";
    absNumDef.levels_[this->num_header].numAlign_ = md::Alignment::Left;
    absNumDef.levels_[this->num_header].fontSize_ = fontsize_normal;
    absNumDef.levels_[this->num_header].font_ = md::RichTextProperties::Font();
    absNumDef.levels_[this->num_header].font_->ascii_ = "Times New Roman";
    absNumDef.levels_[this->num_header].fontStyle_.bold_ = true;
    // bullet编号
    absNumDef.levels_[this->num_header + 1].numStart_ = 1;
    absNumDef.levels_[this->num_header + 1].numStyle_ = md::NumberStyle::Bullet;
    absNumDef.levels_[this->num_header + 1].numFmt_ = md::CHAR_SOLID_CIRCLE;
    absNumDef.levels_[this->num_header + 1].numAlign_ = md::Alignment::Left;
    absNumDef.levels_[this->num_header + 1].fontSize_ = fontsize_normal;
    // 参考文献编号
    absNumDef.levels_[this->num_header + 2].numStart_ = 1;
    absNumDef.levels_[this->num_header + 2].numStyle_ = md::NumberStyle::Decimal;
    absNumDef.levels_[this->num_header + 2].numFmt_ = "[%" + std::to_string(this->num_header + 2 + 1) + "]";
    absNumDef.levels_[this->num_header + 2].numAlign_ = md::Alignment::Left;
    absNumDef.levels_[this->num_header + 2].fontSize_ = fontsize_normal;
    absNumDef.levels_[this->num_header + 2].font_ = md::RichTextProperties::Font();
    absNumDef.levels_[this->num_header + 2].font_->ascii_ = "Times New Roman";
    // 添加抽象编号定义到文档
    md::NumberingId absNumId = this->document_docx.addAbstractNumDefinition(absNumDef);
    // 创建具体编号实例并添加到文档
    md::NumberingDefinition numDef(absNumId); // 使用抽象编号ID
    this->headerNumId = this->document_docx.addNumDefinition(numDef);
    this->listNumId = this->document_docx.addNumDefinition(numDef);
    this->refNumId = this->document_docx.addNumDefinition(numDef);
    // 创建section
    this->section = this->document_docx.addSection();

    // 读取并写入文章基本信息格式
    std::string keys[] = { "top", "bottom", "left", "right", "headsep", "footskip", "marginparsep" };
    for (AST::BlockElement& ast_content : this->document_ast.content)
    {
        if (auto Ptr = std::get_if<std::shared_ptr<AST::Information>>(&ast_content))
        {
            break;
        }
        if (auto attributePtr = std::get_if<std::shared_ptr<AST::Attribute>>(&ast_content))
        {
            std::shared_ptr<AST::Attribute> attribute = *attributePtr;
            for (auto attributeItem : attribute->items)
            {
                std::shared_ptr<AST::AttributeItem> attribute_item = std::get<std::shared_ptr<AST::AttributeItem>>(attributeItem);
                std::string attribute_item_string = attribute_item->content;
                if (attribute_item_string.find("{geometry}") != std::string::npos)
                {
                    for (std::string key : keys)
                    {
                        std::regex pattern_pagestyle(key + R"(\s*=\s*([\d.]+)\s*([a-zA-Z]+))", std::regex_constants::icase);
                        std::smatch match;
                        std::regex_search(attribute_item_string, match, pattern_pagestyle);
                        double value_double = std::stod(match[1].str());
                        Unit unit = mapping_unit(match[2].str());
                        value_double = convertUnit(value_double, unit, Unit::TWIP);
                        size_t value = static_cast<size_t>(std::round(value_double));
                        if (key == "top")
                        {
                            this->section->prop_.margins_.top_ = value;
                        }
                        else if (key == "bottom")
                        {
                            this->section->prop_.margins_.bottom_ = value;
                        }
                        else if (key == "left")
                        {
                            this->section->prop_.margins_.left_ = value;
                        }
                        else if (key == "right")
                        {
                            this->section->prop_.margins_.right_ = value;
                        }
                        else if (key == "headsep")
                        {
                            this->section->prop_.margins_.header_ = value;
                        }
                        else if (key == "footskip")
                        {
                            this->section->prop_.margins_.footer_ = value;
                        }
                        else if (key == "marginparsep")
                        {
                            this->section->prop_.margins_.gutter_ = value;
                        }
                    }
                }
            }
        }
    }
}


/* 函数：写入文章基本信息 */
void
DocxGenerator::generate_information()
{
    // 读取并写入文章基本信息格式
    for (AST::BlockElement& ast_content : this->document_ast.content)
    {
        if (auto Ptr = std::get_if<std::shared_ptr<AST::Paragraph>>(&ast_content)) 
        { 
            break; 
        }
        if (auto informationPtr = std::get_if<std::shared_ptr<AST::Information>>(&ast_content))
        {
            std::shared_ptr<AST::Information> information = *informationPtr;
            for (auto informationItem : information->items)
            {
                std::shared_ptr<AST::InformationItem> information_item = std::get<std::shared_ptr<AST::InformationItem>>(informationItem);
                if (information_item->pos == 0)
                {
                    std::string information_item_string = information_item->content;
                    if (information_item_string.size() > 0)
                    {
                        md::ParagraphPointer information_item_paragraph = this->section->addParagraph();
                        // 对齐
                        information_item_paragraph->prop_.align_ = mapping_alignStyle_docx(information_item->aligning);
                        // 行间距
                        information_item_paragraph->prop_.spacing_ = md::ParagraphProperties::Spacing();
                        information_item_paragraph->prop_.spacing_->lineSpacing_.type_ = md::ParagraphProperties::LineSpacingType::Lines;
                        if (information_item_string.find(".cn") != std::string::npos || information_item_string.find(".com") != std::string::npos)
                        {
                            this->write_text(information_item_paragraph, "E-mail: ", information_item->fontsize, "bold");
                        }
                        // 文本
                        this->write_text(information_item_paragraph, information_item_string, information_item->fontsize);
                    }
                }
            }
            break;
        }
    }
}


void
DocxGenerator::write_text(md::ParagraphPointer& paragraph, const std::string& text_string, const int& fontsize, const bool& isBold, const bool& isItalic)
{
    // 文本
    // 根据上下角标对文本字符串进行分割
    std::vector<std::string> text_list = splitByDollar(text_string);
    for (std::string text_i : text_list)
    {
        if (text_i.find("^") == 1)
        {
            size_t p_start = text_i.find("{") + 1;
            size_t valid_text_length = text_i.size() - 3 - 2;    // 减少处理出现中间有括号的情况：3-{、}、^，2-$、$
            text_i = text_i.substr(p_start, valid_text_length);
            md::RichTextPointer text = paragraph->addRichText(text_i+" ");
            // 上角标
            text->prop_.effects_ = md::RichTextProperties::Effects();
            text->prop_.effects_.vertAlign_ = md::RichTextProperties::VertAlign::Superscript;
            // 字体 
            text->prop_.font_ = md::RichTextProperties::Font();
            text->prop_.font_->ascii_ = "Times New Roman";
            // 字号
            text->prop_.fontSize_ = fontsize;
            // 形式
            text->prop_.fontStyle_ = md::RichTextProperties::FontStyle();
            if (isBold)
            {
                text->prop_.fontStyle_.bold_ = true;
            }
            if (isItalic)
            {
                text->prop_.fontStyle_.italic_ = true;
            }
        }
        else if (text_i.find("_") == 1)
        {
            size_t p_start = text_i.find("{") + 1;
            size_t valid_text_length = text_i.size() - 3 - 2;
            text_i = text_i.substr(p_start, valid_text_length);
            md::RichTextPointer text = paragraph->addRichText(text_i);
            // 下角标
            text->prop_.effects_ = md::RichTextProperties::Effects();
            text->prop_.effects_.vertAlign_ = md::RichTextProperties::VertAlign::Subscript;
            // 字体 
            text->prop_.font_ = md::RichTextProperties::Font();
            text->prop_.font_->ascii_ = "Times New Roman";
            // 字号
            text->prop_.fontSize_ = fontsize;
            // 形式
            text->prop_.fontStyle_ = md::RichTextProperties::FontStyle();
            if (isBold)
            {
                text->prop_.fontStyle_.bold_ = true;
            }
            if (isItalic)
            {
                text->prop_.fontStyle_.italic_ = true;
            }
        }
        else if (text_i.find("$") != std::string::npos)
        {
            // 短公式
            md::RichTextPointer text = paragraph->addRichText(text_i);
            // 字号
            text->prop_.fontSize_ = fontsize;
        }
        else
        {
            md::RichTextPointer text = paragraph->addRichText(text_i);
            // 字体 
            text->prop_.font_ = md::RichTextProperties::Font();
            text->prop_.font_->ascii_ = "Times New Roman";
            // 字号
            text->prop_.fontSize_ = fontsize;
            // 形式
            text->prop_.fontStyle_ = md::RichTextProperties::FontStyle();
            if (isBold)
            {
                text->prop_.fontStyle_.bold_ = true;
            }
            if (isItalic)
            {
                text->prop_.fontStyle_.italic_ = true;
            }
        }
    }
}


//std::vector<std::string> 
//splitByDollar(const std::string& text) 
//{
//    std::vector<std::string> result;
//    std::stringstream ss(text);
//    std::string segment;
//
//    while (std::getline(ss, segment, '$')) 
//    {
//        result.push_back(segment);
//    }
//    return result;
//}


std::vector<std::string>
splitByDollar(const std::string& text)
{
    std::vector<std::string> result;
    std::stack<std::string> dollarStack;
    size_t p_start = 0, p_end = 0;
    while (p_end < text.size() - 1)
    {
        if (text.find("$", p_start) != std::string::npos && dollarStack.empty())
        {
            p_end = text.find("$", p_start);
            std::string substring = text.substr(p_start, p_end - p_start);
            if (substring.size() > 0)
            {
                result.push_back(substring);
            }
            p_start = p_end + 1;
            dollarStack.push("$");
        }
        else if (text.find("$", p_start) != std::string::npos)
        {
            p_end = text.find("$", p_start);
            std::string substring = text.substr(--p_start, p_end - p_start + 2);
            if (substring.size() > 0)
            {
                result.push_back(substring);
            }
            p_start = p_end + 1;
            dollarStack.pop();
        }
        else
        {
            std::string substring = text.substr(p_start, text.size() - p_start);
            if (substring.size() > 0)
            {
                result.push_back(substring);
            }
            p_end = text.size() - 1;
        }
    }
    return result;
}


/* 函数：写入文章摘要 */
void 
DocxGenerator::generate_abstract()
{    
    // 读取并写入文章摘要
    for (AST::BlockElement& ast_content : this->document_ast.content)
    {
        if (auto Ptr = std::get_if<std::shared_ptr<AST::Keyword>>(&ast_content))
        {
            break;
        }
        if (auto abstractPtr = std::get_if<std::shared_ptr<AST::Paragraph>>(&ast_content))
        {
            std::shared_ptr<AST::Paragraph> abstract = *abstractPtr;
            for (AST::InlineElement& abstract_item : abstract->content)
            {
                if (auto abstractTitlePtr = std::get_if<std::shared_ptr<AST::Textbold>>(&abstract_item))
                {
                    //摘要标题
                    std::shared_ptr<AST::Textbold> abstract_title = *abstractTitlePtr;
                    md::ParagraphPointer abstract_title_paragraph = this->section->addParagraph();
                    // 对齐
                    abstract_title_paragraph->prop_.align_ = mapping_alignStyle_docx(abstract_title->aligning);
                    // 行间距
                    abstract_title_paragraph->prop_.spacing_ = md::ParagraphProperties::Spacing();
                    abstract_title_paragraph->prop_.spacing_->lineSpacing_.type_ = md::ParagraphProperties::LineSpacingType::Lines;

                    // 文本
                    if (abstract_title->content.find("bf"))
                    {
                        bool isBold = true;
                        this->write_text(abstract_title_paragraph, "Abstract", abstract_title->fontsize, isBold);
                    }
                    else
                    {
                        this->write_text(abstract_title_paragraph, "Abstract", abstract_title->fontsize);
                    }
                }
                else if (auto abstractContentPtr = std::get_if<std::shared_ptr<AST::Text>>(&abstract_item))
                {
                    // 摘要正文
                    std::shared_ptr<AST::Text> abstract_text = *abstractContentPtr;
                    md::ParagraphPointer abstract_paragraph = this->section->addParagraph();
                    // 对齐
                    abstract_paragraph->prop_.align_ = mapping_alignStyle_docx(abstract_text->aligning);
                    // 左右缩进
                    abstract_paragraph->prop_.indent_ = md::ParagraphProperties::Indentation();
                    abstract_paragraph->prop_.indent_->left_.value_ = 500;
                    abstract_paragraph->prop_.indent_->right_.value_ = 500;
                    // 首行缩进
                    abstract_paragraph->prop_.indent_->special_.type_ = md::ParagraphProperties::SpecialIndentationType::FirstLine;
                    abstract_paragraph->prop_.indent_->special_.chars_ = false;
                    abstract_paragraph->prop_.indent_->special_.value_ = 300;
                    // 行间距
                    abstract_paragraph->prop_.spacing_ = md::ParagraphProperties::Spacing();
                    abstract_paragraph->prop_.spacing_->lineSpacing_.type_ = md::ParagraphProperties::LineSpacingType::Lines;

                    // 文本
                    this->write_text(abstract_paragraph, abstract_text->content, abstract_text->fontsize);
                }
            }
            break;
        }
    }
}


/* 函数：写入文章关键词 */
void
DocxGenerator::generate_keywords()
{
    // 读取并写入文章关键词
    for (AST::BlockElement& ast_content : this->document_ast.content)
    {
        if (auto Ptr = std::get_if<std::shared_ptr<AST::Header>>(&ast_content))
        {
            break;
        }
        ++this->scan_p;
        if (auto keywordPtr = std::get_if<std::shared_ptr<AST::Keyword>>(&ast_content))
        {
            std::shared_ptr<AST::Keyword> keyword = *keywordPtr;
            std::shared_ptr<AST::Text> keyword_text = std::get<std::shared_ptr<AST::Text>>(keyword->items[0]);
            md::ParagraphPointer keyword_paragraph = this->section->addParagraph();
            // 对齐
            keyword_paragraph->prop_.align_ = mapping_alignStyle_docx(keyword_text->aligning);
            // 行间距
            keyword_paragraph->prop_.spacing_ = md::ParagraphProperties::Spacing();
            keyword_paragraph->prop_.spacing_->lineSpacing_.type_ = md::ParagraphProperties::LineSpacingType::Lines;

            // 关键词标题
            bool isBold = true;
            this->write_text(keyword_paragraph, "Keywords: ", keyword_text->fontsize, isBold);
            // 文本
            this->write_text(keyword_paragraph, keyword_text->content, keyword_text->fontsize);
            break;
        }
    }
}


/* 函数：写入文章正文 */
void 
DocxGenerator::generate_body()
{
    // 读取并写入文章基本信息格式
    std::vector<AST::BlockElement>::iterator ast_content_ptr;
    for (ast_content_ptr = this->document_ast.content.begin() + this->scan_p; ast_content_ptr != this->document_ast.content.end(); ast_content_ptr++)
    {
        AST::BlockElement& ast_content = *ast_content_ptr;
        if (auto sectionPtr = std::get_if<std::shared_ptr<AST::Header>>(&ast_content))
        {
            std::shared_ptr<AST::Header> section = *sectionPtr;
            this->generate_section_header(section);
        }
        else if (auto paragraphPtr = std::get_if<std::shared_ptr<AST::Paragraph>>(&ast_content))
        {
            std::shared_ptr<AST::Paragraph> paragraph = *paragraphPtr;
            this->generate_paragraph(paragraph);
        }
        else if (auto listPtr = std::get_if<std::shared_ptr<AST::List>>(&ast_content))
        {
            std::shared_ptr<AST::List> list = *listPtr;
            this->generate_list(list);

        }
        else if (auto figurePtr = std::get_if<std::shared_ptr<AST::Figure>>(&ast_content))
        {
            std::shared_ptr<AST::Figure> figure = *figurePtr;
            this->generate_figure(figure);
        }
        else if (auto equationPtr = std::get_if<std::shared_ptr<AST::Equation>>(&ast_content))
        {
            std::shared_ptr<AST::Equation> equation = *equationPtr;
            this->generate_equation(equation);

        }
        else if (auto tablePtr = std::get_if<std::shared_ptr<AST::Table>>(&ast_content))
        {
            std::shared_ptr<AST::Table> table = *tablePtr;
            this->generate_table(table);
            
        }
        else if (auto codeBlockPtr = std::get_if<std::shared_ptr<AST::CodeBlock>>(&ast_content))
        {

        }
    }
}


void
DocxGenerator::generate_section_header(std::shared_ptr<AST::Header> section)
{
    std::shared_ptr<AST::Textbold> section_text = std::get<std::shared_ptr<AST::Textbold>>(section->content[0]);
    md::ParagraphPointer section_paragraph = this->section->addParagraph();
    // 对齐
    section_paragraph->prop_.align_ = mapping_alignStyle_docx(section_text->aligning);
    // 行间距
    section_paragraph->prop_.spacing_ = md::ParagraphProperties::Spacing();
    section_paragraph->prop_.spacing_->lineSpacing_.type_ = md::ParagraphProperties::LineSpacingType::Lines;
    // 编号
    section_paragraph->numId_ = this->headerNumId;  // 使用具体编号ID
    section_paragraph->level_ = static_cast<md::NumberingLevel>(section->level - 1);

    // 文本
    bool isBold = true;
    this->write_text(section_paragraph, section_text->content, section_text->fontsize, isBold);
    
    if (section->level == 1)
    {
        this->reference_title_fontsize = section_text->fontsize;
        this->reference_title_aligning = section_text->aligning;
    }
}


void
DocxGenerator::generate_paragraph(std::shared_ptr<AST::Paragraph> paragraph)
{
    std::shared_ptr<AST::Text> paragraph_text = std::get<std::shared_ptr<AST::Text>>(paragraph->content[0]);
    md::ParagraphPointer paragraph_paragraph = this->section->addParagraph();
    if (paragraph_text->content.find("bibliography") != std::string::npos)
    {
        return;
    }
    if (paragraph_text->content[0] == '\\')
    {
        size_t key_command_end = 1;
        std::string key;
        while (key_command_end < paragraph_text->content.size() && std::isalpha(paragraph_text->content[key_command_end]))
        {
            key += paragraph_text->content[key_command_end];
            key_command_end++;
        }
        for (AST::BlockElement& ast_content : this->document_ast.content)
        {
            if (auto informationPtr = std::get_if<std::shared_ptr<AST::Information>>(&ast_content))
            {
                std::shared_ptr<AST::Information> information = *informationPtr;
                for (auto informationItem : information->items)
                {
                    std::shared_ptr<AST::InformationItem> information_item = std::get<std::shared_ptr<AST::InformationItem>>(informationItem);
                    if (key == information_item->key && information_item->pos == 2 && paragraph_text->content[key_command_end] == '{')
                    {
                        return;
                    }
                }
            }
        }
    }
    
    // 对齐
    paragraph_paragraph->prop_.align_ = mapping_alignStyle_docx(paragraph_text->aligning);
    // 行间距
    paragraph_paragraph->prop_.spacing_ = md::ParagraphProperties::Spacing();
    paragraph_paragraph->prop_.spacing_->lineSpacing_.type_ = md::ParagraphProperties::LineSpacingType::Lines;
    // 首行缩进
    paragraph_paragraph->prop_.indent_ = md::ParagraphProperties::Indentation();
    paragraph_paragraph->prop_.indent_->special_.type_ = md::ParagraphProperties::SpecialIndentationType::FirstLine;
    paragraph_paragraph->prop_.indent_->special_.chars_ = false;
    paragraph_paragraph->prop_.indent_->special_.value_ = 300;
    // 文本
    std::string paragraph_text_string = paragraph_text->content;
    // 更换参考文献引用
    paragraph_text_string = this->replace_cite_text(paragraph_text_string);
    // 更换表格&图引用
    paragraph_text_string = this->replace_ref_text(paragraph_text_string);
    // 写入文本
    std::vector<std::string> textbf_list = splitTextbf(paragraph_text_string);
    for (std::string& textbf_i : textbf_list)
    {
        std::vector<std::string> textbfit_list = splitTextit(textbf_i);
        for (std::string& textbfit_it : textbfit_list)
        {
            bool isBold = false, isItlatic = false;
            if (textbfit_it.find("\\textbf{") != std::string::npos)
            {
                isBold = true;
                size_t p_start = textbfit_it.find("\\textbf{") + 8;
                size_t valid_text_length = textbfit_it.size() - 8 - 1;
                textbfit_it = textbfit_it.substr(p_start, valid_text_length);
            }
            if (textbfit_it.find("\\textit{") != std::string::npos)
            {
                isItlatic = true;
                size_t p_start = textbfit_it.find("\\textit{") + 8;
                size_t valid_text_length = textbfit_it.size() - 8 - 1;
                textbfit_it = textbfit_it.substr(p_start, valid_text_length);
            }
            this->write_text(paragraph_paragraph, textbfit_it, paragraph_text->fontsize, isBold, isItlatic);
        }
    }
}


std::string
DocxGenerator::replace_cite_text(std::string paragraph_text_string)
{
    size_t p_cite_start = 0, p_cite_end = 0;
    while (paragraph_text_string.find("\\cite", p_cite_start) != std::string::npos)
    {
        p_cite_start = paragraph_text_string.find("\\cite{", p_cite_start) + 6;
        p_cite_end = paragraph_text_string.find("}", p_cite_start);
        std::string cite_key_string = paragraph_text_string.substr(p_cite_start, p_cite_end - p_cite_start);
        std::vector<std::string> cite_keys = splitCiteKey(cite_key_string);
        // 从解析的参考文献中找与cite_key相同的项
        p_cite_start = p_cite_end;
        for (std::string cite_key : cite_keys)
        {
            for (AST::BlockElement& ast_content : this->document_ast.content)
            {
                if (auto referencesPtr = std::get_if<std::shared_ptr<AST::References>>(&ast_content))
                {
                    std::shared_ptr<AST::References> references = *referencesPtr;
                    for (AST::BlockElement& reference_item : references->reference)
                    {
                        std::shared_ptr<AST::Reference> reference = std::get<std::shared_ptr<AST::Reference>>(reference_item);
                        if (cite_key == reference->cite_key)
                        {
                            // 判断参考文献队列中是否有该文献
                            size_t cite_index = searchCiteQueueElement(this->citeQueue, cite_key);
                            if (cite_index == 0)
                            {
                                this->citeQueue.push(reference);
                                cite_index = this->citeQueue.size();
                            }
                            // 更换文段中字符
                            std::regex pattern_cite("\\b" + cite_key + "\\b");
                            paragraph_text_string = std::regex_replace(paragraph_text_string, pattern_cite, std::to_string(cite_index));
                            break;
                        }
                    }
                    break;  // 在上面寻找references中的reference时，只有一个总的references，所以这里直接break
                }
            }
        }
    }
    // 删除“cite{”和“}”
    std::regex pattern_cite(R"(\\cite\{([^}]*)\})");
    paragraph_text_string = std::regex_replace(paragraph_text_string, pattern_cite, "[$1]");
    return paragraph_text_string;
}


std::string
DocxGenerator::replace_ref_text(std::string paragraph_text_string)
{
    size_t p_ref_start = 0, p_ref_end = 0;
    while (paragraph_text_string.find("\\ref{", p_ref_start) != std::string::npos)
    {
        p_ref_start = paragraph_text_string.find("\\ref{", p_ref_start) + 5;
        p_ref_end = paragraph_text_string.find("}", p_ref_start);
        std::string ref_key_string = paragraph_text_string.substr(p_ref_start, p_ref_end - p_ref_start);
        std::vector<std::string> ref_keys = splitCiteKey(ref_key_string);
        // 从解析的表格&图中找与ref_key相同的项
        // 确定是表格还是图
        size_t p_temp = p_ref_start - 7;    // 退至空格前一位字符
        while (paragraph_text_string[p_temp] != ' ')    // 退至类型名之前
        {
            --p_temp;
        }
        std::string ref_type = paragraph_text_string.substr(++p_temp, paragraph_text_string.find(" ", (p_temp + 1)) - p_temp - 1);
        std::transform(ref_type.begin(), ref_type.end(), ref_type.begin(), ::tolower);
        p_ref_start = p_ref_end;
        for (std::string ref_key : ref_keys)
        {
            for (AST::BlockElement& ast_content : this->document_ast.content)
            {
                if (ref_type.find("table") != std::string::npos)
                {
                    if (auto tablePtr = std::get_if<std::shared_ptr<AST::Table>>(&ast_content))
                    {
                        std::shared_ptr<AST::Table> table = *tablePtr;
                        if (ref_key == table->ref_label)
                        {
                            // 判断表格队列中是否有该文献
                            size_t ref_index = searchRefQueueElement(this->refTableQueue, ref_key);
                            if (ref_index == 0)
                            {
                                this->refTableQueue.push(table);
                                ref_index = this->refTableQueue.size();
                                table->ref_index = ref_index;
                            }

                            // 然后处理花括号内的引用标记
                            std::regex pattern_cite("\\{" + ref_key + "\\}");
                            paragraph_text_string = std::regex_replace(paragraph_text_string, pattern_cite, std::to_string(ref_index));
                            break;
                        }
                    }
                }
                else if (ref_type.find("fig") != std::string::npos)
                {
                    // 图
                    if (auto figurePtr = std::get_if<std::shared_ptr<AST::Figure>>(&ast_content))
                    {
                        std::shared_ptr<AST::Figure> figure = *figurePtr;
                        if (ref_key == figure->ref_label)
                        {
                            // 判断图像队列中是否有该文献
                            size_t ref_index = searchRefQueueElement(this->refFigureQueue, ref_key);
                            if (ref_index == 0)
                            {
                                this->refFigureQueue.push(figure);
                                ref_index = this->refFigureQueue.size();
                                figure->ref_index = ref_index;
                            }

                            // 然后处理花括号内的引用标记
                            std::regex pattern_cite("\\{" + ref_key + "\\}");
                            paragraph_text_string = std::regex_replace(paragraph_text_string, pattern_cite, std::to_string(ref_index));
                            break;
                        }
                    }
                }
            }
        }
    }
    // 处理 LaTeX 的 \ref{} 命令
    std::regex pattern_ref(R"(\\ref)");
    paragraph_text_string = std::regex_replace(paragraph_text_string, pattern_ref, "$1");
    return paragraph_text_string;
}


void 
DocxGenerator::generate_list(std::shared_ptr<AST::List> list)
{
    for (auto listItem : list->items)
    {
        std::shared_ptr<AST::ListItem> list_item = std::get<std::shared_ptr<AST::ListItem>>(listItem);
        std::string list_item_string = list_item->content;
        md::ParagraphPointer list_paragraph = this->section->addParagraph();
        // 对齐
        list_paragraph->prop_.align_ = mapping_alignStyle_docx(list_item->aligning);
        // 行间距
        list_paragraph->prop_.spacing_ = md::ParagraphProperties::Spacing();
        list_paragraph->prop_.spacing_->lineSpacing_.type_ = md::ParagraphProperties::LineSpacingType::Lines;
        // 编号
        list_paragraph->numId_ = this->listNumId;  // 使用具体编号ID
        list_paragraph->level_ = static_cast<md::NumberingLevel>(this->num_header);     // 使用小编号
        // 文本
        std::string paragraph_text_string = list_item->content;
        std::vector<std::string> textbf_list = splitTextbf(paragraph_text_string);
        for (std::string& textbf_i : textbf_list)
        {
            std::vector<std::string> textbfit_list = splitTextit(textbf_i);
            for (std::string& textbfit_it : textbfit_list)
            {
                bool isBold = false, isItlatic = false;
                if (textbfit_it.find("\\textbf{") != std::string::npos)
                {
                    isBold = true;
                    size_t p_start = textbfit_it.find("\\textbf{") + 8;
                    size_t valid_text_length = textbfit_it.size() - 8 - 1;
                    textbfit_it = textbfit_it.substr(p_start, valid_text_length);
                }
                if (textbfit_it.find("\\textit{") != std::string::npos)
                {
                    isItlatic = true;
                    size_t p_start = textbfit_it.find("\\textit{") + 8;
                    size_t valid_text_length = textbfit_it.size() - 8 - 1;
                    textbfit_it = textbfit_it.substr(p_start, valid_text_length);
                }
                this->write_text(list_paragraph, textbfit_it, list_item->fontsize, isBold, isItlatic);
            }
        }
    }
}


void
DocxGenerator::generate_equation(std::shared_ptr<AST::Equation> equation)
{
    // 创建表格
    md::TablePointer table_paragraph = this->section->addTable(equation->content.size(), 2);
    // 设置表格属性
    table_paragraph->prop_.layout_ = md::TableProperties::Layout::Fixed;
    table_paragraph->prop_.width_.type_ = md::TableProperties::WidthType::Percent;
    table_paragraph->prop_.align_ = md::TableProperties::Alignment::Center;
    // 设置表格边框
    table_paragraph->prop_.borders_.top_.style_ = md::BorderStyle::Single;
    table_paragraph->prop_.borders_.bottom_.style_ = md::BorderStyle::Single;
    table_paragraph->prop_.borders_.left_.style_ = md::BorderStyle::Single;
    table_paragraph->prop_.borders_.right_.style_ = md::BorderStyle::Single;
    table_paragraph->prop_.borders_.insideHorizontal_.style_ = md::BorderStyle::Single;
    table_paragraph->prop_.borders_.insideVertical_.style_ = md::BorderStyle::Single;

    // 表格内容
    for (int i = 0; i < equation->content.size(); ++i)
    {
        // 公式内容
        md::CellPointer cell_equation = table_paragraph->cellAt(i, 0);
        std::shared_ptr<AST::Math> equation_math = std::get<std::shared_ptr<AST::Math>>(equation->content[i]);
        std::string equation_latex_math = equation_math->latex_math;
        addText(cell_equation, "$" + equation_latex_math + "$", equation->fontsize, false, false);
        // 公式索引
        md::CellPointer cell_index = table_paragraph->cellAt(i, 1);
        addText(cell_index, "(" + std::to_string(equation_math->index)  + ")", equation->fontsize, false, false, md::Alignment::Right);
    }
}


void
DocxGenerator::generate_table(std::shared_ptr<AST::Table> table)
{
    // 表格题注
    std::string table_title = table->caption;
    md::ParagraphPointer table_title_paragraph = this->section->addParagraph();
    // 对齐
    table_title_paragraph->prop_.align_ = mapping_alignStyle_docx(table->aligning);
    // 行间距
    table_title_paragraph->prop_.spacing_ = md::ParagraphProperties::Spacing();
    table_title_paragraph->prop_.spacing_->lineSpacing_.type_ = md::ParagraphProperties::LineSpacingType::Lines;
    // 文本
    this->write_text(table_title_paragraph, "Table " + std::to_string(table->ref_index) + " " + table_title, table->fontsize);

    //// 表格内容
    md::TablePointer table_paragraph = this->section->addTable(table->row_count, table->column_count);
    // 设置表格属性
    table_paragraph->prop_.layout_ = md::TableProperties::Layout::Fixed;
    table_paragraph->prop_.width_.type_ = md::TableProperties::WidthType::Percent;
    table_paragraph->prop_.align_ = md::TableProperties::Alignment::Center;
    // 设置表格边框
    table_paragraph->prop_.borders_.top_.style_ = md::BorderStyle::Single;
    table_paragraph->prop_.borders_.bottom_.style_ = md::BorderStyle::Single;
    table_paragraph->prop_.borders_.left_.style_ = md::BorderStyle::Single;
    table_paragraph->prop_.borders_.right_.style_ = md::BorderStyle::Single;
    table_paragraph->prop_.borders_.insideHorizontal_.style_ = md::BorderStyle::Single;
    table_paragraph->prop_.borders_.insideVertical_.style_ = md::BorderStyle::Single;

    // 表格内容
    for (AST::BlockElement& tableCell : table->cells)
    {
        std::shared_ptr<AST::TableCell> table_cell = std::get<std::shared_ptr<AST::TableCell>>(tableCell);
        //std::regex pattern_rule(R"(\\\s*(top|mid|bottom)rule\s*)");
        std::regex pattern_rule(PATTERN_RULE);
        std::smatch match_rule;
        if (table_cell->content.size() > 0 && !std::regex_search(table_cell->content, match_rule, pattern_rule))
        {
            std::string tablecell_content = table_cell->content;
            bool bold = false, underline = false;
            if (tablecell_content.find("\\textbf{") != std::string::npos) { bold = true; }
            if (tablecell_content.find("\\underline{") != std::string::npos) { underline = true; }
            // 取实际内容
            std::regex pattern_cell_content(R"(\{([^{}]*)\})");
            std::smatch match_cell_content;
            std::regex_search(tablecell_content, match_cell_content, pattern_cell_content);
            if (match_cell_content.size() > 0)
            {
                tablecell_content = match_cell_content[1];
            }
            // 创建单元格
            md::CellPointer cell;
            if (table_cell->rowspan != 1 || table_cell->colspan != 1)
            {
                cell = table_paragraph->merge(table_cell->row_index, table_cell->col_index, table_cell->rowspan, table_cell->colspan);
            }
            else
            {
                cell = table_paragraph->cellAt(table_cell->row_index, table_cell->col_index);
            }
            // 写入单元格内容
            if (tablecell_content.find("\\\\") != std::string::npos)
            {
                std::regex row_regex(R"(\\\\[ \t]*(?:\r?\n)?)");
                std::vector<std::string> cell_row_content = split_string(tablecell_content, row_regex);
                addMultiLineText(cell, cell_row_content, table->fontsize, bold, underline);
            }
            else
            {
                addText(cell, tablecell_content, table->fontsize, bold, underline);
            }
        }
    }
}

// 表格辅助函数：添加文本到单元格并设置属性
void 
addText(md::CellPointer cell, const std::string& text, int fontsize, bool bold, bool underline, md::Alignment align_type)
{
    md::ParagraphPointer para = cell->addParagraph();
    para->prop_.align_ = align_type; // 设置段落居中对齐
    md::RichTextPointer richText = para->addRichText(text);
    richText->prop_.fontSize_ = fontsize;
    richText->prop_.font_ = md::RichTextProperties::Font();
    richText->prop_.font_->ascii_ = "Times New Roman";
    if (bold)
    {
        richText->prop_.fontStyle_.bold_ = true;
    }
    if (underline)
    {
        richText->prop_.underline_.style_ = md::RichTextProperties::UnderlineStyle::Single;
    }
};
// 表格辅助函数：添加多行文本到单元格
void 
addMultiLineText(md::CellPointer cell, const std::vector<std::string>& lines, int fontsize, bool bold, bool underline, md::Alignment align_type)
{
    for (const auto& line : lines)
    {
        addText(cell, line, fontsize, bold, underline, align_type);
    }
};


std::vector<std::string>
splitCiteKey(const std::string& text)
{
    std::vector<std::string> result;
    std::stack<std::string> dollarStack;
    size_t p_start = 0, p_end = 0;
    while (p_end < text.size() - 1)
    {
        if (text.find(",", p_start) != std::string::npos && dollarStack.empty())
        {
            p_end = text.find(",", p_start);
            result.push_back(text.substr(p_start, p_end - p_start));
            p_start = p_end + 2;
            dollarStack.push(",");
        }
        else if (text.find(",", p_start) != std::string::npos)
        {
            p_end = text.find(",", p_start);
            result.push_back(text.substr(p_start, p_end - p_start));
            p_start = p_end + 2;
            dollarStack.pop();
        }
        else
        {
            result.push_back(text.substr(p_start, text.size() - p_start));
            p_end = text.size() - 1;
        }
    }
    return result;
}


size_t
searchCiteQueueElement(const std::queue<std::shared_ptr<AST::Reference>>& input_queue, const std::string key)
{
    size_t p = 1;
    std::queue<std::shared_ptr<AST::Reference>> clone_queue = input_queue;
    while (!clone_queue.empty())
    {
        if (clone_queue.front()->cite_key == key)
        {
            return p;
        }
        clone_queue.pop();
        ++p;
    }
    return 0;
}


template<typename T> size_t
searchRefQueueElement(const std::queue<std::shared_ptr<T>>& input_queue, const std::string& key)
{
    size_t p = 1;
    std::queue<std::shared_ptr<T>> clone_queue = input_queue;
    while (!clone_queue.empty())
    {
        if (clone_queue.front()->ref_label == key)
        {
            return p;
        }
        clone_queue.pop();
        ++p;
    }
    return 0;
}


std::vector<std::string>
splitTextbf(const std::string& text)
{
    std::vector<std::string> result;
    std::stack<std::string> textbfStack;
    size_t p_start = 0, p_end = 0;
    while (p_end < text.size() - 1)
    {
        if (text.find("\\textbf{", p_start) != std::string::npos && textbfStack.empty())
        {
            p_end = text.find("\\textbf{", p_start);
            result.push_back(text.substr(p_start, p_end - p_start));
            p_start = p_end + 1;
            textbfStack.push("\\textbf{");
        }
        else if ((!textbfStack.empty()) && text.find("}", p_start) != std::string::npos)
        {
            p_end = text.find("}", p_start);
            result.push_back(text.substr(--p_start, p_end - p_start + 2));
            p_start = p_end + 1;
            textbfStack.pop();
        }
        else
        {
            result.push_back(text.substr(p_start, text.size() - p_start));
            p_end = text.size() - 1;
        }
    }
    return result;
}


std::vector<std::string>
splitTextit(const std::string& text)
{
    std::vector<std::string> result;
    std::stack<std::string> textbfStack;
    size_t p_start = 0, p_end = 0;
    while (p_end < text.size() - 1)
    {
        if (text.find("\\textit{", p_start) != std::string::npos && textbfStack.empty())
        {
            p_end = text.find("\\textit{", p_start);
            result.push_back(text.substr(p_start, p_end - p_start));
            p_start = p_end + 1;
            textbfStack.push("\\textit{");
        }
        else if ((!textbfStack.empty()) && text.find("}", p_start) != std::string::npos)
        {
            p_end = text.find("}", p_start);
            result.push_back(text.substr(--p_start, p_end - p_start + 2));
            p_start = p_end + 1;
            textbfStack.pop();
        }
        else
        {
            result.push_back(text.substr(p_start, text.size() - p_start));
            p_end = text.size() - 1;
        }
    }
    return result;
}


/* 函数：写入图像信息 */
void
DocxGenerator::generate_figure(std::shared_ptr<AST::Figure> figure)
{
    // 图像
    md::ParagraphPointer figure_image_paragraph = this->section->addParagraph();
    std::shared_ptr<AST::Image> figure_image = std::get<std::shared_ptr<AST::Image>>(figure->image[0]);
    // 对齐
    figure_image_paragraph->prop_.align_ = mapping_alignStyle_docx("center");
    // 行间距
    figure_image_paragraph->prop_.spacing_ = md::ParagraphProperties::Spacing();
    figure_image_paragraph->prop_.spacing_->lineSpacing_.type_ = md::ParagraphProperties::LineSpacingType::Lines;
    // 添加图像
    md::RelationshipId imageId = this->document_docx.addImage(figure_image->path);  // 添加图片到文档关系库
    md::PicturePointer image = figure_image_paragraph->addPicture(imageId);    // 插入图片到段落

    image->prop_.extent_.width_ = ((this->section->prop_.size_.width_ - this->section->prop_.margins_.left_ - this->section->prop_.margins_.right_) / 1440) * 914400;
    image->prop_.extent_.height_ = static_cast<size_t>(image->prop_.extent_.width_ * (static_cast<double>(figure_image->height) / figure_image->width));
    
    // 图像题注
    std::shared_ptr<AST::Text> figure_title = std::get<std::shared_ptr<AST::Text>>(figure->caption[0]);
    std::string table_title_string = figure_title->content;
    md::ParagraphPointer figure_title_paragraph = this->section->addParagraph();
    // 对齐
    figure_title_paragraph->prop_.align_ = mapping_alignStyle_docx(figure_title->aligning);
    // 行间距
    figure_title_paragraph->prop_.spacing_ = md::ParagraphProperties::Spacing();
    figure_title_paragraph->prop_.spacing_->lineSpacing_.type_ = md::ParagraphProperties::LineSpacingType::Lines;
    // 文本
    this->write_text(figure_title_paragraph, "Figure " + std::to_string(figure->ref_index) + " " + table_title_string, figure_title->fontsize);
}


/* 函数：写入补充信息 */
void 
DocxGenerator::generate_supplement()
{
    for (AST::BlockElement& ast_content : this->document_ast.content)
    {
        if (auto informationPtr = std::get_if<std::shared_ptr<AST::Information>>(&ast_content))
        {
            std::shared_ptr<AST::Information> information = *informationPtr;
            for (auto informationItem : information->items)
            {
                std::shared_ptr<AST::InformationItem> information_item = std::get<std::shared_ptr<AST::InformationItem>>(informationItem);
                if (information_item->pos == 2)
                {
                    // 补充信息标题
                    std::string information_title = information_item->title;
                    md::ParagraphPointer information_title_paragraph = this->section->addParagraph();
                    // 对齐
                    information_title_paragraph->prop_.align_ = mapping_alignStyle_docx(this->reference_title_aligning);
                    // 行间距
                    information_title_paragraph->prop_.spacing_ = md::ParagraphProperties::Spacing();
                    information_title_paragraph->prop_.spacing_->lineSpacing_.type_ = md::ParagraphProperties::LineSpacingType::Lines;
                    // 文本
                    bool isBold = true;
                    this->write_text(information_title_paragraph, information_title, this->reference_title_fontsize, isBold);

                    // 补充信息文本
                    std::string information_item_string = information_item->content;
                    if (information_item_string.size() > 0)
                    {
                        md::ParagraphPointer information_item_paragraph = this->section->addParagraph();
                        // 对齐
                        information_item_paragraph->prop_.align_ = mapping_alignStyle_docx(information_item->aligning);
                        // 行间距
                        information_item_paragraph->prop_.spacing_ = md::ParagraphProperties::Spacing();
                        information_item_paragraph->prop_.spacing_->lineSpacing_.type_ = md::ParagraphProperties::LineSpacingType::Lines;
                        // 首行缩进
                        information_item_paragraph->prop_.indent_ = md::ParagraphProperties::Indentation();
                        information_item_paragraph->prop_.indent_->special_.type_ = md::ParagraphProperties::SpecialIndentationType::FirstLine;
                        information_item_paragraph->prop_.indent_->special_.chars_ = false;
                        information_item_paragraph->prop_.indent_->special_.value_ = 300;

                        // 文本
                        this->write_text(information_item_paragraph, information_item_string, information_item->fontsize);
                    }
                }
            }
            break;
        }
    }
}



/* 函数：写入参考文献 */
void
DocxGenerator::generate_reference()
{
    //参考文献标题
    md::ParagraphPointer reference_title_paragraph = this->section->addParagraph();
    // 对齐
    reference_title_paragraph->prop_.align_ = mapping_alignStyle_docx(this->reference_title_aligning);
    // 行间距
    reference_title_paragraph->prop_.spacing_ = md::ParagraphProperties::Spacing();
    reference_title_paragraph->prop_.spacing_->lineSpacing_.type_ = md::ParagraphProperties::LineSpacingType::Lines;
    // 文本
    bool isBold = true;
    this->write_text(reference_title_paragraph, "Reference", this->reference_title_fontsize, isBold);

    // 参考文献
    std::queue<std::shared_ptr<AST::Reference>> reverse_citeQueue;
    std::queue<std::shared_ptr<AST::Reference>> clone_citeQueue = this->citeQueue;
    std::vector<std::string> keys = { "author", "title", "journal", "volume", "number", "pages", "year"};   // 另type作为写法区分
    while (!clone_citeQueue.empty())
    {
        std::shared_ptr<AST::Reference> reference = clone_citeQueue.front();
        md::ParagraphPointer reference_paragraph = this->section->addParagraph();
        for (std::string key : keys)
        {
            for (AST::InlineElement& reference_key_ptr : reference->key)
            {
                std::shared_ptr<AST::ReferenceKey> reference_key = std::get<std::shared_ptr<AST::ReferenceKey>>(reference_key_ptr);
                if (key == reference_key->key)
                {
                    // 对齐
                    reference_paragraph->prop_.align_ = mapping_alignStyle_docx(reference_key->aligning);
                    // 行间距
                    reference_paragraph->prop_.spacing_ = md::ParagraphProperties::Spacing();
                    reference_paragraph->prop_.spacing_->lineSpacing_.type_ = md::ParagraphProperties::LineSpacingType::Lines;
                    // 编号
                    reference_paragraph->numId_ = this->refNumId;  // 使用具体编号ID
                    reference_paragraph->level_ = static_cast<md::NumberingLevel>(this->num_header + 2);     // 使用参考文献编号
                    // 文本
                    std::string reference_text_string = reference_key->content;
                    if (key == "journal")
                    {
                        this->write_text(reference_paragraph, reference_text_string + ", ", reference_key->fontsize, false, true);
                    }
                    else if (key == "number")
                    {
                        this->write_text(reference_paragraph, "(" + reference_text_string + ")" + ": ", reference_key->fontsize);
                    }
                    else if (key == "year")
                    {
                        this->write_text(reference_paragraph, reference_text_string + ".", reference_key->fontsize);
                    }
                    else if (key != "author" && key != "title")
                    {
                        this->write_text(reference_paragraph, reference_text_string + ", ", reference_key->fontsize);
                    }
                    else
                    {
                        this->write_text(reference_paragraph, reference_text_string + ". ", reference_key->fontsize);
                    }
                }
            }
            
        }
        // 弹出写好的参考文献
        clone_citeQueue.pop();
    }
}






/* 函数：范例——word文件生成 */
void
generate_example()
{
    // 创建类md::Document的对象
    md::Document doc;
    // 添加一个section
    md::SectionPointer sect = doc.addSection();
    // 添加一个paragraph
    md::ParagraphPointer para = sect->addParagraph();
    // 设置段落对齐方式为居中
    para->prop_.align_ = md::Alignment::Centered;
    // 添加文本并设置格式
    md::RichTextPointer rich = para->addRichText("Happy Chinese New Year!");
    rich->prop_.fontSize_ = 32;
    rich->prop_.color_ = "FF0000";
    // 保存docx文件
    doc.saveAs("example.docx");
    std::cout << "Successfully create docx file." << std::endl;
}




