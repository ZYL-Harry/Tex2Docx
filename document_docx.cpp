#include"document_docx.h"
#include"pugixml.hpp"
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>
#include"utils_write_xml.h"
#include<iostream>
#include"generator.h"
#include"tex_attribute.h"
#include"parser.h"


static size_t pictCount = 0;


void 
DocxDocument::saveAs(const std::string& filename)
{
    Zip::open(filename, OpenMode::Create);
    flush();
    Zip::close();
}


void 
DocxDocument::flush()
{
    writeOfficeDocument();
    md::Document::writeStyles();
    md::Document::writeNumDefinitions();
    Package::flush();
}


void 
DocxDocument::writeOfficeDocument()
{
    pugi::xml_document doc;
    pugi::xml_node root = doc.append_child("w:document");
    //doc.append_child(pugi::node_declaration).append_attribute("encoding") = "UTF-8";
    root.append_attribute("xmlns:w")
        .set_value("http://schemas.openxmlformats.org/wordprocessingml/2006/main");
    root.append_attribute("xmlns:r")
        .set_value("http://schemas.openxmlformats.org/officeDocument/2006/relationships");
    root.append_attribute("xmlns:wp")
        .set_value("http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing");
    root.append_attribute("xmlns:m")
        .set_value("http://schemas.openxmlformats.org/officeDocument/2006/math");

    pugi::xml_node body = root.append_child("w:body");

    if (this->sections().size() == 0)
        addSection();
    //std::cout << this->sections().size() << std::endl;
    std::list<md::SectionPointer> sections_ = this->sections();
    for (auto iter = sections_.begin(); iter != std::prev(sections_.end()); iter++) {
        auto& ptr = *iter;

        if (ptr->blocks().size() == 0)
            ptr->addParagraph();
        //std::cout << ptr->blocks().size() << std::endl;
        pugi::xml_node w_p;
        for (auto& block : ptr->blocks())
            w_p = this->writeBlock(body, *block);

        if (ptr->blocks().back()->type() == md::BlockType::Table) {}
        w_p = body.append_child("w:p");

        pugi::xml_node w_pPr = w_p.child("w:pPr");
        if (!w_pPr)
            w_pPr = w_p.append_child("w:pPr");

        writeSectionProperties(w_pPr, ptr->prop_);
    }

    auto& ptr = sections_.back();
    if (ptr->blocks().size() == 0)
        ptr->addParagraph();

    for (auto& block : ptr->blocks())
        this->writeBlock(body, *block);

    writeSectionProperties(body, ptr->prop_);

    writePart(mainPart_, doc);
    // 打印 doc 节点的 XML 结构
    /*std::cout << "doc node structure (manual traversal):" << std::endl;
    printNode(doc);*/
    /*std::string filename = "doc.txt";
    doc.save_file(filename.c_str(), "  ");*/
    pictCount = 0;
}


pugi::xml_node 
DocxDocument::writeBlock(pugi::xml_node w_body, md::Block& block)
{
    switch (block.type())
    {
    case md::BlockType::Paragraph:
        return writeParagraph(w_body, dynamic_cast<md::Paragraph&>(block));

    case md::BlockType::Table:
        return writeTable(w_body, dynamic_cast<md::Table&>(block));

    default:
        return pugi::xml_node();
    }
}


pugi::xml_node 
DocxDocument::writeParagraph(pugi::xml_node w_body, md::Paragraph& para)
{
    pugi::xml_node w_p = w_body.append_child("w:p");
    pugi::xml_node w_pPr = w_p.append_child("w:pPr");

    if (para.numId_ > 0) {
        pugi::xml_node w_numPr = w_pPr.append_child("w:numPr");
        w_numPr.append_child("w:numId").append_attribute("w:val") = para.numId_;
        w_numPr.append_child("w:ilvl").append_attribute("w:val") = static_cast<unsigned int>(para.level_);
    }

    writeParagraphProperties(w_pPr, para.prop_);

    for (auto& run : para.runs())
        writeRun(w_p, *run);

    return w_p;
}


void 
DocxDocument::writeRun(pugi::xml_node w_p, md::Run& run)
{
    switch (run.type())
    {
    case md::RunType::RichText:
        writeRichText(w_p, dynamic_cast<md::RichText&>(run));
        break;

    case md::RunType::Picture:
        writePicture(w_p, dynamic_cast<md::Picture&>(run));
        break;
    }
}


void 
DocxDocument::writeRichText(pugi::xml_node w_p, const md::RichText& rich)
{
    std::vector<std::string> rich_text_list = splitByDollar(rich.text());
    for (std::string rich_text : rich_text_list)
    {
        if (size_t p = rich_text.find("$") == 0)
        {
            if (rich_text.find("$", p) == rich_text.size() - 1)
            {
                // 短公式
                pugi::xml_node m_omath = w_p.append_child("m:oMath");
                pugi::xml_node m_r = m_omath.append_child("m:r");
                pugi::xml_node w_rPr = m_r.append_child("w:rPr");
                writeRichTextProperties(w_rPr, rich.prop_);
                // pugi::xml_node w_rFonts = w_rPr.append_child("w:rFonts");
                // w_rFonts.append_attribute("w:ascii") = "Cambria Math";

                std::string formula_text = rich_text.substr(1, rich_text.size() - 2);
                writeFormula(m_r, formula_text, formula_text.size(), rich.prop_.whitespace_);
            }
        }
        else
        {
            // 普通文本
            pugi::xml_node w_r = w_p.append_child("w:r");
            pugi::xml_node w_rPr = w_r.append_child("w:rPr");

            writeRichTextProperties(w_rPr, rich.prop_);

            writeText(w_r, rich_text.c_str(), rich_text.size(), rich.prop_.whitespace_);
        }
    }
}


void 
DocxDocument::writeFormula(pugi::xml_node m_r, const std::string& formula_text, const size_t len, const bool whitespace)
{
    // 创建文本元素 (t)
    pugi::xml_node m_t = m_r.append_child("m:t");
    // 如果需要保留空格，设置属性
    if (whitespace) 
    {
        m_t.append_attribute("xml:space") = "preserve";
    }
    // 文本
    latex2omml(m_r, formula_text);
}


