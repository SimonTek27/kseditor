#include "LuaExpressionEvaluator.h"
#include <cmath>
#include <QRegularExpression>
#include <algorithm>

namespace ks {

LuaExpressionEvaluator::LuaExpressionEvaluator() {}

void LuaExpressionEvaluator::setVariable(const QString& name, double value) {
    m_numericVars[name] = value;
}

void LuaExpressionEvaluator::setVariable(const QString& name, const QString& value) {
    m_stringVars[name] = value;
}

double LuaExpressionEvaluator::evaluateNumeric(const QString& expression) {
    return evalSimpleExpression(expression.trimmed());
}

QString LuaExpressionEvaluator::evaluateString(const QString& expression) {
    QString expr = expression.trimmed();

    if (expr.startsWith('"') || expr.startsWith('\'')) {
        return expr.mid(1, expr.length() - 2);
    }

    if (m_stringVars.contains(expr)) {
        return m_stringVars[expr];
    }

    return expr;
}

bool LuaExpressionEvaluator::evaluateCondition(const QString& condition) {
    QString cond = condition.trimmed();

    QRegularExpression cmpRe(R"(([^=<>!]+)\s*([=<>!]+)\s*([^=<>!]+))");
    QRegularExpressionMatch m = cmpRe.match(cond);
    if (!m.hasMatch()) return false;

    QString lhs = m.captured(1).trimmed();
    QString op = m.captured(2).trimmed();
    QString rhs = m.captured(3).trimmed();

    double lVal = evalSimpleExpression(lhs);
    double rVal = evalSimpleExpression(rhs);

    if (op == "==") return std::abs(lVal - rVal) < 0.0001;
    if (op == "~=") return std::abs(lVal - rVal) >= 0.0001;
    if (op == "<")  return lVal < rVal;
    if (op == ">")  return lVal > rVal;
    if (op == "<=") return lVal <= rVal;
    if (op == ">=") return lVal >= rVal;

    return false;
}

void LuaExpressionEvaluator::registerCallback(const QString& name, const QString& body) {
    m_callbacks[name] = body;
}

QString LuaExpressionEvaluator::executeFormatFunction(const QString& funcName, double value, const QString& unit) {
    if (!m_callbacks.contains(funcName)) {
        return QString::number(value, 'f', 1);
    }

    QString body = m_callbacks[funcName];

    QRegularExpression returnRe(R"(return\s+(.+))");
    QRegularExpressionMatch returnMatch = returnRe.match(body);
    if (!returnMatch.hasMatch()) {
        return QString::number(value, 'f', 1);
    }

    QString returnExpr = returnMatch.captured(1).trimmed();

    // Handle string.format patterns
    QRegularExpression formatRe(R"re(string\.format\s*\(\s*"([^"]*)"\s*,?\s*([^)]*)?\))re");
    QRegularExpressionMatch fmtMatch = formatRe.match(returnExpr);
    if (fmtMatch.hasMatch()) {
        QString fmt = fmtMatch.captured(1);
        QString args = fmtMatch.captured(2).trimmed();

        QRegularExpression varRe(R"(value|speed|rpm|gear)");
        QRegularExpressionMatch varMatch = varRe.match(args);
        if (varMatch.hasMatch()) {
            fmt.replace("%.1f", QString::number(value, 'f', 1));
            fmt.replace("%.2f", QString::number(value, 'f', 2));
            fmt.replace("%d", QString::number(static_cast<int>(std::round(value))));
            fmt.replace("%s", unit.isEmpty() ? QString::number(value, 'f', 1) : unit);
        }

        return fmt;
    }

    // Handle if/then/return patterns
    QRegularExpression ifRe(R"(if\s+(.+)\s+then\s+return\s+(.+)\s+else\s+return\s+(.+)\s+end)");
    QRegularExpressionMatch ifMatch = ifRe.match(returnExpr);
    if (ifMatch.hasMatch()) {
        QString cond = ifMatch.captured(1).trimmed();
        cond.replace("value", QString::number(value));
        QString trueVal = ifMatch.captured(2).trimmed();
        QString falseVal = ifMatch.captured(3).trimmed();

        double condResult = evalSimpleExpression(cond);
        if (condResult > 0.5) {
            return applyFormatTemplate(trueVal, value, unit);
        }
        return applyFormatTemplate(falseVal, value, unit);
    }

    return applyFormatTemplate(returnExpr, value, unit);
}

double LuaExpressionEvaluator::evalSimpleExpression(const QString& expr) {
    QString e = expr.trimmed();

    // Replace variables with their values
    for (auto it = m_numericVars.begin(); it != m_numericVars.end(); ++it) {
        QRegularExpression re("\\b" + QRegularExpression::escape(it.key()) + "\\b");
        e.replace(re, QString::number(it.value(), 'f', 6));
    }

    // Evaluate simple arithmetic: parse left-to-right respecting precedence
    auto parseNumber = [](const QString& s, int& pos) -> double {
        int start = pos;
        bool hasDot = false;
        while (pos < s.size() && (s[pos].isDigit() || (s[pos] == '.' && !hasDot))) {
            if (s[pos] == '.') hasDot = true;
            pos++;
        }
        if (start == pos) return 0;
        return s.mid(start, pos - start).toDouble();
    };

    auto skipWhitespace = [](const QString& s, int& pos) {
        while (pos < s.size() && s[pos] == ' ') pos++;
    };

    auto getOp = [&](const QString& s, int& pos) -> QChar {
        skipWhitespace(s, pos);
        if (pos >= s.size()) return QChar();
        QChar c = s[pos];
        if (c == '+' || c == '-' || c == '*' || c == '/') {
            pos++;
            return c;
        }
        return QChar();
    };

    double result = 0;
    QChar lastOp = QLatin1Char('+');
    bool first = true;

    for (int i = 0; i < e.size();) {
        skipWhitespace(e, i);
        if (i >= e.size()) break;

        QChar op;
        if (!first) {
            op = getOp(e, i);
            if (op.isNull()) break;
        }
        first = false;

        skipWhitespace(e, i);
        double num = parseNumber(e, i);

        if (op == QLatin1Char('\0') || op == QLatin1Char('+')) result += num;
        else if (op == '-') result -= num;
        else if (op == '*') result *= num;
        else if (op == '/' && num != 0) result /= num;
    }

    return result;
}

QString LuaExpressionEvaluator::applyStringFormat(const QString& fmt, const QVector<QVariant>& args) {
    QString result;
    for (int i = 0; i < fmt.size(); ++i) {
        if (fmt[i] == '%' && i + 1 < fmt.size()) {
            i++;
            if (fmt[i] == 'd' && !args.isEmpty()) {
                result += QString::number(args[0].toInt());
            } else if (fmt[i] == 'f' && !args.isEmpty()) {
                result += QString::number(args[0].toDouble(), 'f', 1);
            } else if (fmt[i] == 's' && !args.isEmpty()) {
                result += args[0].toString();
            } else {
                result += '%' + QString(fmt[i]);
            }
        } else {
            result += fmt[i];
        }
    }
    return result;
}

QString LuaExpressionEvaluator::applyFormatTemplate(const QString& template_str, double value, const QString& unit) {
    QString result = template_str;

    QRegularExpression varRe(R"((\w+))");
    auto it = varRe.globalMatch(result);
    while (it.hasNext()) {
        auto m = it.next();
        QString var = m.captured(1);
        if (var == "value") {
            result.replace("value", QString::number(value, 'f', 1));
        } else if (var == "speed") {
            result.replace("speed", QString::number(value, 'f', 0));
        } else if (var == "unit") {
            result.replace("unit", unit);
        }
    }

    return result;
}

} // namespace ks