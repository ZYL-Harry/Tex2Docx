// parser_template.cpp
#include "parser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include"document_ast.h"
#include <stack>
#include"tex_attribute.h"
#include <regex>



/* 函数：默认格式读取（article.cls） */
void
TexParser::get_default_style(const std::string& style_path)
{
    // 读取模板文件
    std::ifstream file(style_path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + style_path + ".");
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string template_attribute_content = buffer.str();
    // 删除注释
    size_t pos = 0;
    while ((pos = template_attribute_content.find('%', pos)) != std::string::npos)
    {
        size_t end_of_line = template_attribute_content.find('\n', pos);
        if (end_of_line != std::string::npos)
        {
            // 删除从“%”到“\n”之前的所有字符
            template_attribute_content.erase(pos, end_of_line - pos);
        }
        else
        {
            // 删除从%到字符串末尾的所有内容
            template_attribute_content.erase(pos);
        }
    }
    // 解析模板
    size_t scan_p_temp = this->scan_p;
    this->template_attribute = std::make_shared<AST::Attribute>();
    // 获取关键属性，导入template_attribute
    // 整体属性、information、各section、abstract、公式、图像、表格、参考文献等
    
    // 整体属性
    size_t p_start = scan_p_temp = template_attribute_content.find("\\ExecuteOptions", scan_p_temp);
    scan_p_temp = this->match(template_attribute_content, "\\ExecuteOptions", scan_p_temp);
    scan_p_temp = this->skip_blank(template_attribute_content, scan_p_temp);
    size_t p_end = this->find_attribute_item_end(template_attribute_content, p_start);
    scan_p_temp = p_end;
    std::string basic_string = template_attribute_content.substr(p_start, p_end - p_start);
    // 写入模板属性
    std::shared_ptr<AST::AttributeItem> basic_item = std::make_shared<AST::AttributeItem>();
    basic_item->content = basic_string;   // 写入整体内容
    // 匹配fontsize并写入
    std::regex pattern_font("(\\d+)pt"); 
    std::smatch match;
    std::regex_search(basic_string, match, pattern_font);
    basic_item->fontsize = std::stoi(match[1].str());
    // 匹配column并写入
    std::regex pattern_column("(one|two)column");
    std::regex_search(basic_string, match, pattern_column);
    basic_item->column = match.str();
    basic_item->aligning = "justifying";
    // 添加属性项
    this->template_attribute->items.push_back(basic_item);

    // information
    p_start = scan_p_temp = template_attribute_content.find("\\def\\@maketitle", scan_p_temp);
    scan_p_temp = this->match(template_attribute_content, "\\def\\@maketitle", scan_p_temp);
    scan_p_temp = this->skip_blank(template_attribute_content, scan_p_temp);
    p_end = this->find_attribute_item_end(template_attribute_content, p_start);
    std::string information_string = template_attribute_content.substr(++scan_p_temp, p_end - scan_p_temp - 2);
    std::string aligning = findAlignStyle(information_string);
    // 写入文章信息属性
    scan_p_temp = 0;
    while (scan_p_temp < information_string.size() && information_string.find("{", scan_p_temp) != std::string::npos)
    {
        std::shared_ptr<AST::InformationItem> information_item = std::make_shared<AST::InformationItem>();
        // 匹配标题（title）、作者（author）、日期（date）
        scan_p_temp = information_string.find("{", scan_p_temp);
        scan_p_temp = this->skip_blank(information_string, scan_p_temp);
        p_end = this->find_attribute_item_end(information_string, scan_p_temp);
        std::string information_item_string = information_string.substr(scan_p_temp, p_end - scan_p_temp);
        information_item->content = information_item_string;
        scan_p_temp = p_end;
        if (information_item_string.find("@title") != std::string::npos)
        {
            // 找fontsize
            information_item->fontsize = findFont(information_item_string);
        }
        else if (information_item_string.find("@author") != std::string::npos)
        {
            information_item->fontsize = findFont(information_item_string);
        }
        else if (information_item_string.find("@date") != std::string::npos)
        {
            information_item->fontsize = findFont(information_item_string);
        }
        information_item->aligning = aligning;
        this->template_attribute->items.push_back(information_item);
    }

    // 各section
    scan_p_temp = 0;
    while (template_attribute_content.find("\\@startsection", scan_p_temp) != std::string::npos)
    {
        std::shared_ptr<AST::Header> section_title = std::make_shared<AST::Header>();
        p_start = scan_p_temp = template_attribute_content.find("\\@startsection", scan_p_temp);
        scan_p_temp = this->match(template_attribute_content, "\\@startsection", scan_p_temp);
        scan_p_temp = this->skip_blank(template_attribute_content, scan_p_temp);
        p_end = this->find_attribute_item_end(template_attribute_content, --p_start);
        std::string section_title_string = template_attribute_content.substr(p_start, p_end - p_start);
        // 写入章节标题属性
        std::shared_ptr<AST::Textbold> section_title_text = std::make_shared<AST::Textbold>();
        if (section_title_string.find("subsubsection") != std::string::npos)
        {
            section_title->level = 3;
        }
        else if (section_title_string.find("subsection") != std::string::npos)
        {
            section_title->level = 2;
        }
        else if (section_title_string.find("section") != std::string::npos)
        {
            section_title->level = 1;
        }
        else if (section_title_string.find("subparagraph") != std::string::npos)
        {
            section_title->level = 5;
        }
        else
        {
            section_title->level = 4;
        }
        section_title_text->content = section_title_string;
        section_title_text->fontsize = findFont(section_title_string);
        section_title_text->aligning = JUSTIFYING;
        section_title->content.push_back(section_title_text);
        this->template_attribute->items.push_back(section_title);
    }

    // abstract：默认不使用titlepage
    scan_p_temp = 0;
    while (template_attribute_content.find("\\newenvironment{abstract}", scan_p_temp) != std::string::npos)
    {
        std::shared_ptr<AST::Paragraph> abstract = std::make_shared<AST::Paragraph>();
        p_start = scan_p_temp = template_attribute_content.find("\\newenvironment{abstract}", scan_p_temp);
        scan_p_temp = this->match(template_attribute_content, "\\newenvironment{abstract}", scan_p_temp);
        scan_p_temp = this->skip_blank(template_attribute_content, scan_p_temp);
        p_end = this->find_attribute_item_end(template_attribute_content, --p_start);
        std::string abstract_string = template_attribute_content.substr(p_start, p_end - p_start);
        if (abstract_string.find("titlepage") != std::string::npos)
        {
            continue;
        }
        else
        {
            std::shared_ptr<AST::Text> abstract_text = std::make_shared<AST::Text>();
            if (template_attribute_content.find("@" + basic_item->column, scan_p_temp) != std::string::npos)
            {
                p_start = scan_p_temp = template_attribute_content.find("\\if", scan_p_temp) + 3;
                p_end = template_attribute_content.find("\\else", scan_p_temp) - 5;
            }
            else
            {
                p_start = scan_p_temp = template_attribute_content.find("\\else", scan_p_temp) + 5;
                p_end = template_attribute_content.find("\\fi", scan_p_temp) - 3;
            }
            abstract_string = template_attribute_content.substr(p_start, p_end - p_start);
            abstract_text->content = abstract_string;
            abstract_text->fontsize = findFont(abstract_string);
            if (abstract_string.find("quotation"))
            {
                abstract_text->aligning = "justifying";
            }
            abstract->content.push_back(abstract_text);
            // 摘要标题属性
            std::shared_ptr<AST::Textbold> abstract_title = std::make_shared<AST::Textbold>();
            p_start = scan_p_temp = template_attribute_content.find("\\begin", scan_p_temp);
            scan_p_temp = this->match(template_attribute_content, "\\begin", scan_p_temp);
            scan_p_temp = this->skip_blank(template_attribute_content, scan_p_temp);
            p_end = template_attribute_content.find("\\end", scan_p_temp);
            p_end = this->find_attribute_item_end(template_attribute_content, p_end);
            std::string abstract_title_string = template_attribute_content.substr(p_start, p_end - p_start);
            abstract_title->content = abstract_title_string;
            if (findFont(abstract_title_string) > 0)
            {
                abstract_title->fontsize = findFont(abstract_title_string);
            }
            else
            {
                abstract_title->fontsize = abstract_text->fontsize;
            }
            abstract_title->aligning = findAlignStyle(abstract_title_string);
            abstract->content.push_back(abstract_title);
        }
        this->template_attribute->items.push_back(abstract);
    }

    // 公式


    // 图像


    // 表格


    // 参考文献

    
}


