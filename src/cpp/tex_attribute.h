#ifndef TEX_ATTRIBUTE
#define TEX_ATTRIBUTE

#include <stdexcept>
#include <unordered_map>

// 字号
#define fontsizeList {"tiny", "scriptsize", "footnotesize", "small", "normalsize", "large", "Large", "LARGE", "huge", "Huge"}
#define tiny 5*2
#define scriptsize 7*2
#define footnotesize 8*2
#define small 9*2
#define normalsize 10*2
#define large 12*2
#define Large 14*2
#define LARGE 18*2
#define huge 20*2
#define Huge 24*2

inline int
mapping_fontsize(const std::string& font_string)
{
	if (font_string == "tiny")	return tiny;
	if (font_string == "scriptsize")	return scriptsize;
	if (font_string == "footnotesize")	return footnotesize;
	if (font_string == "small")	return small;
	if (font_string == "normalsize")	return normalsize;
	if (font_string == "large")	return large;
	if (font_string == "Large")	return Large;
	if (font_string == "LARGE")	return LARGE;
	if (font_string == "huge")	return huge;
	if (font_string == "Huge")	return Huge;
}


// 对齐
#define alignStyleList {"raggedleft", "centering", "raggedright", "justifying", "center", "flushleft", "flushright"}
#define LEFT		"left"
#define RIGHT		"right"
#define CENTER		"center"
#define JUSTIFYING	"justifying"

inline std::string
mapping_alignStyle(const std::string& align_style_string)
{
	if (align_style_string == "raggedleft")	return RIGHT;
	if (align_style_string == "centering")	return CENTER;
	if (align_style_string == "raggedright")	return LEFT;
	if (align_style_string == "justifying")	return JUSTIFYING;
	if (align_style_string == "center")	return CENTER;
	if (align_style_string == "flushleft")	return LEFT;
	if (align_style_string == "flushright")	return RIGHT;
}


// 单位
// 单位枚举类型
enum class Unit
{
    INCH,    // 英寸
    CM,      // 厘米
    MM,      // 毫米
    PT,      // 磅 (Point)
    TWIP     // 缇 (Twentieth of a Point)
};

inline Unit
mapping_unit(const std::string& unit)
{
    if (unit == "inch")	return Unit::INCH;
    if (unit == "cm")	return Unit::CM;
    if (unit == "mm")	return Unit::MM;
    if (unit == "pt")	return Unit::PT;
    if (unit == "twip")	return Unit::TWIP;
}

// 单位转换函数
inline double
convertUnit(double value, Unit fromUnit, Unit toUnit)
{
    // 先转换为缇（Twip）作为中间单位
    double valueInTwips = 0.0;

    // 将输入值转换为缇
    switch (fromUnit) 
    {
    case Unit::INCH:
        valueInTwips = value * 1440.0; // 1英寸 = 1440缇
        break;
    case Unit::CM:
        valueInTwips = value * 567.0;   // 1厘米 ≈ 567缇
        break;
    case Unit::MM:
        valueInTwips = value * 56.7;    // 1毫米 ≈ 56.7缇
        break;
    case Unit::PT:
        valueInTwips = value * 20.0;    // 1磅 = 20缇
        break;
    case Unit::TWIP:
        valueInTwips = value;           // 已经是缇
        break;
    default:
        throw std::invalid_argument("Invalid input unit.");
    }

    // 将缇转换为目标单位
    switch (toUnit) 
    {
    case Unit::INCH:
        return valueInTwips / 1440.0;
    case Unit::CM:
        return valueInTwips / 567.0;
    case Unit::MM:
        return valueInTwips / 56.7;
    case Unit::PT:
        return valueInTwips / 20.0;
    case Unit::TWIP:
        return valueInTwips;
    default:
        throw std::invalid_argument("Invalid target unit.");
    }
}


#define textit italic
#define textbf bold


