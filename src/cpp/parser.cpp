// parser.cpp
#include "parser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include"document_ast.h"
#include <stack>
#include"tex_attribute.h"
#include <regex>

#define NOMINMAX
#include "fpdfview.h"
#include "fpdf_formfill.h"
#include "fpdf_ext.h"
#include "fpdf_dataavail.h"
#include "fpdf_text.h"

#define _CRT_SECURE_NO_WARNINGS
#include "stb_image_write.h"



/* 拷贝构造 */
TexParser::TexParser(const TexParser& tex_parser) {}


/* 拷贝赋值 */
TexParser& 
TexParser::operator=(const TexParser& str) {}


/* 析构函数 */
TexParser::~TexParser() {}


/* 函数：tex文件读取 */
void
TexParser::read_tex_file() 
{
    // 声明一个ifstream类型的对象file，赋予指针filename为参数
    std::ifstream file(this->tex_path);
    // 文件流file与实际文件绑定
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + this->tex_path + ".");
    }
    // 声明一个string缓存器
    std::stringstream buffer;
    // 将file的数据传入buffer中
    buffer << file.rdbuf();
    // 读取buffer中的数据（文件内容）
    std::string content = buffer.str();

    /* 删除关于tex文件中的注释 */
    size_t pos = 0;
    while ((pos = content.find('%', pos)) != std::string::npos) 
    {
        if (content[pos - 1] != '\\')
        {
            size_t end_of_line = content.find('\n', pos);
            if (end_of_line != std::string::npos)
            {
                // 删除从“%”到“\n”之前的所有字符
                content.erase(pos, end_of_line - pos);
            }
            else
            {
                // 删除从%到字符串末尾的所有内容
                content.erase(pos);
            }
        }
        else
        {
            content.erase(pos - 1, 1);
        }
    }
    this->tex_content = content;
    std::cout << "Read tex file successfully." << std::endl;
    return;
}


/* 函数：tex文件解析 */
AST::Document&
TexParser::parse()
{
    /*std::cout << this->scan_p << ", content size: " << this->tex_content.size() << std::endl;*/
    while (this->scan_p < this->tex_content.size())
    {
        // 检查是否到达文档结束
        if (this->match("\\end{document}")) 
        {
            std::cout << "Complete parse tex file." << std::endl;
            break;
        }
        // 跳过空白字符
        this->skip_blank();
        // 解析当前位置文本内容
        this->parse_block();
        //std::cout << this->scan_p << std::endl;
    }
    return this->document_ast;
}


/* 函数：文本匹配 */
bool
TexParser::match(const std::string& condition_str)
{
    bool ismatch = this->tex_content.compare(this->scan_p, condition_str.size(), condition_str) == 0;
    if (ismatch) 
    {
        this->scan_p += condition_str.size();   // 文本匹配后，更新扫描位置
    }
    // 跳过空白字符
    this->skip_blank();
    return ismatch;
}


/* 函数：跳过空字符——空格（' '）、换页（'\f'）、换行（'\n'）、回车（'\r'）、水平制表符（'\t'）和垂直制表符（'\v'） */
void
TexParser::skip_blank()
{
    while (std::isspace(this->tex_content[this->scan_p])) 
    {
        this->scan_p++;     // 更新扫描位置
    }
}


/* 函数：判断段落中公式符号起始端 */
bool 
TexParser::is_symbol_start(size_t& p) 
{
    return this->tex_content[p] == '$';
}


/* 函数：判断空行起始端 */
bool
TexParser::is_empty_line_start(size_t& p)
{

    return this->tex_content[p] == '\n';
}


/* 函数：解析块级元素 */
void
TexParser::parse_block() 
{
    if (this->scan_p == 0)  // 文章格式属性
    {
        std::shared_ptr<AST::Attribute> attribute_content = this->parse_attribute();
        this->document_ast.content.push_back(attribute_content);
    }
    else if (this->match("\\begin{document}"))  // 文章基本信息
    {
        std::shared_ptr<AST::Information> information_content = this->parse_information();
        this->document_ast.content.push_back(information_content);
    }
    else if (this->match("\\begin{abstract}"))  // 摘要
    {
        std::shared_ptr<AST::Paragraph> abstract_content = this->parse_paragraph("abstract");
        this->document_ast.content.push_back(abstract_content);
    }
    else if (this->match("\\keywords"))  // 关键词
    {
        std::shared_ptr<AST::Keyword> keyword_content = this->parse_keyword();
        this->document_ast.content.push_back(keyword_content);
    }
    else if (this->match("\\section"))  // 一级标题
    {
        int level_section = 1;
        std::shared_ptr<AST::Header> section_content = this->parse_section(level_section);
        this->document_ast.content.push_back(section_content);
    }
    else if (this->match("\\subsection"))   // 二级标题
    {
        int level_section = 2;
        std::shared_ptr<AST::Header> subsection_content = this->parse_section(level_section);
        this->document_ast.content.push_back(subsection_content);
    }
    else if (this->match("\\begin{enumerate}"))     // 列举条目
    {
        std::shared_ptr<AST::List> enumerate_content  = this->parse_enumerate();
        this->document_ast.content.push_back(enumerate_content);
    }
    else if (this->match("\\begin{figure}"))    // 图像
    {
        std::shared_ptr<AST::Figure> figure_content = this->parse_figure();
        this->document_ast.content.push_back(figure_content);
    }
    else if (this->match("\\begin{equation}") || this->match("\\begin{align}"))  // 公式
    {
        std::shared_ptr<AST::Equation> equation_content = this->parse_equation();
        this->document_ast.content.push_back(equation_content);
    }
    else if (this->match("\\begin{table}"))     // 表格
    {
        std::shared_ptr<AST::Table> table_content = this->parse_table();
        this->document_ast.content.push_back(table_content);
    }
    else if (this->match("\\ref"))      // 引用（图像、表格）
    {

    }
    else if (this->match("\\cite"))     // 引用（参考文献）
    {

    }
    else if (this->match("\\begin{algorithm}"))     // 伪代码
    {
        std::shared_ptr<AST::CodeBlock> algorithm_content = this->parse_algorithm();
        this->document_ast.content.push_back(algorithm_content);
    }
    else if (this->match("\\{"))
    {
        for (const std::string& key : this->keys)
        {
            if (this->match(key))
            {
                this->scan_p = this->find_attribute_item_end(this->scan_p);
                break;
            }
            else 
            {
                continue;
            }
        }
    }
    else
    {
        std::shared_ptr<AST::Paragraph> paragraph_content = this->parse_paragraph();
        this->document_ast.content.push_back(paragraph_content);
    }
}


