#include "ShowroomPPEditorModule.h"
#include "../../core/sys/LogManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QTextStream>

namespace ks {

ShowroomPPEditorModule::ShowroomPPEditorModule(QWidget* parent) : EditorModule(parent) {}
bool ShowroomPPEditorModule::initialize() { LOG_INFO("ShowroomPPEditorModule", "Initialized"); return true; }
void ShowroomPPEditorModule::shutdown() { if (m_statusLabel) m_statusLabel->setText("Shut down"); }

QDockWidget* ShowroomPPEditorModule::getOrCreateDockWidget(QMainWindow* mainWindow)
{
    if (m_dockWidget) return m_dockWidget;
    m_dockWidget = new QDockWidget("Showroom PP Editor", mainWindow);
    m_dockWidget->setObjectName("ShowroomPPEditorDock");

    auto* centralWidget = new QWidget();
    auto* mainLayout = new QVBoxLayout(centralWidget);

    m_tabWidget = new QTabWidget();

    // Auto Exposure tab
    auto* aeWidget = new QWidget(); auto* aeLayout = new QGridLayout(aeWidget);
    m_autoExposureCheck = new QCheckBox("Enabled"); aeLayout->addWidget(m_autoExposureCheck, 0, 0, 1, 2);
    m_aeDelaySpin = new QDoubleSpinBox(); m_aeDelaySpin->setRange(0, 10); aeLayout->addWidget(new QLabel("Delay:"), 1, 0); aeLayout->addWidget(m_aeDelaySpin, 1, 1);
    m_aeTargetSpin = new QDoubleSpinBox(); m_aeTargetSpin->setRange(0, 1); aeLayout->addWidget(new QLabel("Target:"), 2, 0); aeLayout->addWidget(m_aeTargetSpin, 2, 1);
    m_aeMinSpin = new QDoubleSpinBox(); m_aeMinSpin->setRange(0, 1); aeLayout->addWidget(new QLabel("Min:"), 3, 0); aeLayout->addWidget(m_aeMinSpin, 3, 1);
    m_aeMaxSpin = new QDoubleSpinBox(); m_aeMaxSpin->setRange(0, 10); aeLayout->addWidget(new QLabel("Max:"), 4, 0); aeLayout->addWidget(m_aeMaxSpin, 4, 1);
    m_tabWidget->addTab(aeWidget, "Auto Exposure");

    // Tone Mapping tab
    auto* tmWidget = new QWidget(); auto* tmLayout = new QGridLayout(tmWidget);
    m_exposureSpin = new QDoubleSpinBox(); m_exposureSpin->setRange(0, 5); tmLayout->addWidget(new QLabel("Exposure:"), 0, 0); tmLayout->addWidget(m_exposureSpin, 0, 1);
    m_gammaSpin = new QDoubleSpinBox(); m_gammaSpin->setRange(0.1, 5); tmLayout->addWidget(new QLabel("Gamma:"), 1, 0); tmLayout->addWidget(m_gammaSpin, 1, 1);
    m_tabWidget->addTab(tmWidget, "Tone Mapping");

    // DOF tab
    auto* dofWidget = new QWidget(); auto* dofLayout = new QGridLayout(dofWidget);
    m_dofCheck = new QCheckBox("Enabled"); dofLayout->addWidget(m_dofCheck, 0, 0, 1, 2);
    m_dofApertureSpin = new QDoubleSpinBox(); m_dofApertureSpin->setRange(0.1, 100); dofLayout->addWidget(new QLabel("Aperture:"), 1, 0); dofLayout->addWidget(m_dofApertureSpin, 1, 1);
    m_tabWidget->addTab(dofWidget, "DOF");

    // Glare tab
    auto* glWidget = new QWidget(); auto* glLayout = new QGridLayout(glWidget);
    m_glareCheck = new QCheckBox("Enabled"); glLayout->addWidget(m_glareCheck, 0, 0, 1, 2);
    m_glareLuminanceSpin = new QDoubleSpinBox(); m_glareLuminanceSpin->setRange(0, 100); glLayout->addWidget(new QLabel("Luminance:"), 1, 0); glLayout->addWidget(m_glareLuminanceSpin, 1, 1);
    m_glareThresholdSpin = new QDoubleSpinBox(); m_glareThresholdSpin->setRange(0, 100); glLayout->addWidget(new QLabel("Threshold:"), 2, 0); glLayout->addWidget(m_glareThresholdSpin, 2, 1);
    m_tabWidget->addTab(glWidget, "Glare");

    // God Rays tab
    auto* grWidget = new QWidget(); auto* grLayout = new QGridLayout(grWidget);
    m_godRaysCheck = new QCheckBox("Enabled"); grLayout->addWidget(m_godRaysCheck, 0, 0, 1, 2);
    m_godRaysLengthSpin = new QDoubleSpinBox(); m_godRaysLengthSpin->setRange(0, 100); grLayout->addWidget(new QLabel("Length:"), 1, 0); grLayout->addWidget(m_godRaysLengthSpin, 1, 1);
    m_tabWidget->addTab(grWidget, "God Rays");

    // Color tab
    auto* cWidget = new QWidget(); auto* cLayout = new QGridLayout(cWidget);
    m_saturationSpin = new QDoubleSpinBox(); m_saturationSpin->setRange(0, 5); cLayout->addWidget(new QLabel("Saturation:"), 0, 0); cLayout->addWidget(m_saturationSpin, 0, 1);
    m_brightnessSpin = new QDoubleSpinBox(); m_brightnessSpin->setRange(0, 5); cLayout->addWidget(new QLabel("Brightness:"), 1, 0); cLayout->addWidget(m_brightnessSpin, 1, 1);
    m_contrastSpin = new QDoubleSpinBox(); m_contrastSpin->setRange(0, 5); cLayout->addWidget(new QLabel("Contrast:"), 2, 0); cLayout->addWidget(m_contrastSpin, 2, 1);
    m_colorTempSpin = new QSpinBox(); m_colorTempSpin->setRange(1000, 20000); cLayout->addWidget(new QLabel("Color Temp:"), 3, 0); cLayout->addWidget(m_colorTempSpin, 3, 1);
    m_tabWidget->addTab(cWidget, "Color");

    mainLayout->addWidget(m_tabWidget);

    auto* actionLayout = new QHBoxLayout();
    m_loadBtn = new QPushButton("Load ppeffects.ini"); m_saveBtn = new QPushButton("Save ppeffects.ini"); m_resetBtn = new QPushButton("Reset");
    actionLayout->addWidget(m_loadBtn); actionLayout->addWidget(m_saveBtn); actionLayout->addWidget(m_resetBtn);
    mainLayout->addLayout(actionLayout);

    m_statusLabel = new QLabel("Ready"); mainLayout->addWidget(m_statusLabel);

    connect(m_loadBtn, &QPushButton::clicked, this, &ShowroomPPEditorModule::onLoadFile);
    connect(m_saveBtn, &QPushButton::clicked, this, &ShowroomPPEditorModule::onSaveFile);
    connect(m_resetBtn, &QPushButton::clicked, this, &ShowroomPPEditorModule::onResetDefaults);

    // Connect UI widgets to handlers for live state updates
    connect(m_autoExposureCheck, &QCheckBox::toggled, this, &ShowroomPPEditorModule::onAutoExposureToggled);
    connect(m_aeDelaySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ShowroomPPEditorModule::onAEDelayChanged);
    connect(m_aeTargetSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ShowroomPPEditorModule::onAETargetChanged);
    connect(m_aeMinSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ShowroomPPEditorModule::onAEMinChanged);
    connect(m_aeMaxSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ShowroomPPEditorModule::onAEMaxChanged);
    connect(m_exposureSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ShowroomPPEditorModule::onExposureChanged);
    connect(m_gammaSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ShowroomPPEditorModule::onGammaChanged);
    connect(m_dofCheck, &QCheckBox::toggled, this, &ShowroomPPEditorModule::onDOFToggled);
    connect(m_dofApertureSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ShowroomPPEditorModule::onDOFApertureChanged);
    connect(m_glareCheck, &QCheckBox::toggled, this, &ShowroomPPEditorModule::onGlareToggled);
    connect(m_glareLuminanceSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ShowroomPPEditorModule::onGlareLuminanceChanged);
    connect(m_glareThresholdSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ShowroomPPEditorModule::onGlareThresholdChanged);
    connect(m_godRaysCheck, &QCheckBox::toggled, this, &ShowroomPPEditorModule::onGodRaysToggled);
    connect(m_godRaysLengthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ShowroomPPEditorModule::onGodRaysLengthChanged);
    connect(m_saturationSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ShowroomPPEditorModule::onSaturationChanged);
    connect(m_brightnessSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ShowroomPPEditorModule::onBrightnessChanged);
    connect(m_contrastSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ShowroomPPEditorModule::onContrastChanged);
    connect(m_colorTempSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ShowroomPPEditorModule::onColorTempChanged);

    m_dockWidget->setWidget(centralWidget);
    return m_dockWidget;
}

void ShowroomPPEditorModule::importFile(const QString& f) { m_filePath = f; loadFileToUI(); }
void ShowroomPPEditorModule::exportFile(const QString& f) { m_filePath = f; saveFileFromUI(); }
void ShowroomPPEditorModule::onActivation()
{
    if (m_dockWidget) {
        m_dockWidget->show();
        m_dockWidget->raise();
    }
}

void ShowroomPPEditorModule::onDeactivation()
{
    if (m_dockWidget) m_dockWidget->hide();
}
void ShowroomPPEditorModule::onLoadFile() { QString p = QFileDialog::getOpenFileName(this, "Open ppeffects.ini", QString(), "PP INI (*.ini)"); if (!p.isEmpty()) { m_filePath = p; loadFileToUI(); } }
void ShowroomPPEditorModule::onSaveFile() { QString p = m_filePath.isEmpty() ? QFileDialog::getSaveFileName(this, "Save ppeffects.ini", QString(), "PP INI (*.ini)") : m_filePath; if (!p.isEmpty()) { m_filePath = p; saveFileFromUI(); } }
void ShowroomPPEditorModule::onResetDefaults() { m_autoExposureCheck->setChecked(true); m_aeDelaySpin->setValue(0.0); m_aeTargetSpin->setValue(0.32); m_aeMinSpin->setValue(0.2); m_aeMaxSpin->setValue(0.5); m_exposureSpin->setValue(0.28); m_gammaSpin->setValue(1.2); m_dofCheck->setChecked(true); m_dofApertureSpin->setValue(12.0); m_glareCheck->setChecked(true); m_glareLuminanceSpin->setValue(1.6); m_glareThresholdSpin->setValue(5.0); m_godRaysCheck->setChecked(true); m_godRaysLengthSpin->setValue(11.0); m_saturationSpin->setValue(0.95); m_brightnessSpin->setValue(1.0); m_contrastSpin->setValue(1.0); m_colorTempSpin->setValue(6400); }
void ShowroomPPEditorModule::onAutoExposureToggled(bool) { m_statusLabel->setText("Modified (Auto Exposure)"); }
void ShowroomPPEditorModule::onAEDelayChanged(double) { m_statusLabel->setText("Modified (AE Delay)"); }
void ShowroomPPEditorModule::onAETargetChanged(double) { m_statusLabel->setText("Modified (AE Target)"); }
void ShowroomPPEditorModule::onAEMinChanged(double) { m_statusLabel->setText("Modified (AE Min)"); }
void ShowroomPPEditorModule::onAEMaxChanged(double) { m_statusLabel->setText("Modified (AE Max)"); }
void ShowroomPPEditorModule::onExposureChanged(double) { m_statusLabel->setText("Modified (Exposure)"); }
void ShowroomPPEditorModule::onGammaChanged(double) { m_statusLabel->setText("Modified (Gamma)"); }
void ShowroomPPEditorModule::onDOFToggled(bool) { m_statusLabel->setText("Modified (DOF)"); }
void ShowroomPPEditorModule::onDOFApertureChanged(double) { m_statusLabel->setText("Modified (DOF Aperture)"); }
void ShowroomPPEditorModule::onGlareToggled(bool) { m_statusLabel->setText("Modified (Glare)"); }
void ShowroomPPEditorModule::onGlareLuminanceChanged(double) { m_statusLabel->setText("Modified (Glare Luminance)"); }
void ShowroomPPEditorModule::onGlareThresholdChanged(double) { m_statusLabel->setText("Modified (Glare Threshold)"); }
void ShowroomPPEditorModule::onGodRaysToggled(bool) { m_statusLabel->setText("Modified (God Rays)"); }
void ShowroomPPEditorModule::onGodRaysLengthChanged(double) { m_statusLabel->setText("Modified (God Rays Length)"); }
void ShowroomPPEditorModule::onSaturationChanged(double) { m_statusLabel->setText("Modified (Saturation)"); }
void ShowroomPPEditorModule::onBrightnessChanged(double) { m_statusLabel->setText("Modified (Brightness)"); }
void ShowroomPPEditorModule::onContrastChanged(double) { m_statusLabel->setText("Modified (Contrast)"); }
void ShowroomPPEditorModule::onColorTempChanged(int) { m_statusLabel->setText("Modified (Color Temp)"); }
void ShowroomPPEditorModule::setupUi() { if (m_statusLabel) m_statusLabel->setText("UI Ready"); }

void ShowroomPPEditorModule::loadFileToUI()
{
    QFile file(m_filePath); if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QString c = file.readAll(); file.close();
    auto val = [&](const QString& sec, const QString& key, double def) -> double {
        int si = c.indexOf("[" + sec); if (si < 0) return def;
        int ei = c.indexOf("[", si + 1); if (ei < 0) ei = c.length();
        QString s = c.mid(si, ei - si);
        for (const QString& l : s.split("\n")) { QString t = l.trimmed(); if (t.startsWith(key + "=")) return t.mid(key.length() + 1).toDouble(); }
        return def;
    };
    m_autoExposureCheck->setChecked(val("AUTO_EXPOSURE", "ENABLED", 1) > 0.5);
    m_aeDelaySpin->setValue(val("AUTO_EXPOSURE", "DELAY", 0.0));
    m_aeTargetSpin->setValue(val("AUTO_EXPOSURE", "TARGET", 0.32));
    m_aeMinSpin->setValue(val("AUTO_EXPOSURE", "MIN_VALUE", 0.2));
    m_aeMaxSpin->setValue(val("AUTO_EXPOSURE", "MAX_VALUE", 0.5));
    m_exposureSpin->setValue(val("TONEMAPPING", "EXPOSURE", 0.28));
    m_gammaSpin->setValue(val("TONEMAPPING", "GAMMA", 1.2));
    m_dofCheck->setChecked(val("DOF", "ENABLED", 1) > 0.5);
    m_dofApertureSpin->setValue(val("DOF", "APERTURE_FNUMBER", 12.0));
    m_glareCheck->setChecked(val("GLARE", "ENABLED", 1) > 0.5);
    m_glareLuminanceSpin->setValue(val("GLARE", "LUMINANCE", 1.6));
    m_glareThresholdSpin->setValue(val("GLARE", "THRESHOLD", 5.0));
    m_godRaysCheck->setChecked(val("GODRAYS", "ENABLED", 1) > 0.5);
    m_godRaysLengthSpin->setValue(val("GODRAYS", "LENGTH", 11.0));
    m_saturationSpin->setValue(val("COLOR", "SATURATION", 0.95));
    m_brightnessSpin->setValue(val("COLOR", "BRIGHTNESS", 1.0));
    m_contrastSpin->setValue(val("COLOR", "CONTRAST", 1.0));
    m_colorTempSpin->setValue((int)val("COLOR", "COLOR_TEMP", 6400));
}

void ShowroomPPEditorModule::saveFileFromUI()
{
    QFile file(m_filePath); if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream o(&file);
    o << "[AUTO_EXPOSURE]\nENABLED=" << (m_autoExposureCheck->isChecked() ? 1 : 0) << "\nDELAY=" << m_aeDelaySpin->value() << "\nTARGET=" << m_aeTargetSpin->value() << "\nMIN_VALUE=" << m_aeMinSpin->value() << "\nMAX_VALUE=" << m_aeMaxSpin->value() << "\n\n";
    o << "[TONEMAPPING]\nEXPOSURE=" << m_exposureSpin->value() << "\nGAMMA=" << m_gammaSpin->value() << "\n\n";
    o << "[DOF]\nENABLED=" << (m_dofCheck->isChecked() ? 1 : 0) << "\nAPERTURE_FNUMBER=" << m_dofApertureSpin->value() << "\n\n";
    o << "[GLARE]\nENABLED=" << (m_glareCheck->isChecked() ? 1 : 0) << "\nLUMINANCE=" << m_glareLuminanceSpin->value() << "\nTHRESHOLD=" << m_glareThresholdSpin->value() << "\n\n";
    o << "[GODRAYS]\nENABLED=" << (m_godRaysCheck->isChecked() ? 1 : 0) << "\nLENGTH=" << m_godRaysLengthSpin->value() << "\n\n";
    o << "[COLOR]\nSATURATION=" << m_saturationSpin->value() << "\nBRIGHTNESS=" << m_brightnessSpin->value() << "\nCONTRAST=" << m_contrastSpin->value() << "\nCOLOR_TEMP=" << m_colorTempSpin->value() << "\n";
    file.close();
}

QJsonObject ShowroomPPEditorModule::serializeProject() const
{
    QJsonObject data;
    data["filePath"] = m_filePath;
    QJsonObject ae;
    ae["enabled"] = m_autoExposureCheck ? m_autoExposureCheck->isChecked() : false;
    ae["delay"] = m_aeDelaySpin ? m_aeDelaySpin->value() : 0.0;
    ae["target"] = m_aeTargetSpin ? m_aeTargetSpin->value() : 0.0;
    ae["min"] = m_aeMinSpin ? m_aeMinSpin->value() : 0.0;
    ae["max"] = m_aeMaxSpin ? m_aeMaxSpin->value() : 0.0;
    data["autoExposure"] = ae;
    data["exposure"] = m_exposureSpin ? m_exposureSpin->value() : 0.0;
    data["gamma"] = m_gammaSpin ? m_gammaSpin->value() : 0.0;
    QJsonObject dof;
    dof["enabled"] = m_dofCheck ? m_dofCheck->isChecked() : false;
    dof["aperture"] = m_dofApertureSpin ? m_dofApertureSpin->value() : 0.0;
    data["dof"] = dof;
    QJsonObject glare;
    glare["enabled"] = m_glareCheck ? m_glareCheck->isChecked() : false;
    glare["luminance"] = m_glareLuminanceSpin ? m_glareLuminanceSpin->value() : 0.0;
    glare["threshold"] = m_glareThresholdSpin ? m_glareThresholdSpin->value() : 0.0;
    data["glare"] = glare;
    QJsonObject godRays;
    godRays["enabled"] = m_godRaysCheck ? m_godRaysCheck->isChecked() : false;
    godRays["length"] = m_godRaysLengthSpin ? m_godRaysLengthSpin->value() : 0.0;
    data["godRays"] = godRays;
    data["saturation"] = m_saturationSpin ? m_saturationSpin->value() : 0.0;
    data["brightness"] = m_brightnessSpin ? m_brightnessSpin->value() : 0.0;
    data["contrast"] = m_contrastSpin ? m_contrastSpin->value() : 0.0;
    data["colorTemp"] = m_colorTempSpin ? m_colorTempSpin->value() : 0.0;
    return data;
}

void ShowroomPPEditorModule::deserializeProject(const QJsonObject& data)
{
    m_filePath = data["filePath"].toString();
    QJsonObject ae = data["autoExposure"].toObject();
    if (m_autoExposureCheck) m_autoExposureCheck->setChecked(ae["enabled"].toBool());
    if (m_aeDelaySpin) m_aeDelaySpin->setValue(ae["delay"].toDouble());
    if (m_aeTargetSpin) m_aeTargetSpin->setValue(ae["target"].toDouble());
    if (m_aeMinSpin) m_aeMinSpin->setValue(ae["min"].toDouble());
    if (m_aeMaxSpin) m_aeMaxSpin->setValue(ae["max"].toDouble());
    if (m_exposureSpin) m_exposureSpin->setValue(data["exposure"].toDouble());
    if (m_gammaSpin) m_gammaSpin->setValue(data["gamma"].toDouble());
    QJsonObject dof = data["dof"].toObject();
    if (m_dofCheck) m_dofCheck->setChecked(dof["enabled"].toBool());
    if (m_dofApertureSpin) m_dofApertureSpin->setValue(dof["aperture"].toDouble());
    QJsonObject glare = data["glare"].toObject();
    if (m_glareCheck) m_glareCheck->setChecked(glare["enabled"].toBool());
    if (m_glareLuminanceSpin) m_glareLuminanceSpin->setValue(glare["luminance"].toDouble());
    if (m_glareThresholdSpin) m_glareThresholdSpin->setValue(glare["threshold"].toDouble());
    QJsonObject godRays = data["godRays"].toObject();
    if (m_godRaysCheck) m_godRaysCheck->setChecked(godRays["enabled"].toBool());
    if (m_godRaysLengthSpin) m_godRaysLengthSpin->setValue(godRays["length"].toDouble());
    if (m_saturationSpin) m_saturationSpin->setValue(data["saturation"].toDouble());
    if (m_brightnessSpin) m_brightnessSpin->setValue(data["brightness"].toDouble());
    if (m_contrastSpin) m_contrastSpin->setValue(data["contrast"].toDouble());
    if (m_colorTempSpin) m_colorTempSpin->setValue(data["colorTemp"].toDouble());
}

} // namespace ks
