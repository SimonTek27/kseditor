#include "EngineRigDialog.h"
#include <QMessageBox>
#include <QInputDialog>

namespace ks {

EngineRigDialog::EngineRigDialog(QWidget* parent, bool isTurbo)
    : QDialog(parent)
    , m_isTurbo(isTurbo)
{
    setWindowTitle(isTurbo ? "Turbo Engine Rig Generator" : "Engine Rig Generator");
    setMinimumSize(550, 550);
    setModal(true);
    setupUI();
    populatePresets();
}

EngineRigDialog::~EngineRigDialog() = default;

void EngineRigDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QLabel* infoLabel = new QLabel(
        m_isTurbo 
            ? "Turbo Engine Rig Generator\nCreates a complete engine rig with turbocharger simulation."
            : "Engine Rig Generator\nCreates an engine rigging system with proper crankshaft motion."
    );
    infoLabel->setWordWrap(true);
    mainLayout->addWidget(infoLabel);

    QGroupBox* presetGroup = new QGroupBox("Presets");
    QHBoxLayout* presetLayout = new QHBoxLayout(presetGroup);
    m_presetCombo = new QComboBox(this);
    m_savePresetBtn = new QPushButton("Save Preset", this);
    connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EngineRigDialog::onPresetSelected);
    connect(m_savePresetBtn, &QPushButton::clicked, this, &EngineRigDialog::onSavePreset);
    presetLayout->addWidget(m_presetCombo);
    presetLayout->addWidget(m_savePresetBtn);
    mainLayout->addWidget(presetGroup);

    QGroupBox* meshGroup = new QGroupBox("Mesh Identification");
    QFormLayout* meshLayout = new QFormLayout(meshGroup);

    m_engineMeshInfixEdit = new QLineEdit("ENGINE");
    m_crankshaftInfixEdit = new QLineEdit("CRANK");
    m_rigPrefixEdit = new QLineEdit("ENG");

    meshLayout->addRow("Engine Mesh Infix:", m_engineMeshInfixEdit);
    meshLayout->addRow("Crankshaft Infix:", m_crankshaftInfixEdit);
    meshLayout->addRow("Rig Prefix:", m_rigPrefixEdit);

    mainLayout->addWidget(meshGroup);

    QGroupBox* engGroup = new QGroupBox("Engine Parameters");
    QFormLayout* engLayout = new QFormLayout(engGroup);

    m_cylinderCountSpin = new QSpinBox();
    m_cylinderCountSpin->setRange(2, 16);
    m_cylinderCountSpin->setValue(4);

    m_configCombo = new QComboBox(this);
    m_configCombo->addItems({"Inline", "V6", "V8", "V10", "V12", "Flat-6", "Flat-4"});
    connect(m_configCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { updateFiringAngle(); });

    m_displacementSpin = new QDoubleSpinBox();
    m_displacementSpin->setRange(0.5, 10.0);
    m_displacementSpin->setValue(m_isTurbo ? 3.5 : 2.0);
    m_displacementSpin->setSuffix(" L");
    m_displacementSpin->setDecimals(1);

    engLayout->addRow("Cylinder Count:", m_cylinderCountSpin);
    engLayout->addRow("Configuration:", m_configCombo);
    engLayout->addRow("Displacement:", m_displacementSpin);

    mainLayout->addWidget(engGroup);

    QGroupBox* crankGroup = new QGroupBox("Crankshaft Parameters");
    QFormLayout* crankLayout = new QFormLayout(crankGroup);

    m_rodLengthSpin = new QDoubleSpinBox();
    m_rodLengthSpin->setRange(50, 400);
    m_rodLengthSpin->setValue(150);
    m_rodLengthSpin->setSuffix(" mm");
    m_rodLengthSpin->setDecimals(1);

    m_crankRadiusSpin = new QDoubleSpinBox();
    m_crankRadiusSpin->setRange(10, 150);
    m_crankRadiusSpin->setValue(40);
    m_crankRadiusSpin->setSuffix(" mm");
    m_crankRadiusSpin->setDecimals(1);

    m_strokeSpin = new QDoubleSpinBox();
    m_strokeSpin->setRange(50, 200);
    m_strokeSpin->setValue(80);
    m_strokeSpin->setSuffix(" mm");
    m_strokeSpin->setDecimals(1);

    crankLayout->addRow("Rod Length:", m_rodLengthSpin);
    crankLayout->addRow("Crank Radius:", m_crankRadiusSpin);
    crankLayout->addRow("Stroke:", m_strokeSpin);

    mainLayout->addWidget(crankGroup);

    m_turboGroup = new QGroupBox("Turbocharger Parameters");
    QFormLayout* turboLayout = new QFormLayout(m_turboGroup);

    m_compressorSpin = new QDoubleSpinBox();
    m_compressorSpin->setRange(20, 120);
    m_compressorSpin->setValue(50);
    m_compressorSpin->setSuffix(" mm");
    m_compressorSpin->setDecimals(1);

    m_turbineSpin = new QDoubleSpinBox();
    m_turbineSpin->setRange(30, 150);
    m_turbineSpin->setValue(60);
    m_turbineSpin->setSuffix(" mm");
    m_turbineSpin->setDecimals(1);

    m_boostSpin = new QDoubleSpinBox();
    m_boostSpin->setRange(0.5, 3.0);
    m_boostSpin->setValue(1.5);
    m_boostSpin->setSuffix(" bar");
    m_boostSpin->setDecimals(2);

    turboLayout->addRow("Compressor Wheel:", m_compressorSpin);
    turboLayout->addRow("Turbine Wheel:", m_turbineSpin);
    turboLayout->addRow("Max Boost:", m_boostSpin);

    m_turboGroup->setVisible(m_isTurbo);
    mainLayout->addWidget(m_turboGroup);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    
    QPushButton* cancelBtn = new QPushButton("Cancel");
    QPushButton* generateBtn = new QPushButton("Generate Rig");
    QPushButton* toggleTurboBtn = new QPushButton(m_isTurbo ? "Switch to Normal" : "Add Turbo");

    btnLayout->addWidget(toggleTurboBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(generateBtn);

    mainLayout->addLayout(btnLayout);

    connect(generateBtn, &QPushButton::clicked, this, &EngineRigDialog::onGenerate);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(toggleTurboBtn, &QPushButton::clicked, this, &EngineRigDialog::onToggleTurbo);
}

