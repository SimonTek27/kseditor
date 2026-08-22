#pragma once

#include <QObject>
#include <QVariantMap>
#include <QVariantList>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDir>
#include <QRegularExpression>

namespace ks {

class CspConfigQmlBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool initialized READ isInitialized NOTIFY initializedChanged)

public:
    static CspConfigQmlBridge* instance() {
        static CspConfigQmlBridge* s_instance = new CspConfigQmlBridge();
        return s_instance;
    }

    bool isInitialized() const { return m_initialized; }

    Q_INVOKABLE QVariantMap loadFile(const QString& path) {
        QVariantMap data = parseIniFile(path);
        m_currentPath = path;
        m_initialized = true;
        emit initializedChanged();
        emit configLoaded(path, data);
        return data;
    }

    Q_INVOKABLE bool saveFile(const QString& path, const QVariantMap& data) {
        QDir().mkpath(QFileInfo(path).absolutePath());
        bool ok = writeIniFile(path, data);
        if (ok) {
            m_currentPath = path;
            emit configSaved(path, true);
        } else {
            emit configSaved(path, false);
        }
        return ok;
    }

    Q_INVOKABLE QVariantMap loadCarConfig(const QString& carPath) {
        return loadFile(carPath + "/extension/car.ini");
    }

    Q_INVOKABLE QVariantMap loadTrackConfig(const QString& trackPath) {
        return loadFile(trackPath + "/extension/track.ini");
    }

    Q_INVOKABLE bool saveCarConfig(const QString& carPath, const QVariantMap& data) {
        return saveFile(carPath + "/extension/car.ini", data);
    }

    Q_INVOKABLE bool saveTrackConfig(const QString& trackPath, const QVariantMap& data) {
        return saveFile(trackPath + "/extension/track.ini", data);
    }

    Q_INVOKABLE QStringList listConfigs(const QString& root) {
        QStringList result;
        for (const QString& dir : QStringList() << root + "/extension/config" << root + "/extension/textures") {
            QDir d(dir);
            if (d.exists()) {
                for (const QFileInfo& fi : d.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot))
                    result.append(fi.absoluteFilePath());
            }
        }
        return result;
    }

    Q_INVOKABLE QVariantList parseEmissives(const QVariantMap& sections) {
        QVariantList result;
        for (auto it = sections.constBegin(); it != sections.constEnd(); ++it) {
            if (!it.key().startsWith("EMISSIVE")) continue;
            QVariantMap s = it.value().toMap();
            QVariantMap e;
            e.insert("name", s.value("NAME").toString());
            e.insert("meshName", s.value("MESHES").toString());
            e.insert("materialName", s.value("MATERIALS").toString());
            e.insert("color", s.value("COLOR").toString());
            e.insert("location", s.value("LOCATION").toString());
            e.insert("lag", s.value("LAG", "0").toDouble());
            e.insert("active", s.value("ACTIVE", "1").toString() == "1");
            e.insert("sectionName", it.key());
            result.append(e);
        }
        return result;
    }

    Q_INVOKABLE QVariantList parseTrackLights(const QVariantMap& sections) {
        QVariantList result;
        for (auto it = sections.constBegin(); it != sections.constEnd(); ++it) {
            if (!it.key().startsWith("LIGHT_") || it.key() == "LIGHTING" || it.key() == "LIGHT_POLLUTION") continue;
            QVariantMap s = it.value().toMap();
            QVariantMap l;
            l.insert("meshName", s.value("MESHES").toString());
            l.insert("materialName", s.value("MATERIALS").toString());
            l.insert("color", s.value("COLOR").toString());
            l.insert("direction", s.value("DIRECTION").toString());
            l.insert("spot", s.value("SPOT", "150").toDouble());
            l.insert("range", s.value("RANGE", "20").toDouble());
            l.insert("active", s.value("ACTIVE", "1").toString() == "1");
            l.insert("sectionName", it.key());
            result.append(l);
        }
        return result;
    }

    Q_INVOKABLE QVariantList parseMaterialAdjustments(const QVariantMap& sections) {
        QVariantList result;
        for (auto it = sections.constBegin(); it != sections.constEnd(); ++it) {
            if (!it.key().startsWith("MATERIAL_ADJUSTMENT")) continue;
            QVariantMap s = it.value().toMap();
            QVariantMap a;
            a.insert("meshes", s.value("MESHES").toString());
            a.insert("materials", s.value("MATERIALS").toString());
            a.insert("key0", s.value("KEY_0").toString());
            a.insert("value0", s.value("VALUE_0").toString());
            a.insert("range", s.value("RANGE", "20").toDouble());
            a.insert("active", s.value("ACTIVE", "1").toString() == "1");
            a.insert("sectionName", it.key());
            result.append(a);
        }
        return result;
    }

    Q_INVOKABLE QVariantList parseConditions(const QVariantMap& sections) {
        QVariantList result;
        for (auto it = sections.constBegin(); it != sections.constEnd(); ++it) {
            if (!it.key().startsWith("CONDITION")) continue;
            QVariantMap s = it.value().toMap();
            QVariantMap c;
            c.insert("name", s.value("NAME").toString());
            c.insert("input", s.value("INPUT").toString());
            c.insert("flashingFrequency", s.value("FLASHING_FREQUENCY", "0").toDouble());
            c.insert("sectionName", it.key());
            result.append(c);
        }
        return result;
    }

    Q_INVOKABLE QVariantMap addEmissive(QVariantMap sections, const QVariantMap& emissive) {
        int idx = 0;
        for (auto it = sections.constBegin(); it != sections.constEnd(); ++it)
            if (it.key().startsWith("EMISSIVE")) idx++;
        QVariantMap s;
        s.insert("NAME", emissive.value("name").toString());
        s.insert("MESHES", emissive.value("meshName").toString());
        s.insert("COLOR", emissive.value("color").toString());
        s.insert("ACTIVE", "1");
        sections.insert(QString("EMISSIVE_%1").arg(idx), s);
        return sections;
    }

    Q_INVOKABLE QVariantMap removeSection(QVariantMap sections, const QString& sectionName) {
        sections.remove(sectionName);
        return sections;
    }

    Q_INVOKABLE QStringList getConditionInputs() {
        return {"ONE", "NONE", "TIME", "AMBIENT", "FOG", "SUN", "RACING_FLAG", "YEAR_PROGRESS",
                "SPEED", "RPM", "GEAR", "HEADLIGHTS", "HIGHBEAM", "TURN_LEFT", "TURN_RIGHT",
                "WIPER", "BRAKING", "REVERSING"};
    }

    Q_INVOKABLE QStringList getMaterialKeys() {
        return {"ksEmissive", "ksAlphaRef", "ksSpecular", "ksDiffuse", "ksAmbient"};
    }

    Q_INVOKABLE QStringList getSectionTypes() {
        return {"EMISSIVE", "LIGHT", "LIGHT_SERIES", "MATERIAL_ADJUSTMENT", "CONDITION",
                "BRAKEDISC", "BASIC", "ABOUT", "INCLUDE", "MESH_ADJUSTMENT",
                "SHADER_REPLACEMENT", "MODEL_REPLACEMENT", "ODOMETER", "INSTRUMENTS"};
    }

    Q_INVOKABLE QStringList getSectionNames(const QVariantMap& sections) {
        return sections.keys();
    }

    Q_INVOKABLE QString getSectionType(const QString& name) {
        if (name.startsWith("EMISSIVE")) return "EMISSIVE";
        if (name.startsWith("LIGHT_SERIES")) return "LIGHT_SERIES";
        if (name.startsWith("LIGHT_")) return "LIGHT";
        if (name.startsWith("MATERIAL_ADJUSTMENT")) return "MATERIAL_ADJUSTMENT";
        if (name.startsWith("CONDITION")) return "CONDITION";
        if (name.startsWith("BRAKEDISC")) return "BRAKEDISC";
        if (name == "BASIC") return "BASIC";
        if (name == "ABOUT") return "ABOUT";
        if (name.startsWith("INCLUDE")) return "INCLUDE";
        if (name.startsWith("MESH_ADJUSTMENT")) return "MESH_ADJUSTMENT";
        if (name.startsWith("SHADER_REPLACEMENT")) return "SHADER_REPLACEMENT";
        return "OTHER";
    }

    Q_INVOKABLE QStringList getSectionPropertyKeys(const QString& sectionName, const QVariantMap& sections) {
        QVariantMap s = sections.value(sectionName).toMap();
        return s.keys();
    }

    Q_INVOKABLE QVariantMap updateSection(QVariantMap sections, const QString& sectionName, const QVariantMap& values) {
        sections[sectionName] = values;
        return sections;
    }

    Q_INVOKABLE QVariantMap addSection(QVariantMap sections, const QString& type) {
        int idx = 0;
        QString prefix = type;
        if (type == "EMISSIVE") prefix = "EMISSIVE";
        else if (type == "LIGHT") prefix = "LIGHT_";
        else if (type == "MATERIAL_ADJUSTMENT") prefix = "MATERIAL_ADJUSTMENT_";
        else if (type == "CONDITION") prefix = "CONDITION_";
        else if (type == "BRAKEDISC") prefix = "BRAKEDISC_";
        else if (type == "MESH_ADJUSTMENT") prefix = "MESH_ADJUSTMENT_";

        for (auto it = sections.constBegin(); it != sections.constEnd(); ++it)
            if (it.key().startsWith(prefix)) idx++;

        QString newName = (idx == 0) ? type : QString("%1_%2").arg(prefix).arg(idx);
        sections[newName] = QVariantMap();
        return sections;
    }

    Q_INVOKABLE void openFileDialog() {
        // Emitted so QML can show a FileDialog
        emit fileDialogRequested();
    }

    Q_INVOKABLE QString getCurrentPath() const { return m_currentPath; }
    Q_INVOKABLE void setCurrentPath(const QString& p) { m_currentPath = p; }

    Q_INVOKABLE QVariantMap createDefaultCarConfig() {
        QVariantMap cfg;
        QVariantMap basic;
        basic["DESCRIPTION"] = "AC Car Config";
        basic["ENABLED"] = "1";
        cfg["BASIC"] = basic;
        return cfg;
    }

    Q_INVOKABLE QVariantMap createDefaultTrackConfig() {
        QVariantMap cfg;
        QVariantMap basic;
        basic["DESCRIPTION"] = "AC Track Config";
        basic["ENABLED"] = "1";
        cfg["BASIC"] = basic;
        return cfg;
    }