/* 函数：解析文章格式属性 */
std::shared_ptr<AST::Attribute>
TexParser::parse_attribute()
{
    // 声明一个Attribute对象
    std::shared_ptr<AST::Attribute> attribute = std::make_shared<AST::Attribute>();
    while (!(this->match("\\begin{document}")))
    {
        // 找“\document”、“\usepackage”、“\newcommand”、“\AtBeginDocument”
        size_t p_document = this->tex_content.find("\\document", this->scan_p);
        size_t p_usepackage = this->tex_content.find("\\usepackage", this->scan_p);
        size_t p_newcommand = this->tex_content.find("\\newcommand", this->scan_p);
        size_t p_begindocument = this->tex_content.find("\\AtBeginDocument", this->scan_p);
        // 找到上述关键命令中最先出现的命令
        // 先找找下一个“\n”，在判断之前的内容是否存在未完全取出的“{}”，若已完全取出则定位p_end
        size_t p_former = std::min(std::min(std::min(p_document, p_usepackage), p_newcommand), p_begindocument);
        size_t p_start = this->scan_p = p_former;
        size_t p_end;
        if (p_former == p_document)
        {
            this->match("\\document");
        }
        else if (p_former == p_usepackage)
        {
            this->match("\\usepackage");
        }
        else if (p_former == p_newcommand)
        {
            this->match("\\newcommand");
        }
        else
        {
            this->match("\\AtBeginDocument");
        }
        this->skip_blank();
        p_end = this->find_attribute_item_end(p_start);
        // 提取格式属性
        this->scan_p = p_end;
        std::string attribute_string = this->tex_content.substr(p_start, p_end - p_start);    // 读取属性文字
        //attribute_string.erase(std::remove_if(attribute_string.begin(), attribute_string.end(), ::isspace), attribute_string.end());
        // 判断写入对象，声明对应的对象并写入
        std::shared_ptr<AST::AttributeItem> attribute_item = std::make_shared<AST::AttributeItem>();
        attribute_item->content = attribute_string;
        attribute->items.push_back(attribute_item);
        // 跳过空白字符
        this->skip_blank();
    }
    this->scan_p = this->tex_content.find("\\begin{document}");;
    return attribute;
}


size_t
TexParser::find_attribute_item_end(size_t& p_start)
{
    size_t p_next_n = this->tex_content.find("\n", p_start);
    std::string string_to_next_n = this->tex_content.substr(p_start, p_next_n - p_start);
    while ((!isBracketBalanced(string_to_next_n)) == 1)
    {
        p_next_n = this->tex_content.find("\n", ++p_next_n);
        string_to_next_n = this->tex_content.substr(p_start, p_next_n - p_start);
        //std::cout << string_to_next_n << std::endl;
    }
    return p_next_n;
}


bool
isBracketBalanced(const std::string& expression) {
    // 定义括号映射：右括号 -> 对应的左括号
    std::unordered_map<char, char> bracketPairs = {
        {')', '('},
        {']', '['},
        {'}', '{'}
    };
    // 定义左括号集合，用于快速判断
    std::string leftBrackets = "([{";
    // 使用栈来存储遇到的左括号
    std::stack<char> bracketStack;
    // 遍历字符串中的每个字符
    for (char c : expression) 
    {
        // 如果是左括号，压入栈中
        if (leftBrackets.find(c) != std::string::npos) 
        {
            bracketStack.push(c);
        }
        // 如果是右括号
        else if (bracketPairs.find(c) != bracketPairs.end())    // 在映射bracketPairs中查找键为c的元素，找不到则返回指向容器的“末尾”（最后一个元素之后的位置）
        {
            // 检查栈是否为空或栈顶是否匹配
            if (bracketStack.empty() || bracketStack.top() != bracketPairs[c])  // bracketPairs[c]返回bracketPairs中与键c对应的元素
            {
                return false; // 不匹配
            }
            bracketStack.pop(); // 匹配成功，弹出栈顶
        }
    }
    // 如果栈为空，说明所有括号都匹配
    return bracketStack.empty();
}


