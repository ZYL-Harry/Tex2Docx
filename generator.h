#ifndef GENERATOR
#define GENERATOR

#include"document_ast.h"
#include<string>
#include"minidocx.hpp"
#include"document_docx.h"
#include<stack>
#include<queue>



class DocxGenerator
{
public:
	DocxGenerator(const std::string path, const AST::Document& ast) : docx_path(path), document_ast(ast) { };	//构造函数的声明与定义
	DocxGenerator(const DocxGenerator& word_generator);		//拷贝构造的声明
	DocxGenerator& operator = (const DocxGenerator& word_generator);	//拷贝赋值的声明
	~DocxGenerator();	//析构函数的声明

	void generate();
	void generate_pagestyle();
	void generate_information();
	void generate_abstract();
	void generate_keywords();
	void generate_body();
    void generate_section_header(std::shared_ptr<AST::Header> section);
    void generate_paragraph(std::shared_ptr<AST::Paragraph> paragraph);
    std::string replace_cite_text(std::string paragraph_text_string);
    std::string replace_ref_text(std::string paragraph_text_string);
    void generate_list(std::shared_ptr<AST::List> list);
    void generate_equation(std::shared_ptr<AST::Equation> equation);
    void generate_table(std::shared_ptr<AST::Table> table);
    void generate_figure(std::shared_ptr<AST::Figure> figure);
    void generate_supplement();
    void generate_reference();

	void write_text(md::ParagraphPointer& paragraph, const std::string& text_string, const int& fontsize, const bool& isBold=false, const bool& isItlatic=false);

private:
	std::string docx_path;			// 数据：docx文件地址
	AST::Document document_ast;		// 数据：docx文件内容（AST格式）
	DocxDocument document_docx;		// 数据：docx文件整体
	md::SectionPointer section;		// 数据：docx文件章节
	size_t scan_p;
    
    md::NumberingId headerNumId;
    md::NumberingId listNumId;
    int num_header;
    
    std::queue<std::shared_ptr<AST::Reference>> citeQueue;  // 数据：使用队列后续直接用于顺序生成参考文献文本
    md::NumberingId refNumId;
    size_t reference_title_fontsize;
    std::string reference_title_aligning;

    std::queue<std::shared_ptr<AST::Table>> refTableQueue;  // 数据：使用队列用于搜索已存表格从而给表格编号
    std::queue<std::shared_ptr<AST::Figure>> refFigureQueue;  // 数据：使用队列用于搜索已存图像从而给图像编号
};



inline std::vector<std::string> 
splitByDollar(const std::string& text);


inline std::vector<std::string> 
splitTextbf(const std::string& text);

std::vector<std::string>
splitTextit(const std::string& text);


inline void
generate_example();


inline size_t
searchCiteQueueElement(const std::queue<std::shared_ptr<AST::Reference>>& input_queue, const std::string key);


inline std::vector<std::string>
splitCiteKey(const std::string& text);


template<typename T> size_t
searchRefQueueElement(const std::queue<std::shared_ptr<T>>& input_queue, const std::string& key);


void
addText(md::CellPointer cell, const std::string& text, int fontsize, bool bold = false, bool underline = false, md::Alignment align_type = md::Alignment::Centered);

void
addMultiLineText(md::CellPointer cell, const std::vector<std::string>& lines, int fontsize, bool bold = false, bool underline = false, md::Alignment align_type = md::Alignment::Centered);



