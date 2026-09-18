#pragma once

#include "TrackBuilderModule.h"
#include "TrackViewportWidget.h"
#include <QWidget>
#include <QMatrix4x4>
#include <QVector3D>
#include <QMouseEvent>
#include <QWheelEvent>

namespace ks { namespace track {

class TrackViewport : public QWidget
{
    Q_OBJECT
public:
    explicit TrackViewport(QWidget* parent = nullptr);
    ~TrackViewport() override;

    void setModule(TrackBuilderModule* module);

    void resetCamera();
    void focusOnTerrain();
    void setEditMode(const QString& mode) { m_editMode = mode; }

public slots:
    void onTerrainModified();
    void onProjectChanged();

signals:
    void roadPointPlaced(float x, float y, float z);
    void propPlaced(float x, float y, float z);
    void brushMoved(float x, float z);

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;

private:
    float      m_camYaw   = 30.f;
    float      m_camPitch = 45.f;
    float      m_camDist  = 500.f;
    QVector3D  m_camTarget = {0.f, 0.f, 0.f};

    QPoint     m_lastMousePos;
    bool       m_lmbDown = false;
    bool       m_rmbDown = false;
    bool       m_mmbDown = false;
    QVector3D  m_brushWorldPos;

    TrackBuilderModule* m_module = nullptr;
    QString             m_editMode = "navigate";

    TrackViewportWidget* m_viewportWidget = nullptr;
    bool                  m_terrainDirty = true;
    bool  m_brushVisible = false;
    float m_brushRadius  = 50.f;
};

}} // namespace ks::track