/* 函数：解析文章基本信息 */
std::shared_ptr<AST::Information>
TexParser::parse_information()
{
    // 声明一个Information对象
    std::shared_ptr<AST::Information> information = std::make_shared<AST::Information>();
    // 跳过空白字符
    this->skip_blank();
    // 提取格式属性
    size_t p_start = this->scan_p;
    size_t p_end_all = this->tex_content.find("\\begin{abstract}", p_start);
    std::string information_string = this->tex_content.substr(p_start, p_end_all - p_start);    // 读取属性文字

    p_start = this->scan_p;
    p_end_all = this->tex_content.find("\\section", p_start);
    std::string information_string_1 = this->tex_content.substr(p_start, p_end_all - p_start);    // 读取属性文字

    p_start = this->scan_p;
    p_end_all = this->tex_content.find("\\end{document}", p_start);
    std::string information_string_2 = this->tex_content.substr(p_start, p_end_all - p_start);    // 读取属性文字
    // 创建信息列表，并加上属性中增加声明的信息
    this->keys = { "title", "author", "date" };
     std::string key;
    std::shared_ptr<AST::Attribute> attribute = std::get<std::shared_ptr<AST::Attribute>>(this->document_ast.content[1]);
    for (AST::BlockElement& attribute_item : attribute->items)
    {
        std::string attribute_item_string = std::get<std::shared_ptr<AST::AttributeItem>>(attribute_item)->content;
        if (attribute_item_string.find("\\newcommand") != std::string::npos)
        {
            size_t p_start_a = attribute_item_string.find("{") + 2;
            size_t p_end_a = attribute_item_string.find("}");
            key = attribute_item_string.substr(p_start_a, p_end_a - p_start_a);
            this->keys.push_back(key);
        }
    }
    // 遍历并以InformationItem对象写入
    size_t p_end, scan_p_temp=this->scan_p;
    bool sign_found;
    for (const std::string& key : this->keys)
    {
        while (this->tex_content.find("\\" + key, scan_p_temp) != std::string::npos && scan_p_temp - this->scan_p < information_string_2.size())
        {
            sign_found = false;     // 为每个key提供一个找寻属性的标志，因为需要分别从template和现文档中查找
            std::shared_ptr<AST::InformationItem> information_item = std::make_shared<AST::InformationItem>();
            information_item->key = key;
            if (scan_p_temp - this->scan_p < information_string.size())
            {
                information_item->pos = 0;
            }
            else if (scan_p_temp - this->scan_p < information_string_1.size())
            {
                information_item->pos = 1;
            }
            else
            {
                information_item->pos = 2;
            }
            if (this->tex_content.find("\\" + key, scan_p_temp) != std::string::npos)
            {
                p_start = this->tex_content.find("\\" + key, scan_p_temp);
                p_end = this->find_attribute_item_end(p_start);
                p_start = this->match(this->tex_content, "\\" + key, p_start) + 1;
                std::string information_item_string = this->tex_content.substr(p_start, p_end - p_start - 1);
                information_item->content = information_item_string;
                // 写入各信息属性，分别从template_attribute和文档声明attribute中查找
                for (AST::BlockElement& attribute_item : this->template_attribute->items)
                {
                    if (auto attrItemPtr = std::get_if<std::shared_ptr<AST::InformationItem>>(&attribute_item))
                    {
                        std::shared_ptr<AST::InformationItem> attributeItem = *attrItemPtr;
                        if (attributeItem->content.find(key) != std::string::npos)
                        {
                            information_item->fontsize = attributeItem->fontsize;
                            information_item->aligning = attributeItem->aligning;
                            sign_found = true;
                            break;
                        }
                    }
                }
                if (!sign_found)
                {
                    for (AST::BlockElement& attribute_item : attribute->items)
                    {
                        std::string attribute_item_string = std::get<std::shared_ptr<AST::AttributeItem>>(attribute_item)->content;
                        if (attribute_item_string.find("\\newcommand") != std::string::npos && attribute_item_string.find(key) != std::string::npos)
                        {
                            // 对于特殊补充信息查看是否有标题头
                            if (attribute_item_string.find("\\section*") != std::string::npos)
                            {
                                size_t p_start_item = attribute_item_string.find("\\section*{");
                                p_start_item = this->match(attribute_item_string, "\\section*{", p_start_item);
                                size_t p_end_item = attribute_item_string.find("}", p_start_item);
                                std::string key_title = attribute_item_string.substr(p_start_item, p_end_item - p_start_item);
                                information_item->title = key_title;
                            }
                            // 字号
                            std::regex pattern_fontsize(R"(\\fontsize\{(\d+)\}\{(\d+)\})");
                            std::smatch match;
                            std::regex_search(attribute_item_string, match, pattern_fontsize);
                            if (!match.empty())
                            {
                                information_item->fontsize = std::stoi(match[1].str()) * 2;
                            }
                            else
                            {
                                for (AST::BlockElement& attribute_item : this->template_attribute->items)
                                {
                                    if (auto attrItemPtr = std::get_if<std::shared_ptr<AST::AttributeItem>>(&attribute_item))
                                    {
                                        std::shared_ptr<AST::AttributeItem> attributeItem = *attrItemPtr;
                                        information_item->fontsize = attributeItem->fontsize * 2;
                                        break;
                                    }
                                    else
                                    {
                                        continue;
                                    }
                                }
                            }
                            // 对齐
                            if (ifFindAlignStyle(attribute_item_string))
                            {
                                information_item->aligning = findAlignStyle(attribute_item_string);
                            }
                            else
                            {
                                information_item->aligning = JUSTIFYING;
                            }
                            sign_found = true;
                            break;
                        }
                    }
                }
                information->items.push_back(information_item);
                scan_p_temp = p_end;
                // 跳过空白字符
                scan_p_temp = this->skip_blank(this->tex_content, scan_p_temp);
            }
        }
    }
    this->scan_p = this->tex_content.find("\\begin{abstract}");
    return information;
}


