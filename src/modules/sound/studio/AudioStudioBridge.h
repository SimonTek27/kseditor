#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>

namespace ks {
namespace audio {

// ============================================================================
// AudioStudioBridge — convenience bridge for project-level audio operations
//
// Wraps file I/O for .ksaudio projects and .bank import/export, providing
// a QML-accessible interface used by StudioModule.
// ============================================================================

class AudioStudioBridge : public QObject {
    Q_OBJECT
public:
    explicit AudioStudioBridge(QObject* parent = nullptr);

    // Project I/O
    bool loadProject(const QString& path);
    bool saveProject(const QString& path);

    // Bank I/O
    bool importBank(const QString& bankPath);
    bool exportBank(const QString& bankPath);

    // Accessors
    QString currentProjectPath() const { return m_currentProjectPath; }
    QJsonObject lastProjectData() const { return m_lastProjectData; }

signals:
    void projectLoaded(const QString& path);
    void projectSaved(const QString& path);
    void bankImported(const QString& path);
    void bankExported(const QString& path);
    void errorOccurred(const QString& message);

private:
    QString m_currentProjectPath;
    QJsonObject m_lastProjectData;
};

} // namespace audio
} // namespace ks
