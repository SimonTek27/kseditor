#include "AudioEditorModule.h"
#include "../../../core/Audio/AudioStudioTypes.h"
#include "../../../core/Audio/AudioTypes.h"
#include "../../../core/Audio/FSPROImporter.h"
#include "../../../core/Audio/BankWriter.h"

#include <QFile>
#include <QJsonDocument>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>
#include <QDir>
#include <QJsonArray>
#include <QProcess>
#include <QFileInfo>
#include <QAudioFormat>

namespace ks {

struct AudioEditorModule::Impl {
    audio::AudioStudio* studio = nullptr;
    audio::AudioProject* currentProject = nullptr;
    QString projectPath;
    bool modified = false;
    QStringList recentFiles;
};

AudioEditorModule* AudioEditorModule::s_instance = nullptr;

AudioEditorModule::AudioEditorModule(QObject* parent)
    : QObject(parent), m_impl(new Impl) {
    s_instance = this;
    m_impl->studio = new audio::AudioStudio(this);
}

AudioEditorModule::~AudioEditorModule() {
    delete m_impl;
}

bool AudioEditorModule::initialize() {
    qDebug() << "AudioEditorModule initialized";
    return true;
}

void AudioEditorModule::shutdown() {
    if (m_impl->modified) {
        m_impl->studio->saveProject(m_impl->projectPath);
    }
    m_impl->currentProject = nullptr;
    m_impl->projectPath.clear();
    m_impl->modified = false;
}

void AudioEditorModule::onNewProject() {
    if (m_impl->modified) {
        m_impl->studio->saveProject(m_impl->projectPath);
    }
    m_impl->studio->newProject();
    m_impl->currentProject = new audio::AudioProject(this);
    m_impl->projectPath.clear();
    m_impl->modified = false;
    emit soundChanged();
}

void AudioEditorModule::onOpenProject() {
    QString path = QFileDialog::getOpenFileName(nullptr,
        "Open Audio Project",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "Audio Projects (*.ksaudio);;"
        "FMOD Studio Projects (*.fspro);;"
        "All Files (*)");

    if (path.isEmpty()) return;

    if (path.endsWith(".fspro", Qt::CaseInsensitive)) {
        ks::audio::KSFSPROImporter importer;
        QString ksaudioPath = QFileInfo(path).absolutePath()
                             + "/" + QFileInfo(path).completeBaseName() + ".ksaudio";
        if (importer.convertFile(path, ksaudioPath)) {
            path = ksaudioPath;
            qInfo() << "Auto-imported .fspro -> .ksaudio:" << path;
        } else {
            QMessageBox::warning(nullptr, "Import Failed",
                "Failed to import FMOD project:\n" + importer.lastError());
            return;
        }
    }

    if (m_impl->studio->loadProject(path)) {
        m_impl->projectPath = path;
        m_impl->modified = false;
        emit soundChanged();
    }
}

void AudioEditorModule::onSaveProject() {
    if (m_impl->projectPath.isEmpty()) {
        QString path = QFileDialog::getSaveFileName(nullptr,
            "Save Audio Project",
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            "Audio Projects (*.ksaudio)");
        if (path.isEmpty()) return;
        m_impl->projectPath = path;
    }

    if (m_impl->studio->saveProject(m_impl->projectPath)) {
        m_impl->modified = false;
    }
}

void AudioEditorModule::onImportAsset() {
    QStringList paths = QFileDialog::getOpenFileNames(nullptr,
        "Import Audio / FMOD Project",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "All Supported (*.wav *.ogg *.mp3 *.flac *.aiff *.fspro);;"
        "Audio Files (*.wav *.ogg *.mp3 *.flac *.aiff);;"
        "FMOD Studio Projects (*.fspro);;"
        "All Files (*)");

    if (paths.isEmpty()) return;

    for (const QString& path : paths) {
        if (path.endsWith(".fspro", Qt::CaseInsensitive)) {
            ks::audio::KSFSPROImporter importer;
            QString ksaudioPath = QFileInfo(path).absolutePath()
                                 + "/" + QFileInfo(path).completeBaseName() + ".ksaudio";
            if (importer.convertFile(path, ksaudioPath)) {
                qInfo() << "Imported .fspro -> .ksaudio:" << ksaudioPath;
            } else {
                QMessageBox::warning(nullptr, "Import Failed",
                    "Failed to import FMOD project:\n" + importer.lastError());
            }
            continue;
        }

        QString destDir = QFileInfo(m_impl->projectPath).absolutePath() + "/audio";
        QDir().mkpath(destDir);

        audio::AudioManager manager;
        manager.importAudio(path, destDir);
    }
    m_impl->modified = true;
    emit soundChanged();
}

void AudioEditorModule::onExportAsset() {
    QString path = QFileDialog::getSaveFileName(nullptr,
        "Export Audio Project",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "Audio Files (*.wav);;All Files (*)");

    if (path.isEmpty()) return;

    if (m_impl->studio->saveProject(path)) {
        m_impl->modified = false;
    }
}

void AudioEditorModule::onBuildBanks() {
    if (m_impl->projectPath.isEmpty()) return;

    QString outputDir = QFileInfo(m_impl->projectPath).absolutePath() + "/banks";
    QDir().mkpath(outputDir);

    ks::audio::KSBankWriter writer;
    QObject::connect(&writer, &ks::audio::KSBankWriter::writeCompleted, this, [&](const QString& dir) {
        qInfo() << "Bank build completed:" << dir;
    });

    ks::audio::KSAudioProject project;
    if (project.load(m_impl->projectPath)) {
        if (writer.writeProjectBanks(project, outputDir)) {
            QString guidsPath = QFileInfo(m_impl->projectPath).absolutePath() + "/sfx/GUIDs.txt";
            QDir().mkpath(QFileInfo(guidsPath).absolutePath());
            writer.writeGUIDsFile(project, guidsPath);
        } else {
            qWarning() << "Bank build failed:" << writer.lastError();
        }
    }

    emit soundChanged();
}

} // namespace ks