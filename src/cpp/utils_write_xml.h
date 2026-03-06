#pragma once
#include "word/main/document.hpp"
#include "word/main/section.hpp"
#include "word/main/paragraph.hpp"
#include "word/main/richtext.hpp"
#include "word/main/picture.hpp"
#include "word/main/table.hpp"
#include "word/main/cell.hpp"
#include "utils/file.hpp"
#include "utils/string.hpp"
#include "utils/exceptions.hpp"

#include "pugixml.hpp"


static void writeSectionProperties(pugi::xml_node w_pPr, const md::SectionProperties& prop)
{
    pugi::xml_node w_sectPr = w_pPr.append_child("w:sectPr");

    pugi::xml_node w_type = w_sectPr.append_child("w:type");
    w_type.append_attribute("w:val") = "nextPage";

    pugi::xml_node w_pgSz = w_sectPr.append_child("w:pgSz");
    if (prop.landscape_) {
        w_pgSz.append_attribute("w:w") = prop.size_.height_;
        w_pgSz.append_attribute("w:h") = prop.size_.width_;
        w_pgSz.append_attribute("w:orient") = "landscape";
    }
    else {
        w_pgSz.append_attribute("w:w") = prop.size_.width_;
        w_pgSz.append_attribute("w:h") = prop.size_.height_;
    }

    pugi::xml_node w_pgMar = w_sectPr.append_child("w:pgMar");
    w_pgMar.append_attribute("w:top") = prop.margins_.top_;
    w_pgMar.append_attribute("w:bottom") = prop.margins_.bottom_;
    w_pgMar.append_attribute("w:left") = prop.margins_.left_;
    w_pgMar.append_attribute("w:right") = prop.margins_.right_;
    w_pgMar.append_attribute("w:header") = prop.margins_.header_;
    w_pgMar.append_attribute("w:footer") = prop.margins_.footer_;
    w_pgMar.append_attribute("w:gutter") = prop.margins_.gutter_;
}


static void writeParagraphAlignment(pugi::xml_node w_pPr, const md::Alignment& align)
{
    switch (align) {
    case md::Alignment::Left:
        w_pPr.append_child("w:jc").append_attribute("w:val") = "start";
        break;

    case md::Alignment::Right:
        w_pPr.append_child("w:jc").append_attribute("w:val") = "end";
        break;

    case md::Alignment::Centered:
        w_pPr.append_child("w:jc").append_attribute("w:val") = "center";
        break;

    case md::Alignment::Justified:
        w_pPr.append_child("w:jc").append_attribute("w:val") = "both";
        break;

    case md::Alignment::Distributed:
        w_pPr.append_child("w:jc").append_attribute("w:val") = "distribute";
    }
}


static void writeParagraphIndentation(pugi::xml_node w_pPr, const md::ParagraphProperties::Indentation& indent)
{
    pugi::xml_node w_ind = w_pPr.append_child("w:ind");

    w_ind.append_attribute(
        indent.left_.chars_ ? "w:leftChars" : "w:left") = indent.left_.value_;

    w_ind.append_attribute(
        indent.right_.chars_ ? "w:rightChars" : "w:right") = indent.right_.value_;

    switch (indent.special_.type_)
    {
    case md::ParagraphProperties::SpecialIndentationType::FirstLine:
        w_ind.append_attribute(
            indent.special_.chars_ ? "w:firstLineChars" : "w:firstLine") = indent.special_.value_;
        break;

    case md::ParagraphProperties::SpecialIndentationType::Hanging:
        w_ind.append_attribute(
            indent.special_.chars_ ? "w:hangingChars" : "w:hanging") = indent.special_.value_;
        break;
    }
}


static void writeBorderProperties(pugi::xml_node w_border, const md::BorderProperties& prop)
{
    switch (prop.style_) {
    case md::BorderStyle::Single:
        w_border.append_attribute("w:val") = "single";
        break;

    case md::BorderStyle::Double:
        w_border.append_attribute("w:val") = "double";
        break;

    case md::BorderStyle::Triple:
        w_border.append_attribute("w:val") = "triple";
        break;

    case md::BorderStyle::Dotted:
        w_border.append_attribute("w:val") = "dotted";
        break;

    case md::BorderStyle::Dashed:
        w_border.append_attribute("w:val") = "dashed";
        break;

    case md::BorderStyle::DotDash:
        w_border.append_attribute("w:val") = "dotDash";
        break;

    case md::BorderStyle::Wave:
        w_border.append_attribute("w:val") = "wave";
        break;

    case md::BorderStyle::DoubleWave:
        w_border.append_attribute("w:val") = "doubleWave";
        break;
    }

    w_border.append_attribute("w:sz") = prop.width_;
    w_border.append_attribute("w:color") = prop.color_.c_str();
}


