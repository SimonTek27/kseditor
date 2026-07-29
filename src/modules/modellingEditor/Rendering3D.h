#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QVector3D>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QImage>
#include <QMap>

#include "Geometry3D.h"

namespace ks {
namespace rendering {

class Shader3D : public QObject
{
    Q_OBJECT
public:
    explicit Shader3D(QObject* parent = nullptr) : QObject(parent) {}
    ~Shader3D() {}

    enum ShaderType { PBR, Emission, Transparent, Custom };

    void setType(ShaderType type) { m_type = type; }
    ShaderType type() const { return m_type; }

    struct BSDF {
        float baseColor[3] = {0.8f, 0.8f, 0.8f};
        float metallic = 0.0f;
        float roughness = 0.5f;
        float specular = 0.5f;
        float alpha = 1.0f;
        float ior = 1.45f;
    };

    void setBSDF(const BSDF& bsdf) { m_bsdf = bsdf; }
    BSDF bsdf() const { return m_bsdf; }

    void addNode(const QString& nodeId, const QString& nodeType);
    void removeNode(const QString& nodeId);
    void connectNodes(const QString& fromNode, const QString& toNode, const QString& fromSocket, const QString& toSocket);
    void disconnectNodes(const QString& fromNode, const QString& toNode, const QString& fromSocket, const QString& toSocket);

    QString compile() const;

 signals:
    void shaderModified();

private:
    struct Connection {
        QString fromNode;
        QString toNode;
        QString fromSocket;
        QString toSocket;
    };

    ShaderType m_type = PBR;
    BSDF m_bsdf;
    QMap<QString, QString> m_nodes;
    QVector<Connection> m_connections;
};

class Light3D : public QObject
{
    Q_OBJECT
public:
    explicit Light3D(QObject* parent = nullptr) : QObject(parent) {}
    ~Light3D() {}

    enum LightType { Point, Spot, Sun, Area };

    void setType(LightType type) { m_type = type; }
    LightType type() const { return m_type; }

    void setPosition(const QVector3D& pos) { m_position = pos; }
    QVector3D position() const { return m_position; }

    void setDirection(const QVector3D& dir) { m_direction = dir.normalized(); }
    QVector3D direction() const { return m_direction; }

    void setColor(const QVector3D& color) { m_color = color; }
    QVector3D color() const { return m_color; }

    void setIntensity(float intensity) { m_intensity = intensity; }
    float intensity() const { return m_intensity; }

    void setAngle(float angle) { m_angle = qBound(0.0f, angle, 180.0f); }
    float angle() const { return m_angle; }

    void setFalloff(float falloff) { m_falloff = falloff; }
    float falloff() const { return m_falloff; }

 signals:
    void lightModified();

private:
    LightType m_type = Point;
    QVector3D m_position;
    QVector3D m_direction = QVector3D(0, -1, 0);
    QVector3D m_color = QVector3D(1, 1, 1);
    float m_intensity = 100.0f;
    float m_angle = 45.0f;
    float m_falloff = 1.0f;
};

class Camera3D : public QObject
{
    Q_OBJECT
public:
    explicit Camera3D(QObject* parent = nullptr) : QObject(parent) {}
    ~Camera3D() {}

    void setPosition(const QVector3D& pos) { m_position = pos; }
    QVector3D position() const { return m_position; }

    void setTarget(const QVector3D& target) { m_target = target; }
    QVector3D target() const { return m_target; }

    void setUp(const QVector3D& up) { m_up = up.normalized(); }
    QVector3D up() const { return m_up; }

    void setFOV(float fov) { m_fov = qBound(10.0f, fov, 180.0f); }
    float fov() const { return m_fov; }

    void setNear(float value) { m_near = value; }
    float getNear() const { return m_near; }

    void setFar(float value) { m_far = value; }
    float getFar() const { return m_far; }

    QMatrix4x4 viewMatrix() const;
    QMatrix4x4 projectionMatrix() const;

    void orbit(const QVector3D& center, float azimuth, float elevation);
    void pan(float dx, float dy);
    void zoom(float delta);

 signals:
    void cameraModified();

private:
    QVector3D m_position = QVector3D(0, 5, 10);
    QVector3D m_target = QVector3D(0, 0, 0);
    QVector3D m_up = QVector3D(0, 1, 0);
    float m_fov = 50.0f;
    float m_near = 0.01f;
    float m_far = 1000.0f;
};

class RenderEngine : public QObject
{
    Q_OBJECT
public:
    explicit RenderEngine(QObject* parent = nullptr) : QObject(parent) {}
    ~RenderEngine() {}

    enum RenderEngineType { Eevee, Cycles, OpenGL };

    void setEngine(RenderEngineType type) { m_engine = type; }
    RenderEngineType engine() const { return m_engine; }

    void setScene(geometry::Scene3D* scene) { m_scene = scene; }
    geometry::Scene3D* scene() const { return m_scene; }

    void setCamera(Camera3D* camera) { m_camera = camera; }
    Camera3D* camera() const { return m_camera; }

    void render(int width, int height);
    QImage result() const { return m_result; }

    void setSamples(int samples) { m_samples = samples; }
    int samples() const { return m_samples; }

    void setResolution(int width, int height) { m_width = width; m_height = height; }
    void setBackgroundColor(const QVector3D& color) { m_background = color; }

    void enableShadows(bool enable) { m_shadows = enable; }
    void enableAO(bool enable) { m_ao = enable; }
    void enableGI(bool enable) { m_gi = enable; }

 signals:
    void renderStarted(int width, int height);
    void renderProgress(int percent);
    void renderComplete();

private:
    RenderEngineType m_engine = Eevee;
    geometry::Scene3D* m_scene = nullptr;
    Camera3D* m_camera = nullptr;
    QImage m_result;
    int m_samples = 128;
    int m_width = 1920;
    int m_height = 1080;
    QVector3D m_background = QVector3D(0.05f, 0.05f, 0.05f);
    bool m_shadows = true;
    bool m_ao = true;
    bool m_gi = false;
};

} // namespace rendering
} // namespace ks
