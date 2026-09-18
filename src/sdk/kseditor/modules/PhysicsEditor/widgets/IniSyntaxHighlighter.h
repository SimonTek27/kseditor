#pragma once

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QVector>

namespace ks {

class IniSyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit IniSyntaxHighlighter(QTextDocument* parent = nullptr);

protected:
    void highlightBlock(const QString& text) override;

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat    format;
    };
    QVector<HighlightingRule> m_rules;
    QTextCharFormat m_sectionFormat;
    QTextCharFormat m_keyFormat;
    QTextCharFormat m_commentFormat;
    QTextCharFormat m_numberFormat;
    QTextCharFormat m_stringFormat;
};

} // namespace ks