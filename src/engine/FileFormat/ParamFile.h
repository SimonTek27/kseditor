#pragma once

#include <QString>
#include <QIODevice>
#include <QMap>
#include <QVector>
#include <QStringList>
#include <QVariant>
#include <QTextStream>
#include <QFile>
#include <QRegularExpression>

namespace ks::fileformat {

// ============================================================================
// ParamFile Parser - Bohemia Interactive config/ParamFile format
// Rewritten from CWR cfgManifest.cpp - parse "class CfgPatches" etc.
// Supports both legacy (semi-colon terminated) and modern (brace-delimited) syntax
// ============================================================================

struct ParamClass {
    QString name;
    QMap<QString, QString> properties;
    QMap<QString, ParamClass> subclasses;
    QVector<QString> arrayValues; // for class arrays like requiredAddons[]

    QString getString(const QString& key, const QString& def = QString()) const {
        return properties.value(key, def);
    }

    int getInt(const QString& key, int def = 0) const {
        bool ok;
        int val = properties.value(key).toInt(&ok);
        return ok ? val : def;
    }

    float getFloat(const QString& key, float def = 0.0f) const {
        bool ok;
        float val = properties.value(key).toFloat(&ok);
        return ok ? val : def;
    }

    bool getBool(const QString& key, bool def = false) const {
        const QString& val = properties.value(key);
        if (val.isEmpty()) return def;
        return val == "true" || val == "1" || val == "yes";
    }

    QStringList getStringArray(const QString& key) const {
        QString raw = properties.value(key);
        if (raw.isEmpty()) return {};
        // Handle array syntax: {"item1","item2","item3"}
        raw = raw.trimmed();
        if (raw.startsWith('{') && raw.endsWith('}'))
            raw = raw.mid(1, raw.length() - 2);
        QStringList result;
        for (const QString& item : raw.split(',', Qt::SkipEmptyParts)) {
            QString trimmed = item.trimmed().remove('"').remove('\'');
            if (!trimmed.isEmpty())
                result.append(trimmed);
        }
        return result;
    }
};

class ParamFile {
public:
    ParamClass root;

    bool load(const QString& filePath) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return false;
        return loadFromDevice(file);
    }

    bool loadFromDevice(QIODevice& device) {
        QTextStream stream(&device);
        stream.setEncoding(QStringConverter::Utf8);
        m_content = stream.readAll();
        m_pos = 0;
        m_line = 1;
        root = ParamClass();
        root.name = "root";
        skipWhitespace();
        while (m_pos < m_content.size()) {
            if (m_content.mid(m_pos, 6).startsWith("class ")) {
                ParamClass cls;
                if (parseClass(cls)) {
                    root.subclasses[cls.name] = cls;
                }
            } else {
                // Top-level property
                QString key = readToken();
                if (key.isEmpty()) break;
                skipWhitespace();
                if (m_pos < m_content.size() && m_content[m_pos] == '=') {
                    m_pos++;
                    skipWhitespace();
                    QString value = readValue();
                    root.properties[key] = value;
                }
                skipToNextLine();
            }
        }
        return true;
    }

    // Search all nested classes for a specific property
    QString findProperty(const QString& classPath, const QString& prop) const {
        QStringList parts = classPath.split('/', Qt::SkipEmptyParts);
        const ParamClass* current = &root;
        for (const QString& part : parts) {
            auto it = current->subclasses.find(part);
            if (it == current->subclasses.end())
                return {};
            current = &it.value();
        }
        return current->getString(prop);
    }

    // Get class by path (e.g., "CfgVehicles/Car")
    const ParamClass* getClass(const QString& path) const {
        QStringList parts = path.split('/', Qt::SkipEmptyParts);
        const ParamClass* current = &root;
        for (const QString& part : parts) {
            auto it = current->subclasses.find(part);
            if (it == current->subclasses.end())
                return nullptr;
            current = &it.value();
        }
        return current;
    }

private:
    QString m_content;
    int m_pos = 0;
    int m_line = 1;

    void skipWhitespace() {
        while (m_pos < m_content.size()) {
            QChar c = m_content[m_pos];
            if (c == '\n') { m_line++; m_pos++; }
            else if (c.isSpace() || c == '\r' || c == '\t') m_pos++;
            else if (c == '/' && m_pos + 1 < m_content.size() && m_content[m_pos + 1] == '/') {
                // Line comment
                while (m_pos < m_content.size() && m_content[m_pos] != '\n') m_pos++;
            } else if (c == '/' && m_pos + 1 < m_content.size() && m_content[m_pos + 1] == '*') {
                // Block comment
                m_pos += 2;
                while (m_pos + 1 < m_content.size() && !(m_content[m_pos] == '*' && m_content[m_pos + 1] == '/'))
                    m_pos++;
                m_pos += 2;
            } else break;
        }
    }

