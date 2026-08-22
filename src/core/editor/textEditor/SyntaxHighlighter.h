#pragma once

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QVector>

namespace ks {

struct HighlightRule {
    QRegularExpression pattern;
    QTextCharFormat format;
};

class SyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    enum Language {
        None,
        Cpp,
        Lua,
        Json,
        Ini,
        Xml,
        Python,
        Css,
        Glsl,
        LangCount
    };

    explicit SyntaxHighlighter(QTextDocument* parent = nullptr);
    void setLanguage(Language lang);

    static Language detectLanguage(const QString& filePath);
    static QString languageName(Language lang);

protected:
    void highlightBlock(const QString& text) override;

private:
    void setupCppRules();
    void setupLuaRules();
    void setupJsonRules();
    void setupIniRules();
    void setupXmlRules();
    void setupPythonRules();
    void setupCssRules();
    void setupGlslRules();
    void setupCommonRules();
    void highlightMultiLine(const QString& text);

    Language m_language = None;
    QVector<HighlightRule> m_rules;

    QTextCharFormat m_keywordFormat;
    QTextCharFormat m_stringFormat;
    QTextCharFormat m_commentFormat;
    QTextCharFormat m_numberFormat;
    QTextCharFormat m_preprocessorFormat;
    QTextCharFormat m_typeFormat;
    QTextCharFormat m_functionFormat;
    QTextCharFormat m_operatorFormat;
    QTextCharFormat m_annotationFormat;
    QTextCharFormat m_constantFormat;
    QTextCharFormat m_attributeFormat;
    QTextCharFormat m_valueFormat;

    QRegularExpression m_commentStartExpr;
    QRegularExpression m_commentEndExpr;
    QTextCharFormat m_multiLineCommentFormat;
};

} // namespace ks