void
DocxDocument::latex2omml(pugi::xml_node m_r, const std::string& formula_latex)
{
    size_t p = 0;

    while (p < formula_latex.size())
    {
        std::string formula_latex_temp = formula_latex.substr(p, formula_latex.size() - p);
        std::shared_ptr<FormulaNode> formula_tree = std::make_shared<FormulaNode>();
        if (formula_latex_temp.find("^") != std::string::npos || formula_latex_temp.find("_") != std::string::npos)
        {
            p += writeComplexFormula(m_r, p, formula_latex_temp, formula_tree);

            //std::tuple<size_t, std::vector<std::string>> results = splitBiggestSupSub(formula_latex_temp);    // 分解出：根节点、子节点（一个或两个）
            //p += std::get<0>(results);
            //formula_tree = searchSubTree(formula_latex_temp, formula_tree);
            //writeTree(m_r, formula_tree);
        }
        else
        {
            formula_tree->data = formula_latex_temp;
            writeTree(m_r, formula_tree);
            break;
        }
    }
}


size_t
DocxDocument::writeComplexFormula(pugi::xml_node m_r, size_t p, const std::string& formula_latex, std::shared_ptr<FormulaNode>& formula_tree)
{
    std::tuple<size_t, std::vector<std::string>> results = splitBiggestSupSub(formula_latex);
    size_t p_end = std::get<0>(results);
    std::vector<std::string> text_vector = std::get<1>(results);
    std::vector<std::string> bracket_frac = {
        "\\left(", "\\left[", "\\left\\{", "\\left\\langle",
        "\\langle", "\\begin{cases}",
        "\\bigl", "\\Bigl", "\\biggl", "\\Biggl","\\|",
        "\\frac", "\\tfrac"
    };
    bool has_bracket_frac = std::any_of(bracket_frac.begin(), bracket_frac.end(),
        [&](const std::string& token) { return text_vector[0].find(token) != std::string::npos; });
    if (!has_bracket_frac)
    {
        // 退出条件：根节点不存在left-right式括号（包括case型和bigg型）及匹配性 & 分数
        size_t p_temp = 0, p_prev = 0;
        while (p_temp < formula_latex.size())
        {
            std::string formula_latex_temp = formula_latex.substr(p_temp, formula_latex.size() - p_temp);
            results = splitBiggestSupSub(formula_latex_temp);    // 分解出：根节点、子节点（一个或两个）
            text_vector = std::get<1>(results);
            has_bracket_frac = std::any_of(bracket_frac.begin(), bracket_frac.end(),
                [&](const std::string& token) { return text_vector[0].find(token) != std::string::npos; });
            if (!has_bracket_frac)
            {
                p_temp += std::get<0>(results);
                formula_tree = std::make_shared<FormulaNode>(); // 初始化公式树
                formula_tree = searchSubTree(formula_latex_temp, formula_tree);
                writeTree(m_r, formula_tree);
            }
            else 
            {
                formula_tree = std::make_shared<FormulaNode>(); // 初始化公式树
                p_temp += writeComplexFormula(m_r, p_temp, formula_latex.substr(p_temp, formula_latex.size() - p_temp), formula_tree);
            }
            p_prev += std::get<0>(results);
        }
        p = p_temp;
        return p;
    }
    
    // 处理前序数据
    std::vector<size_t> p_ends_bracket = {
        formula_latex.find("\\left"),
        formula_latex.find("\\langle"),
        formula_latex.find("\\begin{cases}"),
        formula_latex.find("\\bigl"),
        formula_latex.find("\\Bigl"),
        formula_latex.find("\\biggl"),
        formula_latex.find("\\Biggl"),
        formula_latex.find("\\|")
    };
    p_ends_bracket.erase(std::remove(p_ends_bracket.begin(), p_ends_bracket.end(), std::string::npos), p_ends_bracket.end());
    size_t p_end_bracket = *std::min_element(p_ends_bracket.begin(), p_ends_bracket.end());
    std::vector<size_t> p_ends_frac = {
        formula_latex.find("\\frac"),
        formula_latex.find("\\tfrac")
    };
    p_ends_frac.erase(std::remove(p_ends_frac.begin(), p_ends_frac.end(), std::string::npos), p_ends_frac.end());
    size_t p_end_frac = *std::min_element(p_ends_frac.begin(), p_ends_frac.end());
    p_end = std::min(p_end_bracket, p_end_frac);
    p = p_end;
    std::string formula_latex_prior = formula_latex.substr(0, p_end);
    formula_tree = searchSubTree(formula_latex_prior, formula_tree);
    writeTree(m_r, formula_tree);

    // 处理后续复杂数据
    std::string formula_latex_complex = formula_latex.substr(p_end, formula_latex.size() - p_end);
    bool isBracketExist = formula_latex.find("\\left") != std::string::npos || 
        formula_latex.find("\\langle") != std::string::npos ||
        formula_latex.find("\\begin{cases}") != std::string::npos || 
        formula_latex.find("\\bigl") != std::string::npos || 
        formula_latex.find("\\Bigl") != std::string::npos || 
        formula_latex.find("\\biggl") != std::string::npos || 
        formula_latex.find("\\Biggl") != std::string::npos ||
        formula_latex.find("\\|") != std::string::npos;
    bool isFracExist = formula_latex.find("\\frac") != std::string::npos ||
        formula_latex.find("\\tfrac") != std::string::npos;
    if (isBracketExist && isFracExist)
    {
        // 同时存在left-right式括号（包括case型）&分数
        if (p_end_bracket < p_end_frac)
        {
            p += processBracketComplexFormula(m_r, formula_latex_complex);
        }
        else
        {
            p += processFracComplexFormula(m_r, formula_latex_complex);
        }
    }
    else if (isBracketExist && !isFracExist)
    {
        // 仅存在left-right式括号（包括case型）
        p += processBracketComplexFormula(m_r, formula_latex_complex);
    }
    else if (!isBracketExist && isFracExist)
    {
        // 仅存在分数
        p += processFracComplexFormula(m_r, formula_latex_complex);
    }
    return p;
}


