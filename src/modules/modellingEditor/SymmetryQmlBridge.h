#pragma once

#include <QObject>
#include <QString>
#include "core/tools/SymmetryManager.h"

namespace ks {

class SceneGraph;
class SceneObject;

class SymmetryQmlBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(int axis READ axis WRITE setAxis NOTIFY axisChanged)
    Q_PROPERTY(float offset READ offset WRITE setOffset NOTIFY offsetChanged)
    Q_PROPERTY(float weldThreshold READ weldThreshold WRITE setWeldThreshold NOTIFY weldThresholdChanged)
    Q_PROPERTY(int clipMode READ clipMode WRITE setClipMode NOTIFY clipModeChanged)
    Q_PROPERTY(int mergeMode READ mergeMode WRITE setMergeMode NOTIFY mergeModeChanged)
    Q_PROPERTY(bool previewVisible READ previewVisible WRITE setPreviewVisible NOTIFY previewChanged)
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY selectionChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)

public:
    explicit SymmetryQmlBridge(QObject* parent = nullptr);

    static SymmetryQmlBridge& instance() {
        static SymmetryQmlBridge inst;
        return inst;
    }

    int axis() const { return static_cast<int>(m_axis); }
    void setAxis(int a);

    float offset() const { return m_offset; }
    void setOffset(float o);

    float weldThreshold() const { return m_weldThreshold; }
    void setWeldThreshold(float t);

    int clipMode() const { return static_cast<int>(m_clipMode); }
    void setClipMode(int m);

    int mergeMode() const { return static_cast<int>(m_mergeMode); }
    void setMergeMode(int m);

    bool previewVisible() const { return m_previewVisible; }
    void setPreviewVisible(bool v);

    bool hasSelection() const { return m_hasSelection; }
    QString statusText() const { return m_statusText; }

    void setScene(SceneGraph* scene) { m_scene = scene; }
    SceneGraph* scene() const { return m_scene; }

public slots:
    void applySymmetry();
    void previewSymmetry();
    void clearPreview();
    void updateSelection();

signals:
    void axisChanged();
    void offsetChanged();
    void weldThresholdChanged();
    void clipModeChanged();
    void mergeModeChanged();
    void previewChanged();
    void selectionChanged();
    void symmetryApplied(bool success, const QString& info);
    void statusChanged();

private:
    SymmetryManager::Axis m_axis = SymmetryManager::Axis::X;
    float m_offset = 0.0f;
    float m_weldThreshold = 0.001f;
    SymmetryManager::ClipMode m_clipMode = SymmetryManager::ClipMode::None;
    SymmetryManager::MergeMode m_mergeMode = SymmetryManager::MergeMode::Append;
    bool m_previewVisible = false;
    bool m_hasSelection = false;
    QString m_statusText = "Ready";
    SceneGraph* m_scene = nullptr;

    SceneObject* getSelectedMeshObject();
    MeshData sceneMeshToMeshData(SceneObject* obj);
    bool meshDataToScene(MeshData& md, const QString& name);
    int m_previewObjectId = -1;
};

}
