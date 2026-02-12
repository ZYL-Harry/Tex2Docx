#ifndef PARSER
#define PARSER

#include<string>
#include"document_ast.h"
#include"tex_attribute.h"
#include<regex>


class TexParser
{
public:
	TexParser(const std::string path = "\0", const std::string ref_path="\0", const std::string& figure_dir="\0") :
		tex_path(path), ref_path(ref_path), figure_dir(figure_dir), scan_p(0), equation_index(0){ get_default_style(); };	//构造函数的声明
	TexParser(const TexParser& tex_parser);		//拷贝构造的声明
	TexParser& operator = (const TexParser& tex_parser);	//拷贝赋值的声明
	~TexParser();	//析构函数的声明

	// 函数：cls模板文件读取与解析
	size_t match(const std::string& template_content, const std::string& condition_str, size_t& p);	// 函数：文本匹配（模板）
	size_t skip_blank(const std::string& template_content, size_t& p);	// 函数：跳过空字符（模板）
	size_t find_attribute_item_end(const std::string& template_content, size_t& p_start);	// 函数：找到属性每项的最后一个位置（模板）

	// 函数：tex文件读取
	void read_tex_file();

	// 函数：tex文件解析
	AST::Document& parse();
	bool match(const std::string& condition_str);	// 函数：文本匹配
	void skip_blank();		// 函数：跳过空字符
	bool is_symbol_start(size_t& p);		// 函数：判断段落中公式符号起始端
	bool is_empty_line_start(size_t& p);	// 函数：判断空行起始端
	void parse_block();						// 函数：解析块级元素
	std::shared_ptr<AST::Attribute> parse_attribute();			// 函数：解析文章格式属性
	size_t find_attribute_item_end(size_t& p_start);			// 函数：找到属性每项的最后一个位置
	std::shared_ptr<AST::Information> parse_information();		// 函数：解析文章基本信息
	std::shared_ptr<AST::Paragraph> parse_paragraph(const std::string& key = "\0");		// 函数：解析段落
	std::shared_ptr<AST::Keyword> parse_keyword();				// 函数：解析关键词
	std::shared_ptr<AST::Header> parse_section(const int& level_section);	// 函数：解析各级标题
	std::shared_ptr<AST::List> parse_enumerate();				// 函数：解析列举条目
	std::shared_ptr<AST::Figure> parse_figure();				// 函数：解析图像
	std::tuple<int, int, std::string> parse_figure_image(const std::string& image_path);
	std::shared_ptr<AST::Equation> parse_equation();			// 函数：解析公式
	std::shared_ptr<AST::Table> parse_table();					// 函数：解析表格
	std::shared_ptr<AST::CodeBlock> parse_algorithm();			// 函数：解析伪代码

	// 函数：默认格式读取（article.cls）
	void get_default_style(const std::string& style_path ="article.cls");

	// 函数：参考文献读取（ref.bib）
	void parse_reference();
	size_t find_bib_item_end(const std::string& template_content, size_t& p_start);	// 函数：找到属性每项的最后一个位置（参考文献）

private:
	std::string tex_path;				// 数据：tex文件地址
	std::string tex_content;			// 数据：tex文件内容（文本格式）
	std::size_t scan_p;					// 数据：扫描位置
	std::shared_ptr<AST::Attribute> template_attribute;	// 数据：模板格式
	AST::Document document_ast;			// 数据：tex文件内容（AST格式）
	std::vector<std::string> keys;
	std::string ref_path;
	size_t equation_index;
	std::string figure_dir;
};


inline bool
isBracketBalanced(const std::string& expression);

inline int
findFont(const std::string& text);

inline std::string
findAlignStyle(const std::string& text);

inline bool
ifFindAlignStyle(const std::string& text)
{
	for (const std::string& align_style : alignStyleList)
	{
		// 查找字体名称
		if (text.find(align_style) != std::string::npos)
		{
			return true;
		}
	}
	return false; // 未找到
}

inline std::vector<std::string> 
split_string(const std::string& input, const std::regex& regex);

inline std::vector<std::string>
splitByAnd(const std::string& text);

inline bool
isBibBracketBalanced(const std::string& expression);

inline bool 
is_valid_char(char c);


# endif