size_t
DocxDocument::processBracketComplexFormula(pugi::xml_node m_r, std::string& formula_latex_complex)
{
    size_t p = 0;
    std::string left_bracket, right_bracket;
    size_t p_start = formula_latex_complex.size(), p_end;
    for (const std::pair<const std::string, const std::string>& bracket : BracketPairs)
    {
        std::string left_bracket_temp = bracket.first;
        // 遍历括号对，找出最早出现的左括号，并取对应右括号
        if (formula_latex_complex.find(left_bracket_temp) < p_start)
        {
            p_start = formula_latex_complex.find(left_bracket_temp);
            left_bracket = bracket.first;
            right_bracket = bracket.second;
        }
    }
    // 取出括号中内容
    p_end = formula_latex_complex.find(right_bracket);
    if (p_start != std::string::npos && p_end != std::string::npos)
    {
        std::string formula_latex_complex_temp = formula_latex_complex.substr(p_start + left_bracket.size(), p_end - (p_start + left_bracket.size()));
        while (!isComplexBracketBalanced(formula_latex_complex_temp, BracketPairs))
        {
            p_end = formula_latex_complex.find(right_bracket, ++p_end);
            formula_latex_complex_temp = formula_latex_complex.substr(p_start + left_bracket.size(), p_end - (p_start + left_bracket.size()));
        }
        p += p_start + left_bracket.size();
        while (formula_latex_complex_temp.size() < (formula_latex_complex.size() - p - right_bracket.size()))
        {
            // 括号结构根节点
            pugi::xml_node m_d;
            // 对于括号带上下标的情况："\\left\\{\\Sigma_{i=1}^{C}R\\left(h\\right)_{i}\\right\\}_{e=1}^{E}"
            size_t p_next = formula_latex_complex_temp.size() + p + right_bracket.size();
            bool hasSupSub = false;
            pugi::xml_node m_e_r;   // 基数根节点
            if (formula_latex_complex[p_next] == '_' || formula_latex_complex[p_next] == '^')
            {
                hasSupSub = true;
                // 判断上下标类型
                std::string formula_latex_complex_clone = formula_latex_complex;
                std::string formula_latex_complex_temp_replaced = formula_latex_complex_clone.replace(0, p_next, "A");
                // 初始化formula_tree
                std::shared_ptr<FormulaNode> formula_tree_search = std::make_shared<FormulaNode>();
                formula_tree_search = searchSubTree(formula_latex_complex_temp_replaced, formula_tree_search);
                // 创建上下标结构
                if (formula_tree_search->sup && formula_tree_search->sub)
                {
                    // 创建上下标结构
                    pugi::xml_node m_subsup = m_r.append_child("m:sSubSup");
                    // 创建基数写入结构
                    m_e_r = m_subsup.append_child("m:e");
                    // 写入上标
                    pugi::xml_node m_sup_element = m_subsup.append_child("m:sup");
                    writeTree(m_sup_element, formula_tree_search->sup);
                    p += formula_tree_search->sup->data.size() + 3; // 3-^、{、}
                    // 写入下标
                    pugi::xml_node m_sub_element = m_subsup.append_child("m:sub");
                    writeTree(m_sub_element, formula_tree_search->sub);
                    p += formula_tree_search->sub->data.size() + 3; // 3-_、{、}
                }
                else if (formula_tree_search->sup)
                {
                    // 创建上标结构
                    pugi::xml_node m_sup = m_r.append_child("m:sSup");
                    // 写入基数字符
                    m_e_r = m_sup.append_child("m:e");
                    // 写入上标
                    pugi::xml_node m_sup_element = m_sup.append_child("m:sup");
                    writeTree(m_sup_element, formula_tree_search->sup);
                    p += formula_tree_search->sup->data.size() + 3; // 3-^、{、}
                }
                else if (formula_tree_search->sub)
                {
                    // 创建下标结构
                    pugi::xml_node m_sub = m_r.append_child("m:sSub");
                    // 写入基数字符
                    m_e_r = m_sub.append_child("m:e");
                    // 写入下标
                    pugi::xml_node m_sub_element = m_sub.append_child("m:sub");
                    writeTree(m_sub_element, formula_tree_search->sub);
                    p += formula_tree_search->sub->data.size() + 3; // 3-_、{、}
                }
                m_d = m_e_r.append_child("m:d");
            }
            else 
            {
                m_d = m_r.append_child("m:d");
            }
            // 根据匹配到的括号对创建括号节点
            pugi::xml_node m_dPr = m_d.append_child("m:dPr");
            pugi::xml_node m_begChr = m_dPr.append_child("m:begChr");
            std::unordered_map<std::string, std::string>::const_iterator it_left = spectialCharacterMap.find(left_bracket);
            m_begChr.append_attribute("m:val") = it_left->second;
            pugi::xml_node m_sepChr = m_dPr.append_child("m:sepChr");
            m_sepChr.append_attribute("m:val") = "";
            pugi::xml_node m_endChr = m_dPr.append_child("m:endChr");
            std::unordered_map<std::string, std::string>::const_iterator it_right = spectialCharacterMap.find(right_bracket);
            m_endChr.append_attribute("m:val") = it_right->second;
            // 数值部分
            pugi::xml_node m_e = m_d.append_child("m:e");
            // 初始化formula_tree
            std::shared_ptr<FormulaNode> formula_tree = std::make_shared<FormulaNode>();
            //p += writeComplexFormula(m_e, 0, formula_latex_complex_temp, formula_tree) + right_bracket.size();
            size_t p_current = writeComplexFormula(m_e, 0, formula_latex_complex_temp, formula_tree);
            while (p_current < formula_latex_complex_temp.size())    // 已提取的括号中间内容 vs. 括号中的实际内容
            {
                // 初始化formula_tree
                formula_tree = std::make_shared<FormulaNode>();
                p_current += writeComplexFormula(m_e, 0, formula_latex_complex_temp.substr(p_current, formula_latex_complex_temp.size() - p_current), formula_tree);
            }
            p += p_current + right_bracket.size();
            // 后续部分
            if (!hasSupSub)
            {
                p_end = formula_latex_complex_temp.size() + p_start + left_bracket.size() + right_bracket.size();
                p_current = writeComplexFormula(m_r, 0, formula_latex_complex.substr(p_end, formula_latex_complex.size() - p_end), formula_tree);
                while (p_current < formula_latex_complex.substr(p_end, formula_latex_complex.size() - p_end).size())    // 已提取的括号中间内容 vs. 括号中的实际内容
                {
                    p_current += writeComplexFormula(m_r, 0, formula_latex_complex.substr(p_end + p_current, formula_latex_complex.size() - p_current - p_end), formula_tree);
                }
                p += p_current + right_bracket.size();
            }
            return p;
        }
        // 根据匹配到的括号对创建括号节点
        pugi::xml_node m_d = m_r.append_child("m:d");
        pugi::xml_node m_dPr = m_d.append_child("m:dPr");
        pugi::xml_node m_begChr = m_dPr.append_child("m:begChr");
        std::unordered_map<std::string, std::string>::const_iterator it_left = spectialCharacterMap.find(left_bracket);
        m_begChr.append_attribute("m:val") = it_left->second;
        pugi::xml_node m_sepChr = m_dPr.append_child("m:sepChr");
        m_sepChr.append_attribute("m:val") = "";
        pugi::xml_node m_endChr = m_dPr.append_child("m:endChr");
        std::unordered_map<std::string, std::string>::const_iterator it_right = spectialCharacterMap.find(right_bracket);
        m_endChr.append_attribute("m:val") = it_right->second;
        // 数值部分
        pugi::xml_node m_e = m_d.append_child("m:e");
        // 初始化formula_tree
        std::shared_ptr<FormulaNode> formula_tree = std::make_shared<FormulaNode>();
        size_t p_current = writeComplexFormula(m_e, 0, formula_latex_complex_temp, formula_tree);
        while (p_current < formula_latex_complex_temp.size())    // 已提取的括号中间内容 vs. 括号中的实际内容
        {
            p_current += writeComplexFormula(m_e, 0, formula_latex_complex_temp.substr(p_current, formula_latex_complex_temp.size() - p_current), formula_tree);
        }
        p += p_current + right_bracket.size();
        //p += writeComplexFormula(m_e, 0, formula_latex_complex_temp, formula_tree) + right_bracket.size();
    }
    return p;
}

