#include "EditorModule.h"
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace ks {

phys_EditorModule* phys_EditorModule::s_instance = nullptr;

phys_EditorModule::phys_EditorModule(QObject* parent)
    : QObject(parent)
{}

phys_EditorModule* phys_EditorModule::instance() {
    if (!s_instance) {
        s_instance = new phys_EditorModule();
    }
    return s_instance;
}

void phys_EditorModule::initialize() {
    if (!m_initialized) {
        m_initialized = true;
        emit editorReady();
        qDebug() << "Physics Editor initialized";
    }
}

void phys_EditorModule::shutdown() {
    m_initialized = false;
}

void phys_EditorModule::setTool(const QString& tool) {
    if (m_currentTool != tool) {
        m_currentTool = tool;
        emit toolChanged(tool);
    }
}

void phys_EditorModule::setMode(const QString& mode) {
    if (m_currentMode != mode) {
        m_currentMode = mode;
        emit modeChanged(mode);
    }
}

void phys_EditorModule::loadPhysicsFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "phys_EditorModule: Cannot open file:" << path;
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull()) {
        qWarning() << "phys_EditorModule: Invalid JSON in file:" << path;
        return;
    }

    m_currentFile = path;
    emit physicsLoaded(path);
    qDebug() << "phys_EditorModule: Loaded physics file:" << path;
}

void phys_EditorModule::savePhysicsFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "phys_EditorModule: Cannot write to file:" << path;
        return;
    }

    QJsonObject data;
    data["mode"] = m_currentMode;
    data["tool"] = m_currentTool;

    QJsonDocument doc(data);
    file.write(doc.toJson());
    file.close();

    m_currentFile = path;
    emit physicsSaved(path);
    qDebug() << "phys_EditorModule: Saved physics file:" << path;
}

void phys_EditorModule::exportToAC(const QString& outputPath) {
    if (m_currentFile.isEmpty()) {
        qWarning() << "phys_EditorModule: No physics file loaded to export";
        return;
    }

    QFile source(m_currentFile);
    if (source.exists()) {
        QFile::copy(m_currentFile, outputPath);
        qDebug() << "phys_EditorModule: Exported to AC path:" << outputPath;
    }
}

bool phys_EditorModule::validatePhysics() {
    if (m_currentFile.isEmpty()) {
        qWarning() << "phys_EditorModule: No file loaded for validation";
        return false;
    }

    QFile file(m_currentFile);
    if (!file.exists()) {
        qWarning() << "phys_EditorModule: File does not exist:" << m_currentFile;
        return false;
    }

    return true;
}

}