#pragma once

#include <QWidget>
#include <QListWidget>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include "CharacterEditor.h"

namespace ks {

struct CharacterSkeleton;
struct SkeletonTool;

class CharacterEditorWidget : public QWidget {
    Q_OBJECT
public:
    explicit CharacterEditorWidget(QWidget* parent = nullptr);

    void createHumanoid(float height = 1.8f);
    void createQuadruped(float height = 1.5f, float length = 2.0f);

    CharacterSkeleton* skeleton() const;
    void applyFK(int boneIndex, const QVector3D& rotation);
    void applyIK(int rootBone, int endBone, const QVector3D& target, bool stretch = false);

signals:
    void boneSelected(int index);
    void skeletonModified();

private slots:
    void onBoneSelected(QListWidgetItem* current);
    void onAddBone();
    void onRemoveBone();
    void onNameChanged(const QString& text);
    void onPositionChanged();
    void onRotationChanged();
    void onCreateHumanoid();
    void onCreateQuadruped();
    void updateBoneUI();
    void clearBoneUI();

private:
    void setupUI();
    void refreshBoneList();

    CharacterEditor* m_editor;
    QListWidget* m_boneList;
    QPushButton* m_addBtn;
    QPushButton* m_removeBtn;

    QComboBox* m_skeletonTypeCombo;
    QPushButton* m_createBtn;

    QGroupBox* m_propsGroup;
    QLineEdit* m_nameEdit;

    QDoubleSpinBox* m_posX, *m_posY, *m_posZ;
    QDoubleSpinBox* m_rotX, *m_rotY, *m_rotZ;

    QLabel* m_boneInfo;

    bool m_updatingUI;
};

}
