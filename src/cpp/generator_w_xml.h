#ifndef GENERATOR
#define GENERATOR

#include"document_ast.h"
#include<string>
#include"pugixml.hpp"
#include"zip.h"


class WordGenerator
{
public:
	WordGenerator(const std::string path, const AST::Document& ast) : word_path(path), document_ast(ast) {};	//构造函数的声明
	WordGenerator(const WordGenerator& word_generator);		//拷贝构造的声明
	WordGenerator& operator = (const WordGenerator& word_generator);	//拷贝赋值的声明
	~WordGenerator();	//析构函数的声明

	void generate();
	void create_zip_structure();
	void add_file_to_zip(const std::string& path, const std::string& content);		// 函数：添加文件入zip
	void add_xml_file_to_zip(const std::string& path, const pugi::xml_document& content);		// 函数：添加xml文件入zip
	void generate_attribute(const std::shared_ptr<AST::Attribute>& ast_attribute);
	// pugi::xml_node generate_information();
	// pugi::xml_node generate_keyword();
	void generate_content_types();
	void generate_rels();
	void generate_document();
	void generate_styles();
	void generate_core_props();
	void generate_app_props();
	// pugi::xml_node generate_section();
	// pugi::xml_node generate_enumerate();
	// pugi::xml_node generate_figure();
	// pugi::xml_node generate_equation();
	// pugi::xml_node generate_simple_equation();
	// pugi::xml_node generate_table();
	// pugi::xml_node generate_algorithm();

private:
	std::string word_path;			// 数据：word文件地址
	AST::Document document_ast;		// 数据：word文件内容（AST格式）
	zip* zip_archive;		// 编译器不知道结构体zip的大小，所以必须创建指针
};

#endif