#pragma once

#include <QDialog>
#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QJsonObject>

#include "EngineRigDialog.h"

class TireRigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TireRigDialog(QWidget* parent = nullptr);
    ~TireRigDialog();

    QString getTireMeshInfix() const { return m_tireMeshInfixEdit->text(); }
    QString getDeformerInfix() const { return m_deformerInfixEdit->text(); }
    QString getGroundInfix() const { return m_groundInfixEdit->text(); }
    QString getTireRigPrefix() const { return m_rigPrefixEdit->text(); }
    QString getVertexGroupName() const { return m_vertGroupEdit->text(); }
    
    int getBoneCount() const { return m_boneCountSpin->value(); }
    double getRigRadius() const { return m_radiusSpin->value(); }
    double getTireStiffness() const { return m_stiffnessSpin->value(); }
    bool isVerticalMode() const { return m_verticalCheck->isChecked(); }
    bool isMirrorEnabled() const { return m_mirrorCheck->isChecked(); }

    QJsonObject getParameters() const;
    void setParameters(const QJsonObject& params);

public slots:
    void onPresetSelected(int index);
    void onLoadPreset();
    void onSavePreset();

private slots:
    void onGenerate();
    void onLoadDefaults();

private:
    void setupUI();
    void populatePresets();
    QString getPresetName() const;

    QLineEdit* m_tireMeshInfixEdit;
    QLineEdit* m_deformerInfixEdit;
    QLineEdit* m_groundInfixEdit;
    QLineEdit* m_rigPrefixEdit;
    QLineEdit* m_vertGroupEdit;

    QSpinBox* m_boneCountSpin;
    QDoubleSpinBox* m_radiusSpin;
    QDoubleSpinBox* m_stiffnessSpin;
    QCheckBox* m_verticalCheck;
    QCheckBox* m_mirrorCheck;

    QPushButton* m_generateBtn;
    QPushButton* m_defaultsBtn;
    QComboBox* m_presetCombo;
    QPushButton* m_savePresetBtn;

    QMap<QString, QJsonObject> m_presets;
};