/* 函数：解析段落 */
std::shared_ptr<AST::Paragraph>
TexParser::parse_paragraph(const std::string& key)
{
    // 声明一个Information对象
    std::shared_ptr<AST::Paragraph> paragraph = std::make_shared<AST::Paragraph>();
    // 跳过空白字符
    this->skip_blank();
    // 提取格式属性
    std::shared_ptr<AST::Text> paragraph_text = std::make_shared<AST::Text>();  // 声明一个Text对象并写入
    size_t p_start = this->scan_p, p_end = this->scan_p;
    std::string attribute_item_string;
    if (key == "abstract")
    {
        p_end = this->tex_content.find("\\end{abstract}", p_start);

        // 查看template_attribute中abstract属性
        for (AST::BlockElement& attribute_item : this->template_attribute->items)
        {
            if (auto attrItemPtr = std::get_if<std::shared_ptr<AST::Paragraph>>(&attribute_item)) 
            {
                std::shared_ptr<AST::Paragraph> attributeItem = *attrItemPtr;
                
                std::shared_ptr<AST::Paragraph> paragraph_abstract = std::make_shared<AST::Paragraph>();
                // 摘要标题
                std::shared_ptr<AST::Textbold> abstract_title = std::get<std::shared_ptr<AST::Textbold>>(attributeItem->content[1]);
                paragraph_abstract->content.push_back(abstract_title);
                // 摘要正文
                std::shared_ptr<AST::Text> attributeItemText = std::get<std::shared_ptr<AST::Text>>(attributeItem->content[0]);
                paragraph_text->fontsize = attributeItemText->fontsize;
                paragraph_text->aligning = attributeItemText->aligning;
                this->scan_p = p_end;
                this->match("\\end{abstract}");
                std::string paragraph_string = this->tex_content.substr(p_start, p_end - p_start);    // 读取属性文字
                paragraph_text->content = paragraph_string;
                paragraph_abstract->content.push_back(paragraph_text);
                
                return paragraph_abstract;
            }
            else 
            { 
                continue; 
            }
        }
    }
    else
    {
        //while (!this->is_symbol_start(p_end) && !this->is_empty_line_start(p_end))
        while (!this->is_empty_line_start(p_end))
        {
            p_end++;
        }
        // 查看template_attribute中的整体属性
        for (AST::BlockElement& attribute_item : this->template_attribute->items)
        {
            if (auto attrItemPtr = std::get_if<std::shared_ptr<AST::AttributeItem>>(&attribute_item))
            {
                std::shared_ptr<AST::AttributeItem> attributeItem = *attrItemPtr;
                paragraph_text->fontsize = attributeItem->fontsize * 2;
                paragraph_text->aligning = attributeItem->aligning;
                this->scan_p = p_end;
                this->match("\\end{abstract}");
                std::string paragraph_string = this->tex_content.substr(p_start, p_end - p_start);    // 读取属性文字
                paragraph_text->content = paragraph_string;
                paragraph->content.push_back(paragraph_text);
                return paragraph;
            }
            else
            {
                continue;
            }
        }
    }
}


/* 函数：解析关键词 */
std::shared_ptr<AST::Keyword>
TexParser::parse_keyword()
{
    // 声明一个Keyword对象
    std::shared_ptr<AST::Keyword> keyword = std::make_shared<AST::Keyword>();
    // 跳过空白字符
    this->skip_blank();
    // 提取格式属性
    size_t p_start = ++this->scan_p;
    size_t p_end = this->tex_content.find("}", p_start);
    this->scan_p = p_end + 1;
    std::string keyword_string = this->tex_content.substr(p_start, p_end - p_start);    // 读取属性文字
    // 声明一个Text对象并写入
    std::shared_ptr<AST::Text> keyword_item = std::make_shared<AST::Text>();
    keyword_item->content = keyword_string;

    std::shared_ptr<AST::Attribute> attribute = std::get<std::shared_ptr<AST::Attribute>>(this->document_ast.content[1]);
    for (AST::BlockElement& attribute_item : attribute->items)
    {
        std::string attribute_item_string = std::get<std::shared_ptr<AST::AttributeItem>>(attribute_item)->content;
        if (attribute_item_string.find("keywords") != std::string::npos)
        {
            // 字号
            std::regex pattern_fontsize(R"(\\fontsize\{(\d+)\}\{(\d+)\})");
            std::smatch match;
            std::regex_search(attribute_item_string, match, pattern_fontsize);
            if (!match.empty())
            {
                keyword_item->fontsize = std::stoi(match[1].str()) * 2;
            }
            else
            {
                for (AST::BlockElement& attribute_item : this->template_attribute->items)
                {
                    if (auto attrItemPtr = std::get_if<std::shared_ptr<AST::AttributeItem>>(&attribute_item))
                    {
                        std::shared_ptr<AST::AttributeItem> attributeItem = *attrItemPtr;
                        keyword_item->fontsize = attributeItem->fontsize;
                        break;
                    }
                    else
                    {
                        continue;
                    }
                }
            }
            // 对齐
            if (ifFindAlignStyle(attribute_item_string))
            {
                keyword_item->aligning = findAlignStyle(attribute_item_string);
            }
            else
            {
                keyword_item->aligning = JUSTIFYING;
            }
        }
    }
    keyword->items.push_back(keyword_item);

    // 关键词标题

    return keyword;
}


