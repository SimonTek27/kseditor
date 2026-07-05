#include "../editor/SettingsModule.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace ks {

// ─── IniHandler ──────────────────────────────────────────────────────────────

IniHandler::IniHandler(QObject* parent) : QObject(parent) {}

QVariantMap IniHandler::read(const QString& path) {
    return parseIni(path);
}

QVariantMap IniHandler::readDir(const QString& dirPath) {
    QVariantMap result;
    QDir dir(dirPath);
    if (!dir.exists()) return result;
    QStringList iniFiles = dir.entryList(QStringList{"*.ini"}, QDir::Files);
    foreach (const QString& fname, iniFiles) {
        QString full = dir.absoluteFilePath(fname);
        QVariantMap fileMap = parseIni(full);
        QString key = QFileInfo(fname).baseName();
        result.insert(key, fileMap);
    }
    return result;
}

void IniHandler::write(const QString& path, const QVariantMap& data) {
    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&f);
        writeSection(out, "DATA", data);
        f.close();
    }
}

void IniHandler::writeFromResource(const QString& dirPath, const QVariantMap& resources) {
    QDir d(dirPath);
    if (!d.exists()) return;
    for (auto it = resources.constBegin(); it != resources.constEnd(); ++it) {
        const QString fileName = it.key();
        const QVariant value = it.value();
        if (value.metaType().id() == QMetaType::QVariantMap) {
            QVariantMap sectionsMap = value.toMap();
            QString filePath = dirPath + "/" + fileName + ".ini";
            QFile f(filePath);
            if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&f);
                for (auto sIt = sectionsMap.constBegin(); sIt != sectionsMap.constEnd(); ++sIt) {
                    const QString sectionName = sIt.key();
                    const QVariant sectionVal = sIt.value();
                    if (sectionVal.metaType().id() == QMetaType::QVariantMap) {
                        writeSection(out, sectionName, sectionVal.toMap());
                    } else {
                        writeSection(out, sectionName, QVariantMap{{"VALUE", sectionVal}});
                    }
                }
                f.close();
            }
        }
    }
}

QVariantMap IniHandler::parseIni(const QString& path) {
    QVariantMap root;
    QFileInfo fi(path);
    if (!fi.exists()) return root;
    QSettings settings(path, QSettings::IniFormat);

    foreach (const QString& group, settings.childGroups()) {
        settings.beginGroup(group);
        QVariantMap inner;
        foreach (const QString& key, settings.childKeys()) inner.insert(key, settings.value(key));
        settings.endGroup();
        root.insert(group, inner);
    }

    foreach (const QString& key, settings.allKeys()) {
        if (!root.contains(key)) {
            root.insert(key, settings.value(key));
        }
    }
    return root;
}

void IniHandler::writeSection(QTextStream& out, const QString& section, const QVariantMap& inner) {
    out << "[" << section << "]\n";
    for (auto it = inner.constBegin(); it != inner.constEnd(); ++it) {
        const QString key = it.key();
        const QVariant& val = it.value();
        if (val.metaType().id() == QMetaType::QVariantMap) {
            QVariantMap sub = val.toMap();
            out << key << " = " << "" << "\n";
            for (auto subIt = sub.constBegin(); subIt != sub.constEnd(); ++subIt) {
                out << key << "." << subIt.key() << "=" << subIt.value().toString() << "\n";
            }
        } else {
            out << key << "=" << val.toString() << "\n";
        }
    }
    out << "\n";
}

} // namespace ks