#define PATTERN_RULE R"(\\\s*(toprule|midrule|bottomrule|cmidrule|morecmidrules|specialrule|addlinespace|hline|cline|vline)\s*)"


static const std::unordered_map<std::string, std::string> spectialCharacterMap = {
    {"in",      reinterpret_cast<const char*>(u8"∈")},   // U+2208
    {"cdot",    reinterpret_cast<const char*>(u8"⋅")},  // U+22C5
    {"cdots",   reinterpret_cast<const char*>(u8"⋯")},   // U+22EF
    {"neq",     reinterpret_cast<const char*>(u8"≠")},   // U+2260
    {"times",   reinterpret_cast<const char*>(u8"×")},   // U+00D7
    {"prime",   reinterpret_cast<const char*>(u8"'")},  // U+2032
    {"leq",     reinterpret_cast<const char*>(u8"≤")},   // U+2264
    {"geq",     reinterpret_cast<const char*>(u8"≥")},   // U+2265
    {"forall",  reinterpret_cast<const char*>(u8"∀")},   // U+2200
    {"exists",  reinterpret_cast<const char*>(u8"∃")},   // U+2203
    {"pm",      reinterpret_cast<const char*>(u8"±")},   // U+00B1
    {"mp",      reinterpret_cast<const char*>(u8"∓")},   // U+2213
    {"infty",   reinterpret_cast<const char*>(u8"∞")},   // U+221E
    // 希腊字母（小写）
    {"alpha",   reinterpret_cast<const char*>(u8"α")},   // U+03B1
    {"beta",    reinterpret_cast<const char*>(u8"β")},   // U+03B2
    {"gamma",   reinterpret_cast<const char*>(u8"γ")},   // U+03B3
    {"delta",   reinterpret_cast<const char*>(u8"δ")},   // U+03B4
    {"epsilon", reinterpret_cast<const char*>(u8"ε")},   // U+03B5
    {"zeta",    reinterpret_cast<const char*>(u8"ζ")},   // U+03B6
    {"eta",     reinterpret_cast<const char*>(u8"η")},   // U+03B7
    {"theta",   reinterpret_cast<const char*>(u8"θ")},   // U+03B8
    {"iota",    reinterpret_cast<const char*>(u8"ι")},   // U+03B9
    {"kappa",   reinterpret_cast<const char*>(u8"κ")},   // U+03BA
    {"lambda",  reinterpret_cast<const char*>(u8"λ")},   // U+03BB
    {"mu",      reinterpret_cast<const char*>(u8"μ")},   // U+03BC
    {"nu",      reinterpret_cast<const char*>(u8"ν")},   // U+03BD
    {"xi",      reinterpret_cast<const char*>(u8"ξ")},   // U+03BE
    {"omicron", reinterpret_cast<const char*>(u8"ο")},   // U+03BF
    {"pi",      reinterpret_cast<const char*>(u8"π")},   // U+03C0
    {"rho",     reinterpret_cast<const char*>(u8"ρ")},   // U+03C1
    {"sigma",   reinterpret_cast<const char*>(u8"σ")},   // U+03C3
    {"tau",     reinterpret_cast<const char*>(u8"τ")},   // U+03C4
    {"upsilon", reinterpret_cast<const char*>(u8"υ")},   // U+03C5
    {"phi",     reinterpret_cast<const char*>(u8"φ")},   // U+03C6
    {"chi",     reinterpret_cast<const char*>(u8"χ")},   // U+03C7
    {"psi",     reinterpret_cast<const char*>(u8"ψ")},   // U+03C8
    {"omega",   reinterpret_cast<const char*>(u8"ω")},   // U+03C9
    // 希腊字母（大写）
    {"Alpha",   reinterpret_cast<const char*>(u8"Α")},   // U+0391
    {"Beta",    reinterpret_cast<const char*>(u8"Β")},   // U+0392
    {"Gamma",   reinterpret_cast<const char*>(u8"Γ")},   // U+0393
    {"Delta",   reinterpret_cast<const char*>(u8"Δ")},   // U+0394
    {"Epsilon", reinterpret_cast<const char*>(u8"Ε")},   // U+0395
    {"Zeta",    reinterpret_cast<const char*>(u8"Ζ")},   // U+0396
    {"Eta",     reinterpret_cast<const char*>(u8"Η")},   // U+0397
    {"Theta",   reinterpret_cast<const char*>(u8"Θ")},   // U+0398
    {"Iota",    reinterpret_cast<const char*>(u8"Ι")},   // U+0399
    {"Kappa",   reinterpret_cast<const char*>(u8"Κ")},   // U+039A
    {"Lambda",  reinterpret_cast<const char*>(u8"Λ")},   // U+039B
    {"Mu",      reinterpret_cast<const char*>(u8"Μ")},   // U+039C
    {"Nu",      reinterpret_cast<const char*>(u8"Ν")},   // U+039D
    {"Xi",      reinterpret_cast<const char*>(u8"Ξ")},   // U+039E
    {"Omicron", reinterpret_cast<const char*>(u8"Ο")},   // U+039F
    {"Pi",      reinterpret_cast<const char*>(u8"Π")},   // U+03A0
    {"Rho",     reinterpret_cast<const char*>(u8"Ρ")},   // U+03A1
    {"Sigma",   reinterpret_cast<const char*>(u8"Σ")},   // U+03A3
    {"Tau",     reinterpret_cast<const char*>(u8"Τ")},   // U+03A4
    {"Upsilon", reinterpret_cast<const char*>(u8"Υ")},   // U+03A5
    {"Phi",     reinterpret_cast<const char*>(u8"Φ")},   // U+03A6
    {"Chi",     reinterpret_cast<const char*>(u8"Χ")},   // U+03A7
    {"Psi",     reinterpret_cast<const char*>(u8"Ψ")},   // U+03A8
    {"Omega",   reinterpret_cast<const char*>(u8"Ω")},   // U+03A9
    // 希腊字母变体
    {"vartheta", reinterpret_cast<const char*>(u8"ϑ")},  // U+03D1
    {"varphi",   reinterpret_cast<const char*>(u8"ϕ")},  // U+03D5
    {"varrho",   reinterpret_cast<const char*>(u8"ϱ")},  // U+03F1
    {"varepsilon", reinterpret_cast<const char*>(u8"ϵ")}, // U+03F5
    {"varsigma", reinterpret_cast<const char*>(u8"ς")},  // U+03C2 (词尾sigma)
    {"varpi",    reinterpret_cast<const char*>(u8"ϖ")},  // U+03D6
    // 其他数学希腊字母符号
    {"partial",  reinterpret_cast<const char*>(u8"∂")},  // U+2202 (偏微分符号)
    {"nabla",    reinterpret_cast<const char*>(u8"∇")},  // U+2207 (纳布拉算子)
    // 其他
    {"to",      reinterpret_cast<const char*>(u8"→")},   // U+2192
    {"leftarrow",  reinterpret_cast<const char*>(u8"←")},// U+2190
    {"Rightarrow", reinterpret_cast<const char*>(u8"⇒")},// U+21D2
    {"Leftarrow",  reinterpret_cast<const char*>(u8"⇐")},// U+21D0
    {"int",     reinterpret_cast<const char*>(u8"∫")},  // U+222B
    {"\\langle",  reinterpret_cast<const char*>(u8"⟨")},  // U+27E8
    {"\\rangle",  reinterpret_cast<const char*>(u8"⟩")},  // U+27E9
    {"(",       reinterpret_cast<const char*>(u8"(")},  // U+0028
    {")",       reinterpret_cast<const char*>(u8")")},  // U+0029
    {"\\left(", reinterpret_cast<const char*>(u8"(")},
    {"\\right)", reinterpret_cast<const char*>(u8")")},
    {"[",       reinterpret_cast<const char*>(u8"[")},  // U+005B
    {"]",       reinterpret_cast<const char*>(u8"]")},  // U+005D
    {"\\left[", reinterpret_cast<const char*>(u8"[")},
    {"\\right]", reinterpret_cast<const char*>(u8"]")},
    {"\\{",       reinterpret_cast<const char*>(u8"{")},  // U+007B
    {"\\}",       reinterpret_cast<const char*>(u8"}")},  // U+007D
    {"\\left\\{",       reinterpret_cast<const char*>(u8"{")},  // U+007B
    {"\\right\\}",      reinterpret_cast<const char*>(u8"}")},  // U+007D
    {"sum",     reinterpret_cast<const char*>(u8"∑")},  // U+2211
    {"begin{cases}",    reinterpret_cast<const char*>(u8"{")},
    {"end{cases}",      reinterpret_cast<const char*>(u8"")},
    {"\\bigl(",  reinterpret_cast<const char*>(u8"(")},
    {"\\bigr)",  reinterpret_cast<const char*>(u8")")},
    {"\\Bigl(",  reinterpret_cast<const char*>(u8"(")},
    {"\\Bigr)",  reinterpret_cast<const char*>(u8")")},
    {"\\biggl(",  reinterpret_cast<const char*>(u8"(")},
    {"\\biggr)",  reinterpret_cast<const char*>(u8")")},
    {"\\Biggl(",  reinterpret_cast<const char*>(u8"(")},
    {"\\Biggr)",  reinterpret_cast<const char*>(u8")")},
    { "\\bigl[",  reinterpret_cast<const char*>(u8"[") },
    { "\\bigr]",  reinterpret_cast<const char*>(u8"]") },
    { "\\Bigl[",  reinterpret_cast<const char*>(u8"[") },
    { "\\Bigr]",  reinterpret_cast<const char*>(u8"]") },
    { "\\biggl[",  reinterpret_cast<const char*>(u8"[") },
    { "\\biggr]",  reinterpret_cast<const char*>(u8"]") },
    { "\\Biggl[",  reinterpret_cast<const char*>(u8"[") },
    { "\\Biggr]",  reinterpret_cast<const char*>(u8"]") },
    { "\\bigl\\{",  reinterpret_cast<const char*>(u8"{") },
    { "\\bigr\\}",  reinterpret_cast<const char*>(u8"}") },
    { "\\Bigl\\{",  reinterpret_cast<const char*>(u8"{") },
    { "\\Bigr\\}",  reinterpret_cast<const char*>(u8"}") },
    { "\\biggl\\{",  reinterpret_cast<const char*>(u8"{") },
    { "\\biggr\\}",  reinterpret_cast<const char*>(u8"}") },
    { "\\Biggl\\{",  reinterpret_cast<const char*>(u8"{") },
    { "\\Biggr\\}",  reinterpret_cast<const char*>(u8"}") },
    {"\\|",         reinterpret_cast<const char*>(u8"‖")},

};


static const std::unordered_map<std::string, std::string> BracketPairs = {
    {"\\begin{cases}", "\\end{cases}"},
    {"\\left(", "\\right)"},
    {"\\left[", "\\right]"},
    {"\\left\\{", "\\right\\}"},
    {"\\left\\langle", "\\right\\rangle"},
    {"\\langle", "\\rangle"},
    {"\\bigl(", "\\bigr)"},
    {"\\Bigl(", "\\Bigr)"},
    {"\\biggl(", "\\biggr)"},
    {"\\Biggl(", "\\Biggr)"},
    {"\\bigl[", "\\bigr]"},
    {"\\Bigl[", "\\Bigr]"},
    {"\\biggl[", "\\biggr]"},
    {"\\Biggl[", "\\Biggr]"},
    {"\\bigl\\{", "\\bigr\\}"},
    {"\\Bigl\\{", "\\Bigr\\}"},
    {"\\biggl\\{", "\\biggr\\}"},
    {"\\Biggl\\{", "\\Biggr\\}"},
    {"\\|", "\\|"},

};


#endif