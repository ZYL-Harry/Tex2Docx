// parser_reference.cpp
#include "parser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include"document_ast.h"
#include <stack>
#include"tex_attribute.h"
#include <regex>
#include<map>
#include<algorithm>


/* 函数：参考文献读取（ref） */
void
TexParser::parse_reference()
{
    // 读取模板文件
    std::ifstream file(this->ref_path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + this->ref_path + ".");
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string reference_content = buffer.str();
    //// 删除注释
    //size_t pos = 0;
    //while ((pos = reference_content.find('%', pos)) != std::string::npos)
    //{
    //    size_t end_of_line = reference_content.find('\n', pos);
    //    if (end_of_line != std::string::npos)
    //    {
    //        // 删除从“%”到“\n”之前的所有字符
    //        reference_content.erase(pos, end_of_line - pos);
    //    }
    //    else
    //    {
    //        // 删除从%到字符串末尾的所有内容
    //        reference_content.erase(pos);
    //    }
    //}

    // 解析参考文献
    size_t scan_p_temp = this->scan_p;
    std::shared_ptr<AST::References> references = std::make_shared<AST::References>();

    size_t p_start = 0, p_end = 0;
    while (reference_content.find("@", p_start) != std::string::npos)
    {
        p_start = reference_content.find("@", p_start);
        p_start = this->match(reference_content, "@", p_start);
        p_start = this->skip_blank(reference_content, p_start);
        p_end = this->find_bib_item_end(reference_content, p_start);
        std::string reference_string = reference_content.substr(p_start, p_end - p_start);
        

        std::shared_ptr<AST::Reference> reference = std::make_shared<AST::Reference>();
        reference->content = reference_string;

        int fontsize;
        std::string aligning;
        for (AST::BlockElement& attribute_item : this->template_attribute->items)
        {
            if (auto attrItemPtr = std::get_if<std::shared_ptr<AST::AttributeItem>>(&attribute_item))
            {
                std::shared_ptr<AST::AttributeItem> attributeItem = *attrItemPtr;
                fontsize = attributeItem->fontsize * 2;
                aligning = attributeItem->aligning;
                break;
            }
            else
            {
                continue;
            }
        }

        size_t p_start_key = reference_content.find("{", p_start);
        size_t p_end_key = reference_content.find(",", p_start_key);
        reference->cite_key = reference_content.substr(++p_start_key, p_end_key - p_start_key - 1);
        p_start = p_end;

        size_t p_temp_start = 0, p_temp_end = 0;
        while (reference_string.find("=", p_temp_start) != std::string::npos)
        {
            p_temp_start = reference_string.find(",", p_temp_start) + 1;
            p_temp_start = this->skip_blank(reference_string, p_temp_start);
            p_temp_end = reference_string.find("}", p_temp_start) + 1;
            /*if (!std::isspace(reference_string[p_temp_end]))
            {
                --p_temp_end;
            }*/
            std::string key_string = reference_string.substr(p_temp_start, p_temp_end - p_temp_start);
            std::regex pattern_item(R"((\w+)\s*=\s*\{([^}]+)\})");
            std::smatch match;
            std::regex_search(key_string, match, pattern_item);
            std::string key = match[1].str();
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            std::string value = match[2].str();
            std::shared_ptr<AST::ReferenceKey> reference_key = std::make_shared<AST::ReferenceKey>();
            reference_key->key = key;
            reference_key->content = value;
            reference_key->fontsize = fontsize;
            reference_key->aligning = aligning;
            reference->key.push_back(reference_key);
            p_temp_start = p_temp_end - 1;
        }
        
        references->reference.push_back(reference);
    }
    this->document_ast.content.push_back(references);
}

bool
isBibBracketBalanced(const std::string& expression) 
{
    // 定义括号映射：右括号 -> 对应的左括号
    std::unordered_map<char, char> bracketPairs = {
        {'}', '{'}
    };
    // 定义左括号集合，用于快速判断
    std::string leftBrackets = "{";
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


size_t
TexParser::find_bib_item_end(const std::string& template_content, size_t& p_start)
{
    size_t p_next_n = template_content.find("\n", p_start);
    std::string string_to_next_n = template_content.substr(p_start, p_next_n - p_start);
    while ((!isBibBracketBalanced(string_to_next_n)) == 1)
    {
        p_next_n = template_content.find("\n", ++p_next_n);
        string_to_next_n = template_content.substr(p_start, p_next_n - p_start);
    }
    return p_next_n;
}