/* 函数：解析各级标题 */
std::shared_ptr<AST::Header> 
TexParser::parse_section(const int& level_section)
{
    std::shared_ptr<AST::Header> section_title = std::make_shared<AST::Header>();
    // 写入标题登记
    section_title->level = level_section;
    // 写入标题文字
    size_t p_start = ++this->scan_p;
    size_t p_end = this->tex_content.find('}', p_start);
    this->scan_p = p_end + 1;    // 更新扫描位置
    std::string section_title_string = this->tex_content.substr(p_start, p_end - p_start); // 读取标题文字
    std::shared_ptr<AST::Textbold> section_title_text = std::make_shared<AST::Textbold>();
    section_title_text->content = section_title_string;
    // 查看template_attribute中的标题属性
    for (AST::BlockElement& attribute_item : this->template_attribute->items)
    {
        if (auto attrItemPtr = std::get_if<std::shared_ptr<AST::Header>>(&attribute_item))
        {
            std::shared_ptr<AST::Header> attributeItem = *attrItemPtr;
            if (attributeItem->level == level_section)
            {
                std::shared_ptr<AST::Textbold> attribute_text = std::get<std::shared_ptr<AST::Textbold>>(attributeItem->content[0]);
                section_title_text->fontsize = attribute_text->fontsize;
                section_title_text->aligning = attribute_text->aligning;
                break;
            }
        }
    }
    section_title->content.push_back(section_title_text);
    return section_title;
}


/* 函数：解析列举条目 */
std::shared_ptr<AST::List>
TexParser::parse_enumerate()
{
    std::shared_ptr<AST::List> enumerate_list = std::make_shared<AST::List>();
    // 写入顺序符
    enumerate_list->is_ordered = false;
    // 跳过空白字符
    this->skip_blank();
    // 写入条目文字
    while (!(this->match("\\end{enumerate}")))
    {
        // 找到各“\item”之间的内容
        this->match("\\item");
        this->skip_blank();
        size_t p_start = this->scan_p;
        size_t p_end = this->tex_content.find("\n", p_start);
        this->scan_p = p_end;    // 更新扫描位置至下一个“\item”前
        std::string enumerate_string = this->tex_content.substr(p_start, p_end - p_start); // 读取条目文字
        // 创建ListItem对象并写入
        std::shared_ptr<AST::ListItem> enumerate_list_item = std::make_shared<AST::ListItem>();
        enumerate_list_item->content = enumerate_string;
        // 查看template_attribute中的整体属性
        for (AST::BlockElement& attribute_item : this->template_attribute->items)
        {
            if (auto attrItemPtr = std::get_if<std::shared_ptr<AST::AttributeItem>>(&attribute_item))
            {
                std::shared_ptr<AST::AttributeItem> attributeItem = *attrItemPtr;
                enumerate_list_item->fontsize = attributeItem->fontsize * 2;
                enumerate_list_item->aligning = attributeItem->aligning;
                break;
            }
            else
            {
                continue;
            }
        }
        enumerate_list->items.push_back(enumerate_list_item);
        // 跳过空白字符
        this->skip_blank();
    }
    return enumerate_list;
}


/* 函数：解析图像 */
std::shared_ptr<AST::Figure>
TexParser::parse_figure()
{
    // 声明一个Information对象
    std::shared_ptr<AST::Figure> figure = std::make_shared<AST::Figure>();
    // 跳过空白字符
    this->skip_blank();
    // 提取格式属性
    size_t p_start = this->scan_p;
    size_t p_end = this->tex_content.find("\\end{figure}", p_start);
    this->scan_p = p_end;
    this->match("\\end{figure}");
    std::string image_string = this->tex_content.substr(p_start, p_end - p_start);    // 读取属性文字
    if (image_string.find("\\label{") != std::string::npos)
    {
        // 引用标签
        size_t p_start = image_string.find("\\label{") + 7;
        size_t p_end = image_string.find("}", p_start);
        figure->ref_label = image_string.substr(p_start, p_end - p_start);
    }
    if (image_string.find("\\caption{") != std::string::npos)
    {
        // 题注
        size_t p_start = image_string.find("\\caption{") + 9;
        size_t p_end = image_string.find("}", p_start);
        std::shared_ptr<AST::Text> caption_text = std::make_shared<AST::Text>();
        caption_text->content = image_string.substr(p_start, p_end - p_start);
        // 查看template_attribute中的整体属性
        for (AST::BlockElement& attribute_item : this->template_attribute->items)
        {
            if (auto attrItemPtr = std::get_if<std::shared_ptr<AST::AttributeItem>>(&attribute_item))
            {
                std::shared_ptr<AST::AttributeItem> attributeItem = *attrItemPtr;
                caption_text->fontsize = attributeItem->fontsize * 2;
                caption_text->aligning = CENTER;
                break;
            }
            else
            {
                continue;
            }
        }
        figure->caption.push_back(caption_text);
    }
    // 声明一个Image对象并写入
    std::shared_ptr<AST::Image> image = std::make_shared<AST::Image>();
    p_start = image_string.find("\\includegraphics");
    p_start = image_string.find("{", p_start) + 1;
    p_end = image_string.find("}", p_start);
    std::string image_path_pdf = image_string.substr(p_start, p_end - p_start);
    // pdf转png，并获取图像参数
    std::tuple<int, int, std::string> image_param = this->parse_figure_image(image_path_pdf);
    image->width = std::get<0>(image_param);
    image->height = std::get<1>(image_param);
    image->path = std::get<2>(image_param);
    // 存入Figure对象
    figure->image.push_back(image);
    return figure;
}

