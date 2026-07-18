#include "TreeSitterHighlighter.h"
#include <QTextBlock>
#include <QDebug>

namespace ks {

TreeSitterHighlighter::TreeSitterHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
{
    initializeFormats();
    setLanguage(Language::Cpp); // Default
}

TreeSitterHighlighter::~TreeSitterHighlighter() {
    // Clean up tree-sitter resources if any
}

void TreeSitterHighlighter::setLanguage(Language lang) {
    if (m_language != lang) {
        m_language = lang;
        rehighlight();
        emit languageChanged(lang);
    }
}

void TreeSitterHighlighter::addCustomPattern(const QString& pattern, const QTextCharFormat& format) {
    HighlightRule rule;
    rule.pattern = QRegularExpression(pattern);
    rule.format = format;
    m_customRules.append(rule);
    rehighlight();
}

void TreeSitterHighlighter::clearCustomPatterns() {
    m_customRules.clear();
    rehighlight();
}

void TreeSitterHighlighter::initializeFormats() {
    // Keywords
    m_keywordFormat.setForeground(QColor(200, 120, 50));     // Orange
    m_keywordFormat.setFontWeight(QFont::Bold);
    
    // Types
    m_typeFormat.setForeground(QColor(80, 160, 220));        // Blue
    m_typeFormat.setFontWeight(QFont::Bold);
    
    // Functions
    m_functionFormat.setForeground(QColor(100, 180, 100));   // Green
    m_functionFormat.setFontWeight(QFont::Bold);
    
    // Variables
    m_variableFormat.setForeground(QColor(220, 220, 170));   // Yellow
    
    // Strings
    m_stringFormat.setForeground(QColor(230, 180, 80));      // Gold
    
    // Numbers
    m_numberFormat.setForeground(QColor(180, 130, 200));     // Purple
    
    // Comments
    m_commentFormat.setForeground(QColor(120, 120, 120));    // Gray
    m_commentFormat.setFontItalic(true);
    
    // Preprocessor
    m_preprocessorFormat.setForeground(QColor(150, 100, 200)); // Violet
    m_preprocessorFormat.setFontWeight(QFont::Bold);
    
    // Operators
    m_operatorFormat.setForeground(QColor(200, 120, 50));    // Orange
    
    // Brackets
    m_bracketFormat.setForeground(QColor(220, 220, 220));    // White
    m_bracketFormat.setFontWeight(QFont::Bold);
}

void TreeSitterHighlighter::initializeCppRules() {
    m_rules.clear();
    
    // Keywords
    QStringList keywords = {
        "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor",
        "bool", "break", "case", "catch", "char", "char16_t", "char32_t", "class",
        "compl", "concept", "const", "consteval", "constexpr", "const_cast", "continue",
        "co_await", "co_return", "co_yield", "decltype", "default", "delete", "do",
        "double", "dynamic_cast", "else", "enum", "explicit", "export", "extern",
        "false", "float", "for", "friend", "goto", "if", "inline", "int", "long",
        "mutable", "namespace", "new", "noexcept", "not", "not_eq", "nullptr", "operator",
        "or", "or_eq", "private", "protected", "public", "register", "reinterpret_cast",
        "requires", "return", "short", "signed", "sizeof", "static", "static_assert",
        "static_cast", "struct", "switch", "template", "this", "thread_local", "throw",
        "true", "try", "typedef", "typeid", "typename", "union", "unsigned", "using",
        "virtual", "void", "volatile", "wchar_t", "while", "xor", "xor_eq"
    };
    
    for (const QString& kw : keywords) {
        HighlightRule rule;
        rule.pattern = QRegularExpression("\\b" + kw + "\\b");
        rule.format = m_keywordFormat;
        m_rules.append(rule);
    }
    
    // Types
    QStringList types = {
        "int", "short", "long", "char", "bool", "float", "double", "void",
        "size_t", "ptrdiff_t", "int8_t", "int16_t", "int32_t", "int64_t",
        "uint8_t", "uint16_t", "uint32_t", "uint64_t", "auto", "decltype"
    };
    for (const QString& t : types) {
        HighlightRule rule;
        rule.pattern = QRegularExpression("\\b" + t + "\\b");
        rule.format = m_typeFormat;
        m_rules.append(rule);
    }
    
    // Standard library types
    QStringList stdTypes = {
        "string", "vector", "map", "unordered_map", "set", "unordered_set",
        "list", "deque", "array", "optional", "variant", "any", "tuple",
        "pair", "shared_ptr", "unique_ptr", "weak_ptr", "function", "thread",
        "mutex", "condition_variable", "atomic", "chrono::duration",
        "chrono::time_point", "filesystem::path"
    };
    for (const QString& t : stdTypes) {
        HighlightRule rule;
        rule.pattern = QRegularExpression("\\b" + t + "\\b");
        rule.format = m_typeFormat;
        m_rules.append(rule);
    }
    
    // Functions
    HighlightRule funcRule;
    funcRule.pattern = QRegularExpression("\\b[A-Za-z_][A-Za-z0-9_]*\\s*(?=\\()");
    funcRule.format = m_functionFormat;
    m_rules.append(funcRule);
    
    // Numbers
    HighlightRule numRule;
    numRule.pattern = QRegularExpression("\\b\\d+(\\.\\d+)?([eE][+-]?\\d+)?\\b");
    numRule.format = m_numberFormat;
    m_rules.append(numRule);
    
    // Hex numbers
    HighlightRule hexRule;
    hexRule.pattern = QRegularExpression("\\b0x[0-9a-fA-F]+\\b");
    hexRule.format = m_numberFormat;
    m_rules.append(hexRule);
    
    // Binary numbers
    HighlightRule binRule;
    binRule.pattern = QRegularExpression("\\b0b[01]+\\b");
    binRule.format = m_numberFormat;
    m_rules.append(binRule);
    
    // Strings
    HighlightRule strRule;
    strRule.pattern = QRegularExpression("\"([^\"\\\\]|\\\\.)*\"");
    strRule.format = m_stringFormat;
    m_rules.append(strRule);
    
    // Char literals
    HighlightRule charRule;
    charRule.pattern = QRegularExpression("'([^'\\\\]|\\\\.)*'");
    charRule.format = m_stringFormat;
    m_rules.append(charRule);
    
    // Raw strings
    HighlightRule rawRule;
    rawRule.pattern = QRegularExpression("R\"([^)]*)\"");
    rawRule.format = m_stringFormat;
    m_rules.append(rawRule);
    
    // Comments
    HighlightRule commentRule;
    commentRule.pattern = QRegularExpression("//.*");
    commentRule.format = m_commentFormat;
    m_rules.append(commentRule);
    
    // Preprocessor
    HighlightRule preprocRule;
    preprocRule.pattern = QRegularExpression("^\\s*#.*");
    preprocRule.format = m_preprocessorFormat;
    m_rules.append(preprocRule);
    
    // Operators
    QStringList operators = {"=", "+", "-", "*", "/", "%", "==", "!=", "<", ">", "<=", ">=",
                            "&&", "||", "!", "++", "--", "+=", "-=", "*=", "/=", "%=",
                            "&", "|", "^", "~", "<<", ">>", "&=", "|=", "^=", "<<=", ">>=",
                            "->", ".", "::", "?", ":", ";", ",", "(", ")", "{", "}", "[", "]"};
    for (const QString& op : operators) {
        HighlightRule rule;
        rule.pattern = QRegularExpression(QRegularExpression::escape(op));
        rule.format = m_operatorFormat;
        m_rules.append(rule);
    }
    
    // Brackets
    QStringList brackets = {"\\(", "\\)", "\\[", "\\]", "\\{", "\\}"};
    for (const QString& br : brackets) {
        HighlightRule rule;
        rule.pattern = QRegularExpression(br);
        rule.format = m_bracketFormat;
        m_rules.append(rule);
    }
    
    // Member access
    HighlightRule memberRule;
    memberRule.pattern = QRegularExpression("\\.\\s*[A-Za-z_][A-Za-z0-9_]*");
    memberRule.format = m_variableFormat;
    m_rules.append(memberRule);
}

void TreeSitterHighlighter::initializePythonRules() {
    m_rules.clear();
    
    // Keywords
    QStringList keywords = {
        "and", "as", "assert", "async", "await", "break", "class", "continue",
        "def", "del", "elif", "else", "except", "finally", "for", "from",
        "global", "if", "import", "in", "is", "lambda", "nonlocal", "not",
        "or", "pass", "raise", "return", "try", "while", "with", "yield",
        "True", "False", "None"
    };
    for (const QString& kw : keywords) {
        HighlightRule rule;
        rule.pattern = QRegularExpression("\\b" + kw + "\\b");
        rule.format = m_keywordFormat;
        m_rules.append(rule);
    }
    
    // Builtins
    QStringList builtins = {
        "print", "len", "range", "str", "int", "float", "list", "dict", "set",
        "tuple", "open", "close", "read", "write", "input", "type", "isinstance",
        "issubclass", "hasattr", "getattr", "setattr", "delattr", "dir", "vars",
        "locals", "globals", "eval", "exec", "compile", "super", "property",
        "staticmethod", "classmethod", "object", "Exception", "ValueError",
        "TypeError", "IndexError", "KeyError", "AttributeError", "ImportError"
    };
    for (const QString& b : builtins) {
        HighlightRule rule;
        rule.pattern = QRegularExpression("\\b" + b + "\\b");
        rule.format = m_functionFormat;
        m_rules.append(rule);
    }
    
    // Strings (triple quotes)
    HighlightRule tripleStr;
    tripleStr.pattern = QRegularExpression("(\"\"\".*?\"\"\"|'''.*?''')", QRegularExpression::DotMatchesEverythingOption);
    tripleStr.format = m_stringFormat;
    m_rules.append(tripleStr);
    
    // Regular strings
    HighlightRule strRule;
    strRule.pattern = QRegularExpression("([\"'])(?:\\\\.|[^\\\\])*?\\1");
    strRule.format = m_stringFormat;
    m_rules.append(strRule);
    
    // F-strings
    HighlightRule fstrRule;
    fstrRule.pattern = QRegularExpression("f[\"'].*?[\"']");
    fstrRule.format = QTextCharFormat(m_stringFormat);
    fstrRule.format.setForeground(QColor(255, 200, 50)); // Orange-gold
    m_rules.append(fstrRule);
    
    // Numbers
    HighlightRule numRule;
    numRule.pattern = QRegularExpression("\\b\\d+(\\.\\d+)?([eE][+-]?\\d+)?[jJ]?\\b");
    numRule.format = m_numberFormat;
    m_rules.append(numRule);
    
    // Comments
    HighlightRule commentRule;
    commentRule.pattern = QRegularExpression("#.*");
    commentRule.format = m_commentFormat;
    m_rules.append(commentRule);
    
    // Decorators
    HighlightRule decoRule;
    decoRule.pattern = QRegularExpression("@[A-Za-z_][A-Za-z0-9_]*");
    decoRule.format = m_preprocessorFormat;
    m_rules.append(decoRule);
    
    // Function definitions
    HighlightRule defRule;
    defRule.pattern = QRegularExpression("\\bdef\\s+([A-Za-z_][A-Za-z0-9_]*)");
    defRule.format = m_functionFormat;
    m_rules.append(defRule);
    
    // Class definitions
    HighlightRule classRule;
    classRule.pattern = QRegularExpression("\\bclass\\s+([A-Za-z_][A-Za-z0-9_]*)");
    classRule.format = m_typeFormat;
    m_rules.append(classRule);
    
    // Types
    QStringList types = {"int", "str", "float", "bool", "list", "dict", "set", "tuple",
                         "Optional", "List", "Dict", "Set", "Tuple", "Union", "Any",
                         "Callable", "Iterator", "Generator", "AsyncIterator", "Coroutine"};
    for (const QString& t : types) {
        HighlightRule rule;
        rule.pattern = QRegularExpression("\\b" + t + "\\b");
        rule.format = m_typeFormat;
        m_rules.append(rule);
    }
}

void TreeSitterHighlighter::initializeLuaRules() {
    m_rules.clear();
    
    QStringList keywords = {
        "and", "break", "do", "else", "elseif", "end", "false", "for", "function",
        "goto", "if", "in", "local", "nil", "not", "or", "repeat", "return", "then",
        "true", "until", "while"
    };
    for (const QString& kw : keywords) {
        HighlightRule rule;
        rule.pattern = QRegularExpression("\\b" + kw + "\\b");
        rule.format = m_keywordFormat;
        m_rules.append(rule);
    }
    
    // Strings
    HighlightRule strRule;
    strRule.pattern = QRegularExpression("([\"'])(?:\\\\.|[^\\\\])*?\\1");
    strRule.format = m_stringFormat;
    m_rules.append(strRule);
    
    // Long strings
    HighlightRule longStr;
    longStr.pattern = QRegularExpression("\\[=*\\[.*?\\]=*\\]", QRegularExpression::DotMatchesEverythingOption);
    longStr.format = m_stringFormat;
    m_rules.append(longStr);
    
    // Numbers
    HighlightRule numRule;
    numRule.pattern = QRegularExpression("\\b\\d+(\\.\\d+)?([eE][+-]?\\d+)?\\b");
    numRule.format = m_numberFormat;
    m_rules.append(numRule);
    
    // Comments
    HighlightRule commentRule;
    commentRule.pattern = QRegularExpression("--.*");
    commentRule.format = m_commentFormat;
    m_rules.append(commentRule);
    
    // Long comments
    HighlightRule longComment;
    longComment.pattern = QRegularExpression("--\\[=*\\[.*?\\]=*\\]", QRegularExpression::DotMatchesEverythingOption);
    longComment.format = m_commentFormat;
    m_rules.append(longComment);
    
    // Functions
    HighlightRule funcRule;
    funcRule.pattern = QRegularExpression("\\bfunction\\s+([A-Za-z_][A-Za-z0-9_.:]*)");
    funcRule.format = m_functionFormat;
    m_rules.append(funcRule);
}

void TreeSitterHighlighter::initializeGLSLRules() {
    m_rules.clear();
    
    // Keywords
    QStringList keywords = {
        "attribute", "const", "uniform", "varying", "in", "out", "inout",
        "float", "vec2", "vec3", "vec4", "int", "ivec2", "ivec3", "ivec4",
        "bool", "bvec2", "bvec3", "bvec4", "mat2", "mat3", "mat4",
        "sampler1D", "sampler2D", "sampler3D", "samplerCube",
        "struct", "void", "while", "for", "if", "else", "return",
        "break", "continue", "discard", "do", "switch", "case", "default",
        "precision", "highp", "mediump", "lowp", "invariant", "centroid",
        "flat", "smooth", "noperspective", "layout", "binding", "location"
    };
    for (const QString& kw : keywords) {
        HighlightRule rule;
        rule.pattern = QRegularExpression("\\b" + kw + "\\b");
        rule.format = m_keywordFormat;
        m_rules.append(rule);
    }
    
    // Built-in functions
    QStringList builtins = {
        "radians", "degrees", "sin", "cos", "tan", "asin", "acos", "atan",
        "pow", "exp", "log", "exp2", "log2", "sqrt", "inversesqrt", "abs",
        "sign", "floor", "ceil", "fract", "mod", "min", "max", "clamp",
        "mix", "step", "smoothstep", "length", "distance", "dot", "cross",
        "normalize", "faceforward", "reflect", "refract", "matrixCompMult",
        "lessThan", "lessThanEqual", "greaterThan", "greaterThanEqual",
        "equal", "notEqual", "any", "all", "not", "texture", "textureSize",
        "textureLod", "textureProj", "dFdx", "dFdy", "fwidth"
    };
    for (const QString& fn : builtins) {
        HighlightRule rule;
        rule.pattern = QRegularExpression("\\b" + fn + "\\b");
        rule.format = m_functionFormat;
        m_rules.append(rule);
    }
    
    // Numbers
    HighlightRule numRule;
    numRule.pattern = QRegularExpression("\\b\\d+(\\.\\d+)?([eE][+-]?\\d+)?\\b");
    numRule.format = m_numberFormat;
    m_rules.append(numRule);
    
    // Strings
    HighlightRule strRule;
    strRule.pattern = QRegularExpression("\"([^\"\\\\]|\\\\.)*\"");
    strRule.format = m_stringFormat;
    m_rules.append(strRule);
    
    // Comments
    HighlightRule commentRule;
    commentRule.pattern = QRegularExpression("//.*");
    commentRule.format = m_commentFormat;
    m_rules.append(commentRule);
    
    // Preprocessor
    HighlightRule preprocRule;
    preprocRule.pattern = QRegularExpression("^\\s*#.*");
    preprocRule.format = m_preprocessorFormat;
    m_rules.append(preprocRule);
}

void TreeSitterHighlighter::initializeJSONRules() {
    m_rules.clear();
    
    // Strings (keys and values)
    HighlightRule strRule;
    strRule.pattern = QRegularExpression("\"([^\"\\\\]|\\\\.)*\"");
    strRule.format = m_stringFormat;
    m_rules.append(strRule);
    
    // Numbers
    HighlightRule numRule;
    numRule.pattern = QRegularExpression("\\b-?\\d+(\\.\\d+)?([eE][+-]?\\d+)?\\b");
    numRule.format = m_numberFormat;
    m_rules.append(numRule);
    
    // Booleans and null
    HighlightRule boolRule;
    boolRule.pattern = QRegularExpression("\\b(true|false|null)\\b");
    boolRule.format = m_keywordFormat;
    m_rules.append(boolRule);
    
    // Keys (strings followed by colon)
    HighlightRule keyRule;
    keyRule.pattern = QRegularExpression("\"([^\"]*)\"\\s*:");
    keyRule.format = m_typeFormat;
    m_rules.append(keyRule);
    
    // Punctuation
    HighlightRule punctRule;
    punctRule.pattern = QRegularExpression("[{}\\[\\],:]");
    punctRule.format = m_operatorFormat;
    m_rules.append(punctRule);
}

void TreeSitterHighlighter::initializeXMLRules() {
    m_rules.clear();
    
    // Tags
    HighlightRule tagRule;
    tagRule.pattern = QRegularExpression("</?[A-Za-z_:][A-Za-z0-9_:.-]*");
    tagRule.format = m_keywordFormat;
    m_rules.append(tagRule);
    
    // Attributes
    HighlightRule attrRule;
    attrRule.pattern = QRegularExpression("\\b[A-Za-z_:][A-Za-z0-9_:.-]*\\s*=");
    attrRule.format = m_typeFormat;
    m_rules.append(attrRule);
    
    // String values
    HighlightRule strRule;
    strRule.pattern = QRegularExpression("\"([^\"\\\\]|\\\\.)*\"");
    strRule.format = m_stringFormat;
    m_rules.append(strRule);
    
    // Comments
    HighlightRule commentRule;
    commentRule.pattern = QRegularExpression("<!--.*?-->", QRegularExpression::DotMatchesEverythingOption);
    commentRule.format = m_commentFormat;
    m_rules.append(commentRule);
    
    // DOCTYPE and processing instructions
    HighlightRule procRule;
    procRule.pattern = QRegularExpression("<\\?.*?\\?>");
    procRule.format = m_preprocessorFormat;
    m_rules.append(procRule);
}

void TreeSitterHighlighter::initializeYAMLRules() {
    m_rules.clear();
    
    // Keys
    HighlightRule keyRule;
    keyRule.pattern = QRegularExpression("^\\s*[A-Za-z_][A-Za-z0-9_-]*\\s*:");
    keyRule.format = m_typeFormat;
    m_rules.append(keyRule);
    
    // Strings
    HighlightRule strRule;
    strRule.pattern = QRegularExpression("\"([^\"\\\\]|\\\\.)*\"|'([^'\\\\]|\\\\.)*'");
    strRule.format = m_stringFormat;
    m_rules.append(strRule);
    
    // Numbers
    HighlightRule numRule;
    numRule.pattern = QRegularExpression("\\b-?\\d+(\\.\\d+)?([eE][+-]?\\d+)?\\b");
    numRule.format = m_numberFormat;
    m_rules.append(numRule);
    
    // Booleans
    HighlightRule boolRule;
    boolRule.pattern = QRegularExpression("\\b(true|false|yes|no|on|off|null|~)\\b");
    boolRule.format = m_keywordFormat;
    m_rules.append(boolRule);
    
    // Comments
    HighlightRule commentRule;
    commentRule.pattern = QRegularExpression("#.*");
    commentRule.format = m_commentFormat;
    m_rules.append(commentRule);
    
    // Document markers
    HighlightRule docRule;
    docRule.pattern = QRegularExpression("^---|^\\.\\.\\.");
    docRule.format = m_preprocessorFormat;
    m_rules.append(docRule);
}

void TreeSitterHighlighter::initializeMarkdownRules() {
    m_rules.clear();
    
    // Headers
    HighlightRule headerRule;
    headerRule.pattern = QRegularExpression("^#{1,6}\\s+.*$");
    headerRule.format = m_typeFormat;
    m_rules.append(headerRule);
    
    // Bold
    HighlightRule boldRule;
    boldRule.pattern = QRegularExpression("\\*\\*([^\\*]+)\\*\\*|__([^_]+)__");
    boldRule.format = m_keywordFormat;
    m_rules.append(boldRule);
    
    // Italic
    HighlightRule italicRule;
    italicRule.pattern = QRegularExpression("\\*([^\\*]+)\\*|_([^_]+)_");
    italicRule.format = QTextCharFormat(m_keywordFormat);
    italicRule.format.setFontItalic(true);
    m_rules.append(italicRule);
    
    // Code blocks
    HighlightRule codeRule;
    codeRule.pattern = QRegularExpression("```.*?```", QRegularExpression::DotMatchesEverythingOption);
    codeRule.format = m_stringFormat;
    m_rules.append(codeRule);
    
    // Inline code
    HighlightRule inlineCodeRule;
    inlineCodeRule.pattern = QRegularExpression("`([^`]+)`");
    inlineCodeRule.format = m_stringFormat;
    m_rules.append(inlineCodeRule);
    
    // Links
    HighlightRule linkRule;
    linkRule.pattern = QRegularExpression("\\[([^\\]]+)\\]\\(([^)]+)\\)");
    linkRule.format = QTextCharFormat(m_functionFormat);
    linkRule.format.setUnderlineStyle(QTextCharFormat::SingleUnderline);
    m_rules.append(linkRule);
    
    // Images
    HighlightRule imgRule;
    imgRule.pattern = QRegularExpression("!\\[([^\\]]*)\\]\\(([^)]+)\\)");
    imgRule.format = QTextCharFormat(m_stringFormat);
    imgRule.format.setUnderlineStyle(QTextCharFormat::SingleUnderline);
    m_rules.append(imgRule);
    
    // Blockquotes
    HighlightRule quoteRule;
    quoteRule.pattern = QRegularExpression("^>.*$");
    quoteRule.format = m_commentFormat;
    m_rules.append(quoteRule);
    
    // Lists
    HighlightRule listRule;
    listRule.pattern = QRegularExpression("^\\s*[-*+]\\s+");
    listRule.format = m_operatorFormat;
    m_rules.append(listRule);
    
    // Horizontal rule
    HighlightRule hrRule;
    hrRule.pattern = QRegularExpression("^\\s*([-*_])\\s*\\1\\s*\\1\\s*$");
    hrRule.format = m_preprocessorFormat;
    m_rules.append(hrRule);
}

void TreeSitterHighlighter::initializeINIRules() {
    m_rules.clear();
    
    // Sections
    HighlightRule sectionRule;
    sectionRule.pattern = QRegularExpression("^\\[[^\\]]*\\]");
    sectionRule.format = m_typeFormat;
    m_rules.append(sectionRule);
    
    // Keys
    HighlightRule keyRule;
    keyRule.pattern = QRegularExpression("^[A-Za-z_][A-Za-z0-9_]*\\s*=");
    keyRule.format = m_typeFormat;
    m_rules.append(keyRule);
    
    // Values (after =)
    HighlightRule valRule;
    valRule.pattern = QRegularExpression("=\\s*[^;#]*");
    valRule.format = m_stringFormat;
    m_rules.append(valRule);
    
    // Comments
    HighlightRule commentRule;
    commentRule.pattern = QRegularExpression("[;#].*");
    commentRule.format = m_commentFormat;
    m_rules.append(commentRule);
    
    // Numbers
    HighlightRule numRule;
    numRule.pattern = QRegularExpression("\\b-?\\d+(\\.\\d+)?([eE][+-]?\\d+)?\\b");
    numRule.format = m_numberFormat;
    m_rules.append(numRule);
}

QString TreeSitterHighlighter::detectLanguage(const QString& text) const {
    // Simple heuristic based on content
    if (text.contains(QRegularExpression("#include\\s*[<\"].*[>\"]"))) return "cpp";
    if (text.contains(QRegularExpression("^\\s*import\\s+\\w+"))) return "python";
    if (text.contains(QRegularExpression("^\\s*function\\s+"))) return "lua";
    if (text.contains(QRegularExpression("^\\s*#version\\s+"))) return "glsl";
    if (text.contains(QRegularExpression("^\\s*[{}\\[\\]"))) return "json";
    if (text.contains(QRegularExpression("<\\?xml"))) return "xml";
    if (text.contains(QRegularExpression("^---"))) return "yaml";
    if (text.contains(QRegularExpression("^#\\s+"))) return "markdown";
    if (text.contains(QRegularExpression("^\\[.*\\]"))) return "ini";
    return "unknown";
}

void TreeSitterHighlighter::highlightBlock(const QString& text) {
    if (text.isEmpty()) {
        setCurrentBlockState(Normal);
        return;
    }
    
    // Use custom rules first
    for (const auto& rule : m_customRules) {
        QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
    
    // Use language-specific rules
    for (const auto& rule : m_rules) {
        QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
    
    // Handle multi-line comments/strings based on block state
    int state = previousBlockState();
    applyRules(text, state);
    setCurrentBlockState(state);
}

void TreeSitterHighlighter::applyRules(const QString& text, int& state) {
    // Handle multi-line comments and strings based on previous block state
    switch (state) {
        case MultiLineComment:
            handleMultiLineComment(text, state, 0);
            break;
        case MultiLineString:
        case MultiLineRawString:
            // Continue string
            break;
        default:
            // Check for start of multi-line constructs
            if (m_language == Language::Cpp || m_language == Language::C) {
                int pos = text.indexOf("/*");
                if (pos >= 0) {
                    int endPos = text.indexOf("*/", pos + 2);
                    if (endPos == -1) {
                        state = MultiLineComment;
                        setFormat(pos, text.length() - pos, m_commentFormat);
                    }
                }
            }
            break;
    }
}

void TreeSitterHighlighter::handleMultiLineComment(const QString& text, int& state, int startPos) {
    int endPos = text.indexOf("*/", startPos);
    if (endPos == -1) {
        state = MultiLineComment;
        setFormat(startPos, text.length() - startPos, m_commentFormat);
    } else {
        state = Normal;
        setFormat(startPos, endPos - startPos + 2, m_commentFormat);
    }
}

void TreeSitterHighlighter::handleMultiLineString(const QString& text, int state, int& pos) {
    // Implementation for multi-line strings
    // Would need to track quote type and handle escaping
}

QTextCharFormat TreeSitterHighlighter::formatForTokenType(const QString& tokenType) const {
    QTextCharFormat format;
    if (tokenType == "keyword") return m_keywordFormat;
    if (tokenType == "type") return m_typeFormat;
    if (tokenType == "function") return m_functionFormat;
    if (tokenType == "variable") return m_variableFormat;
    if (tokenType == "string") return m_stringFormat;
    if (tokenType == "number") return m_numberFormat;
    if (tokenType == "comment") return m_commentFormat;
    if (tokenType == "preprocessor") return m_preprocessorFormat;
    if (tokenType == "operator") return m_operatorFormat;
    if (tokenType == "bracket") return m_bracketFormat;
    return format;
}

} // namespace ks