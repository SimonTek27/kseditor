#pragma once

#include <QWidget>
#include <QListWidget>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QScrollArea>
#include "CarEditor.h"

namespace ks {

class CarEditorWidget : public QWidget {
    Q_OBJECT
public:
    explicit CarEditorWidget(QWidget* parent = nullptr);

    void loadCar(const QString& path);
    void saveCar(const QString& path);

signals:
    void partSelected(const QString& partId);
    void carModified();

private slots:
    void onPartSelected(QListWidgetItem* current);
    void onAddPart();
    void onRemovePart();
    void onNameChanged(const QString& text);
    void onPositionChanged();
    void onRotationChanged();
    void onScaleChanged();
    void onMeshFileChanged(const QString& text);
    void onParentChanged(const QString& text);
    void onVisibilityChanged(int state);
    void updatePartUI();
    void clearPartUI();

private:
    void setupUI();
    void refreshPartList();

    CarEditor* m_editor;
    QListWidget* m_partList;
    QPushButton* m_addBtn;
    QPushButton* m_removeBtn;

    QGroupBox* m_propsGroup;
    QLineEdit* m_nameEdit;
    QLineEdit* m_meshFileEdit;
    QComboBox* m_parentCombo;

    QDoubleSpinBox* m_posX, *m_posY, *m_posZ;
    QDoubleSpinBox* m_rotX, *m_rotY, *m_rotZ;
    QDoubleSpinBox* m_scaleX, *m_scaleY, *m_scaleZ;

    QPushButton* m_visibilityBtn;

    bool m_updatingUI;
};

}
