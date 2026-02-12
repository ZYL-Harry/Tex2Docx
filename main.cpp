#include"document_ast.h"
#include<iostream>
#include"parser.h"
#include"generator.h"
#include <fstream>

using namespace std;

void main()
{
	string tex_path = "../../test_pandoc/document.tex";
	string ref_path = "../../test_pandoc/refs.bib";
	string figure_dir = "figures/";
	TexParser tex_parser(tex_path, ref_path, figure_dir);
	/* 1. 读取bib文件 */
	tex_parser.parse_reference();
	/* 2. 读取tex文件 */
	tex_parser.read_tex_file();
	/* 3. 解析tex文件，映射至抽象语法树（AST） */
	AST::Document& document_ast = tex_parser.parse();
	/* 4. 生成word文件 */
	string docx_path;
	docx_path = "document.docx";
	DocxGenerator docx_generator(docx_path, document_ast);
	docx_generator.generate();

	/*std::ifstream ifs("special_characteristic/word/document.xml", std::ios::binary);
	std::ostringstream oss;
	oss << ifs.rdbuf();
	std::string content = oss.str();*/

	generate_figure_example();

	return;
}