int
findFont(const std::string& text)
{
    for (const std::string& font : fontsizeList) 
    {
        // 查找字体名称
        std::string fontWithSlash = "\\" + font;
        if (text.find(fontWithSlash) != std::string::npos) 
        {
            return mapping_fontsize(font);
        }
    }
    return -1; // 未找到
}


std::string
findAlignStyle(const std::string& text)
{
    for (const std::string& align_style : alignStyleList)
    {
        // 查找字体名称
        if (text.find(align_style) != std::string::npos)
        {
            return mapping_alignStyle(align_style);
        }
    }
    return JUSTIFYING; // 未找到
}


/* 函数：文本匹配（模板） */
size_t
TexParser::match(const std::string& template_content, const std::string& condition_str, size_t& p)
{
    bool ismatch = template_content.compare(p, condition_str.size(), condition_str) == 0;
    if (ismatch)
    {
        p += condition_str.size();   // 文本匹配后，更新扫描位置
    }
    // 跳过空白字符
    p = this->skip_blank(template_content, p);
    return p;
}


/* 函数：跳过空字符（模板）——空格（' '）、换页（'\f'）、换行（'\n'）、回车（'\r'）、水平制表符（'\t'）和垂直制表符（'\v'） */
size_t
TexParser::skip_blank(const std::string& template_content, size_t& p)
{
    while (std::isspace(template_content[p]))
    {
        p++;     // 更新扫描位置
    }
    return p;
}


size_t
TexParser::find_attribute_item_end(const std::string& template_content, size_t& p_start)
{
    size_t p_next_n = template_content.find("\n", p_start);
    std::string string_to_next_n = template_content.substr(p_start, p_next_n - p_start);
    while ((!isBracketBalanced(string_to_next_n)) == 1)
    {
        p_next_n = template_content.find("\n", ++p_next_n);
        string_to_next_n = template_content.substr(p_start, p_next_n - p_start);
    }
    return p_next_n;
}