    void skipToNextLine() {
        while (m_pos < m_content.size() && m_content[m_pos] != '\n') m_pos++;
    }

    QString readToken() {
        int start = m_pos;
        while (m_pos < m_content.size()) {
            QChar c = m_content[m_pos];
            if (c.isLetterOrNumber() || c == '_' || c == '.') m_pos++;
            else break;
        }
        return m_content.mid(start, m_pos - start);
    }

    QString readValue() {
        skipWhitespace();
        if (m_pos >= m_content.size()) return {};

        // Quoted string
        if (m_content[m_pos] == '"' || m_content[m_pos] == '\'') {
            QChar quote = m_content[m_pos++];
            int start = m_pos;
            while (m_pos < m_content.size() && m_content[m_pos] != quote) {
                if (m_content[m_pos] == '\\') m_pos++;
                m_pos++;
            }
            QString val = m_content.mid(start, m_pos - start);
            if (m_pos < m_content.size()) m_pos++;
            return val;
        }

        // Array/brace
        if (m_content[m_pos] == '{') {
            int start = m_pos;
            int depth = 1;
            m_pos++;
            while (m_pos < m_content.size() && depth > 0) {
                if (m_content[m_pos] == '{') depth++;
                else if (m_content[m_pos] == '}') depth--;
                m_pos++;
            }
            return m_content.mid(start, m_pos - start);
        }

        // Number or identifier
        int start = m_pos;
        while (m_pos < m_content.size() && !m_content[m_pos].isSpace() &&
               m_content[m_pos] != ';' && m_content[m_pos] != '\n' &&
               m_content[m_pos] != '}' && m_content[m_pos] != '{') {
            m_pos++;
        }
        return m_content.mid(start, m_pos - start).trimmed();
    }

    bool parseClass(ParamClass& cls) {
        // "class Name" or "class Name : Base"
        skipWhitespace();
        QString name = readToken();
        if (name.isEmpty() || name != "class") return false;
        skipWhitespace();
        cls.name = readToken();
        skipWhitespace();

        // Optional inheritance
        if (m_pos < m_content.size() && m_content[m_pos] == ':') {
            m_pos++;
            skipWhitespace();
            cls.properties[":base"] = readToken();
        }

        skipWhitespace();
        if (m_pos >= m_content.size()) return false;

        // Skip semicolons (forward declarations)
        while (m_pos < m_content.size() && m_content[m_pos] == ';') m_pos++;
        skipWhitespace();

        // Class body
        if (m_pos < m_content.size() && m_content[m_pos] == '{') {
            m_pos++;
            skipWhitespace();
            while (m_pos < m_content.size() && m_content[m_pos] != '}') {
                skipWhitespace();
                if (m_pos >= m_content.size()) break;

                // Nested class
                if (m_content.mid(m_pos, 6).startsWith("class ")) {
                    ParamClass nested;
                    if (parseClass(nested)) {
                        cls.subclasses[nested.name] = nested;
                    }
                }
                // Array property: type name[] = {...}
                else if (m_content.mid(m_pos, 6).startsWith("class ") ||
                         (m_content.mid(m_pos, 4) == "enum")) {
                    skipToNextLine();
                }
                else {
                    // Regular property
                    skipWhitespace();
                    QString type = readToken();
                    skipWhitespace();
                    QString nameOrArray = readToken();
                    bool isArray = false;
                    if (nameOrArray.endsWith("[]")) {
                        isArray = true;
                        nameOrArray.chop(2);
                    }
                    skipWhitespace();
                    if (m_pos < m_content.size() && m_content[m_pos] == '=') {
                        m_pos++;
                        skipWhitespace();
                        QString value = readValue();
                        cls.properties[nameOrArray] = value;
                    }
                    skipWhitespace();
                    while (m_pos < m_content.size() && m_content[m_pos] == ';') m_pos++;
                }
                skipWhitespace();
            }
            if (m_pos < m_content.size()) m_pos++; // skip '}'
        }
        skipWhitespace();
        while (m_pos < m_content.size() && m_content[m_pos] == ';') m_pos++;
        return true;
    }
};

} // namespace ks::fileformat
