#include "TireRigDialog.h"
#include "../3DModeling_RigGenerator.h"
#include <QMessageBox>
#include <QInputDialog>

using namespace ks;

TireRigDialog::TireRigDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Tire Rig Generator");
    setMinimumSize(550, 450);
    setModal(true);
    setupUI();
    populatePresets();
}

TireRigDialog::~TireRigDialog() = default;

void TireRigDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QLabel* infoLabel = new QLabel(
        "Tire Rig Generator\n"
        "Creates a deformation rig for tire physics simulation.\n"
        "Objects are identified by name infixes - meshes must contain the specified text."
    );
    infoLabel->setWordWrap(true);
    mainLayout->addWidget(infoLabel);

    QGroupBox* presetGroup = new QGroupBox("Presets");
    QHBoxLayout* presetLayout = new QHBoxLayout(presetGroup);
    m_presetCombo = new QComboBox(this);
    m_savePresetBtn = new QPushButton("Save Preset", this);
    connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TireRigDialog::onPresetSelected);
    connect(m_savePresetBtn, &QPushButton::clicked, this, &TireRigDialog::onSavePreset);
    presetLayout->addWidget(m_presetCombo);
    presetLayout->addWidget(m_savePresetBtn);
    mainLayout->addWidget(presetGroup);

    QGroupBox* infixGroup = new QGroupBox("Mesh Name Infixes");
    QFormLayout* infixLayout = new QFormLayout(infixGroup);

    m_tireMeshInfixEdit = new QLineEdit("TIRE");
    m_deformerInfixEdit = new QLineEdit("DEFORMER");
    m_groundInfixEdit = new QLineEdit("GROUND");
    m_rigPrefixEdit = new QLineEdit("RIG");
    m_vertGroupEdit = new QLineEdit("DEFORM");

    infixLayout->addRow("Tire Mesh Infix:", m_tireMeshInfixEdit);
    infixLayout->addRow("Deformer Infix:", m_deformerInfixEdit);
    infixLayout->addRow("Ground Infix:", m_groundInfixEdit);
    infixLayout->addRow("Rig Prefix:", m_rigPrefixEdit);
    infixLayout->addRow("Vertex Group:", m_vertGroupEdit);

    mainLayout->addWidget(infixGroup);

    QGroupBox* paramGroup = new QGroupBox("Rig Parameters");
    QFormLayout* paramLayout = new QFormLayout(paramGroup);

    m_boneCountSpin = new QSpinBox();
    m_boneCountSpin->setRange(4, 64);
    m_boneCountSpin->setValue(12);
    m_boneCountSpin->setToolTip("Number of bones around the tire circumference");

    m_radiusSpin = new QDoubleSpinBox();
    m_radiusSpin->setRange(0.05, 2.0);
    m_radiusSpin->setValue(0.35);
    m_radiusSpin->setSuffix(" m");
    m_radiusSpin->setDecimals(3);
    m_radiusSpin->setToolTip("Tire radius");

    m_stiffnessSpin = new QDoubleSpinBox();
    m_stiffnessSpin->setRange(0.1, 10.0);
    m_stiffnessSpin->setValue(1.0);
    m_stiffnessSpin->setDecimals(2);
    m_stiffnessSpin->setToolTip("Tire deformation stiffness");

    m_verticalCheck = new QCheckBox("Vertical Deformation Mode");
    m_verticalCheck->setChecked(true);
    m_verticalCheck->setToolTip("Deformation acts vertically (realistic)");

    m_mirrorCheck = new QCheckBox("Mirror Rig (Left Side)");
    m_mirrorCheck->setChecked(true);

    paramLayout->addRow("Bone Count:", m_boneCountSpin);
    paramLayout->addRow("Tire Radius:", m_radiusSpin);
    paramLayout->addRow("Stiffness:", m_stiffnessSpin);
    paramLayout->addRow("", m_verticalCheck);
    paramLayout->addRow("", m_mirrorCheck);

    mainLayout->addWidget(paramGroup);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    
    m_defaultsBtn = new QPushButton("Load Defaults");
    m_generateBtn = new QPushButton("Generate Rig");
    QPushButton* cancelBtn = new QPushButton("Cancel");

    btnLayout->addWidget(m_defaultsBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(m_generateBtn);

    mainLayout->addLayout(btnLayout);

    connect(m_defaultsBtn, &QPushButton::clicked, this, &TireRigDialog::onLoadDefaults);
    connect(m_generateBtn, &QPushButton::clicked, this, &TireRigDialog::onGenerate);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void TireRigDialog::populatePresets()
{
    m_presetCombo->clear();
    m_presetCombo->addItem("-- Select Preset --", QJsonObject());
    m_presetCombo->addItem("Street (205/50R15)", QJsonObject({{"width", 205}, {"profile", 50}, {"diameter", 15}}));
    m_presetCombo->addItem("Sport (225/40R18)", QJsonObject({{"width", 225}, {"profile", 40}, {"diameter", 18}}));
    m_presetCombo->addItem("Track (265/35R19)", QJsonObject({{"width", 265}, {"profile", 35}, {"diameter", 19}}));
    m_presetCombo->addItem("Drag (275/30R18)", QJsonObject({{"width", 275}, {"profile", 30}, {"diameter", 18}}));
}

QString TireRigDialog::getPresetName() const
{
    int idx = m_presetCombo->currentIndex();
    if (idx > 0) {
        return m_presetCombo->currentText().split(" (").first();
    }
    return QString();
}

void TireRigDialog::onPresetSelected(int index)
{
    if (index <= 0) return;

    QJsonObject params = m_presetCombo->currentData().toJsonObject();
    if (!params.isEmpty()) {
        setParameters(params);
    }
}

void TireRigDialog::onLoadPreset()
{
    onPresetSelected(m_presetCombo->currentIndex());
}

void TireRigDialog::onSavePreset()
{
    QString name = QInputDialog::getText(this, "Save Preset", 
        "Enter preset name:", QLineEdit::Normal, getPresetName());
    
    if (name.isEmpty()) return;

    QString description = QInputDialog::getText(this, "Save Preset",
        "Enter description:", QLineEdit::Normal, "Custom preset");
    
    if (description.isEmpty()) description = "Custom preset";

    m_presets.insert(name, getParameters());
    
    populatePresets();
    QMessageBox::information(this, "Preset Saved", QString("Preset '%1' saved successfully").arg(name));
}

void TireRigDialog::onLoadDefaults()
{
    m_tireMeshInfixEdit->setText("TIRE");
    m_deformerInfixEdit->setText("DEFORMER");
    m_groundInfixEdit->setText("GROUND");
    m_rigPrefixEdit->setText("RIG");
    m_vertGroupEdit->setText("DEFORM");
    m_boneCountSpin->setValue(12);
    m_radiusSpin->setValue(0.35);
    m_stiffnessSpin->setValue(1.0);
    m_verticalCheck->setChecked(true);
    m_mirrorCheck->setChecked(true);
    m_presetCombo->setCurrentIndex(0);
}

void TireRigDialog::onGenerate()
{
    if (m_tireMeshInfixEdit->text().isEmpty() || m_deformerInfixEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Validation Error", 
            "Tire Mesh and Deformer infixes are required!");
        return;
    }

    QString fullName = m_rigPrefixEdit->text() + "_" + m_tireMeshInfixEdit->text();
    if (fullName.length() > 40) {
        QMessageBox::warning(this, "Validation Error",
            QString("Combined name length (%1) exceeds 40 characters!\n"
                    "This may cause issues with bone naming in Blender.").arg(fullName.length()));
        return;
    }

    accept();
}

