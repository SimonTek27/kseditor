#pragma once

#include <QDialog>
#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QLabel>
#include <QJsonObject>

#include "../3DModeling_RigGenerator.h"
#include "../3DModeling_io.h"

namespace ks {

class EngineRigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EngineRigDialog(QWidget* parent = nullptr, bool isTurbo = false);
    ~EngineRigDialog();

    bool isTurboMode() const { return m_isTurbo; }
    
    QString getEngineMeshInfix() const { return m_engineMeshInfixEdit->text(); }
    QString getCrankshaftInfix() const { return m_crankshaftInfixEdit->text(); }
    int getCylinderCount() const { return m_cylinderCountSpin->value(); }
    QString getRigPrefix() const { return m_rigPrefixEdit->text(); }

    double getRodLength() const { return m_rodLengthSpin->value(); }
    double getCrankRadius() const { return m_crankRadiusSpin->value(); }
    double getStroke() const { return m_strokeSpin->value(); }

    double getCompressorRadius() const { return m_compressorSpin->value(); }
    double getTurbineRadius() const { return m_turbineSpin->value(); }
    double getBoostPressure() const { return m_boostSpin->value(); }

    QJsonObject getParameters() const;
    void setParameters(const QJsonObject& params);

public slots:
    void onPresetSelected(int index);
    void onSavePreset();

signals:
    void generateRig(const QString& params);
    void turboToggled(bool enabled);

private slots:
    void onGenerate();
    void onToggleTurbo(bool enabled);

private:
    void setupUI();
    void populatePresets();
    void updateFiringAngle();

    bool m_isTurbo;

    QLineEdit* m_engineMeshInfixEdit;
    QLineEdit* m_crankshaftInfixEdit;
    QLineEdit* m_rigPrefixEdit;

    QSpinBox* m_cylinderCountSpin;
    QComboBox* m_configCombo;
    QDoubleSpinBox* m_displacementSpin;

    QDoubleSpinBox* m_rodLengthSpin;
    QDoubleSpinBox* m_crankRadiusSpin;
    QDoubleSpinBox* m_strokeSpin;

    QGroupBox* m_turboGroup;
    QLabel* m_turboLabel;
    QDoubleSpinBox* m_compressorSpin;
    QDoubleSpinBox* m_turbineSpin;
    QDoubleSpinBox* m_boostSpin;
    QDoubleSpinBox* m_turboBoostSpin;

    QComboBox* m_presetCombo;
    QPushButton* m_savePresetBtn;

    QMap<QString, QJsonObject> m_presets;
};

} // namespace ks