void EngineRigDialog::updateFiringAngle()
{
    int cylinders = m_cylinderCountSpin->value();
    QString config = m_configCombo->currentText();
    
    float firingAngle = 720.0f / cylinders;
    qDebug() << "Firing angle for" << config << cylinders << "cylinders:" << firingAngle << "deg";
}

void EngineRigDialog::populatePresets()
{
    m_presetCombo->clear();
    m_presetCombo->addItem("-- Select Preset --", QJsonObject());
    m_presetCombo->addItem("Default Inline-4", QJsonObject({{"config", "Inline"}}));
    m_presetCombo->addItem("Sport V6", QJsonObject({{"config", "V6"}}));
    m_presetCombo->addItem("Race V8", QJsonObject({{"config", "V8"}}));
    m_presetCombo->addItem("F1 V10", QJsonObject({{"config", "V10"}}));
    m_presetCombo->addItem("Super V12", QJsonObject({{"config", "V12"}}));
}

void EngineRigDialog::onPresetSelected(int index)
{
    if (index <= 0) return;

    QJsonObject params = m_presetCombo->currentData().toJsonObject();
    if (!params.isEmpty()) {
        setParameters(params);
        if (params.contains("isTurbo") && params["isTurbo"].toBool() != m_isTurbo) {
            m_isTurbo = params["isTurbo"].toBool();
            m_turboGroup->setVisible(m_isTurbo);
        }
    }
}