inline void 
generate_numbered_example()
{
    // 创建文档对象
    md::Document doc;

    // 1. 创建抽象编号定义
    md::AbstractNumberingDefinition absNumDef;

    // 设置一级编号样式
    absNumDef.levels_[0].numStart_ = 1;
    absNumDef.levels_[0].numStyle_ = md::NumberStyle::Decimal;
    absNumDef.levels_[0].numFmt_ = "%1.";  // 格式如 "1."、"2."
    absNumDef.levels_[0].numAlign_ = md::Alignment::Left;

    // 设置缩进 - 关键步骤！
    md::ParagraphProperties::Indentation indent;
    indent.left_.value_ = convertUnit(0.5, Unit::INCH, Unit::TWIP);  // 0.5英寸左缩进
    indent.special_.type_ = md::ParagraphProperties::SpecialIndentationType::Hanging;
    indent.special_.value_ = convertUnit(0.25, Unit::INCH, Unit::TWIP);  // 0.25英寸悬挂缩进
    absNumDef.levels_[0].indent_ = indent;

    // 2. 添加抽象编号定义到文档
    md::NumberingId absNumId = doc.addAbstractNumDefinition(absNumDef);

    // 3. 创建具体编号实例并添加到文档
    md::NumberingDefinition numDef(absNumId); // 使用抽象编号ID
    md::NumberingId numId = doc.addNumDefinition(numDef);

    // 4. 添加一个section
    md::SectionPointer sect = doc.addSection();

    // 5. 添加带编号的段落
    md::ParagraphPointer numberedPara = sect->addParagraph();
    numberedPara->numId_ = numId;  // 使用具体编号ID
    numberedPara->level_ = md::NumberingLevel::Level1;  // 使用一级编号

    // 添加段落内容
    md::RichTextPointer rich = numberedPara->addRichText("first paragraph");
    rich->prop_.fontSize_ = 12;
    rich->prop_.color_ = "000000";  // 黑色

    // 6. 添加第二个带编号的段落（自动递增）
    md::ParagraphPointer numberedPara2 = sect->addParagraph();
    numberedPara2->numId_ = numId;
    numberedPara2->level_ = md::NumberingLevel::Level1;
    numberedPara2->addRichText("second paragraph");

    // 7. 添加多级编号的子段落
    md::ParagraphPointer subPara = sect->addParagraph();
    subPara->numId_ = numId;
    subPara->level_ = md::NumberingLevel::Level2;  // 二级编号

    // 设置二级编号格式
    md::LevelDefinition subLevelDef;
    subLevelDef.numStyle_ = md::NumberStyle::LowerLetter;
    subLevelDef.numFmt_ = "(%1)";  // 格式如 "(a)"、"(b)"
    subLevelDef.indent_->left_.value_ = static_cast<size_t>(std::round(convertUnit(1.0, Unit::INCH, Unit::TWIP)));
    subPara->prop_ = subLevelDef;

    subPara->addRichText("second level paragraph");

    // 8. 添加一个普通段落（无编号）
    md::ParagraphPointer normalPara = sect->addParagraph();
    normalPara->addRichText("no number paragraph");

    // 9. 保存docx文件
    doc.saveAs("numbered_example.docx");
}


