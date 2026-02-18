* 第一版：实现Tex文件到Docx文件的转换，包括文段、公式、表格、图像、参考文献等。
  * 未完成功能：表格的尺寸控制，伪代码，接口
* 项目结构：
  ```text
  Tex2Docx/
  ├── main.cpp                 // 测试主函数
  ├── document_ast.h           // 抽象语法树
  ├── parse.cpp                // tex文件解析
  ├── parse.h
  ├── parse_template.cpp       // tex文章格式解析
  ├── parse_reference.cpp      // bib参考文献解析
  ├── tex_attribute.h          // tex文本基本属性
  ├── generator.cpp            // docx文件生成
  ├── generator.h
  ├── document_docx.cpp        // docx文件生成辅助
  ├── document_docx.h
  ├── utils_write_xml.h
  ├── document.tex             // 测试转换的tex文件
  ├── refs.bib                 // 测试转换的bib文件
  ├── figures/                 // 转换过程中使用&生成的图片文件
  └── document.docx            // 测试生成的docx文件
* 主要文件说明: 
  * main.cpp：转换示例，将“document.tex”转换为“document.docx”，参考文献使用“refs.bib”，图像的操作目录为“figures/”
  * document_ast.h：基于tex语法结构的抽象语法树，封装文档中元素的结构化表示，包括标题、文段、公式、图像、表格等
  * parse.cpp：Tex文件解析器，基于字符串的提取，将文章结构、内容及属性解析并存储于抽象语法树
      * 主要头文件：parse.h
  * parse_template.cpp：cls文件解析器（以article.cls为例），基于字符串的提取，提取文章格式属性并存储于抽象语法树
  * parse_reference.cpp：bib文件解析器，基于字符串的提取，提取参考文献的关键元素并存储于抽象语法树
  * generator.cpp：Docx文件生成器，根据抽象语法树内容生成Docx文件
      * 主要头文件：generator.h
  * document_docx.cpp：基于minidocx进行Docx文件相关功能的拓展，包括公式、图像、表格等
      * 主要头文件：document_docx.h, utils_write_xml.h
  * tex_attribute.h：tex文件中基本元素的属性，共解析与写入配置使用，包含字号、段落、特殊符号等
* 主要库：
  * minidocx: 生成Docx文件的基本元素，包括设置对齐、段落、字体、大小、颜色、图像、表格
  * pdfium: 读取PDF图片文件，辅助实现图片由PDF到高清PNG的转换
* introduction.txt：实现的具体思路与流程
* 项目配置属性：
  * 开发平台：Visual Studio 2022
  * 语言：ISO C++20 标准 (/std:c++20)
  * 配置-Release，平台-x64
  * 附加包含目录：
    * ...\minidocx-next\minidocx-next\include\minidocx
    * ...\pugixml-master\src
    * ...\pdfium\include\public
  * 附加库目录：
    * ...\minidocx-next\minidocx-next\build\bin\lib\Release
    * ...\pdfium\include\public
  * 附加依赖项：
    * minidocx.lib
    * pdfium.lib