static void writeParagraphBorders(pugi::xml_node w_pPr, const md::ParagraphProperties::ParagraphBorders& borders)
{
    pugi::xml_node w_pBdr = w_pPr.append_child("w:pBdr");

    pugi::xml_node w_top = w_pBdr.append_child("w:top");
    pugi::xml_node w_bottom = w_pBdr.append_child("w:bottom");
    pugi::xml_node w_between = w_pBdr.append_child("w:between");
    pugi::xml_node w_left = w_pBdr.append_child("w:left");
    pugi::xml_node w_right = w_pBdr.append_child("w:right");

    writeBorderProperties(w_top, borders.top_);
    writeBorderProperties(w_bottom, borders.bottom_);
    writeBorderProperties(w_between, borders.bottom_);
    writeBorderProperties(w_left, borders.left_);
    writeBorderProperties(w_right, borders.right_);
}


static void writeParagraphSpacing(pugi::xml_node w_pPr, const md::ParagraphProperties::Spacing& spacing)
{
    pugi::xml_node w_spacing = w_pPr.append_child("w:spacing");

    switch (spacing.before_.type_)
    {
    case md::ParagraphProperties::SpacingType::Auto:
        w_spacing.append_attribute("w:beforeAutospacing") = "1";
        break;

    case md::ParagraphProperties::SpacingType::Lines:
        w_spacing.append_attribute("w:beforeAutospacing") = "0";
        w_spacing.append_attribute("w:beforeLines") = spacing.before_.value_;
        break;

    case md::ParagraphProperties::SpacingType::Absolute:
        w_spacing.append_attribute("w:beforeAutospacing") = "0";
        w_spacing.append_attribute("w:before") = spacing.before_.value_;
        break;
    }

    switch (spacing.after_.type_)
    {
    case md::ParagraphProperties::SpacingType::Auto:
        w_spacing.append_attribute("w:afterAutospacing") = "1";
        break;

    case md::ParagraphProperties::SpacingType::Lines:
        w_spacing.append_attribute("w:afterAutospacing") = "0";
        w_spacing.append_attribute("w:afterLines") = spacing.after_.value_;
        break;

    case md::ParagraphProperties::SpacingType::Absolute:
        w_spacing.append_attribute("w:afterAutospacing") = "0";
        w_spacing.append_attribute("w:after") = spacing.after_.value_;
        break;
    }

    switch (spacing.lineSpacing_.type_)
    {
    case md::ParagraphProperties::LineSpacingType::Lines:
        w_spacing.append_attribute("w:lineRule") = "auto";
        w_spacing.append_attribute("w:line") = spacing.lineSpacing_.value_;
        break;

    case md::ParagraphProperties::LineSpacingType::AtLeast:
        w_spacing.append_attribute("w:lineRule") = "atLeast";
        w_spacing.append_attribute("w:line") = spacing.lineSpacing_.value_;
        break;

    case md::ParagraphProperties::LineSpacingType::Exactly:
        w_spacing.append_attribute("w:lineRule") = "exact";
        w_spacing.append_attribute("w:line") = spacing.lineSpacing_.value_;
        break;
    }
}


std::string removeSpaces(std::string str) {
    std::string tmp{ std::move(str) };
    //tmp.erase(std::remove_if(tmp.begin(), tmp.end(), std::isspace), tmp.end());
    tmp.erase(std::remove_if(tmp.begin(), tmp.end(), [](unsigned char c) { return std::isspace(c); }), tmp.end());
    return tmp;
}


static void writeParagraphProperties(pugi::xml_node w_pPr, const md::ParagraphProperties& prop)
{
    if (prop.style_.size() > 0)
        w_pPr.append_child("w:pStyle").append_attribute("w:val") = removeSpaces(prop.style_).c_str();

    if (prop.align_.has_value())
        writeParagraphAlignment(w_pPr, prop.align_.value());

    if (prop.outlineLevel_ != md::ParagraphProperties::OutlineLevel::BodyText)
        w_pPr.append_child("w:outlineLvl").append_attribute("w:val") = static_cast<unsigned int>(prop.outlineLevel_);

    if (prop.indent_.has_value())
        writeParagraphIndentation(w_pPr, prop.indent_.value());

    if (prop.spacing_.has_value())
        writeParagraphSpacing(w_pPr, prop.spacing_.value());

    if (prop.borders_.has_value())
        writeParagraphBorders(w_pPr, prop.borders_.value());

    if (prop.keepNext_)
        w_pPr.append_child("w:keepNext");

    if (prop.keepLines_)
        w_pPr.append_child("w:keeplines");

    if (prop.pageBreakBefore_)
        w_pPr.append_child("w:pageBreakBefore");
}


