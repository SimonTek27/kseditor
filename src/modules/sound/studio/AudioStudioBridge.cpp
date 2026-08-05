#include "AudioStudioBridge.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

namespace ks {
namespace audio {

AudioStudioBridge::AudioStudioBridge(QObject* parent)
    : QObject(parent) {}

bool AudioStudioBridge::loadProject(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        emit errorOccurred("Cannot open project file: " + path);
        return false;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        emit errorOccurred("JSON parse error: " + parseError.errorString());
        return false;
    }

    m_lastProjectData = doc.object();
    m_currentProjectPath = path;
    emit projectLoaded(path);
    return true;
}

bool AudioStudioBridge::saveProject(const QString& path) {
    QFileInfo fi(path);
    QDir().mkpath(fi.absolutePath());

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        emit errorOccurred("Cannot write project file: " + path);
        return false;
    }

    QJsonDocument doc(m_lastProjectData);
    file.write(doc.toJson(QJsonDocument::Indented));
    m_currentProjectPath = path;
    emit projectSaved(path);
    return true;
}

bool AudioStudioBridge::importBank(const QString& bankPath) {
    QFile file(bankPath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit errorOccurred("Cannot open bank file: " + bankPath);
        return false;
    }

    // Store the bank reference in project data
    QJsonObject bankInfo;
    bankInfo["path"] = bankPath;
    bankInfo["name"] = QFileInfo(bankPath).completeBaseName();
    bankInfo["size"] = file.size();

    QJsonArray banks = m_lastProjectData["importedBanks"].toArray();
    banks.append(bankInfo);
    m_lastProjectData["importedBanks"] = banks;

    emit bankImported(bankPath);
    return true;
}

bool AudioStudioBridge::exportBank(const QString& bankPath) {
    if (m_lastProjectData.isEmpty()) {
        emit errorOccurred("No project data to export");
        return false;
    }

    QFileInfo fi(bankPath);
    QDir().mkpath(fi.absolutePath());

    QFile file(bankPath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit errorOccurred("Cannot write bank file: " + bankPath);
        return false;
    }

    // Write project data as bank representation
    QJsonDocument doc(m_lastProjectData);
    file.write(doc.toJson());

    emit bankExported(bankPath);
    return true;
}

} // namespace audio
} // namespace ks