bool
isComplexBracketBalanced(const std::string& formula, const std::unordered_map<std::string, std::string>& BracketPairs)
{
    std::stack<std::string> st;

    for (size_t i = 0; i < formula.size();) 
    {
        bool matched = false;
        // 遍历 BracketPairs，尝试匹配左右符号
        for (auto& [left, right] : BracketPairs) 
        {
            // 特殊情况：对称括号
            if (left == right) {
                if (formula.compare(i, left.size(), left) == 0) 
                {
                    if (!st.empty() && st.top() == left) 
                    {
                        // 栈顶是同一个符号 → 这是右括号
                        st.pop();
                    }
                    else 
                    {
                        // 否则作为左括号
                        st.push(left);
                    }
                    i += left.size();
                    matched = true;
                    break;
                }
            }
            // 普通括号：先检查左括号
            if (formula.compare(i, left.size(), left) == 0) 
            {
                st.push(left);
                i += left.size();
                matched = true;
                break;
            }
            // 普通括号：再检查右括号
            if (formula.compare(i, right.size(), right) == 0) 
            {
                if (st.empty() || BracketPairs.at(st.top()) != right) 
                {
                    return false; // 不匹配
                }
                st.pop();
                i += right.size();
                matched = true;
                break;
            }
        }
        // 没匹配到括号，跳过普通字符
        if (!matched) 
        {
            i++;
        }
    }
    return st.empty();
}

size_t
DocxDocument::processFracComplexFormula(pugi::xml_node m_r, const std::string& formula_latex_complex)
{
    pugi::xml_node m_f = m_r.append_child("m:f");
    size_t p = formula_latex_complex.find("{");
    // 分子
    size_t p_num_end = find_supsub_item_end(formula_latex_complex, p);
    std::string formula_num = formula_latex_complex.substr(p, p_num_end - p);
    pugi::xml_node m_num = m_f.append_child("m:num");
    // 初始化formula_tree
    std::shared_ptr<FormulaNode> formula_tree_num = std::make_shared<FormulaNode>();
    p += writeComplexFormula(m_num, 0, formula_num.substr(1, formula_num.size() - 2), formula_tree_num) + 2;    // 2-括号
    // 分母
    pugi::xml_node m_den = m_f.append_child("m:den");
    size_t p_den_end = find_supsub_item_end(formula_latex_complex, p_num_end);
    std::string formula_den = formula_latex_complex.substr(p_num_end, p_den_end - p_num_end);
    // 初始化formula_tree
    std::shared_ptr<FormulaNode> formula_tree_den = std::make_shared<FormulaNode>();
    p += writeComplexFormula(m_den, 0, formula_den.substr(1, formula_den.size() - 2), formula_tree_den) + 2;    // 2-括号
    return p;
}



std::shared_ptr<FormulaNode>
DocxDocument::searchSubTree(const std::string& formula_latex, std::shared_ptr<FormulaNode>& formula_tree)
{
    // 找到该字符串中上下标的范围，并取出
    std::tuple<size_t, std::vector<std::string>> results = splitBiggestSupSub(formula_latex);    // 分解出：根节点、子节点（一个或两个）
    size_t p_end = std::get<0>(results);
    std::vector<std::string> text_vector = std::get<1>(results);
    // 赋值子树根节点
    formula_tree->data = text_vector[0];

    if (text_vector.size() == 1)
    {
        // 退出条件：仅子树根节点，即仅字符
        return formula_tree;
    }
    // 处理上下标
    for (std::vector<std::string>::iterator it = std::next(text_vector.begin()); it != text_vector.end(); ++it)
    {
        std::string text_i = *it;
        if (text_i.find("^") == 0)
        {
            std::shared_ptr<FormulaNode> formula_suptree = std::make_shared<FormulaNode>();
            formula_tree->sup = searchSubTree(text_i.substr(2, text_i.size() - 3), formula_suptree);
        }
        else if (text_i.find("_") == 0)
        {
            std::shared_ptr<FormulaNode> formula_subtree = std::make_shared<FormulaNode>();
            formula_tree->sub = searchSubTree(text_i.substr(2, text_i.size() - 3), formula_subtree);
        }
    }
    return formula_tree;
}


