#pragma once
#include <QObject>
#include <QString>

namespace ks {

class phys_EditorModule : public QObject {
    Q_OBJECT

public:
    static phys_EditorModule* instance();

    void initialize();
    void shutdown();
    QString getCurrentTool() const { return m_currentTool; }
    void setTool(const QString& tool);

    void setMode(const QString& mode);
    void loadPhysicsFile(const QString& path);
    void savePhysicsFile(const QString& path);
    void exportToAC(const QString& outputPath);
    bool validatePhysics();

signals:
    void toolChanged(const QString& tool);
    void editorReady();
    void modeChanged(const QString& mode);
    void physicsLoaded(const QString& path);
    void physicsSaved(const QString& path);

private:
    explicit phys_EditorModule(QObject* parent = nullptr);
    static phys_EditorModule* s_instance;

    QString m_currentTool = "suspension";
    bool m_initialized = false;
    QString m_currentMode;
    QString m_currentFile;
};

}