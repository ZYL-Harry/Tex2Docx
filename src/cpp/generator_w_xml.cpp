#include"generator_w_xml.h"
#include"document_ast.h"
#include<zip.h>
#include<stdexcept>
#include <sstream>
#include <iostream>


/* 拷贝构造 */
WordGenerator::WordGenerator(const WordGenerator& word_generator) {}


/* 拷贝赋值 */
WordGenerator&
WordGenerator::operator=(const WordGenerator& str) {}


/* 析构函数 */
WordGenerator::~WordGenerator() {}


/* 函数：word文件生成 */
void
WordGenerator::generate()
{
    // 创建zip文件
    int error = 0;
    this->zip_archive = zip_open(this->word_path.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &error);
    if (!this->zip_archive)
    {
        throw std::runtime_error("Failed to create zip archive: " + std::to_string(error) + ".");
    }
    // 创建基本zip结构
    this->create_zip_structure();
    // 生成各个xml文件
    this->generate_content_types();
    this->generate_rels();
    this->generate_document();
    this->generate_styles();
    this->generate_core_props();
    this->generate_app_props();
    // 关闭zip文件，完成docx文件的创建
    if (zip_close(this->zip_archive) < 0)
    {
        throw std::runtime_error("Failed to close zip archive.");
    }
}


void
WordGenerator::create_zip_structure()
{
    const char* folders[] = { "_rels/", "word/", "word/_rels/", "docProps/" };
    for (const char* folder : folders)
    {
        if (zip_dir_add(this->zip_archive, folder, ZIP_FL_ENC_GUESS) < 0)
        {
            throw std::runtime_error("Failed to create directory: " + std::string(folder) + ".");
        }
    }
}


void 
WordGenerator::add_file_to_zip(const std::string& path, const std::string& content)
{
    zip_source* source = zip_source_buffer(this->zip_archive, content.c_str(), content.size(), 0);
    if (source) {
        if (zip_file_add(this->zip_archive, path.c_str(), source, ZIP_FL_OVERWRITE) < 0) {
            zip_source_free(source);
            throw std::runtime_error("Failed to add file to zip: " + path);
        }
    }
    else {
        throw std::runtime_error("Failed to create zip source for: " + path);
    }
}


void
WordGenerator::add_xml_file_to_zip(const std::string& path, const pugi::xml_document& content)
{
    std::stringstream stream;
    content.save(stream);
    this->add_file_to_zip(path, stream.str());
}


/* 函数：生成文章格式属性 */
void
WordGenerator::generate_attribute(const std::shared_ptr<AST::Attribute>& ast_attribute)
{
    // 创建xml文档对象
    std::unique_ptr<pugi::xml_document> styles_xml = std::make_unique<pugi::xml_document>();
    std::cout << ast_attribute << std::endl;
    // 添加xml声明
    pugi::xml_node decl = styles_xml->prepend_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "UTF-8";
    // 创建根节点
    pugi::xml_node styles = styles_xml->append_child("w:styles");
    styles.append_attribute("xmlns:w") = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"; // 添加命名空间声明，指向WordProcessingML规范
    // 依据AST配置属性
    std::string attribute_content = std::get<std::shared_ptr<AST::AttributeItem>>(ast_attribute->items[0])->content;


    // 生成xml文件
    this->add_xml_file_to_zip("word/styles.xml", *styles_xml);
}


void
WordGenerator::generate_content_types() 
{
    std::string content = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
    <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
    <Default Extension="xml" ContentType="application/xml"/>
    <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
    <Override PartName="/word/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/>
    <Override PartName="/docProps/core.xml" ContentType="application/vnd.openxmlformats-package.core-properties+xml"/>
    <Override PartName="/docProps/app.xml" ContentType="application/vnd.openxmlformats-officedocument.extended-properties+xml"/>
</Types>)";
    this->add_file_to_zip("[Content_Types].xml", content);
}


void
WordGenerator::generate_rels()
{
    std::string relsContent = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
    <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/document.xml"/>
    <Relationship Id="rId2" Type="http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties" Target="docProps/core.xml"/>
    <Relationship Id="rId3" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties" Target="docProps/app.xml"/>
</Relationships>)";

    this->add_file_to_zip("_rels/.rels", relsContent);

    std::string documentRels = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
    <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/>
</Relationships>)";

    this->add_file_to_zip("word/_rels/document.xml.rels", documentRels);
}


void
WordGenerator::generate_document()
{
    pugi::xml_document doc;

    pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "UTF-8";
    decl.append_attribute("standalone") = "yes";

    pugi::xml_node document = doc.append_child("w:document");
    document.append_attribute("xmlns:w") = "http://schemas.openxmlformats.org/wordprocessingml/2006/main";
    document.append_attribute("xmlns:r") = "http://schemas.openxmlformats.org/officeDocument/2006/relationships";

    pugi::xml_node body = document.append_child("w:body");

    // 添加一个标题
    pugi::xml_node heading = body.append_child("w:p");
    pugi::xml_node headingPr = heading.append_child("w:pPr");
    pugi::xml_node headingStyle = headingPr.append_child("w:pStyle");
    headingStyle.append_attribute("w:val") = "Heading1";
    pugi::xml_node headingRun = heading.append_child("w:r");
    pugi::xml_node headingText = headingRun.append_child("w:t");
    headingText.text().set("Example Title");

    // 添加一个段落
    pugi::xml_node paragraph = body.append_child("w:p");
    pugi::xml_node run = paragraph.append_child("w:r");
    pugi::xml_node text = run.append_child("w:t");
    text.text().set("This is a simple .docx file generated from LaTeX.");

    // 添加节属性（sectPr）
    pugi::xml_node sectPr = body.append_child("w:sectPr");
    pugi::xml_node pgSz = sectPr.append_child("w:pgSz");
    pgSz.append_attribute("w:w") = "11906";  // A4纸宽度（twips）
    pgSz.append_attribute("w:h") = "16838";  // A4纸高度（twips）
    pugi::xml_node pgMar = sectPr.append_child("w:pgMar");
    pgMar.append_attribute("w:top") = "1440";
    pgMar.append_attribute("w:right") = "1440";
    pgMar.append_attribute("w:bottom") = "1440";
    pgMar.append_attribute("w:left") = "1440";
    pgMar.append_attribute("w:header") = "708";
    pgMar.append_attribute("w:footer") = "708";
    pgMar.append_attribute("w:gutter") = "0";
    pugi::xml_node cols = sectPr.append_child("w:cols");
    cols.append_attribute("w:space") = "708";
    pugi::xml_node docGrid = sectPr.append_child("w:docGrid");
    docGrid.append_attribute("w:linePitch") = "360";

    this->add_xml_file_to_zip("word/document.xml", doc);
}