void EngineRigDialog::onSavePreset()
{
    QString name = QInputDialog::getText(this, "Save Preset", 
        "Enter preset name:", QLineEdit::Normal, m_presetCombo->currentText().split(" (").first());
    
    if (name.isEmpty()) return;

    QString description = QInputDialog::getText(this, "Save Preset",
        "Enter description:", QLineEdit::Normal, "Custom preset");
    
    if (description.isEmpty()) description = "Custom preset";

    m_presets.insert(name, getParameters());
    
    populatePresets();
    QMessageBox::information(this, "Preset Saved", QString("Preset '%1' saved successfully").arg(name));
}

void EngineRigDialog::onToggleTurbo(bool enabled)
{
    m_isTurbo = enabled;
    if (m_turboLabel) {
        m_turboLabel->setText(enabled ? "Turbo: Enabled" : "Turbo: Disabled");
    }
    if (m_turboBoostSpin) {
        m_turboBoostSpin->setEnabled(enabled);
    }
    emit turboToggled(enabled);
}

void EngineRigDialog::onGenerate()
{
    if (m_engineMeshInfixEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Engine mesh infix is required!");
        return;
    }

    if (m_cylinderCountSpin->value() < 2) {
        QMessageBox::warning(this, "Validation Error", "At least 2 cylinders required!");
        return;
    }

    accept();
}

QJsonObject EngineRigDialog::getParameters() const
{
    QJsonObject params;
    params["engineMeshInfix"] = m_engineMeshInfixEdit->text();
    params["crankshaftInfix"] = m_crankshaftInfixEdit->text();
    params["rigPrefix"] = m_rigPrefixEdit->text();
    params["cylinderCount"] = m_cylinderCountSpin->value();
    params["config"] = m_configCombo->currentText();
    params["displacement"] = m_displacementSpin->value();
    params["rodLength"] = m_rodLengthSpin->value();
    params["crankRadius"] = m_crankRadiusSpin->value();
    params["stroke"] = m_strokeSpin->value();
    params["isTurbo"] = m_isTurbo;
    if (m_isTurbo) {
        params["compressorRadius"] = m_compressorSpin->value();
        params["turbineRadius"] = m_turbineSpin->value();
        params["boost"] = m_boostSpin->value();
    }
    return params;
}

void EngineRigDialog::setParameters(const QJsonObject& params)
{
    if (params.contains("engineMeshInfix")) m_engineMeshInfixEdit->setText(params["engineMeshInfix"].toString());
    if (params.contains("crankshaftInfix")) m_crankshaftInfixEdit->setText(params["crankshaftInfix"].toString());
    if (params.contains("rigPrefix")) m_rigPrefixEdit->setText(params["rigPrefix"].toString());
    if (params.contains("cylinderCount")) m_cylinderCountSpin->setValue(params["cylinderCount"].toInt());
    if (params.contains("config")) {
        int idx = m_configCombo->findText(params["config"].toString());
        if (idx >= 0) m_configCombo->setCurrentIndex(idx);
    }
    if (params.contains("displacement")) m_displacementSpin->setValue(params["displacement"].toDouble());
    if (params.contains("rodLength")) m_rodLengthSpin->setValue(params["rodLength"].toDouble());
    if (params.contains("crankRadius")) m_crankRadiusSpin->setValue(params["crankRadius"].toDouble());
    if (params.contains("stroke")) m_strokeSpin->setValue(params["stroke"].toDouble());
    if (params.contains("isTurbo")) {
        m_isTurbo = params["isTurbo"].toBool();
        m_turboGroup->setVisible(m_isTurbo);
    }
    if (params.contains("compressorRadius")) m_compressorSpin->setValue(params["compressorRadius"].toDouble());
    if (params.contains("turbineRadius")) m_turbineSpin->setValue(params["turbineRadius"].toDouble());
    if (params.contains("boost")) m_boostSpin->setValue(params["boost"].toDouble());
}

} // namespace ks