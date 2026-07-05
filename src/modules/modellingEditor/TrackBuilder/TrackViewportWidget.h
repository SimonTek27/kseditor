#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QMatrix4x4>
#include <QVector3D>
#include <QVector2D>
#include <QColor>
#include <QElapsedTimer>
#include "TrackBuilderModule.h"

namespace ks { namespace track {

class TrackViewportWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit TrackViewportWidget(QWidget* parent = nullptr);
    ~TrackViewportWidget() override;

    void setModule(TrackBuilderModule* module);

    void rebuildTerrainMesh();
    void rebuildRoadMeshes();
    void setBrushVisible(bool v) { m_brushVisible = v; update(); }
    void setBrushPosition(float wx, float wz) { m_brushWX = wx; m_brushWZ = wz; update(); }
    void setBrushRadius(float r) { m_brushRadius = r; update(); }

    void setGridVisible(bool v) { m_gridVisible = v; update(); }
    void setAxesVisible(bool v) { m_axesVisible = v; update(); }

    void setOrbit(float yaw, float pitch, float dist) {
        m_camYaw = yaw; m_camPitch = pitch; m_camDist = dist;
        update();
    }
    void pan(float dx, float dy) {
        float speed = m_camDist * 0.002f;
        float yawRad = qDegreesToRadians(m_camYaw);
        float pitchRad = qDegreesToRadians(m_camPitch);
        QVector3D fwd(std::cos(pitchRad) * std::sin(yawRad),
                       std::sin(pitchRad),
                       std::cos(pitchRad) * std::cos(yawRad));
        QVector3D right = QVector3D::crossProduct(fwd, QVector3D(0, 1, 0)).normalized();
        QVector3D up = QVector3D::crossProduct(right, fwd).normalized();
        m_camTarget += right * (-dx * speed) + up * (dy * speed);
        update();
    }
    void zoom(float delta) {
        m_camDist *= (1.0f - delta * 0.001f);
        m_camDist = qMax(0.5f, m_camDist);
        update();
    }
    void setCameraTarget(const QVector3D& target) { m_camTarget = target; update(); }
    QVector3D cameraTarget() const { return m_camTarget; }

signals:
    void frameRendered(int triangles, int fps);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    struct TerrainGL {
        GLuint vao = 0, vbo = 0, ebo = 0;
        int indexCount = 0;
        int vertexCount = 0;
    };
    struct RoadGL {
        GLuint vao = 0, vbo = 0, ebo = 0;
        int indexCount = 0;
        int vertexCount = 0;
    };

    void setupShaders();
    void setupGrid();
    void teardownTerrain();
    void teardownRoads();
    void updateMatrices();

    void drawGrid(const QMatrix4x4& view, const QMatrix4x4& proj);
    void drawTerrain(const QMatrix4x4& view, const QMatrix4x4& proj);
    void drawRoads(const QMatrix4x4& view, const QMatrix4x4& proj);
    void drawBrush(const QMatrix4x4& view, const QMatrix4x4& proj);
    void drawAxes(const QMatrix4x4& view, const QMatrix4x4& proj);

    QOpenGLShaderProgram m_gridShader;
    QOpenGLShaderProgram m_terrainShader;
    QOpenGLShaderProgram m_roadShader;
    QOpenGLShaderProgram m_brushShader;

    GLuint m_gridVAO = 0;
    GLuint m_gridVBO = 0;
    int m_gridVertexCount = 0;

    using pfn_glGenVertexArrays     = void(*)(int, GLuint*);
    using pfn_glBindVertexArray     = void(*)(GLuint);
    using pfn_glDeleteVertexArrays  = void(*)(int, const GLuint*);

    pfn_glGenVertexArrays    _glGenVertexArrays    = nullptr;
    pfn_glBindVertexArray    _glBindVertexArray    = nullptr;
    pfn_glDeleteVertexArrays _glDeleteVertexArrays = nullptr;

    TerrainGL m_terrainGL;
    QVector<RoadGL> m_roadGL;

    TrackBuilderModule* m_module = nullptr;

    bool m_gridVisible = true;
    bool m_axesVisible = true;
    bool m_brushVisible = false;
    float m_brushWX = 0.f, m_brushWZ = 0.f;
    float m_brushRadius = 50.f;

    // Camera state
    QMatrix4x4 m_view;
    QMatrix4x4 m_proj;
    QVector3D m_camTarget;
    float m_camYaw = 30.f;
    float m_camPitch = 45.f;
    float m_camDist = 500.f;
    float m_fov = 50.f;

    int m_triCount = 0;
    int m_frameCount = 0;
    QElapsedTimer m_fpsTimer;
    int m_fps = 0;
};

}} // namespace ks::track