inline void
generate_table_example()
{
    // 创建文档对象
    md::Document doc;

    // 添加一个section
    md::SectionPointer sect = doc.addSection();

    // 添加标题段落
    md::ParagraphPointer titlePara = sect->addParagraph();
    titlePara->prop_.align_ = md::Alignment::Centered;
    md::RichTextPointer titleText = titlePara->addRichText("Table 1: Detailed configuration of experiment data");
    titleText->prop_.fontSize_ = 12;
    titleText->prop_.fontStyle_.bold_ = true;

    // 创建表格 - 13行6列（包括标题行和分隔行）
    md::TablePointer table = sect->addTable(13, 6);

    // 设置表格属性
    table->prop_.layout_ = md::TableProperties::Layout::Fixed;
    table->prop_.width_.type_ = md::TableProperties::WidthType::Percent;
    table->prop_.align_ = md::TableProperties::Alignment::Center;

    // 设置表格边框
    table->prop_.borders_.top_.style_ = md::BorderStyle::Single;
    table->prop_.borders_.bottom_.style_ = md::BorderStyle::Single;
    table->prop_.borders_.left_.style_ = md::BorderStyle::Single;
    table->prop_.borders_.right_.style_ = md::BorderStyle::Single;
    table->prop_.borders_.insideHorizontal_.style_ = md::BorderStyle::Single;
    table->prop_.borders_.insideVertical_.style_ = md::BorderStyle::Single;

    // 辅助函数：添加居中对齐的文本到单元格
    auto addCenteredText = [](md::CellPointer cell, const std::string& text, bool bold = false) {
        md::ParagraphPointer para = cell->addParagraph();
        para->prop_.align_ = md::Alignment::Centered; // 设置段落居中对齐
        md::RichTextPointer richText = para->addRichText(text);
        if (bold) {
            richText->prop_.fontStyle_.bold_ = true;
        }
        return richText;
    };

    // 设置表头行（第0行）
    addCenteredText(table->cellAt(0, 0), "Dataset", true);
    addCenteredText(table->cellAt(0, 1), "Bearing", true);
    addCenteredText(table->cellAt(0, 2), "Case Index", true);
    addCenteredText(table->cellAt(0, 3), "Speed (rpm)", true);
    addCenteredText(table->cellAt(0, 4), "Load", true);
    addCenteredText(table->cellAt(0, 5), "Fault", true);

    // CWRU 数据部分 (行1-4)
    // 合并第一列 (Dataset) 的4个单元格
    md::CellPointer cwruCell = table->merge(1, 0, 4, 1);
    addCenteredText(cwruCell, "CWRU", true);

    // 合并第二列 (Bearing) 的4个单元格
    md::CellPointer bearingCell1 = table->merge(1, 1, 4, 1);
    addCenteredText(bearingCell1, "6205");

    // 填充 CWRU 数据
    addCenteredText(table->cellAt(1, 2), "C1");
    addCenteredText(table->cellAt(1, 3), "1797");
    addCenteredText(table->cellAt(1, 4), "0-HP");

    addCenteredText(table->cellAt(2, 2), "C2");
    addCenteredText(table->cellAt(2, 3), "1772");
    addCenteredText(table->cellAt(2, 4), "1-HP");

    addCenteredText(table->cellAt(3, 2), "C3");
    addCenteredText(table->cellAt(3, 3), "1750");
    addCenteredText(table->cellAt(3, 4), "2-HP");

    addCenteredText(table->cellAt(4, 2), "C4");
    addCenteredText(table->cellAt(4, 3), "1730");
    addCenteredText(table->cellAt(4, 4), "3-HP");

    // 合并最后一列 (Fault) 的4个单元格
    md::CellPointer faultCell1 = table->merge(1, 5, 4, 1);
    addCenteredText(faultCell1, "N, IR, OR, B");

    // PU 数据部分 (行5-8)
    // 合并第一列 (Dataset) 的4个单元格
    md::CellPointer puCell = table->merge(5, 0, 4, 1);
    addCenteredText(puCell, "PU", true);

    // 合并第二列 (Bearing) 的4个单元格
    md::CellPointer bearingCell2 = table->merge(5, 1, 4, 1);
    addCenteredText(bearingCell2, "6203");

    // 填充 PU 数据
    addCenteredText(table->cellAt(5, 2), "P1");
    addCenteredText(table->cellAt(5, 3), "1500");
    addCenteredText(table->cellAt(5, 4), "1000-N");

    addCenteredText(table->cellAt(6, 2), "P2");
    addCenteredText(table->cellAt(6, 3), "900");
    addCenteredText(table->cellAt(6, 4), "1000-N");

    addCenteredText(table->cellAt(7, 2), "P3");
    addCenteredText(table->cellAt(7, 3), "1500");
    addCenteredText(table->cellAt(7, 4), "1000-N");

    addCenteredText(table->cellAt(8, 2), "P4");
    addCenteredText(table->cellAt(8, 3), "1500");
    addCenteredText(table->cellAt(8, 4), "400-N");

    // 合并最后一列 (Fault) 的4个单元格
    md::CellPointer faultCell2 = table->merge(5, 5, 4, 1);
    addCenteredText(faultCell2, "N, IR, OR");

    // HIT 数据部分 (行9-12)
    // 合并第一列 (Dataset) 的4个单元格
    md::CellPointer hitCell = table->merge(9, 0, 4, 1);
    addCenteredText(hitCell, "HIT", true);

    // HIT 的第二列不需要合并，每行都有不同内容
    addCenteredText(table->cellAt(9, 1), "15 balls");
    addCenteredText(table->cellAt(10, 1), "30-mm inner");
    addCenteredText(table->cellAt(11, 1), "65-mm outer");
    addCenteredText(table->cellAt(12, 1), "7.5 mm ball");

    // 填充 HIT 数据
    addCenteredText(table->cellAt(9, 2), "H1");
    addCenteredText(table->cellAt(9, 3), "1500-2500");
    addCenteredText(table->cellAt(9, 4), "0-N");

    addCenteredText(table->cellAt(10, 2), "H2");
    addCenteredText(table->cellAt(10, 3), "3000-3700");
    addCenteredText(table->cellAt(10, 4), "0-N");

    addCenteredText(table->cellAt(11, 2), "H3");
    addCenteredText(table->cellAt(11, 3), "3800-4100");
    addCenteredText(table->cellAt(11, 4), "0-N");

    addCenteredText(table->cellAt(12, 2), "H4");
    addCenteredText(table->cellAt(12, 3), "4200-4500");
    addCenteredText(table->cellAt(12, 4), "0-N");

    // 合并最后一列 (Fault) 的4个单元格
    md::CellPointer faultCell3 = table->merge(9, 5, 4, 1);
    addCenteredText(faultCell3, "N, IR, OR");

    // 保存docx文件
    doc.saveAs("table_example.docx");
}


inline void
generate_figure_example()
{
    // 创建文档
    md::Document doc;

    // 添加图片到文档关系库
    md::RelationshipId imageId = doc.addImage("数据集测试台.png");

    // 创建section和段落
    md::SectionPointer section = doc.addSection();
    md::ParagraphPointer paragraph = section->addParagraph(); // 需要确认Section类的具体接口
    paragraph->prop_.align_ = mapping_alignStyle_docx("center");
    // 行间距
    paragraph->prop_.spacing_ = md::ParagraphProperties::Spacing();
    paragraph->prop_.spacing_->lineSpacing_.type_ = md::ParagraphProperties::LineSpacingType::Lines;

    // 插入图片到段落
    auto picture = paragraph->addPicture(imageId);

    // 设置图片属性
    picture->prop_.extent_.setSize(4, 3, 96); // 4x3英寸

    // 保存文档
    doc.saveAs("figur_example.docx");
}

#endif
