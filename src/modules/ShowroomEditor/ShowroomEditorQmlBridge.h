#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVariantList>
#include "ShowroomSystem.h"

namespace ks {

class ShowroomEditorQmlBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString showroomName READ showroomName NOTIFY showroomChanged)
    Q_PROPERTY(float cameraDistance READ cameraDistance WRITE setCameraDistance NOTIFY configChanged)
    Q_PROPERTY(float cameraHeight READ cameraHeight WRITE setCameraHeight NOTIFY configChanged)
    Q_PROPERTY(float cameraAngle READ cameraAngle WRITE setCameraAngle NOTIFY configChanged)
    Q_PROPERTY(float cameraFov READ cameraFov WRITE setCameraFov NOTIFY configChanged)
    Q_PROPERTY(float rotateSpeed READ rotateSpeed WRITE setRotateSpeed NOTIFY configChanged)
    Q_PROPERTY(bool autoRotate READ autoRotate WRITE setAutoRotate NOTIFY configChanged)
    Q_PROPERTY(QString backgroundPath READ backgroundPath WRITE setBackgroundPath NOTIFY configChanged)
    Q_PROPERTY(QString ambientColor READ ambientColor WRITE setAmbientColor NOTIFY configChanged)
    Q_PROPERTY(QString sunColor READ sunColor WRITE setSunColor NOTIFY configChanged)
    Q_PROPERTY(float sunIntensity READ sunIntensity WRITE setSunIntensity NOTIFY configChanged)
    Q_PROPERTY(float ambientIntensity READ ambientIntensity WRITE setAmbientIntensity NOTIFY configChanged)
    Q_PROPERTY(bool hasUnsavedChanges READ hasUnsavedChanges NOTIFY unsavedChangesChanged)

public:
    static ShowroomEditorQmlBridge* instance();

    // Properties
    QString showroomName() const { return m_config.name; }
    float cameraDistance() const { return m_config.cameraDistance; }
    float cameraHeight() const { return m_config.cameraHeight; }
    float cameraAngle() const { return m_config.cameraAngle; }
    float cameraFov() const { return m_config.cameraFov; }
    float rotateSpeed() const { return m_config.rotateSpeed; }
    bool autoRotate() const { return m_config.autoRotate; }
    QString backgroundPath() const { return m_config.backgroundPath; }
    QString ambientColor() const { return m_config.ambientColor.name(); }
    QString sunColor() const { return m_config.sunColor.name(); }
    float sunIntensity() const { return m_config.sunIntensity; }
    float ambientIntensity() const { return m_config.ambientIntensity; }
    bool hasUnsavedChanges() const { return m_hasUnsavedChanges; }

    // Setters
    void setCameraDistance(float v);
    void setCameraHeight(float v);
    void setCameraAngle(float v);
    void setCameraFov(float v);
    void setRotateSpeed(float v);
    void setAutoRotate(bool v);
    void setBackgroundPath(const QString& v);
    void setAmbientColor(const QString& v);
    void setSunColor(const QString& v);
    void setSunIntensity(float v);
    void setAmbientIntensity(float v);

    // Q_INVOKABLE methods
    Q_INVOKABLE bool loadShowroom(const QString& path);
    Q_INVOKABLE bool saveShowroom(const QString& path);
    Q_INVOKABLE bool loadFromAc(const QString& acPath, const QString& showroomName);
    Q_INVOKABLE bool saveToAc(const QString& acPath, const QString& showroomName);
    Q_INVOKABLE QVariantList getCameras();
    Q_INVOKABLE QVariantMap getCamera(int index);
    Q_INVOKABLE void addCamera(const QVariantMap& camera);
    Q_INVOKABLE void removeCamera(int index);
    Q_INVOKABLE void updateCamera(int index, const QVariantMap& data);
    Q_INVOKABLE QVariantList getLights();
    Q_INVOKABLE QVariantMap getLight(int index);
    Q_INVOKABLE void addLight(const QVariantMap& light);
    Q_INVOKABLE void removeLight(int index);
    Q_INVOKABLE void updateLight(int index, const QVariantMap& data);
    Q_INVOKABLE QVariantMap validateConfig();
    Q_INVOKABLE QVariantMap validatePreviewConfig(int width, int height, int samples, float fov);
    Q_INVOKABLE QStringList getAvailableShowrooms(const QString& dir);
    Q_INVOKABLE bool generatePreview(const QString& carPath, int width, int height, const QString& outputPath);
    Q_INVOKABLE bool generateThumbnail(const QString& carPath, const QString& outputPath);
    Q_INVOKABLE void resetToDefaults();

signals:
    void showroomChanged();
    void configChanged();
    void unsavedChangesChanged();
    void showroomLoaded(const QString& name);
    void showroomSaved(const QString& name);
    void previewGenerated(const QString& outputPath);

private:
    static ShowroomEditorQmlBridge* s_instance;
    ShowroomEditorQmlBridge(QObject* parent = nullptr) : QObject(parent) {}

    void markUnsaved();

    ShowroomSystem::ShowroomConfig m_config;
    QVector<ShowroomSystem::ShowroomCamera> m_cameras;
    QVector<ShowroomSystem::ShowroomLight> m_lights;
    bool m_hasUnsavedChanges = false;
};

} // namespace ks