void
DocxDocument::writeTree(pugi::xml_node m_r, const std::shared_ptr<FormulaNode>& formula_tree)
{
    if (!formula_tree->sup && !formula_tree->sub)
    {
        // 退出条件：子树仅根节点
        addCharacter(m_r, formula_tree->data);
        return;
    }

    if (formula_tree->sup && formula_tree->sub)
    {
        // 创建上下标结构
        pugi::xml_node m_subsup = m_r.append_child("m:sSubSup");
        // 写入基数字符
        pugi::xml_node m_e = m_subsup.append_child("m:e");
        addCharacter(m_e, formula_tree->data);
        // 写入上标
        pugi::xml_node m_sup_element = m_subsup.append_child("m:sup");
        writeTree(m_sup_element, formula_tree->sup);
        // 写入下标
        pugi::xml_node m_sub_element = m_subsup.append_child("m:sub");
        writeTree(m_sub_element, formula_tree->sub);
    }
    else if (formula_tree->sup)
    {
        // 创建上标结构
        pugi::xml_node m_sup = m_r.append_child("m:sSup");
        // 写入基数字符
        pugi::xml_node m_e = m_sup.append_child("m:e");
        addCharacter(m_e, formula_tree->data);
        // 写入上标
        pugi::xml_node m_sup_element = m_sup.append_child("m:sup");
        writeTree(m_sup_element, formula_tree->sup);
    }
    else if (formula_tree->sub)
    {
        // 创建下标结构
        pugi::xml_node m_sub = m_r.append_child("m:sSub");
        // 写入基数字符
        pugi::xml_node m_e = m_sub.append_child("m:e");
        addCharacter(m_e, formula_tree->data);
        // 写入下标
        pugi::xml_node m_sub_element = m_sub.append_child("m:sub");
        writeTree(m_sub_element, formula_tree->sub);
    }
}


size_t
find_supsub_item_end(const std::string& template_content, size_t& p_start)
{
    size_t p_next_n;
    if (template_content.find("}", p_start) != std::string::npos)
    {
        p_next_n = template_content.find("}", p_start) + 1;
        std::string string_to_next_n = template_content.substr(p_start, p_next_n - p_start);
        while ((!isBibBracketBalanced(string_to_next_n)) == 1)
        {
            if (template_content[p_next_n] == '}')
            {
                // 后续字符就是“}”，则直接赋值不在寻找，后续其实是直接跳出循环了
                string_to_next_n = template_content.substr(p_start, p_next_n - p_start +1);
                ++p_next_n;
            }
            else
            {
                p_next_n = template_content.find("}", ++p_next_n) + 1;
                string_to_next_n = template_content.substr(p_start, p_next_n - p_start);
            }
        }
    }
    else
    {
        p_next_n = template_content.size();
    }
    return p_next_n;
}


std::tuple<size_t, std::vector<std::string>>
splitBiggestSupSub(const std::string& text)
{
    std::vector<std::string> result;
    size_t p_start = 0, p_end = 0;
    while (1)
    {
        if (text.find("^", p_start) == std::string::npos && text.find("_", p_start) == std::string::npos)
        {
            // 仅字符
            result.push_back(text);
            p_end = text.size();
            return { p_end, result };
        }
        else if (text.find("^", p_start) != std::string::npos && text.find("_", p_start) == std::string::npos)
        {
            size_t p_start_sup = text.find("^", p_start);
            if (p_start_sup != p_start)
            {
                // 上标号前序有数据则存入数据
                result.push_back(text.substr(p_start, p_start_sup - p_start));
            }
            p_end = find_supsub_item_end(text, p_start_sup);
            result.push_back(text.substr(p_start_sup, p_end - p_start_sup));
        }
        else if (text.find("_", p_start) != std::string::npos && text.find("^", p_start) == std::string::npos)
        {
            size_t p_start_sub = text.find("_", p_start);
            if (p_start_sub != p_start)
            {
                // 下标号前序有数据则存入数据
                result.push_back(text.substr(p_start, p_start_sub - p_start));
            }
            p_end = find_supsub_item_end(text, p_start_sub);
            result.push_back(text.substr(p_start_sub, p_end - p_start_sub));
        }
        else
        {
            // 同时存在上下标，通过比较先后，读取第一个标对应数据作为这一轮循环数据
            if (text.find("^", p_start) < text.find("_", p_start))
            {
                size_t p_start_sup = text.find("^", p_start);
                result.push_back(text.substr(p_start, p_start_sup - p_start));
                p_end = find_supsub_item_end(text, p_start_sup);
                result.push_back(text.substr(p_start_sup, p_end - p_start_sup));
            }
            else
            {
                size_t p_start_sub = text.find("_", p_start);
                result.push_back(text.substr(p_start, p_start_sub - p_start));
                p_end = find_supsub_item_end(text, p_start_sub);
                result.push_back(text.substr(p_start_sub, p_end - p_start_sub));
            }
        }
        p_start = p_end;
        if (text[p_start] == '^' || text[p_start] == '_')
        {
            // 同时存在上下标则进入下一轮循环
            continue;
        }
        else
        {
            return { p_end, result };
        }
    }
}