QJsonObject TireRigDialog::getParameters() const
{
    QJsonObject params;
    params["tireMeshInfix"] = m_tireMeshInfixEdit->text();
    params["deformerInfix"] = m_deformerInfixEdit->text();
    params["groundInfix"] = m_groundInfixEdit->text();
    params["rigPrefix"] = m_rigPrefixEdit->text();
    params["vertexGroup"] = m_vertGroupEdit->text();
    params["boneCount"] = m_boneCountSpin->value();
    params["radius"] = m_radiusSpin->value();
    params["stiffness"] = m_stiffnessSpin->value();
    params["verticalMode"] = m_verticalCheck->isChecked();
    params["mirrorEnabled"] = m_mirrorCheck->isChecked();
    return params;
}

void TireRigDialog::setParameters(const QJsonObject& params)
{
    if (params.contains("tireMeshInfix")) m_tireMeshInfixEdit->setText(params["tireMeshInfix"].toString());
    if (params.contains("deformerInfix")) m_deformerInfixEdit->setText(params["deformerInfix"].toString());
    if (params.contains("groundInfix")) m_groundInfixEdit->setText(params["groundInfix"].toString());
    if (params.contains("rigPrefix")) m_rigPrefixEdit->setText(params["rigPrefix"].toString());
    if (params.contains("vertexGroup")) m_vertGroupEdit->setText(params["vertexGroup"].toString());
    if (params.contains("boneCount")) m_boneCountSpin->setValue(params["boneCount"].toInt());
    if (params.contains("radius")) m_radiusSpin->setValue(params["radius"].toDouble());
    if (params.contains("stiffness")) m_stiffnessSpin->setValue(params["stiffness"].toDouble());
    if (params.contains("verticalMode")) m_verticalCheck->setChecked(params["verticalMode"].toBool());
    if (params.contains("mirrorEnabled")) m_mirrorCheck->setChecked(params["mirrorEnabled"].toBool());
}