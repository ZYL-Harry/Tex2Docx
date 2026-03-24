#ifndef DOCUMENT_AST
#define DOCUMENT_AST

#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <optional>
#include <unordered_map>

// AST前置声明
namespace AST {
    struct Document;

    // 行内元素 (Inline elements)
    struct Text;
    struct Textit;
    struct Textbold;
    struct Code;
    struct Link;
    struct Image;
    struct Math;
    struct ReferenceKey;

    // 块级元素 (Block elements)
    struct AttributeItem;
    struct Attribute;
    struct InformationItem;
    struct Information;
    struct Paragraph;
    struct Keyword;
    struct Header;
    struct List;
    struct ListItem;
    struct Table;
    struct TableRow;
    struct TableCell;
    struct Figure;
    struct CodeBlock;
    struct Equation;
    struct Reference;
    struct References;

    // 使用 variant 定义所有行内元素类型
    using InlineElement = std::variant<
        std::monostate,
        std::shared_ptr<Text>,
        std::shared_ptr<Textit>,
        std::shared_ptr<Textbold>,
        std::shared_ptr<Code>,
        std::shared_ptr<Link>,
        std::shared_ptr<Image>,
        std::shared_ptr<Math>,
        std::shared_ptr<ReferenceKey>
    >;

    // 使用 variant 定义所有块级元素类型
    using BlockElement = std::variant <
        std::monostate,
        std::shared_ptr<AttributeItem>,
        std::shared_ptr<Attribute>,
        std::shared_ptr<InformationItem>,
        std::shared_ptr<Information>,
        std::shared_ptr<Keyword>,
        std::shared_ptr<Paragraph>,
        std::shared_ptr<Header>,
        std::shared_ptr<List>,
        std::shared_ptr<ListItem>,
        std::shared_ptr<TableCell>,
        std::shared_ptr<TableRow>,
        std::shared_ptr<Table>,
        std::shared_ptr<Figure>,
        std::shared_ptr<CodeBlock>,
        std::shared_ptr<Equation>,
        std::shared_ptr<Reference>,
        std::shared_ptr<References>
    >;

    // 元数据，可以用键值对存储文档信息
    using MetaData = std::unordered_map<std::string, std::string>;

    // === 1. 行内元素定义 ===
    struct Text {
        std::string content;
        int fontsize;               // 字号
        std::string colour;         // 颜色
        std::string spacing;        // 间距
        std::string aligning;       // 对齐
    };

    struct Textit { // 斜体
        std::string content;
        int fontsize;               // 字号
        std::string colour;         // 颜色
        std::string spacing;        // 间距
        std::string aligning;       // 对齐
    };

    struct Textbold { // 粗体
        std::string content;
        int fontsize;               // 字号
        std::string colour;         // 颜色
        std::string spacing;        // 间距
        std::string aligning;       // 对齐
    };

    struct Code { // 伪代码
        std::string content;
        int fontsize;               // 字号
        std::string colour;         // 颜色
        std::string spacing;        // 间距
        std::string aligning;       // 对齐
    };

    struct Link {
        std::string url;
        std::vector<InlineElement> alt_text; // 链接显示文本，可能包含格式化内容
    };

    struct Image {
        std::string path;    // 图片路径
        int width;
        int height;
    };

    struct Math {
        size_t index;
        std::string latex_math; // 存储原始的 LaTeX 公式字符串
    };

    struct ReferenceKey {
        std::string key;
        std::string content;
        int fontsize;               // 字号
        std::string colour;         // 颜色
        std::string spacing;        // 间距
        std::string aligning;       // 对齐
    };

    // === 2. 块级元素定义 ===
    struct AttributeItem {
        std::string content;
        int fontsize;   // 字号
        std::string column;
        std::string aligning;
    };

    struct Attribute {
        std::vector<BlockElement> items;
    };

    struct InformationItem {
        std::string key;
        int pos;    // 0-摘要前，1-摘要后正文前，2-正文后
        std::string title;
        std::string content;
        int fontsize;               // 字号
        std::string colour;         // 颜色
        std::string spacing;        // 间距
        std::string aligning;       // 对齐
    };

    struct Information {
        std::vector<BlockElement> items;
    };

    struct Paragraph {
        std::vector<InlineElement> content;
    };

    struct Keyword {
        std::vector<InlineElement> items;
    };

    struct Header {
        int level; // 1 for \section, 2 for \subsection, etc.
        std::vector<InlineElement> content;
    };

    struct ListItem {
        std::string content;
        int fontsize;               // 字号
        std::string colour;         // 颜色
        std::string spacing;        // 间距
        std::string aligning;       // 对齐
    };

    struct List {
        bool is_ordered; // 有序条目还是无序条目
        std::vector<BlockElement> items;
    };

    struct TableCell 
    {
        std::string content;
        size_t row_index = 0;
        size_t col_index = 0;
        size_t rowspan = 1;
        size_t colspan = 1;
    };

    struct Table {
        size_t ref_index;
        std::string ref_label;
        std::string caption;
        std::vector<BlockElement> cells;
        size_t column_count = 0;
        size_t row_count = 0;
        int fontsize;       // 字号
        std::string aligning;
    };

    struct Figure {
        size_t ref_index;
        std::string ref_label;
        std::vector<InlineElement> image;
        std::vector<InlineElement> caption; // 图注
    };

    struct CodeBlock {
        std::vector<InlineElement> code;
        std::optional<std::string> language; // 用于伪代码可能的值："pseudocode", "c++", "python"
    };

    struct Equation {
        std::string ref_label;
        std::vector<InlineElement> content;
        int fontsize;       // 字号
        std::string aligning;
    };

    struct Reference {
        int index;
        std::string content;

        std::string cite_key;
        std::vector<InlineElement> key;
    };

    struct References {
        std::vector<BlockElement> reference;
    };

    // === 3. 文档根节点 ===
    struct Document {
        MetaData metadata;
        std::vector<BlockElement> content;
    };

}

#endif