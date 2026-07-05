#pragma once
#include <QWidget>
#include <QListWidget>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QGroupBox>
#include <QPushButton>
#include "core/Graphics/SceneData.h"
#include <core/sys/UndoStack.h>

namespace Ks {
using ks::UndoStack;

class PhysicsPanel : public QWidget {
    Q_OBJECT
public:
    explicit PhysicsPanel(UndoStack* undoStack, QWidget* parent = nullptr);

    void setScene(Scene* scene);
    void setSelectedNode(const QString& nodeName);
    void refresh();

signals:
    void physicsChanged();

private slots:
    void onAddCollider();
    void onRemoveCollider();
    void onColliderSelected(int idx);
    void onColliderTypeChanged(int idx);
    void onBodyTypeChanged(int idx);
    void onParamChanged();
    void onSurfaceTypeChanged(const QString&);

private:
    void buildUI();
    void populateColliderList();
    void loadCollider(int idx);
    void saveCurrentCollider();

    Scene*       m_scene     = nullptr;
    UndoStack*   m_undo;
    QString      m_nodeName;
    int          m_currentCollider = -1;

    QComboBox*       m_bodyType;
    QDoubleSpinBox*  m_mass;
    QDoubleSpinBox*  m_linDamp;
    QDoubleSpinBox*  m_angDamp;

    QListWidget*     m_colliderList;
    QPushButton*     m_addBtn;
    QPushButton*     m_removeBtn;

    QGroupBox*       m_colliderGroup;
    QComboBox*       m_colType;
    QDoubleSpinBox*  m_hx, *m_hy, *m_hz;
    QDoubleSpinBox*  m_radius;
    QDoubleSpinBox*  m_height;
    QDoubleSpinBox*  m_cx, *m_cy, *m_cz;
    QDoubleSpinBox*  m_friction;
    QDoubleSpinBox*  m_restitution;
    QCheckBox*       m_isTrigger;
    QComboBox*       m_surfaceType;

    static const QStringList kSurfaceTypes;
};

} // namespace Ks