// 函数：带特殊字符串分割
std::vector<std::string>
splitSpecialCharacter(const std::string& text)
{
    std::vector<std::string> result;
    std::stack<std::string> spectialCharacterStack;
    if (text.find("\\") != std::string::npos)
    {
        size_t p = 0;
        while (p < text.size())
        {
            size_t p_start = p;
            std::string item;
            if (text.find("\\", p) != std::string::npos)
            {
                p_start = text.find("\\", p);
                if (p != p_start)
                {
                    result.push_back(text.substr(p, p_start - p));
                }
                ++p_start;  // 覆盖当前"\\"
                item = "\\";
            }
            // 条件：下一个转义符之前 && 下一个空格之前
            while (p_start < text.size() && (std::isalpha(text[p_start]) || std::isdigit(text[p_start]) || text[p_start] == '{' || text[p_start] == '}'))
            {
                item += text[p_start];
                ++p_start;
            }
            result.push_back(item);
            p = p_start;
            // 跳过空格
            while (p < text.size())
            {
                std::regex pattern_text(R"(\\text\{)");
                std::smatch match_text;
                if (std::regex_search(result[result.size() - 1], match_text, pattern_text))
                {
                    // 如果处于\text{...}中就一直提取到平衡的“}”
                    while (!isBibBracketBalanced(result[result.size() - 1]))
                    {
                        result[result.size() - 1] += text.substr(p, 1);
                        ++p;
                    }
                    break;
                }
                else if (text[p] == ' ')
                {
                    ++p;
                }
                else
                {
                    break;
                }
            }
            // 存非空格其他字符
            if (p < text.size() && text[p] != ' ' && text[p] != '\\')
            {
                result.push_back(text.substr(p, 1));
                ++p;
            }
        }
    }
    else
    {
        result.push_back(text);
    }
    return result;
}


// 辅助函数：添加普通公式字符
void
DocxDocument::addCharacter(pugi::xml_node parent_node, const std::string& character)
{
    std::vector<std::string> base_string_vector = splitSpecialCharacter(character);
    std::queue<std::string> fracBracketQueue;
    std::string frac_string;
    std::stack<std::string> bracketStack;
    std::string bracket_string;
    for (std::string string_i : base_string_vector)
    {
        bool isEMathbb = false, isRoman = false, isScript = false, isMathbf = false;
        // 查看转义符内容：粗体、正体、特殊字符；或正在处理分数
        if (string_i.find("\\") != std::string::npos || !fracBracketQueue.empty() || !bracketStack.empty())
        {
            // 取转义符内容：如果正在处理分数||括号，则跳过（分数：frac的转义符去除剩余内容保留；括号：left括号的转义符去除剩余内容保留）
            if (fracBracketQueue.empty() && bracketStack.empty())
            {
                string_i = string_i.substr(1, string_i.size() - 1);
            }
            size_t special_command_end = 0;
            std::string special_command;
            while (special_command_end < string_i.size() && isalpha(string_i[special_command_end]))
            {
                special_command += string_i[special_command_end];
                special_command_end++;
            }

            // 括号
            if ((special_command == "left" || !bracketStack.empty()) && fracBracketQueue.empty())
            {
                if (special_command == "left")
                {
                    // 带left或right的转义符一定是括号
                    bracketStack.push("left");
                    continue;
                }
                if (string_i[0] == '\\' && string_i.substr(1, 5) == "right")
                {
                    bracketStack.push("right");
                    continue;   // 下一个才是右括号表示符
                }
                else
                {
                    bracket_string += string_i;
                }
                if ((!isBracketBalanced(bracket_string)) == 1 || (bracketStack.size() % 2) != 0) // 显示括号不完全匹配 || 存在left-right不匹配
                {
                    continue;
                }
                else
                {
                    std::string left_bracket, right_bracket, bracket_data;
                    std::regex pattern_bracket(R"((\\langle)(.*?)(\\rangle))");
                    std::smatch match_bracket;
                    std::regex_search(bracket_string, match_bracket, pattern_bracket);
                    if (match_bracket.size() > 0)
                    {
                        left_bracket = match_bracket[1].str();
                        right_bracket = match_bracket[3].str();
                        bracket_data = match_bracket[2].str();
                    }
                    else
                    {
                        left_bracket = bracket_string[0];
                        right_bracket = bracket_string[bracket_string.size() - 1];
                        bracket_data = bracket_string.substr(1, bracket_string.size() - 2);
                    }
                    pugi::xml_node m_d = parent_node.append_child("m:d");
                    pugi::xml_node m_dPr = m_d.append_child("m:dPr");
                    pugi::xml_node m_begChr = m_dPr.append_child("m:begChr");
                    std::unordered_map<std::string, std::string>::const_iterator it_left = spectialCharacterMap.find(left_bracket);
                    m_begChr.append_attribute("m:val") = it_left->second;
                    pugi::xml_node m_sepChr = m_dPr.append_child("m:sepChr");
                    m_sepChr.append_attribute("m:val") = "";
                    pugi::xml_node m_endChr = m_dPr.append_child("m:endChr");
                    std::unordered_map<std::string, std::string>::const_iterator it_right = spectialCharacterMap.find(right_bracket);
                    m_endChr.append_attribute("m:val") = it_right->second;
                    // 数值部分
                    pugi::xml_node m_e = m_d.append_child("m:e");
                    addCharacter(m_e, bracket_data);
                    continue;
                }
            }
            
            // 分数
            if ((special_command == "frac" || !fracBracketQueue.empty()) && bracketStack.empty())
            {
                if (!fracBracketQueue.empty())
                {
                    fracBracketQueue.pop();
                    frac_string += string_i;
                }
                else
                {
                    frac_string += string_i;
                }
                if ((!isBibBracketBalanced(frac_string)) == 1)
                {
                    fracBracketQueue.push(string_i);
                    continue;
                }
                pugi::xml_node m_f = parent_node.append_child("m:f");
                special_command_end = frac_string.find("{");
                // 分子
                size_t p_num_end = find_supsub_item_end(frac_string, special_command_end);
                std::string num_data = frac_string.substr(special_command_end, p_num_end - special_command_end);
                pugi::xml_node m_num = m_f.append_child("m:num");
                addCharacter(m_num, num_data.substr(1, num_data.size() - 2));
                // 分母
                pugi::xml_node m_den = m_f.append_child("m:den");
                size_t p_den_end = find_supsub_item_end(frac_string, p_num_end);
                std::string den_data = frac_string.substr(p_num_end, p_den_end - p_num_end);
                addCharacter(m_den, den_data.substr(1, den_data.size() - 2));
                continue;
            }
            // 普通转义符内容解析
            if (special_command == "sqrt")
            {
                // 根号
                pugi::xml_node m_rad = parent_node.append_child("m:rad");
                pugi::xml_node m_radPr = m_rad.append_child("m:radPr");
                pugi::xml_node m_degHide = m_radPr.append_child("m:degHide");
                // 数值部分
                size_t p_sqrt_end = find_supsub_item_end(string_i, special_command_end);
                std::string sqrt_data = string_i.substr(special_command_end, p_sqrt_end - special_command_end);
                pugi::xml_node m_e = m_rad.append_child("m:e");
                addCharacter(m_e, sqrt_data.substr(1, sqrt_data.size() - 2));
                continue;
            }
            if (special_command == "mathbb")
            {
                isEMathbb = true;
                string_i = string_i.substr(special_command_end + 1, string_i.find('}', special_command_end) - special_command_end - 1);
            }
            else if (special_command == "mathrm" || special_command == "text")
            {
                isRoman = true;
                string_i = string_i.substr(special_command_end + 1, string_i.find('}', special_command_end) - special_command_end - 1);
            }
            else if (special_command == "mathcal")
            {
                isScript = true;
                string_i = string_i.substr(special_command_end + 1, string_i.find('}', special_command_end) - special_command_end - 1);
            }
            else if (special_command == "mathbf")
            {
                isMathbf = true;
                string_i = string_i.substr(special_command_end + 1, string_i.find('}', special_command_end) - special_command_end - 1);
            }
            else
            {
                // 特殊字符直接输出
                std::unordered_map<std::string, std::string>::const_iterator it = spectialCharacterMap.find(special_command);
                if (it != spectialCharacterMap.end())
                {
                    std::string special_command_string = it->second;
                    addMathCommand(parent_node, special_command_string);
                    if (special_command.size() == string_i.size())
                    {
                        // special_command是string_i的唯一字符串
                        continue;
                    }
                    else
                    {
                        // 取string_i中的剩余字符串进行处理
                        string_i = string_i.substr(special_command.size(), string_i.size() - special_command.size());
                    }
                }
            }
        }
        if (!isEMathbb)
        {
            pugi::xml_node m_r = parent_node.append_child("m:r");
            if (!isRoman && !isScript && !isMathbf)
            {
                pugi::xml_node w_rPr = m_r.append_child("w:rPr");
                pugi::xml_node w_rFonts = w_rPr.append_child("w:rFonts");
                w_rFonts.append_attribute("w:ascii") = "Cambria Math";
                w_rFonts.append_attribute("w:hAnsi") = "Cambria Math";
            }
            else
            {
                pugi::xml_node m_rPr = m_r.append_child("m:rPr");
                if (isRoman)
                {
                    pugi::xml_node m_sty = m_rPr.append_child("m:sty");
                    m_sty.append_attribute("m:val") = "p";
                    pugi::xml_node w_rFonts = m_r.append_child("w:rFonts");
                    w_rFonts.append_attribute("w:ascii") = "Times New Roman";
                    w_rFonts.append_attribute("w:hAnsi") = "Times New Roman";
                }
                if (isScript)
                {
                    pugi::xml_node m_scr = m_rPr.append_child("m:scr");
                    m_scr.append_attribute("m:val") = "script";
                    pugi::xml_node m_sty = m_rPr.append_child("m:sty");
                    m_sty.append_attribute("m:val") = "p";
                }
                if (isMathbf)
                {
                    pugi::xml_node m_sty = m_rPr.append_child("m:sty");
                    m_sty.append_attribute("m:val") = "b";
                }
            }
            pugi::xml_node m_t = m_r.append_child("m:t");
            m_t.text().set(string_i.c_str());
        }
        else
        {
            addMathbb(parent_node, string_i);
        }
    }

}