/* 函数：pdf转Png，并获取图像参数 */
std::tuple<int, int, std::string>
TexParser::parse_figure_image(const std::string& image_path_pdf)
{
    // pdf转png
    FPDF_InitLibrary(); // 初始化Pdfium库
    // 加载pdf文件
    FPDF_DOCUMENT image_pdf = FPDF_LoadDocument((this->figure_dir + image_path_pdf).c_str(), "");
    // 加载单页数据，页码从0开始
    FPDF_PAGE image_pdf_page = FPDF_LoadPage(image_pdf, 0);
    // 获取页面数据宽高
    double page_width_pt = FPDF_GetPageWidth(image_pdf_page);   // point
    double page_height_pt = FPDF_GetPageHeight(image_pdf_page); // point
    double target_dpi = 160.0; // 目标分辨率
    int width = static_cast<int>(page_width_pt * target_dpi / 72.0);    // 1 point = 1/72 inch
    int height = static_cast<int>(page_height_pt * target_dpi / 72.0);
    //创建PDF图片结构体
    FPDF_BITMAP bitmap = FPDFBitmap_CreateEx(width, height, FPDFBitmap_BGR, NULL, 0);
    //图片背景涂色
    FPDFBitmap_FillRect(bitmap, 0, 0, width, height, 0xFFFFFFFF);
    // 渲染图片
    FPDF_RenderPageBitmap(bitmap, image_pdf_page, 0, 0, width, height, 0, 0);
    //获取图片指针
    unsigned char* buffer = (unsigned char*)FPDFBitmap_GetBuffer(bitmap);
    int stride = FPDFBitmap_GetStride(bitmap);
    // BGR -> RGB
    std::vector<unsigned char> rgb_buffer(width * height * 3);
    for (int y = 0; y < height; y++) 
    {
        unsigned char* src_row = buffer + y * stride;
        unsigned char* dst_row = rgb_buffer.data() + y * width * 3;
        for (int x = 0; x < width; x++) 
        {
            dst_row[x * 3 + 0] = src_row[x * 3 + 2]; // R
            dst_row[x * 3 + 1] = src_row[x * 3 + 1]; // G
            dst_row[x * 3 + 2] = src_row[x * 3 + 0]; // B
        }
    }
    // 保存为图片
    std::string image_path_png = this->figure_dir + image_path_pdf.substr(0, image_path_pdf.size() - 4) + ".png";
    //stbi_write_png((this->figure_dir + image_path_png).c_str(), width, height, 3, rgb_buffer.data(), width * 3);
    if (!stbi_write_png(image_path_png.c_str(), width, height, 3, rgb_buffer.data(), width * 3)) 
    {
    throw std::runtime_error("Failed to write PNG: " + image_path_png);
    }
    // 释放图片结构体
    FPDFBitmap_Destroy(bitmap);
    // 释放打开的页
    FPDF_ClosePage(image_pdf_page);
    // 释放pdf文件
    FPDF_CloseDocument(image_pdf);
    // 释放库
    FPDF_DestroyLibrary();
    return { width, height, image_path_png };
}


/* 函数：解析公式 */
std::shared_ptr<AST::Equation>
TexParser::parse_equation()
{
    // 声明一个Information对象
    std::shared_ptr<AST::Equation> equation = std::make_shared<AST::Equation>();
    // 跳过空白字符
    this->skip_blank();
    // 提取格式属性
    size_t p_start = this->scan_p, p_end;
    bool isEquation = false, isAlign = false;
    if (this->tex_content.find("\\end{equation}", p_start) != std::string::npos && this->tex_content.find("\\end{align}", p_start) != std::string::npos)
    {
        if (this->tex_content.find("\\end{equation}", p_start) < this->tex_content.find("\\end{align}", p_start))
        {
            isEquation = true;
        }
        else 
        {
            isAlign = true;
        }
    }
    else if (this->tex_content.find("\\end{equation}", p_start) != std::string::npos)
    {
        isEquation = true;
    }
    else if (this->tex_content.find("\\end{align}", p_start) != std::string::npos)
    {
        isAlign = true;
    }
    if (isEquation)
    {
        p_end = this->tex_content.find("\\end{equation}", p_start);
        this->scan_p = p_end;
        this->match("\\end{equation}");
    }
    else if (isAlign)
    {
        p_end = this->tex_content.find("\\end{align}", p_start);
        this->scan_p = p_end;
        this->match("\\end{align}");
    }
    std::string equation_string = this->tex_content.substr(p_start, p_end - p_start);    // 读取属性文字
    // 读取公式和引用标签
    std::shared_ptr<AST::Math> equation_math;
    if (equation_string.find("\\label{") != std::string::npos)
    {
        size_t p_temp = 0;
        while (equation_string.find("\\label{", p_temp) != std::string::npos)
        {
            equation_string.erase(0, equation_string.find_first_not_of(" \t\n\r\f\v"));
            equation_string.erase(equation_string.find_last_not_of(" \t\n\r\f\v") + 1);
            equation_string.erase(std::remove_if(equation_string.begin(), equation_string.end(), is_valid_char), equation_string.end());
            p_start = equation_string.find("\\label{", p_temp) + 7;
            p_end = equation_string.find("}", p_start);
            equation->ref_label = equation_string.substr(p_start, p_end - p_start);
            p_temp = p_end;
            // 取出该子公式
            equation_math = std::make_shared<AST::Math>();
            size_t p_temp_i = 0;
            std::string equation_latex_math;
            if (equation_string.find("\\begin{split}") != std::string::npos)
            {
                p_temp_i = equation_string.find("\\begin{split}") + 13;
                equation_latex_math = equation_string.substr(p_temp_i, p_start - 7 - p_temp_i);
            }
            else 
            {
                equation_latex_math = equation_string.substr(p_temp_i, p_start - 7);
            }
            std::regex pattern_setlength(R"(\\setlength\s*\{[^{}]*\}\s*\{[^{}]*\})");
            equation_latex_math = std::regex_replace(equation_latex_math, pattern_setlength, "");
            std::regex pattern_mkern(R"(\\mkern-3mu)");
            equation_latex_math = std::regex_replace(equation_latex_math, pattern_mkern, "");
            equation_latex_math = std::regex_replace(equation_latex_math, std::regex(R"(\\!)"), "");
            equation_math->latex_math = equation_latex_math;
            // 删除equation_string中的标签信息，为寻找可能存在的下一个标签
            std::regex pattern_label("\\\\label\\{" + equation->ref_label + "\\}");
            equation_string = std::regex_replace(equation_string, pattern_label, "");
            ++this->equation_index;
            equation_math->index = this->equation_index;
        }
    }
    else 
    {
        // 声明一个Math对象并写入
        equation_math = std::make_shared<AST::Math>();
        equation_string.erase(0, equation_string.find_first_not_of(" \t\n\r\f\v"));
        equation_string.erase(equation_string.find_last_not_of(" \t\n\r\f\v") + 1);
        equation_string.erase(std::remove_if(equation_string.begin(), equation_string.end(), is_valid_char), equation_string.end());
        size_t p_temp_i = 0;
        std::string equation_latex_math;
        if (equation_string.find("\\begin{split}") != std::string::npos)
        {
            p_temp_i = equation_string.find("\\begin{split}") + 13;
            equation_latex_math = equation_string.substr(p_temp_i, equation_string.size() - p_temp_i - 11);
        }
        else
        {
            equation_latex_math = equation_string;
        }
        std::regex pattern_setlength(R"(\\setlength\s*\{[^{}]*\}\s*\{[^{}]*\})");
        equation_latex_math = std::regex_replace(equation_latex_math, pattern_setlength, "");
        std::regex pattern_mkern(R"(\\mkern-3mu)");
        equation_latex_math = std::regex_replace(equation_latex_math, pattern_mkern, "");
        equation_latex_math = std::regex_replace(equation_latex_math, std::regex(R"(\\!)"), "");
        equation_math->latex_math = equation_latex_math;
        ++this->equation_index;
        equation_math->index = this->equation_index;
    }
    // 查看template_attribute中的整体属性
    for (AST::BlockElement& attribute_item : this->template_attribute->items)
    {
        if (auto attrItemPtr = std::get_if<std::shared_ptr<AST::AttributeItem>>(&attribute_item))
        {
            std::shared_ptr<AST::AttributeItem> attributeItem = *attrItemPtr;
            equation->fontsize = attributeItem->fontsize * 2;
            equation->aligning = CENTER;
            break;
        }
        else
        {
            continue;
        }
    }
    equation->content.push_back(equation_math);
    return equation;
}

