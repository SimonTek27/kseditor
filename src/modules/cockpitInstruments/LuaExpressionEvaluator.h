#pragma once
#include <QString>
#include <QMap>
#include <QVariant>
#include <QVector>

namespace ks {

class LuaExpressionEvaluator {
public:
    LuaExpressionEvaluator();

    void setVariable(const QString& name, double value);
    void setVariable(const QString& name, const QString& value);
    double evaluateNumeric(const QString& expression);
    QString evaluateString(const QString& expression);
    bool evaluateCondition(const QString& condition);

    struct FormatCallback {
        QString name;
        QString body;
    };

    void registerCallback(const QString& name, const QString& body);
    QString executeFormatFunction(const QString& funcName, double value, const QString& unit = "");

private:
    QMap<QString, double> m_numericVars;
    QMap<QString, QString> m_stringVars;
    QMap<QString, QString> m_callbacks;

    double evalSimpleExpression(const QString& expr);
    double evalBinaryOp(const QString& expr, const QString& op, double (*opFunc)(double, double));
    QString applyStringFormat(const QString& fmt, const QVector<QVariant>& args);
    QString applyFormatTemplate(const QString& template_str, double value, const QString& unit);
};

} // namespace ks