// 辅助函数：添加数学命令
void
DocxDocument::addMathCommand(pugi::xml_node parent_node, const std::string& symbol)
{
    pugi::xml_node m_r = parent_node.append_child("m:r");
    pugi::xml_node m_rPr = m_r.append_child("m:rPr");

    pugi::xml_node m_sty = m_rPr.append_child("m:sty");
    m_sty.append_attribute("m:val") = "p";

    pugi::xml_node m_t = m_r.append_child("m:t");
    m_t.text().set(symbol.c_str());

    // 打印 m_r 节点的 XML 结构
    std::cout << "m_r node structure:" << std::endl;
    m_r.print(std::cout); // 这将输出 m_t 节点的完整 XML 结构
}

// 辅助函数：处理 \mathbb{} 命令
void
DocxDocument::addMathbb(pugi::xml_node parent_node, const std::string& arg)
{
    // 创建双线体字符
    pugi::xml_node m_r = parent_node.append_child("m:r");
    pugi::xml_node m_rPr = m_r.append_child("m:rPr");
    pugi::xml_node m_src = m_rPr.append_child("m:scr");
    m_src.append_attribute("m:val") = "double-struck";
    pugi::xml_node m_sty = m_rPr.append_child("m:sty");
    m_sty.append_attribute("m:val") = "p";
    pugi::xml_node m_t = m_r.append_child("m:t");
    m_t.text().set(arg.c_str());

    // 打印 m_r 节点的 XML 结构
    std::cout << "m_r node structure:" << std::endl;
    m_r.print(std::cout); // 这将输出 m_t 节点的完整 XML 结构
}


