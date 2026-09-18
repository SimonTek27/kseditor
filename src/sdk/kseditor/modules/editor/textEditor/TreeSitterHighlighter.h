#pragma once

#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QMap>
#include <QVector>
#include <QColor>
#include <QSet>

namespace ks {

class TreeSitterHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    enum class Language {
        Unknown,
        Cpp,
        C,
        Python,
        Lua,
        GLSL,
        HLSL,
        JSON,
        XML,
        YAML,
        Markdown,
        INI,
        ShaderToy
    };

    explicit TreeSitterHighlighter(QTextDocument* parent = nullptr);
    ~TreeSitterHighlighter() override;

    void setLanguage(Language lang);
    Language language() const { return m_language; }

    void addCustomPattern(const QString& pattern, const QTextCharFormat& format);
    void clearCustomPatterns();

    // Tree-sitter integration (when available)
    void setTreeSitterEnabled(bool enabled) { m_treeSitterEnabled = enabled; }
    bool isTreeSitterEnabled() const { return m_treeSitterEnabled; }

signals:
    void languageChanged(Language lang);

protected:
    void highlightBlock(const QString& text) override;

private:
    struct HighlightRule {
        QRegularExpression pattern;
        QTextCharFormat format;
        bool minimal = false;
    };

    Language m_language = Language::Unknown;
    bool m_treeSitterEnabled = false;
    
    QVector<HighlightRule> m_rules;
    QVector<HighlightRule> m_customRules;
    
    QTextCharFormat m_keywordFormat;
    QTextCharFormat m_typeFormat;
    QTextCharFormat m_functionFormat;
    QTextCharFormat m_variableFormat;
    QTextCharFormat m_stringFormat;
    QTextCharFormat m_numberFormat;
    QTextCharFormat m_commentFormat;
    QTextCharFormat m_preprocessorFormat;
    QTextCharFormat m_operatorFormat;
    QTextCharFormat m_bracketFormat;
    
    // For block states (multi-line comments/strings)
    enum BlockState {
        Normal = 0,
        MultiLineComment = 1,
        MultiLineString = 2,
        MultiLineRawString = 3
    };
    
    void initializeFormats();
    void initializeCppRules();
    void initializePythonRules();
    void initializeLuaRules();
    void initializeGLSLRules();
    void initializeJSONRules();
    void initializeXMLRules();
    void initializeYAMLRules();
    void initializeMarkdownRules();
    void initializeINIRules();
    
    void applyRules(const QString& text, int& state);
    void handleMultiLineComment(const QString& text, int& state, int startPos);
    void handleMultiLineString(const QString& text, int state, int& pos);
    
    QTextCharFormat formatForTokenType(const QString& tokenType) const;
    QString detectLanguage(const QString& text) const;
    
    // Tree-sitter integration (placeholder)
    void* m_treeSitterParser = nullptr;
    void* m_treeSitterTree = nullptr;
};

} // namespace ks