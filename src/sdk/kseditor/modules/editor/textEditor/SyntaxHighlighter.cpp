#include "SyntaxHighlighter.h"
#include <QFileInfo>

namespace ks {

SyntaxHighlighter::SyntaxHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
{
    m_keywordFormat.setForeground(QColor(0x56, 0x9c, 0xd6));
    m_keywordFormat.setFontWeight(QFont::Bold);
    m_stringFormat.setForeground(QColor(0xce, 0x91, 0x78));
    m_commentFormat.setForeground(QColor(0x6a, 0x99, 0x5f));
    m_commentFormat.setFontItalic(true);
    m_numberFormat.setForeground(QColor(0xb5, 0xce, 0xa8));
    m_preprocessorFormat.setForeground(QColor(0xc5, 0x86, 0xc0));
    m_typeFormat.setForeground(QColor(0x4e, 0xc9, 0xb0));
    m_functionFormat.setForeground(QColor(0xdcdc, 0xca, 0xa0));
    m_operatorFormat.setForeground(QColor(0xd4, 0xd4, 0xd4));
    m_annotationFormat.setForeground(QColor(0xd7, 0xba, 0x7d));
    m_constantFormat.setForeground(QColor(0x9a, 0xcd, 0xfe));
    m_attributeFormat.setForeground(QColor(0x9a, 0xcd, 0xfe));
    m_valueFormat.setForeground(QColor(0xce, 0x91, 0x78));

    m_multiLineCommentFormat.setForeground(QColor(0x6a, 0x99, 0x5f));
    m_multiLineCommentFormat.setFontItalic(true);
}

void SyntaxHighlighter::setLanguage(Language lang)
{
    m_language = lang;
    m_rules.clear();
    setupCommonRules();

    switch (lang) {
        case Cpp:    setupCppRules();    break;
        case Lua:    setupLuaRules();    break;
        case Json:   setupJsonRules();   break;
        case Ini:    setupIniRules();    break;
        case Xml:    setupXmlRules();    break;
        case Python: setupPythonRules(); break;
        case Css:    setupCssRules();    break;
        case Glsl:   setupGlslRules();   break;
        default: break;
    }
    rehighlight();
}

SyntaxHighlighter::Language SyntaxHighlighter::detectLanguage(const QString& filePath)
{
    QString ext = QFileInfo(filePath).suffix().toLower();
    if (ext == "cpp" || ext == "cxx" || ext == "cc" || ext == "c" || ext == "h" ||
        ext == "hpp" || ext == "hxx" || ext == "hh" || ext == "h++" || ext == "c++" ||
        ext == "tpp" || ext == "inl")
        return Cpp;
    if (ext == "lua") return Lua;
    if (ext == "json") return Json;
    if (ext == "ini" || ext == "cfg" || ext == "conf") return Ini;
    if (ext == "xml" || ext == "ui" || ext == "qrc" || ext == "xaml") return Xml;
    if (ext == "py") return Python;
    if (ext == "css" || ext == "qss") return Css;
    if (ext == "glsl" || ext == "vert" || ext == "frag" || ext == "geom" ||
        ext == "tesc" || ext == "tese" || ext == "comp" || ext == "vs" ||
        ext == "fs" || ext == "gs")
        return Glsl;
    return None;
}

QString SyntaxHighlighter::languageName(Language lang)
{
    switch (lang) {
        case Cpp:    return "C++";
        case Lua:    return "Lua";
        case Json:   return "JSON";
        case Ini:    return "INI";
        case Xml:    return "XML";
        case Python: return "Python";
        case Css:    return "CSS";
        case Glsl:   return "GLSL";
        default:     return "Plain Text";
    }
}

void SyntaxHighlighter::setupCommonRules()
{
    HighlightRule numRule;
    numRule.pattern = QRegularExpression(R"(\b(0[xX][0-9a-fA-F]+|\d+\.?\d*([eE][+-]?\d+)?)\b)");
    numRule.format = m_numberFormat;
    m_rules.append(numRule);
}

void SyntaxHighlighter::setupCppRules()
{
    QStringList keywords = {
        "alignas", "alignof", "auto", "bool", "break", "case", "catch", "char",
        "char8_t", "char16_t", "char32_t", "class", "concept", "const", "consteval",
        "constexpr", "constinit", "continue", "co_await", "co_return", "co_yield",
        "decltype", "default", "delete", "do", "double", "dynamic_cast", "else",
        "enum", "explicit", "export", "extern", "false", "float", "for", "friend",
        "goto", "if", "inline", "int", "long", "mutable", "namespace", "new",
        "noexcept", "nullptr", "operator", "override", "private", "protected",
        "public", "register", "reinterpret_cast", "requires", "return", "short",
        "signed", "sizeof", "static", "static_cast", "struct", "switch", "template",
        "this", "throw", "true", "try", "typedef", "typeid", "typename", "union",
        "unsigned", "using", "virtual", "void", "volatile", "wchar_t", "while",
        "Q_OBJECT", "Q_", "signals", "slots", "emit"
    };
    for (const QString& kw : keywords) {
        HighlightRule rule;
        rule.pattern = QRegularExpression(QString(R"(\b%1\b)").arg(kw));
        rule.format = m_keywordFormat;
        m_rules.append(rule);
    }

    HighlightRule typeRule;
    typeRule.pattern = QRegularExpression(R"(\b(Q[A-Z]\w*|std::\w+|boost::\w+)\b)");
    typeRule.format = m_typeFormat;
    m_rules.append(typeRule);

    HighlightRule funcRule;
    funcRule.pattern = QRegularExpression(R"(\b([A-Za-z_]\w*)\s*\()");
    funcRule.format = m_functionFormat;
    m_rules.append(funcRule);

    HighlightRule preprocRule;
    preprocRule.pattern = QRegularExpression(R"(^#\s*\w+)");
    preprocRule.format = m_preprocessorFormat;
    m_rules.append(preprocRule);

    HighlightRule annotRule;
    annotRule.pattern = QRegularExpression(R"(//[^\n]*)");
    annotRule.format = m_commentFormat;
    m_rules.append(annotRule);

    m_commentStartExpr = QRegularExpression(R"(/\*)");
    m_commentEndExpr = QRegularExpression(R"(\*/)");
}

void SyntaxHighlighter::setupLuaRules()
{
    QStringList keywords = {
        "and", "break", "do", "else", "elseif", "end", "false", "for", "function",
        "goto", "if", "in", "local", "nil", "not", "or", "repeat", "return", "then",
        "true", "until", "while"
    };
    for (const QString& kw : keywords) {
        HighlightRule rule;
        rule.pattern = QRegularExpression(QString(R"(\b%1\b)").arg(kw));
        rule.format = m_keywordFormat;
        m_rules.append(rule);
    }

    HighlightRule funcRule;
    funcRule.pattern = QRegularExpression(R"(function\s+([A-Za-z_]\w*))");
    funcRule.format = m_functionFormat;
    m_rules.append(funcRule);

    HighlightRule commentRule;
    commentRule.pattern = QRegularExpression(R"(--[^\n]*)");
    commentRule.format = m_commentFormat;
    m_rules.append(commentRule);

    HighlightRule stringRule;
    stringRule.pattern = QRegularExpression(R"("(?:[^"\\]|\\.)*")");
    stringRule.format = m_stringFormat;
    m_rules.append(stringRule);

    stringRule.pattern = QRegularExpression(R"('(?:[^'\\]|\\.)*')");
    stringRule.format = m_stringFormat;
    m_rules.append(stringRule);

    m_commentStartExpr = QRegularExpression(R"(--\[\[)");
    m_commentEndExpr = QRegularExpression(R"(\]\])");
}

void SyntaxHighlighter::setupJsonRules()
{
    HighlightRule keyRule;
    keyRule.pattern = QRegularExpression(R"("(?:[^"\\]|\\.)*"\s*:)");
    keyRule.format = m_attributeFormat;
    m_rules.append(keyRule);

    HighlightRule stringRule;
    stringRule.pattern = QRegularExpression(R"("(?:[^"\\]|\\.)*")");
    stringRule.format = m_stringFormat;
    m_rules.append(stringRule);

    QStringList keywords = { "true", "false", "null" };
    for (const QString& kw : keywords) {
        HighlightRule rule;
        rule.pattern = QRegularExpression(QString(R"(\b%1\b)").arg(kw));
        rule.format = m_constantFormat;
        m_rules.append(rule);
    }
}

void SyntaxHighlighter::setupIniRules()
{
    HighlightRule sectionRule;
    sectionRule.pattern = QRegularExpression(R"(\[[^\]]*\])");
    sectionRule.format = m_typeFormat;
    sectionRule.format.setFontWeight(QFont::Bold);
    m_rules.append(sectionRule);

    HighlightRule keyRule;
    keyRule.pattern = QRegularExpression(R"(^[^\[]([^=]+?)\s*=)");
    keyRule.format = m_attributeFormat;
    m_rules.append(keyRule);

    HighlightRule commentRule;
    commentRule.pattern = QRegularExpression(R"(^[;#][^\n]*)");
    commentRule.format = m_commentFormat;
    m_rules.append(commentRule);

    HighlightRule valueRule;
    valueRule.pattern = QRegularExpression(R"(=\s*(.+)$)");
    valueRule.format = m_valueFormat;
    m_rules.append(valueRule);
}

void SyntaxHighlighter::setupXmlRules()
{
    HighlightRule tagRule;
    tagRule.pattern = QRegularExpression(R"(</?[A-Za-z_:][\w:.-]*)");
    tagRule.format = m_functionFormat;
    m_rules.append(tagRule);

    HighlightRule attrRule;
    attrRule.pattern = QRegularExpression(R"(\b[A-Za-z_:][\w:.-]*\s*=)");
    attrRule.format = m_attributeFormat;
    m_rules.append(attrRule);

    HighlightRule valueRule;
    valueRule.pattern = QRegularExpression(R"("(?:[^"\\]|\\.)*")");
    valueRule.format = m_stringFormat;
    m_rules.append(valueRule);

    HighlightRule commentRule;
    commentRule.pattern = QRegularExpression(R"(<!--[\s\S]*?-->)");
    commentRule.format = m_commentFormat;
    m_rules.append(commentRule);
}

void SyntaxHighlighter::setupPythonRules()
{
    QStringList keywords = {
        "False", "None", "True", "and", "as", "assert", "async", "await",
        "break", "class", "continue", "def", "del", "elif", "else", "except",
        "finally", "for", "from", "global", "if", "import", "in", "is",
        "lambda", "nonlocal", "not", "or", "pass", "raise", "return",
        "try", "while", "with", "yield"
    };
    for (const QString& kw : keywords) {
        HighlightRule rule;
        rule.pattern = QRegularExpression(QString(R"(\b%1\b)").arg(kw));
        rule.format = m_keywordFormat;
        m_rules.append(rule);
    }

    HighlightRule decorRule;
    decorRule.pattern = QRegularExpression(R"(@[A-Za-z_]\w*)");
    decorRule.format = m_annotationFormat;
    m_rules.append(decorRule);

    HighlightRule funcRule;
    funcRule.pattern = QRegularExpression(R"(def\s+([A-Za-z_]\w*))");
    funcRule.format = m_functionFormat;
    m_rules.append(funcRule);

    HighlightRule classRule;
    classRule.pattern = QRegularExpression(R"(class\s+([A-Za-z_]\w*))");
    classRule.format = m_typeFormat;
    m_rules.append(classRule);

    HighlightRule commentRule;
    commentRule.pattern = QRegularExpression(R"(#[^\n]*)");
    commentRule.format = m_commentFormat;
    m_rules.append(commentRule);
}

void SyntaxHighlighter::setupCssRules()
{
    HighlightRule selectorRule;
    selectorRule.pattern = QRegularExpression(R"([.#]?[A-Za-z_][\w-]*(?=\s*\{))");
    selectorRule.format = m_functionFormat;
    m_rules.append(selectorRule);

    HighlightRule propRule;
    propRule.pattern = QRegularExpression(R"([\w-]+(?=\s*:))");
    propRule.format = m_attributeFormat;
    m_rules.append(propRule);

    HighlightRule valueRule;
    valueRule.pattern = QRegularExpression(R"(:\s*[^;{}]+)");
    valueRule.format = m_valueFormat;
    m_rules.append(valueRule);

    HighlightRule commentRule;
    commentRule.pattern = QRegularExpression(R"(/\*[\s\S]*?\*/)");
    commentRule.format = m_commentFormat;
    m_rules.append(commentRule);
}

void SyntaxHighlighter::setupGlslRules()
{
    QStringList keywords = {
        "void", "bool", "int", "float", "double", "vec2", "vec3", "vec4",
        "mat2", "mat3", "mat4", "sampler2D", "sampler3D", "samplerCube",
        "struct", "if", "else", "for", "while", "do", "break", "continue",
        "return", "discard", "in", "out", "inout", "uniform", "varying",
        "attribute", "layout", "precision", "highp", "mediump", "lowp",
        "const", "true", "false", "void", "float", "int", "bool", "bvec2",
        "bvec3", "bvec4", "ivec2", "ivec3", "ivec4", "uvec2", "uvec3", "uvec4"
    };
    for (const QString& kw : keywords) {
        HighlightRule rule;
        rule.pattern = QRegularExpression(QString(R"(\b%1\b)").arg(kw));
        rule.format = m_keywordFormat;
        m_rules.append(rule);
    }

    HighlightRule preprocRule;
    preprocRule.pattern = QRegularExpression(R"(#\s*\w+)");
    preprocRule.format = m_preprocessorFormat;
    m_rules.append(preprocRule);

    HighlightRule commentRule;
    commentRule.pattern = QRegularExpression(R"(//[^\n]*)");
    commentRule.format = m_commentFormat;
    m_rules.append(commentRule);

    m_commentStartExpr = QRegularExpression(R"(/\*)");
    m_commentEndExpr = QRegularExpression(R"(\*/)");
}

void SyntaxHighlighter::highlightBlock(const QString& text)
{
    for (const HighlightRule& rule : m_rules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    if (!m_commentStartExpr.pattern().isEmpty() &&
        !m_commentEndExpr.pattern().isEmpty()) {
        highlightMultiLine(text);
    }

    HighlightRule strRule;
    strRule.format = m_stringFormat;

    if (m_language != Lua && m_language != Json && m_language != Xml && m_language != Ini) {
        strRule.pattern = QRegularExpression(R"("(?:[^"\\]|\\.)*")");
        QRegularExpressionMatchIterator strIt = strRule.pattern.globalMatch(text);
        while (strIt.hasNext()) {
            auto m = strIt.next();
            setFormat(m.capturedStart(), m.capturedLength(), m_stringFormat);
        }
        strRule.pattern = QRegularExpression(R"('(?:[^'\\]|\\.)*')");
        strRule.pattern.globalMatch(text);
        QRegularExpressionMatchIterator sIt = strRule.pattern.globalMatch(text);
        while (sIt.hasNext()) {
            auto m = sIt.next();
            setFormat(m.capturedStart(), m.capturedLength(), m_stringFormat);
        }
    }
}

void SyntaxHighlighter::highlightMultiLine(const QString& text)
{
    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = text.indexOf(m_commentStartExpr);

    while (startIndex >= 0) {
        QRegularExpressionMatch endMatch;
        int endIndex = text.indexOf(m_commentEndExpr, startIndex, &endMatch);
        int commentLength;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + endMatch.capturedLength();
        }
        setFormat(startIndex, commentLength, m_multiLineCommentFormat);
        startIndex = text.indexOf(m_commentStartExpr, startIndex + commentLength);
    }
}

} // namespace ks