pugi::xml_node 
DocxDocument::writeTable(pugi::xml_node w_body, const md::Table& tbl)
{
    pugi::xml_node w_tbl = w_body.append_child("w:tbl");
    pugi::xml_node w_tblPr = w_tbl.append_child("w:tblPr");

    writeTableProperties(w_tblPr, tbl.prop_);

    const md::Rect& tblRect = tbl.rect();

    pugi::xml_node w_tblGrid = w_tbl.append_child("w:tblGrid");
    for (size_t j = 0; j < tblRect.cols(); j++)
        w_tblGrid.append_child("w:gridCol");

    const size_t colWidth = 5000 / tblRect.cols();

    for (size_t i = 0; i < tblRect.rows(); i++) {
        pugi::xml_node w_tr = w_tbl.append_child("w:tr");

        size_t j = 0;
        while (j < tblRect.cols()) {
            const md::Cell& cell = *tbl.cellAtUnsafe(i, j);
            const md::Rect& cellRect = cell.rect();

            pugi::xml_node w_tc = w_tr.append_child("w:tc");
            pugi::xml_node w_tcPr = w_tc.append_child("w:tcPr");

            pugi::xml_node w_vAlign = w_tcPr.append_child("w:vAlign");
            w_vAlign.append_attribute("w:val") = "center";  // 单元格内容垂直居中

            if (cellRect.cols() > 1)
                w_tcPr.append_child("w:gridSpan").append_attribute("w:val") = cellRect.cols();

            if (i == cellRect.row()) {
                if (cellRect.rows() > 1)
                    w_tcPr.append_child("w:vMerge").append_attribute("w:val") = "restart";

                pugi::xml_node w_tcW = w_tcPr.append_child("w:tcW");
                w_tcW.append_attribute("w:type") = "pct";
                w_tcW.append_attribute("w:w") = colWidth * cellRect.cols();
                /*for (auto& block : cell.blocks())
                {
                    md::Paragraph block_i = dynamic_cast<md::Paragraph&>(*block);
                    for (auto& run : block_i.runs())
                    {
                        md::RichText run_i = dynamic_cast<md::RichText&>(*run);
                        run_i.text();
                    }
                }*/

                if (cell.blocks().size() == 0) {
                    w_tc.append_child("w:p");
                }
                else {
                    for (auto& block : cell.blocks())
                        writeBlock(w_tc, *block);
                }
            }
            else {
                if (i == cellRect.rrow())
                    w_tcPr.append_child("w:vMerge");
                else
                    w_tcPr.append_child("w:vMerge").append_attribute("w:val") = "continue";
                w_tc.append_child("w:p");
            }

            j += cellRect.cols();
        }
    }

    return w_tbl;
}


void 
DocxDocument::writePicture(pugi::xml_node w_p, const md::Picture& pict)
{
    const unsigned int index = ++pictCount;
    const std::string name("Picture " + std::to_string(index));

    const md::PictureProperties& prop = pict.prop_;
    const auto width = prop.extent_.width_;
    const auto height = prop.extent_.height_;

    pugi::xml_node wp_inline = w_p.append_child("w:r")
        .append_child("w:drawing").append_child("wp:inline");
    wp_inline.append_attribute("distT") = "0";
    wp_inline.append_attribute("distB") = "0";
    wp_inline.append_attribute("distL") = "0";
    wp_inline.append_attribute("distR") = "0";

    pugi::xml_node wp_extent = wp_inline.append_child("wp:extent");
    wp_extent.append_attribute("cx") = width;
    wp_extent.append_attribute("cy") = height;

    pugi::xml_node wp_effectExtent = wp_inline.append_child("wp:effectExtent");
    wp_effectExtent.append_attribute("l") = "0";
    wp_effectExtent.append_attribute("t") = "0";
    wp_effectExtent.append_attribute("r") = "0";
    wp_effectExtent.append_attribute("b") = "0";

    pugi::xml_node wp_docPr = wp_inline.append_child("wp:docPr");
    wp_docPr.append_attribute("id") = index;
    wp_docPr.append_attribute("name") = name.c_str();

    pugi::xml_node a_graphicFrameLocks = wp_inline.append_child("wp:cNvGraphicFramePr")
        .append_child("a:graphicFrameLocks");
    a_graphicFrameLocks.append_attribute("xmlns:a")
        .set_value("http://schemas.openxmlformats.org/drawingml/2006/main");
    a_graphicFrameLocks.append_attribute("noChangeAspect") = "1";

    pugi::xml_node a_graphic = wp_inline.append_child("a:graphic");
    pugi::xml_node a_graphicData = a_graphic.append_child("a:graphicData");
    pugi::xml_node pic_pic = a_graphicData.append_child("pic:pic");

    a_graphic.append_attribute("xmlns:a")
        .set_value("http://schemas.openxmlformats.org/drawingml/2006/main");
    a_graphicData.append_attribute("uri")
        .set_value("http://schemas.openxmlformats.org/drawingml/2006/picture");
    pic_pic.append_attribute("xmlns:pic")
        .set_value("http://schemas.openxmlformats.org/drawingml/2006/picture");

    pugi::xml_node pic_nvPicPr = pic_pic.append_child("pic:nvPicPr");
    pugi::xml_node pic_cNvPr = pic_nvPicPr.append_child("pic:cNvPr");
    pic_cNvPr.append_attribute("id") = index;
    pic_cNvPr.append_attribute("name") = name.c_str();
    pugi::xml_node a_picLocks = pic_nvPicPr.append_child("pic:cNvPicPr")
        .append_child("a:picLocks");
    a_picLocks.append_attribute("noChangeAspect") = "1";
    a_picLocks.append_attribute("noChangeArrowheads") = "1";

    pugi::xml_node pic_blipFill = pic_pic.append_child("pic:blipFill");
    pugi::xml_node a_blip = pic_blipFill.append_child("a:blip");
    a_blip.append_attribute("r:embed") = md::Relationship::stringifyId(pict.id_).c_str();
    pic_blipFill.append_child("a:srcRect"); // Cropping
    pic_blipFill.append_child("a:stretch").append_child("a:fillRect"); // Stretching 

    pugi::xml_node pic_spPr = pic_pic.append_child("pic:spPr");
    pic_spPr.append_attribute("bwMode") = "auto";

    pugi::xml_node a_xfrm = pic_spPr.append_child("a:xfrm");
    pugi::xml_node a_off = a_xfrm.append_child("a:off");
    a_off.append_attribute("x") = "0";
    a_off.append_attribute("y") = "0";
    pugi::xml_node a_ext = a_xfrm.append_child("a:ext");
    a_ext.append_attribute("cx") = width;
    a_ext.append_attribute("cy") = height;

    pugi::xml_node a_prstGeom = pic_spPr.append_child("a:prstGeom");
    a_prstGeom.append_attribute("prst") = "rect";
    a_prstGeom.append_child("a:avLst");

    pic_spPr.append_child("a:noFill");
    pic_spPr.append_child("a:ln").append_child("a:noFill");
}
