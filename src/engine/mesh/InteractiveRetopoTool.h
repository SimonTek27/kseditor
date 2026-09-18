#pragma once

#include <QObject>
#include <QWidget>
#include <QVector>
#include <QVector3D>
#include <QJsonObject>
#include <QUuid>

namespace ks {

class MeshData;

class InteractiveRetopoTool : public QObject {
    Q_OBJECT
public:
    explicit InteractiveRetopoTool(QObject* parent = nullptr) : QObject(parent) {}
    ~InteractiveRetopoTool() override = default;

    bool isActive() const { return m_active; }
    void setActive(bool active) { m_active = active; emit activeChanged(active); }

    float brushRadius() const { return m_brushRadius; }
    void setBrushRadius(float r) { m_brushRadius = r; }

    float detail() const { return m_detail; }
    void setDetail(float d) { m_detail = d; }

    void setTargetMesh(MeshData* mesh) { m_targetMesh = mesh; }

    void beginStroke(const QVector3D& pos);
    void continueStroke(const QVector3D& pos);
    void endStroke();
    void applyRetopology();

    QJsonObject toJson() const { return {}; }
    void fromJson(const QJsonObject&) {}

signals:
    void activeChanged(bool active);
    void strokeStarted();
    void strokeEnded();
    void meshUpdated();

private:
    bool m_active = false;
    float m_brushRadius = 0.1f;
    float m_detail = 0.5f;
    MeshData* m_targetMesh = nullptr;
    QVector<QVector3D> m_strokePoints;
};

} // namespace ks