signals:
    void initializedChanged();
    void configLoaded(const QString& path, const QVariantMap& data);
    void configSaved(const QString& path, bool success);
    void fileDialogRequested();

private:
    explicit CspConfigQmlBridge(QObject* parent = nullptr) : QObject(parent) {}

    static QVariantMap parseIniFile(const QString& path) {
        QVariantMap root;
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return root;

        QTextStream in(&file);
        QString currentSection;
        static QRegularExpression sectionRe(R"(\[([^\]]+)\])");
        static QRegularExpression commentRe(R"(^(;|#).*)");

        while (!in.atEnd()) {
            QString line = in.readLine();
            if (commentRe.match(line).hasMatch()) continue;

            QRegularExpressionMatch sectionMatch = sectionRe.match(line);
            if (sectionMatch.hasMatch()) {
                currentSection = sectionMatch.captured(1).trimmed();
                if (!root.contains(currentSection))
                    root[currentSection] = QVariantMap();
                continue;
            }

            int eq = line.indexOf('=');
            if (eq > 0) {
                QString key = line.left(eq).trimmed();
                QString val = line.mid(eq + 1).trimmed();
                if (val.startsWith('"') && val.endsWith('"'))
                    val = val.mid(1, val.size() - 2);
                if (!key.isEmpty() && !currentSection.isEmpty()) {
                    QVariantMap section = root[currentSection].toMap();
                    section[key] = val;
                    root[currentSection] = section;
                }
            }
        }

        file.close();
        return root;
    }

    static bool writeIniFile(const QString& path, const QVariantMap& sections) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            return false;

        QTextStream out(&file);
        out << "; Generated by ksEditor CSP Config Editor\n\n";

        for (auto sit = sections.constBegin(); sit != sections.constEnd(); ++sit) {
            out << "[" << sit.key() << "]\n";
            QVariantMap vals = sit.value().toMap();
            for (auto vit = vals.constBegin(); vit != vals.constEnd(); ++vit)
                out << vit.key() << "=" << vit.value().toString() << "\n";
            out << "\n";
        }

        file.close();
        return true;
    }

    bool m_initialized = false;
    QString m_currentPath;
};

} // namespace ks