static int hasChar(const char ch, const char* list, const size_t len)
{
    for (int i = 0; i < len; i++)
        if (list[i] == ch)
            return i;
    return -1;
}


static void writeText(pugi::xml_node w_r, const char* str, const size_t len, const bool whitespace)
{
    if (len > 0) {
        const char chars[] = "\n\t\r";
        const char* tags[] = { "w:br", "w:tab", nullptr };
        const size_t num = sizeof(chars) - 1;

        const char* start = str;
        const char* const end = start + len;
        const char* p = start;

        int index = -1;
        while (p < end && -1 == (index = hasChar(*p, chars, num))) {
            p++;
        }
        while (p < end) {
            const int count = p - start;
            if (count > 0) {
                pugi::xml_node w_t = w_r.append_child("w:t");
                if (whitespace)
                    w_t.append_attribute("xml:space") = "preserve";
                w_t.text().set(start, count);
            }

            auto tag = tags[index];
            if (tag != nullptr)
                w_r.append_child(tag);

            start = ++p;
            if (start >= end)
                break;

            while (p < end && -1 == (index = hasChar(*p, chars, num))) {
                p++;
            }
        }
        if (start < end) {
            w_r.append_child("w:t").text().set(start);
        }
    }
}


static void writeRichTextProperties(pugi::xml_node w_rPr, const md::RichTextProperties& prop)
{
    if (prop.style_.size() > 0)
        w_rPr.append_child("w:rStyle").append_attribute("w:val") = removeSpaces(prop.style_).c_str();

    if (prop.font_.has_value()) {
        pugi::xml_node w_rFonts = w_rPr.append_child("w:rFonts");
        const md::RichTextProperties::Font& font = prop.font_.value();

        if (font.ascii_.size() > 0)
            w_rFonts.append_attribute("w:ascii") = font.ascii_.c_str();

        if (font.eastAsia_.size() > 0)
            w_rFonts.append_attribute("w:eastAsia") = font.eastAsia_.c_str();

        if (font.hAnsi_.size() > 0)
            w_rFonts.append_attribute("w:hAnsi") = font.hAnsi_.c_str();

        if (font.cs_.size() > 0)
            w_rFonts.append_attribute("w:cs") = font.cs_.c_str();

        switch (font.hint_)
        {
        case md::RichTextProperties::FontTypeHint::EastAsia:
            w_rFonts.append_attribute("w:hint") = "eastAsia";
            break;

        case md::RichTextProperties::FontTypeHint::ComplexScript:
            w_rFonts.append_attribute("w:hint") = "cs";
            break;
        }
    }

    if (prop.fontStyle_.bold_) {
        w_rPr.append_child("w:b");
        w_rPr.append_child("w:bCs");
    }

    if (prop.fontStyle_.italic_) {
        w_rPr.append_child("w:i");
        w_rPr.append_child("w:iCs");
    }

    if (prop.fontSize_ > 0) {
        w_rPr.append_child("w:sz").append_attribute("w:val") = prop.fontSize_;
        w_rPr.append_child("w:szCs").append_attribute("w:val") = prop.fontSize_;
    }

    if (prop.color_.size() > 0) {
        w_rPr.append_child("w:color").
            append_attribute("w:val") = prop.color_.c_str();
    }

    if (prop.underline_.style_ != md::RichTextProperties::UnderlineStyle::None) {
        pugi::xml_node w_u = w_rPr.append_child("w:u");

        switch (prop.underline_.style_)
        {
        case md::RichTextProperties::UnderlineStyle::Words:
            w_u.append_attribute("w:val") = "words";
            break;

        case md::RichTextProperties::UnderlineStyle::Single:
            w_u.append_attribute("w:val") = "single";
            break;

        case md::RichTextProperties::UnderlineStyle::Double:
            w_u.append_attribute("w:val") = "double";
            break;

        case md::RichTextProperties::UnderlineStyle::Thick:
            w_u.append_attribute("w:val") = "thick";
            break;

        case md::RichTextProperties::UnderlineStyle::Dotted:
            w_u.append_attribute("w:val") = "dotted";
            break;

        case md::RichTextProperties::UnderlineStyle::DottedHeavy:
            w_u.append_attribute("w:val") = "dottedHeavy";
            break;

        case md::RichTextProperties::UnderlineStyle::Dash:
            w_u.append_attribute("w:val") = "dash";
            break;

        case md::RichTextProperties::UnderlineStyle::DashedHeavy:
            w_u.append_attribute("w:val") = "dashedHeavy";
            break;

        case md::RichTextProperties::UnderlineStyle::DashLong:
            w_u.append_attribute("w:val") = "dashLong";
            break;

        case md::RichTextProperties::UnderlineStyle::DashLongHeavy:
            w_u.append_attribute("w:val") = "dashLongHeavy";
            break;

        case md::RichTextProperties::UnderlineStyle::DotDash:
            w_u.append_attribute("w:val") = "dotDash";
            break;

        case md::RichTextProperties::UnderlineStyle::DashDotHeavy:
            w_u.append_attribute("w:val") = "dashDotHeavy";
            break;

        case md::RichTextProperties::UnderlineStyle::DotDotDash:
            w_u.append_attribute("w:val") = "dotDotDash";
            break;

        case md::RichTextProperties::UnderlineStyle::DashDotDotHeavy:
            w_u.append_attribute("w:val") = "dashDotDotHeavy";
            break;

        case md::RichTextProperties::UnderlineStyle::Wave:
            w_u.append_attribute("w:val") = "wave";
            break;

        case md::RichTextProperties::UnderlineStyle::WavyDouble:
            w_u.append_attribute("w:val") = "wavyDouble";
            break;

        case md::RichTextProperties::UnderlineStyle::WavyHeavy:
            w_u.append_attribute("w:val") = "wavyHeavy";
            break;
        }

        w_u.append_attribute("w:color") = prop.underline_.color_.c_str();
    }

    switch (prop.effects_.strike_)
    {
    case md::RichTextProperties::StrikeStyle::Single:
        w_rPr.append_child("w:strike");
        break;
    case md::RichTextProperties::StrikeStyle::Double:
        w_rPr.append_child("w:dstrike");
        break;
    }

    switch (prop.effects_.vertAlign_)
    {
    case md::RichTextProperties::VertAlign::Superscript:
        w_rPr.append_child("w:vertAlign").append_attribute("w:val") = "superscript";
        break;
    case md::RichTextProperties::VertAlign::Subscript:
        w_rPr.append_child("w:vertAlign").append_attribute("w:val") = "subscript";
        break;
    }

    switch (prop.highlight_)
    {
    case md::RichTextProperties::Highlight::Black:
        w_rPr.append_child("w:highlight").append_attribute("w:val") = "black";
        break;
    case md::RichTextProperties::Highlight::White:
        w_rPr.append_child("w:highlight").append_attribute("w:val") = "white";
        break;
    case md::RichTextProperties::Highlight::Red:
        w_rPr.append_child("w:highlight").append_attribute("w:val") = "red";
        break;
    case md::RichTextProperties::Highlight::Green:
        w_rPr.append_child("w:highlight").append_attribute("w:val") = "green";
        break;
    case md::RichTextProperties::Highlight::Blue:
        w_rPr.append_child("w:highlight").append_attribute("w:val") = "blue";
        break;
    case md::RichTextProperties::Highlight::Yellow:
        w_rPr.append_child("w:highlight").append_attribute("w:val") = "yellow";
        break;
    case md::RichTextProperties::Highlight::Cyan:
        w_rPr.append_child("w:highlight").append_attribute("w:val") = "cyan";
        break;
    case md::RichTextProperties::Highlight::Magenta:
        w_rPr.append_child("w:highlight").append_attribute("w:val") = "magenta";
        break;
    case md::RichTextProperties::Highlight::DarkRed:
        w_rPr.append_child("w:highlight").append_attribute("w:val") = "darkRed";
        break;
    case md::RichTextProperties::Highlight::DarkGreen:
        w_rPr.append_child("w:highlight").append_attribute("w:val") = "darkGreen";
        break;
    case md::RichTextProperties::Highlight::DarkBlue:
        w_rPr.append_child("w:highlight").append_attribute("w:val") = "darkBlue";
        break;
    case md::RichTextProperties::Highlight::DarkYellow:
        w_rPr.append_child("w:highlight").append_attribute("w:val") = "darkYellow";
        break;
    case md::RichTextProperties::Highlight::DarkCyan:
        w_rPr.append_child("w:highlight").append_attribute("w:val") = "darkCyan";
        break;
    case md::RichTextProperties::Highlight::DarkMagenta:
        w_rPr.append_child("w:highlight").append_attribute("w:val") = "darkMagenta";
        break;
    case md::RichTextProperties::Highlight::DarkGray:
        w_rPr.append_child("w:highlight").append_attribute("w:val") = "darkGray";
        break;
    case md::RichTextProperties::Highlight::LightGray:
        w_rPr.append_child("w:highlight").append_attribute("w:val") = "lightGray";
        break;
    }

    if (prop.scale_ != 100)
        w_rPr.append_child("w:w").append_attribute("w:val") = prop.scale_;

    switch (prop.spacing_.type_)
    {
    case md::RichTextProperties::SpacingType::Normal:
        break;
    case md::RichTextProperties::SpacingType::Expanded:
        w_rPr.append_child("w:spacing").append_attribute("w:val") = prop.spacing_.by_;
        break;
    case md::RichTextProperties::SpacingType::Condensed:
        w_rPr.append_child("w:spacing").append_attribute("w:val") = -static_cast<long long>(prop.spacing_.by_);
        break;
    }

    switch (prop.position_.type_)
    {
    case md::RichTextProperties::PositionType::Normal:
        break;
    case md::RichTextProperties::PositionType::Raised:
        w_rPr.append_child("w:position").append_attribute("w:val") = prop.position_.by_;
        break;
    case md::RichTextProperties::PositionType::Lowered:
        w_rPr.append_child("w:position").append_attribute("w:val") = -static_cast<long long>(prop.position_.by_);
        break;
    }

    if (prop.border_.has_value()) {
        pugi::xml_node w_bdr = w_rPr.append_child("w:bdr");
        writeBorderProperties(w_bdr, prop.border_.value());
    }
}