/* 辅助函数：保留公式中间的字母、数字、空格 */
bool is_valid_char(char c) 
{
    return c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}


/* 函数：解析表格 */
std::shared_ptr<AST::Table>
TexParser::parse_table()
{
    // 声明一个Table对象
    std::shared_ptr<AST::Table> table = std::make_shared<AST::Table>();
    // 跳过空白字符
    this->skip_blank();
    // 提取格式属性
    size_t p_start = this->scan_p;
    size_t p_end = this->tex_content.find("\\end{table}", p_start);
    this->scan_p = p_end;
    this->match("\\end{table}");
    std::string table_string = this->tex_content.substr(p_start, p_end - p_start);    // 读取属性文字

    // 提取ref_label
    std::regex label_regex(R"(\\label\{([^}]+)\})");
    std::smatch label_match;
    if (std::regex_search(table_string, label_match, label_regex)) 
    {
        table->ref_label = label_match[1];
    }

    // 提取caption
    std::regex caption_regex(R"(\\caption\{([^}]+)\})");
    std::smatch caption_match;
    if (std::regex_search(table_string, caption_match, caption_regex)) 
    {
        table->caption = caption_match[1];
    }

    // 提取表格内容
    std::regex tabular_regex(R"(\\begin\{tabular\}([^}]+)\}([\s\S]*?)\\end\{tabular\})");
    std::smatch tabular_match;

    if (std::regex_search(table_string, tabular_match, tabular_regex)) 
    {
        std::string column_definitions = tabular_match[1];
        std::string table_content = tabular_match[2];

        // 计算列数
        table->column_count = std::count(column_definitions.begin(), column_definitions.end(), 'c');
        size_t p_temp_row = 0;
        while (table_string.find("\\\\\n", p_temp_row) != std::string::npos)    // 以双斜杠和换行符作为字符串换行切分标准
        {
            
            p_temp_row = table_string.find("\\\\\n", p_temp_row) + 3;
            ++table->row_count;
        }
        if (table_string.find("\\bottomrule") == std::string::npos)
        {
            // 无bottomrule作为最后一行则最后一行没有“\\”，需要另外加一计数
            ++table->row_count;
        }
        
        // 分割行
        std::regex row_regex(R"(\\\\[ \t]*(?:\r?\n)?)");
        std::vector<std::string> row_strings = split_string(table_content, row_regex);

        // 处理每一行
        size_t row_index = 0;
        for (auto& row_str : row_strings) 
        {
            // 跳过空行和规则行
            if (row_str.empty() || row_str.find("\\toprule") != std::string::npos || row_str.find("\\midrule") != std::string::npos || row_str.find("\\bottomrule") != std::string::npos) 
            {
                std::shared_ptr<AST::TableCell> separator_cell = std::make_shared<AST::TableCell>();
                //std::regex pattern_rule(R"(\\\s*(top|mid|bottom|cmid)rule\s*)");
                
                std::regex pattern_rule(PATTERN_RULE);
                std::smatch match;
                if (std::regex_search(row_str, match, pattern_rule)) 
                {
                    separator_cell->content = match.str(); // 保存完整的规则命令
                    table->cells.push_back(separator_cell);
                    if (row_str.find("bottomrule") != std::string::npos)
                    {
                        break;
                    }
                    // 删除完整的规则命令（包括反斜杠和规则名称）
                    row_str = std::regex_replace(row_str, pattern_rule, "");
                }
            }

            // 删除前后无关字符
            size_t start = row_str.find_first_not_of(" \t\n\r");
            size_t end = row_str.find_last_not_of(" \t\n\r");
            row_str = row_str.substr(start, end - start + 1);

            // 分割单元格
            std::vector<std::string> tablecell_string = splitByAnd(row_str);
            
            // 处理该行的各列单元格
            size_t col_index = 0;
            for (std::string tablecell_i_string : tablecell_string)
            {
                std::shared_ptr<AST::TableCell> tablecell = std::make_shared<AST::TableCell>();
                tablecell_i_string.erase(0, tablecell_i_string.find_first_not_of(" \t\n\r\f\v"));
                tablecell_i_string.erase(tablecell_i_string.find_last_not_of(" \t\n\r\f\v") + 1);
                
                tablecell->row_index = row_index;
                tablecell->col_index = col_index;

                // 处理multirow和\multicolumn
                if (tablecell_i_string.find("\\multirow") != std::string::npos)
                {
                    std::regex multirow_regex(R"(\\multirow\{([0-9]+)\}\{([^}]+)\}\{((?:[^{}]*(?:\{[^{}]*\}[^{}]*)*)*)\})");
                    std::smatch multirow_match;
                    std::regex_search(tablecell_i_string, multirow_match, multirow_regex);
                    tablecell->rowspan = static_cast<size_t>(std::stoi(multirow_match[1]));
                    tablecell->content = multirow_match[3];
                }
                else if (tablecell_i_string.find("\\\multicolumn") != std::string::npos)
                {
                    std::regex multicolumn_regex(R"(\\multicolumn\{([0-9]+)\}\{([^}]+)\}\{([^}]+)\})");
                    std::smatch multicolumn_match;
                    std::regex_search(tablecell_i_string, multicolumn_match, multicolumn_regex);
                    tablecell->colspan = static_cast<size_t>(std::stoi(multicolumn_match[1]));
                    tablecell->content = multicolumn_match[3];
                    col_index += tablecell->colspan - 1;    // 代表的是当前单元格最右侧的列索引
                }
                else 
                {
                    tablecell->content = tablecell_i_string;
                }
                table->cells.push_back(tablecell);
                ++col_index;
            }
            ++row_index;
        }
        // 查看template_attribute中的整体属性
        for (AST::BlockElement& attribute_item : this->template_attribute->items)
        {
            if (auto attrItemPtr = std::get_if<std::shared_ptr<AST::AttributeItem>>(&attribute_item))
            {
                std::shared_ptr<AST::AttributeItem> attributeItem = *attrItemPtr;
                table->fontsize = attributeItem->fontsize * 2;
                table->aligning = CENTER;
                break;
            }
            else
            {
                continue;
            }
        }
    }
    return table;
}