void
WordGenerator::generate_styles()
{
    pugi::xml_document styles;
    pugi::xml_node decl = styles.prepend_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "UTF-8";
    decl.append_attribute("standalone") = "yes";

    pugi::xml_node stylesRoot = styles.append_child("w:styles");
    stylesRoot.append_attribute("xmlns:w") = "http://schemas.openxmlformats.org/wordprocessingml/2006/main";

    // 定义Normal样式
    pugi::xml_node normalStyle = stylesRoot.append_child("w:style");
    normalStyle.append_attribute("w:type") = "paragraph";
    normalStyle.append_attribute("w:styleId") = "Normal";
    normalStyle.append_attribute("w:default") = "1";
    pugi::xml_node normalName = normalStyle.append_child("w:name");
    normalName.append_attribute("w:val") = "Normal";
    pugi::xml_node normalPr = normalStyle.append_child("w:pPr");
    pugi::xml_node normalSpacing = normalPr.append_child("w:spacing");
    normalSpacing.append_attribute("w:after") = "0";
    normalSpacing.append_attribute("w:line") = "240";
    normalSpacing.append_attribute("w:lineRule") = "auto";

    // 定义Heading1样式
    pugi::xml_node heading1Style = stylesRoot.append_child("w:style");
    heading1Style.append_attribute("w:type") = "paragraph";
    heading1Style.append_attribute("w:styleId") = "Heading1";
    pugi::xml_node heading1Name = heading1Style.append_child("w:name");
    heading1Name.append_attribute("w:val") = "Heading 1";
    pugi::xml_node heading1Pr = heading1Style.append_child("w:pPr");
    pugi::xml_node heading1Spacing = heading1Pr.append_child("w:spacing");
    heading1Spacing.append_attribute("w:before") = "240";
    heading1Spacing.append_attribute("w:after") = "60";
    pugi::xml_node heading1RunPr = heading1Style.append_child("w:rPr");
    pugi::xml_node heading1Bold = heading1RunPr.append_child("w:b");
    pugi::xml_node heading1Size = heading1RunPr.append_child("w:sz");
    heading1Size.append_attribute("w:val") = "32";
    pugi::xml_node heading1SizeCs = heading1RunPr.append_child("w:szCs");
    heading1SizeCs.append_attribute("w:val") = "32";

    this->add_xml_file_to_zip("word/styles.xml", styles);
}


void
WordGenerator::generate_core_props() {
    std::string content = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<cp:coreProperties xmlns:cp="http://schemas.openxmlformats.org/package/2006/metadata/core-properties" xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:dcterms="http://purl.org/dc/terms/" xmlns:dcmitype="http://purl.org/dc/dcmitype/" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
  <dc:title>Title</dc:title>
  <dc:subject>Subject</dc:subject>
  <dc:creator>Creator</dc:creator>
  <cp:keywords>Keywords</cp:keywords>
  <dc:description>Description</dc:description>
  <cp:lastModifiedBy>Last Modified By</cp:lastModifiedBy>
  <cp:revision>1</cp:revision>
  <dcterms:created xsi:type="dcterms:W3CDTF">2025-09-10T08:47:05Z</dcterms:created>
  <dcterms:modified xsi:type="dcterms:W3CDTF">2025-09-10T08:47:12Z</dcterms:modified>
</cp:coreProperties>)";

    this->add_file_to_zip("docProps/core.xml", content);
}


void
WordGenerator::generate_app_props() {
    std::string content = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Properties xmlns="http://schemas.openxmlformats.org/officeDocument/2006/extended-properties" xmlns:vt="http://schemas.openxmlformats.org/officeDocument/2006/docPropsVTypes">
  <Application>WordGenerator</Application>
  <DocSecurity>0</DocSecurity>
  <ScaleCrop>false</ScaleCrop>
  <HeadingPairs>
    <vt:vector size="2" baseType="variant">
      <vt:variant>
        <vt:lpstr>Title</vt:lpstr>
      </vt:variant>
      <vt:variant>
        <vt:i4>1</vt:i4>
      </vt:variant>
    </vt:vector>
  </HeadingPairs>
  <TitlesOfParts>
    <vt:vector size="1" baseType="lpstr">
      <vt:lpstr>Document</vt:lpstr>
    </vt:vector>
  </TitlesOfParts>
  <Company>Company</Company>
  <LinksUpToDate>false</LinksUpToDate>
  <CharactersWithSpaces>100</CharactersWithSpaces>
  <SharedDoc>false</SharedDoc>
  <HyperlinksChanged>false</HyperlinksChanged>
  <AppVersion>1.0</AppVersion>
</Properties>)";

    this->add_file_to_zip("docProps/app.xml", content);
}
