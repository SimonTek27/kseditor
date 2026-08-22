#pragma once

#include <QQuickFramebufferObject>
#include <QVulkanWindow>
#include <QVulkanWindowRenderer>
#include <QMatrix4x4>
#include <QVector3D>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QHoverEvent>
#include <QTabletEvent>

#include "3DModeling_Viewport.h"

namespace ks {

class VulkanViewportRendererWrapper : public QVulkanWindowRenderer {
public:
    VulkanViewportRendererWrapper(QVulkanWindow* window, SceneGraph* scene)
        : m_renderer(window, scene) {}

    void initResources() override { m_renderer.initResources(); }
    void initSwapChainResources() override { m_renderer.initSwapChainResources(); }
    void releaseSwapChainResources() override { m_renderer.releaseSwapChainResources(); }
    void releaseResources() override { m_renderer.releaseResources(); }
    void startNextFrame() override { m_renderer.startNextFrame(); }

    void setScene(SceneGraph* scene) { m_renderer.setScene(scene); }
    void updateViewportSize(int w, int h) { m_renderer.updateViewportSize(w, h); }

    VulkanViewportRenderer& renderer() { return m_renderer; }

private:
    VulkanViewportRenderer m_renderer;
};

class VulkanViewportItem : public QQuickFramebufferObject {
    Q_OBJECT
    Q_PROPERTY(qreal camYaw READ camYaw WRITE setCamYaw NOTIFY camYawChanged)
    Q_PROPERTY(qreal camPitch READ camPitch WRITE setCamPitch NOTIFY camPitchChanged)
    Q_PROPERTY(qreal camDistance READ camDistance WRITE setCamDistance NOTIFY camDistanceChanged)
    Q_PROPERTY(QVector3D camTarget READ camTarget WRITE setCamTarget NOTIFY camTargetChanged)
    Q_PROPERTY(bool gridVisible READ gridVisible WRITE setGridVisible NOTIFY gridVisibleChanged)
    Q_PROPERTY(int viewMode READ viewMode WRITE setViewMode NOTIFY viewModeChanged)
    Q_PROPERTY(bool gizmoVisible READ gizmoVisible WRITE setGizmoVisible NOTIFY gizmoVisibleChanged)
    Q_PROPERTY(int gizmoMode READ gizmoMode WRITE setGizmoMode NOTIFY gizmoModeChanged)

public:
    explicit VulkanViewportItem(QQuickItem* parent = nullptr);
    ~VulkanViewportItem() override;

    Renderer* createRenderer() const override;

    qreal camYaw() const { return m_camYaw; }
    qreal camPitch() const { return m_camPitch; }
    qreal camDistance() const { return m_camDistance; }
    QVector3D camTarget() const { return m_camTarget; }
    bool gridVisible() const { return m_gridVisible; }
    int viewMode() const { return m_viewMode; }
    bool gizmoVisible() const { return m_gizmoVisible; }
    int gizmoMode() const { return m_gizmoMode; }

    void setCamYaw(qreal yaw);
    void setCamPitch(qreal pitch);
    void setCamDistance(qreal dist);
    void setCamTarget(const QVector3D& target);
    void setGridVisible(bool visible);
    void setViewMode(int mode);
    void setGizmoVisible(bool visible);
    void setGizmoMode(int mode);

    Q_INVOKABLE void orbit(qreal dx, qreal dy);
    Q_INVOKABLE void pan(qreal dx, qreal dy);
    Q_INVOKABLE void zoom(qreal delta);
    Q_INVOKABLE void resetCamera();
    Q_INVOKABLE void focusOnSelection();

    void setSceneGraph(SceneGraph* scene);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void hoverMoveEvent(QHoverEvent* event) override;
    void hoverLeaveEvent(QHoverEvent* event) override;
    void tabletEvent(QTabletEvent* event) override;

signals:
    void camYawChanged();
    void camPitchChanged();
    void camDistanceChanged();
    void camTargetChanged();
    void gridVisibleChanged();
    void viewModeChanged();
    void gizmoVisibleChanged();
    void gizmoModeChanged();
    void objectSelected(int objectId, const QString& name);
    void frameRendered(int triangles, int fps);

private:
    void updateCamera();
    void updateGizmo();

    qreal m_camYaw = 30.0;
    qreal m_camPitch = 45.0;
    qreal m_camDistance = 50.0;
    QVector3D m_camTarget = QVector3D(0, 0, 0);
    bool m_gridVisible = true;
    int m_viewMode = 0;
    bool m_gizmoVisible = false;
    int m_gizmoMode = 0;

    QPointF m_lastMousePos;
    bool m_lmbDown = false;
    bool m_mmbDown = false;
    bool m_rmbDown = false;
    int m_dragAxis = -1;

    SceneGraph* m_sceneGraph = nullptr;
    mutable VulkanViewportRendererWrapper* m_rendererWrapper = nullptr;
};

} // namespace ks