static void writeTableBorders(pugi::xml_node w_tblPr, const md::TableBorders& borders)
{
    pugi::xml_node w_tblBorders = w_tblPr.append_child("w:tblBorders");

    pugi::xml_node w_top = w_tblBorders.append_child("w:top");
    pugi::xml_node w_bottom = w_tblBorders.append_child("w:bottom");
    pugi::xml_node w_start = w_tblBorders.append_child("w:start");
    pugi::xml_node w_end = w_tblBorders.append_child("w:end");
    pugi::xml_node w_insideH = w_tblBorders.append_child("w:insideH");
    pugi::xml_node w_insideV = w_tblBorders.append_child("w:insideV");

    writeBorderProperties(w_top, borders.top_);
    writeBorderProperties(w_bottom, borders.bottom_);
    writeBorderProperties(w_start, borders.left_);
    writeBorderProperties(w_end, borders.right_);
    writeBorderProperties(w_insideH, borders.insideHorizontal_);
    writeBorderProperties(w_insideV, borders.insideVertical_);
}


static void writeTableProperties(pugi::xml_node w_tblPr, const md::TableProperties& prop)
{
    if (prop.layout_ == md::TableProperties::Layout::Fixed)
        w_tblPr.append_child("w:tblLayout").append_attribute("w:type") = "fixed";

    pugi::xml_node w_tblW = w_tblPr.append_child("w:tblW");
    switch (prop.width_.type_)
    {
    case md::TableProperties::WidthType::Auto:
        w_tblW.append_attribute("w:type") = "auto";
        break;

    case md::TableProperties::WidthType::Percent:
        w_tblW.append_attribute("w:type") = "pct";
        w_tblW.append_attribute("w:w") = prop.width_.value_;
        break;

    case md::TableProperties::WidthType::Absolute:
        w_tblW.append_attribute("w:type") = "dxa";
        w_tblW.append_attribute("w:w") = prop.width_.value_;
        break;
    }

    pugi::xml_node w_jc = w_tblPr.append_child("w:jc");
    switch (prop.align_) {
    case md::TableProperties::Alignment::Left:
        w_jc.append_attribute("w:val") = "start";
        break;

    case md::TableProperties::Alignment::Right:
        w_jc.append_attribute("w:val") = "end";
        break;

    case md::TableProperties::Alignment::Center:
        w_jc.append_attribute("w:val") = "center";
        break;
    }

    writeTableBorders(w_tblPr, prop.borders_);
}



