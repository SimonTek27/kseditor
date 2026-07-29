#include "IniSyntaxHighlighter.h"
#include <QColor>
#include <QFont>

namespace ks {

IniSyntaxHighlighter::IniSyntaxHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
{
    m_sectionFormat.setForeground(QColor("#4A90E2"));
    m_sectionFormat.setFontWeight(QFont::Bold);

    m_keyFormat.setForeground(QColor("#E8C97D"));

    m_commentFormat.setForeground(QColor("#6A9955"));
    m_commentFormat.setFontItalic(true);

    m_numberFormat.setForeground(QColor("#B5CEA8"));

    m_stringFormat.setForeground(QColor("#CE9178"));

    HighlightingRule rule;

    rule.pattern = QRegularExpression("^\\[.+\\]");
    rule.format  = m_sectionFormat;
    m_rules.append(rule);

    rule.pattern = QRegularExpression("^[^=\\n]+=");
    rule.format  = m_keyFormat;
    m_rules.append(rule);

    rule.pattern = QRegularExpression(";.*$");
    rule.format  = m_commentFormat;
    m_rules.append(rule);

    rule.pattern = QRegularExpression("-?\\b\\d+\\.?\\d*\\b");
    rule.format  = m_numberFormat;
    m_rules.append(rule);

    rule.pattern = QRegularExpression("\"[^\"]*\"");
    rule.format  = m_stringFormat;
    m_rules.append(rule);
}

void IniSyntaxHighlighter::highlightBlock(const QString& text) {
    for (const HighlightingRule& rule : m_rules) {
        QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}

} // namespace ks