std::vector<std::string> 
split_string(const std::string& input, const std::regex& regex) 
{
    std::vector<std::string> result;
    std::sregex_token_iterator it(input.begin(), input.end(), regex, -1);
    std::sregex_token_iterator end;

    std::stack<std::string> temp_makecell_stack;
    while (it != end) 
    {
        if (!it->str().empty()) 
        {
            std::string current_it = it->str();
            if (current_it.find("\\makecell") != std::string::npos)
            {
                temp_makecell_stack.push(current_it + "\\\\");
                ++it;
                continue;
            }
            if (!temp_makecell_stack.empty())
            {
                while (!temp_makecell_stack.empty())
                {
                    current_it = temp_makecell_stack.top() + current_it;
                    temp_makecell_stack.pop();
                }
            }
            result.push_back(current_it);
        }
        ++it;
    }
    return result;
}


std::vector<std::string>
splitByAnd(const std::string& text)
{
    std::vector<std::string> result;
    std::stack<std::string> dollarStack;
    size_t p_start = 0, p_end = 0;
    while (p_end < text.size() - 1)
    {
        if (text.find("&", p_start) != std::string::npos && dollarStack.empty())
        {
            p_end = text.find("&", p_start);
            result.push_back(text.substr(p_start, p_end - p_start));
            p_start = p_end + 1;
            dollarStack.push("&");
        }
        else if (text.find("&", p_start) != std::string::npos)
        {
            p_end = text.find("&", p_start);
            result.push_back(text.substr(p_start, p_end - p_start));
            p_start = p_end + 1;
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


/* 函数：解析伪代码 */
std::shared_ptr<AST::CodeBlock>
TexParser::parse_algorithm()
{
    // 声明一个Information对象
    std::shared_ptr<AST::CodeBlock> algorithm = std::make_shared<AST::CodeBlock>();
    // 跳过空白字符
    this->skip_blank();
    // 提取格式属性
    size_t p_start = this->scan_p;
    size_t p_end = this->tex_content.find("\\end{algorithm}", p_start);
    this->scan_p = p_end;
    this->match("\\end{algorithm}");
    std::string algorithm_string = this->tex_content.substr(p_start, p_end - p_start);    // 读取属性文字
    // 声明一个Text对象并写入
    std::shared_ptr<AST::Code> code = std::make_shared<AST::Code>();
    code->content = algorithm_string;
    algorithm->code.push_back(code);
    return algorithm;
}


