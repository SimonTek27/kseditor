#include "TireCurveEditor.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QTabWidget>
#include <QScrollArea>
#include <cmath>
#include <algorithm>

using namespace Qt;

namespace ks {

static const double k_maxLatSlip = 15.0;
static const double k_maxLonSlip = 0.3;
static const int    k_curvePoints = 120;

static const double k_bMin = 0.0,   k_bMax = 50.0,  k_bDefault = 10.0;
static const double k_cMin = 0.5,   k_cMax = 3.0,   k_cDefault = 1.65;
static const double k_dMin = 0.0,   k_dMax = 6000.0,k_dDefault = 3000.0;
static const double k_eMin = -3.0,  k_eMax = 3.0,   k_eDefault = 0.0;

static double sliderToValue(int slider, double min, double max) {
    return min + (double)slider / 1000.0 * (max - min);
}
static int valueToSlider(double value, double min, double max) {
    return (int)((value - min) / (max - min) * 1000.0);
}

// ============================================================================
// TireCurveEditor
// ============================================================================

TireCurveEditor::TireCurveEditor(QWidget* parent)
    : QWidget(parent)
{
    populateCompoundPresets();
    m_primaryModel = PacejkaTireModel(m_presets[0].coeffs);
    m_compareModel = PacejkaTireModel(m_presets[1].coeffs);
    buildUI();
    refreshCurves();
}

void TireCurveEditor::populateCompoundPresets() {
    m_presets.clear();

    TireCurvePreset street;
    street.name = "Street";
    street.coeffs = PacejkaTireModel::getStreetTireCoefficients();
    street.normalForce = 4000.0;
    street.frictionCoeff = 1.0;
    m_presets.append(street);

    TireCurvePreset slick;
    slick.name = "Slick";
    slick.coeffs = PacejkaTireModel::getSlickTireCoefficients();
    slick.normalForce = 4000.0;
    slick.frictionCoeff = 1.4;
    m_presets.append(slick);

    TireCurvePreset wet;
    wet.name = "Wet";
    wet.coeffs = PacejkaTireModel::getWetTireCoefficients();
    wet.normalForce = 4000.0;
    wet.frictionCoeff = 0.7;
    m_presets.append(wet);

    TireCurvePreset rally;
    rally.name = "Rally";
    rally.coeffs = PacejkaTireModel::getRallyTireCoefficients();
    rally.normalForce = 4000.0;
    rally.frictionCoeff = 1.1;
    m_presets.append(rally);
}

void TireCurveEditor::buildUI() {
    auto* mainLayout = new QVBoxLayout(this);

    // ── Toolbar ──
    auto* toolbar = new QHBoxLayout;
    m_compoundCombo = new QComboBox(this);
    m_compoundCombo->setMinimumWidth(120);
    for (const auto& p : m_presets)
        m_compoundCombo->addItem(p.name);
    m_compoundCombo->setCurrentIndex(0);

    m_compareCombo = new QComboBox(this);
    m_compareCombo->setMinimumWidth(120);
    for (const auto& p : m_presets)
        m_compareCombo->addItem(p.name);
    m_compareCombo->setCurrentIndex(1);

    m_comparisonToggle = new QCheckBox("Compare", this);
    m_comparisonToggle->setToolTip("Overlay a second compound for comparison");

    m_copyToCompareBtn = new QPushButton("Copy → Compare", this);

    m_resetBtn = new QPushButton("Reset", this);
    m_exportBtn = new QPushButton("Export CSV", this);
    m_importBtn = new QPushButton("Import CSV", this);

    toolbar->addWidget(new QLabel("Compound:", this));
    toolbar->addWidget(m_compoundCombo);
    toolbar->addSpacing(12);
    toolbar->addWidget(m_comparisonToggle);
    toolbar->addWidget(m_compareCombo);
    toolbar->addWidget(m_copyToCompareBtn);
    toolbar->addStretch();
    toolbar->addWidget(m_resetBtn);
    toolbar->addWidget(m_exportBtn);
    toolbar->addWidget(m_importBtn);
    mainLayout->addLayout(toolbar);

    // ── Load & friction ──
    auto* paramBar = new QHBoxLayout;
    m_normalLoadSpin = new QDoubleSpinBox(this);
    m_normalLoadSpin->setRange(500.0, 12000.0);
    m_normalLoadSpin->setValue(4000.0);
    m_normalLoadSpin->setSingleStep(100.0);
    m_normalLoadSpin->setSuffix(" N");
    m_normalLoadSpin->setDecimals(0);

    m_frictionSpin = new QDoubleSpinBox(this);
    m_frictionSpin->setRange(0.1, 2.0);
    m_frictionSpin->setValue(1.0);
    m_frictionSpin->setSingleStep(0.05);
    m_frictionSpin->setSuffix(" μ");

    paramBar->addWidget(new QLabel("Normal Load:", this));
    paramBar->addWidget(m_normalLoadSpin);
    paramBar->addSpacing(16);
    paramBar->addWidget(new QLabel("Friction:", this));
    paramBar->addWidget(m_frictionSpin);
    paramBar->addStretch();
    mainLayout->addLayout(paramBar);

    // ── Series ──
    m_latPrimarySeries = new QLineSeries();
    m_latPrimarySeries->setName("Lateral (primary)");
    m_latPrimarySeries->setColor(QColor("#E74C3C"));

    m_latCompareSeries = new QLineSeries();
    m_latCompareSeries->setName("Lateral (compare)");
    m_latCompareSeries->setColor(QColor("#E67E22"));
    m_latCompareSeries->setPen(QPen(QColor("#E67E22"), 1, Qt::DashLine));

    m_lonPrimarySeries = new QLineSeries();
    m_lonPrimarySeries->setName("Longitudinal (primary)");
    m_lonPrimarySeries->setColor(QColor("#3498DB"));

    m_lonCompareSeries = new QLineSeries();
    m_lonCompareSeries->setName("Longitudinal (compare)");
    m_lonCompareSeries->setColor(QColor("#2ECC71"));
    m_lonCompareSeries->setPen(QPen(QColor("#2ECC71"), 1, Qt::DashLine));

    m_gripCloudSeries = new QScatterSeries();
    m_gripCloudSeries->setName("Achievable forces");
    m_gripCloudSeries->setColor(QColor("#95A5A6"));
    m_gripCloudSeries->setMarkerSize(2);
    m_gripCloudSeries->setBorderColor(Qt::transparent);

    m_gripEnvelopeSeries = new QLineSeries();
    m_gripEnvelopeSeries->setName("Grip envelope");
    m_gripEnvelopeSeries->setColor(QColor("#E74C3C"));

    m_gripLatAxis = new QLineSeries();
    m_gripLatAxis->setName("Pure lateral");
    m_gripLatAxis->setColor(QColor("#E67E22"));
    m_gripLatAxis->setPen(QPen(QColor("#E67E22"), 1, Qt::DashLine));

    m_gripLonAxis = new QLineSeries();
    m_gripLonAxis->setName("Pure longitudinal");
    m_gripLonAxis->setColor(QColor("#3498DB"));
    m_gripLonAxis->setPen(QPen(QColor("#3498DB"), 1, Qt::DashLine));

    // ── Tabbed charts ──
    m_chartTabs = new QTabWidget(this);
    m_chartTabs->setTabPosition(QTabWidget::South);

    auto* curvePage = new QWidget(this);
    buildCurvePage(curvePage);
    m_chartTabs->addTab(curvePage, "Lateral / Longitudinal");

    auto* gripPage = buildGripCirclePage(this);
    m_chartTabs->addTab(gripPage, "Grip Circle");

    mainLayout->addWidget(m_chartTabs, 1);

    // ── Sliders panel ──
    auto* slidersWidget = new QWidget(this);
    auto* slidersLayout = new QHBoxLayout(slidersWidget);

    // Lateral sliders
    auto* latSlidersGroup = new QGroupBox("Lateral Magic Formula (B/C/D/E)", this);
    auto* latSlidersForm = new QFormLayout(latSlidersGroup);

    m_latBLabel = new QLabel("B (stiffness):", this);
    m_latBSlider = new QSlider(Qt::Horizontal, this);
    m_latBSlider->setRange(0, 1000);
    m_latBSlider->setValue(valueToSlider(k_bDefault, k_bMin, k_bMax));
    m_latBSpin = new QDoubleSpinBox(this);
    m_latBSpin->setRange(k_bMin, k_bMax);
    m_latBSpin->setDecimals(2);
    m_latBSpin->setValue(k_bDefault);
    auto* latBRow = new QHBoxLayout;
    latBRow->addWidget(m_latBSlider, 1);
    latBRow->addWidget(m_latBSpin);
    latSlidersForm->addRow(m_latBLabel, latBRow);

    m_latCLabel = new QLabel("C (shape):", this);
    m_latCSlider = new QSlider(Qt::Horizontal, this);
    m_latCSlider->setRange(0, 1000);
    m_latCSlider->setValue(valueToSlider(k_cDefault, k_cMin, k_cMax));
    m_latCSpin = new QDoubleSpinBox(this);
    m_latCSpin->setRange(k_cMin, k_cMax);
    m_latCSpin->setDecimals(2);
    m_latCSpin->setValue(k_cDefault);
    auto* latCRow = new QHBoxLayout;
    latCRow->addWidget(m_latCSlider, 1);
    latCRow->addWidget(m_latCSpin);
    latSlidersForm->addRow(m_latCLabel, latCRow);

    m_latDLabel = new QLabel("D (peak):", this);
    m_latDSlider = new QSlider(Qt::Horizontal, this);
    m_latDSlider->setRange(0, 1000);
    m_latDSlider->setValue(valueToSlider(k_dDefault, k_dMin, k_dMax));
    m_latDSpin = new QDoubleSpinBox(this);
    m_latDSpin->setRange(k_dMin, k_dMax);
    m_latDSpin->setDecimals(0);
    m_latDSpin->setValue(k_dDefault);
    auto* latDRow = new QHBoxLayout;
    latDRow->addWidget(m_latDSlider, 1);
    latDRow->addWidget(m_latDSpin);
    latSlidersForm->addRow(m_latDLabel, latDRow);

    m_latELabel = new QLabel("E (curvature):", this);
    m_latESlider = new QSlider(Qt::Horizontal, this);
    m_latESlider->setRange(0, 1000);
    m_latESlider->setValue(valueToSlider(k_eDefault, k_eMin, k_eMax));
    m_latESpin = new QDoubleSpinBox(this);
    m_latESpin->setRange(k_eMin, k_eMax);
    m_latESpin->setDecimals(2);
    m_latESpin->setValue(k_eDefault);
    auto* latERow = new QHBoxLayout;
    latERow->addWidget(m_latESlider, 1);
    latERow->addWidget(m_latESpin);
    latSlidersForm->addRow(m_latELabel, latERow);

    slidersLayout->addWidget(latSlidersGroup);

    // Longitudinal sliders
    auto* lonSlidersGroup = new QGroupBox("Longitudinal Magic Formula (B/C/D/E)", this);
    auto* lonSlidersForm = new QFormLayout(lonSlidersGroup);

    m_lonBLabel = new QLabel("B (stiffness):", this);
    m_lonBSlider = new QSlider(Qt::Horizontal, this);
    m_lonBSlider->setRange(0, 1000);
    m_lonBSlider->setValue(valueToSlider(k_bDefault, k_bMin, k_bMax));
    m_lonBSpin = new QDoubleSpinBox(this);
    m_lonBSpin->setRange(k_bMin, k_bMax);
    m_lonBSpin->setDecimals(2);
    m_lonBSpin->setValue(k_bDefault);
    auto* lonBRow = new QHBoxLayout;
    lonBRow->addWidget(m_lonBSlider, 1);
    lonBRow->addWidget(m_lonBSpin);
    lonSlidersForm->addRow(m_lonBLabel, lonBRow);

    m_lonCLabel = new QLabel("C (shape):", this);
    m_lonCSlider = new QSlider(Qt::Horizontal, this);
    m_lonCSlider->setRange(0, 1000);
    m_lonCSlider->setValue(valueToSlider(k_cDefault, k_cMin, k_cMax));
    m_lonCSpin = new QDoubleSpinBox(this);
    m_lonCSpin->setRange(k_cMin, k_cMax);
    m_lonCSpin->setDecimals(2);
    m_lonCSpin->setValue(k_cDefault);
    auto* lonCRow = new QHBoxLayout;
    lonCRow->addWidget(m_lonCSlider, 1);
    lonCRow->addWidget(m_lonCSpin);
    lonSlidersForm->addRow(m_lonCLabel, lonCRow);

    m_lonDLabel = new QLabel("D (peak):", this);
    m_lonDSlider = new QSlider(Qt::Horizontal, this);
    m_lonDSlider->setRange(0, 1000);
    m_lonDSlider->setValue(valueToSlider(k_dDefault, k_dMin, k_dMax));
    m_lonDSpin = new QDoubleSpinBox(this);
    m_lonDSpin->setRange(k_dMin, k_dMax);
    m_lonDSpin->setDecimals(0);
    m_lonDSpin->setValue(k_dDefault);
    auto* lonDRow = new QHBoxLayout;
    lonDRow->addWidget(m_lonDSlider, 1);
    lonDRow->addWidget(m_lonDSpin);
    lonSlidersForm->addRow(m_lonDLabel, lonDRow);

    m_lonELabel = new QLabel("E (curvature):", this);
    m_lonESlider = new QSlider(Qt::Horizontal, this);
    m_lonESlider->setRange(0, 1000);
    m_lonESlider->setValue(valueToSlider(k_eDefault, k_eMin, k_eMax));
    m_lonESpin = new QDoubleSpinBox(this);
    m_lonESpin->setRange(k_eMin, k_eMax);
    m_lonESpin->setDecimals(2);
    m_lonESpin->setValue(k_eDefault);
    auto* lonERow = new QHBoxLayout;
    lonERow->addWidget(m_lonESlider, 1);
    lonERow->addWidget(m_lonESpin);
    lonSlidersForm->addRow(m_lonELabel, lonERow);

    slidersLayout->addWidget(lonSlidersGroup);
    mainLayout->addWidget(slidersWidget);

    // ── Stats bar ──
    auto* statsLayout = new QHBoxLayout;
    m_peakLatForceLabel = new QLabel("Peak Lat: -- N", this);
    m_peakSlipAngleLabel = new QLabel("Peak Angle: --°", this);
    m_peakLonForceLabel = new QLabel("Peak Lon: -- N", this);
    m_peakSlipRatioLabel = new QLabel("Peak Slip: --%", this);
    m_latStiffnessLabel = new QLabel("Lat Stiff: -- N/°", this);
    m_lonStiffnessLabel = new QLabel("Lon Stiff: -- N/%", this);

    statsLayout->addWidget(m_peakLatForceLabel);
    statsLayout->addWidget(m_peakSlipAngleLabel);
    statsLayout->addWidget(m_peakLonForceLabel);
    statsLayout->addWidget(m_peakSlipRatioLabel);
    statsLayout->addWidget(m_latStiffnessLabel);
    statsLayout->addWidget(m_lonStiffnessLabel);
    statsLayout->addStretch();
    mainLayout->addLayout(statsLayout);

    // ── Advanced: Full Pacejka Coefficients ──
    m_advancedCoeffGroup = new QGroupBox("Advanced: Full Pacejka Coefficients", this);
    m_advancedCoeffGroup->setCheckable(true);
    m_advancedCoeffGroup->setChecked(false);
    m_advancedCoeffGroup->setVisible(false); // hidden until the user enables it
    connect(m_advancedCoeffGroup, &QGroupBox::toggled, this, [this](bool) {
        // When expanded, refresh coefficients from preset
        if (m_currentCompoundIdx >= 0 && m_currentCompoundIdx < m_presets.size())
            applyCompoundToSliders(m_currentCompoundIdx);
    });
    buildCoefficientPanel(m_advancedCoeffGroup);
    // Add an "Edit Full Coefficients" toggle button
    auto* toggleCoeffBtn = new QPushButton("Edit Full Coefficients", this);
    toggleCoeffBtn->setCheckable(true);
    connect(toggleCoeffBtn, &QPushButton::toggled, this, [this, toggleCoeffBtn](bool checked) {
        m_advancedCoeffGroup->setVisible(checked);
        toggleCoeffBtn->setText(checked ? "Hide Full Coefficients" : "Edit Full Coefficients");
        if (checked && m_currentCompoundIdx >= 0 && m_currentCompoundIdx < m_presets.size())
            applyCompoundToSliders(m_currentCompoundIdx);
    });
    mainLayout->addWidget(toggleCoeffBtn);
    mainLayout->addWidget(m_advancedCoeffGroup);

    // ── Connections ──
    connect(m_compoundCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TireCurveEditor::onCompoundChanged);
    connect(m_compareCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TireCurveEditor::onCompareCompoundChanged);
    connect(m_comparisonToggle, &QCheckBox::toggled,
            this, &TireCurveEditor::onComparisonToggled);
    connect(m_copyToCompareBtn, &QPushButton::clicked,
            this, &TireCurveEditor::onCopyToCompare);
    connect(m_resetBtn, &QPushButton::clicked,
            this, &TireCurveEditor::resetToDefaults);
    connect(m_exportBtn, &QPushButton::clicked,
            this, &TireCurveEditor::onExportCurve);
    connect(m_importBtn, &QPushButton::clicked,
            this, &TireCurveEditor::onImportCurve);

    connect(m_normalLoadSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &TireCurveEditor::onLoadChanged);
    connect(m_frictionSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &TireCurveEditor::onFrictionChanged);

    auto connectSlider = [this](QSlider* slider, QDoubleSpinBox* spin) {
        connect(slider, &QSlider::valueChanged, this, [this, spin, slider](int v) {
            double mn = spin->minimum(), mx = spin->maximum();
            spin->setValue(sliderToValue(v, mn, mx));
            onParameterChanged();
        });
        connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, [this, spin, slider](double v) {
            slider->setValue(valueToSlider(v, spin->minimum(), spin->maximum()));
        });
    };

    connectSlider(m_latBSlider, m_latBSpin);
    connectSlider(m_latCSlider, m_latCSpin);
    connectSlider(m_latDSlider, m_latDSpin);
    connectSlider(m_latESlider, m_latESpin);
    connectSlider(m_lonBSlider, m_lonBSpin);
    connectSlider(m_lonCSlider, m_lonCSpin);
    connectSlider(m_lonDSlider, m_lonDSpin);
    connectSlider(m_lonESlider, m_lonESpin);
}

void TireCurveEditor::buildCurvePage(QWidget* parent) {
    auto* layout = new QVBoxLayout(parent);
    auto* splitter = new QSplitter(Qt::Horizontal, parent);

    auto* latGroup = new QGroupBox("Lateral Force vs Slip Angle", parent);
    auto* latLayout = new QVBoxLayout(latGroup);
    m_latChart = new QChart();
    m_latChart->setTitle("Lateral Force");
    m_latChart->setBackgroundVisible(false);
    m_latChart->legend()->setVisible(true);
    m_latChartView = new QChartView(m_latChart, parent);
    m_latChartView->setRenderHint(QPainter::Antialiasing);
    m_latChartView->setMinimumHeight(250);
    latLayout->addWidget(m_latChartView);
    splitter->addWidget(latGroup);

    auto* lonGroup = new QGroupBox("Longitudinal Force vs Slip Ratio", parent);
    auto* lonLayout = new QVBoxLayout(lonGroup);
    m_lonChart = new QChart();
    m_lonChart->setTitle("Longitudinal Force");
    m_lonChart->setBackgroundVisible(false);
    m_lonChart->legend()->setVisible(true);
    m_lonChartView = new QChartView(m_lonChart, parent);
    m_lonChartView->setRenderHint(QPainter::Antialiasing);
    m_lonChartView->setMinimumHeight(250);
    lonLayout->addWidget(m_lonChartView);
    splitter->addWidget(lonGroup);

    splitter->setSizes({500, 500});
    layout->addWidget(splitter);
}

QWidget* TireCurveEditor::buildGripCirclePage(QWidget* parent) {
    auto* page = new QWidget(parent);
    auto* layout = new QVBoxLayout(page);

    m_gripChart = new QChart();
    m_gripChart->setTitle("Grip Circle / Friction Ellipse");
    m_gripChart->setBackgroundVisible(false);
    m_gripChart->legend()->setVisible(true);

    m_gripChartView = new QChartView(m_gripChart, parent);
    m_gripChartView->setRenderHint(QPainter::Antialiasing);
    m_gripChartView->setMinimumHeight(300);
    layout->addWidget(m_gripChartView);

    auto* info = new QLabel(
        "The grip circle shows the maximum combined force the tire can produce. "
        "Points inside the ellipse are achievable; the boundary is the grip limit. "
        "The dashed axes show pure lateral (orange) and pure longitudinal (blue) grip.",
        page);
    info->setWordWrap(true);
    info->setStyleSheet("color: #888; padding: 4px;");
    layout->addWidget(info);

    return page;
}

void TireCurveEditor::buildCoefficientPanel(QWidget* parent) {
    auto* colLayout = new QHBoxLayout(parent);

    auto makeSpin = [](double min, double max, int decimals, double step) {
        auto* s = new QDoubleSpinBox();
        s->setRange(min, max);
        s->setDecimals(decimals);
        s->setSingleStep(step);
        s->setFixedWidth(90);
        return s;
    };

    // Lateral coefficients (a1-a13)
    {
        auto* g = new QGroupBox("Lateral (a)");
        auto* f = new QFormLayout(g);
        struct { double mn, mx; int dec; double step; } lat[] = {
            {-100,100,2,0.1}, {0,5000,1,1.0}, {0,5000,1,1.0}, {0,20,3,0.01},
            {-5,5,4,0.001}, {-10,10,3,0.01}, {-10,10,3,0.01}, {-5,5,3,0.01},
            {-5,5,4,0.001}, {-10,10,3,0.01}, {-100,100,2,0.1}, {-5,5,4,0.001},
            {-10,10,3,0.01}
        };
        for (int i = 0; i < 13; ++i) {
            m_latCoeffSpin[i] = makeSpin(lat[i].mn, lat[i].mx, lat[i].dec, lat[i].step);
            f->addRow(QString("a%1:").arg(i+1), m_latCoeffSpin[i]);
        }
        colLayout->addWidget(g);
    }

    // Longitudinal coefficients (b1-b8)
    {
        auto* g = new QGroupBox("Longitudinal (b)");
        auto* f = new QFormLayout(g);
        struct { double mn, mx; int dec; double step; } lon[] = {
            {-100,100,2,0.1}, {0,5000,1,1.0}, {-500,500,2,0.1}, {-1000,1000,1,1.0},
            {-10,10,4,0.001}, {-10,10,4,0.001}, {-10,10,4,0.001}, {-10,10,4,0.001}
        };
        for (int i = 0; i < 8; ++i) {
            m_lonCoeffSpin[i] = makeSpin(lon[i].mn, lon[i].mx, lon[i].dec, lon[i].step);
            f->addRow(QString("b%1:").arg(i+1), m_lonCoeffSpin[i]);
        }
        colLayout->addWidget(g);
    }

    // Aligning moment coefficients (c1-c13)
    {
        auto* g = new QGroupBox("Aligning Moment (c)");
        auto* f = new QFormLayout(g);
        struct { double mn, mx; int dec; double step; } aln[] = {
            {-50,50,3,0.01}, {-50,50,3,0.01}, {-50,50,3,0.01}, {-50,50,3,0.01},
            {-10,10,4,0.001}, {-10,10,4,0.001}, {-10,10,4,0.001}, {-50,50,3,0.01},
            {-5,5,4,0.001}, {-10,10,4,0.001}, {-10,10,4,0.001}, {-10,10,4,0.001},
            {-10,10,4,0.001}
        };
        for (int i = 0; i < 13; ++i) {
            m_alignCoeffSpin[i] = makeSpin(aln[i].mn, aln[i].mx, aln[i].dec, aln[i].step);
            f->addRow(QString("c%1:").arg(i+1), m_alignCoeffSpin[i]);
        }
        colLayout->addWidget(g);
    }

    // Connect all coefficient spins
    for (int i = 0; i < 13; ++i) {
        connect(m_latCoeffSpin[i], QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &TireCurveEditor::onCoefficientChanged);
        connect(m_alignCoeffSpin[i], QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &TireCurveEditor::onCoefficientChanged);
    }
    for (int i = 0; i < 8; ++i) {
        connect(m_lonCoeffSpin[i], QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &TireCurveEditor::onCoefficientChanged);
    }
}

void TireCurveEditor::applyCompoundToSliders(int idx) {
    if (idx < 0 || idx >= m_presets.size()) return;
    const auto& preset = m_presets[idx];
    float Fz = (float)m_normalLoadSpin->value() / 1000.0f;

    // Populate full coefficient spin boxes from the preset
    const auto& c = preset.coeffs;
    float vals[13] = {c.a1,c.a2,c.a3,c.a4,c.a5,c.a6,c.a7,c.a8,c.a9,c.a10,c.a11,c.a12,c.a13};
    for (int i = 0; i < 13; ++i)
        m_latCoeffSpin[i]->setValue(vals[i]);
    float bVals[8] = {c.b1,c.b2,c.b3,c.b4,c.b5,c.b6,c.b7,c.b8};
    for (int i = 0; i < 8; ++i)
        m_lonCoeffSpin[i]->setValue(bVals[i]);
    float cVals[13] = {c.c1,c.c2,c.c3,c.c4,c.c5,c.c6,c.c7,c.c8,c.c9,c.c10,c.c11,c.c12,c.c13};
    for (int i = 0; i < 13; ++i)
        m_alignCoeffSpin[i]->setValue(cVals[i]);

    // Compute effective B/C/D/E from the full Pacejka model for lateral
    float latB = preset.coeffs.a3 * std::sin(2.0f * std::atan(Fz / preset.coeffs.a4));
    float latC = preset.coeffs.a1;
    float latD = (preset.coeffs.a1 * Fz * Fz + preset.coeffs.a2 * Fz) * (float)m_frictionSpin->value();
    float latE = preset.coeffs.a6 * Fz + preset.coeffs.a7;

    m_latBSpin->setValue(latB);
    m_latCSpin->setValue(latC);
    m_latDSpin->setValue(latD);
    m_latESpin->setValue(latE);

    // Compute for longitudinal
    float lonB = preset.coeffs.b3 * Fz * Fz + preset.coeffs.b4 * Fz;
    float lonC = 1.65f;
    float lonD = (preset.coeffs.b1 * Fz * Fz + preset.coeffs.b2 * Fz) * (float)m_frictionSpin->value();
    float lonE = preset.coeffs.b5 * Fz * Fz + preset.coeffs.b6 * Fz + preset.coeffs.b7;

    m_lonBSpin->setValue(lonB);
    m_lonCSpin->setValue(lonC);
    m_lonDSpin->setValue(lonD);
    m_lonESpin->setValue(lonE);
}

void TireCurveEditor::refreshCurves() {
    computeCurves();
    updateCharts();
    updateGripCircle();
    emit curveChanged();
}

void TireCurveEditor::computeCurves() {
    float latB = (float)m_latBSpin->value();
    float latC = (float)m_latCSpin->value();
    float latD = (float)m_latDSpin->value();
    float latE = (float)m_latESpin->value();

    float lonB = (float)m_lonBSpin->value();
    float lonC = (float)m_lonCSpin->value();
    float lonD = (float)m_lonDSpin->value();
    float lonE = (float)m_lonESpin->value();

    // Generate lateral curve (positive and negative slip)
    QVector<QPair<float, float>> latCurve;
    int halfPts = k_curvePoints;
    latCurve.reserve(halfPts * 2 - 1);
    for (int i = -halfPts + 1; i < halfPts; ++i) {
        float sa = (float)i * (float)k_maxLatSlip / (float)(halfPts - 1);
        float Fy = PacejkaTireModel::magicFormula(sa, latB, latC, latD, latE);
        latCurve.append({sa, Fy});
    }

    // Generate longitudinal curve
    QVector<QPair<float, float>> lonCurve;
    lonCurve.reserve(k_curvePoints);
    for (int i = 0; i < k_curvePoints; ++i) {
        float sr = (float)i * (float)k_maxLonSlip / (float)(k_curvePoints - 1);
        float srPct = sr * 100.0f;
        float Fx = PacejkaTireModel::magicFormula(srPct, lonB, lonC, lonD, lonE);
        lonCurve.append({srPct, Fx});
    }

    m_currentResult = buildResult(latCurve, lonCurve);

    // Compute comparison curve from the full Pacejka model of the compare compound
    int cmpIdx = m_compareCombo->currentIndex();
    if (cmpIdx >= 0 && cmpIdx < m_presets.size()) {
        float Fz = (float)m_normalLoadSpin->value();
        float mu = (float)m_frictionSpin->value();
        const auto& cmpCoeffs = m_presets[cmpIdx].coeffs;

        QVector<QPair<float, float>> cmpLat;
        cmpLat.reserve(halfPts * 2 - 1);
        for (int i = -halfPts + 1; i < halfPts; ++i) {
            float sa = (float)i * (float)k_maxLatSlip / (float)(halfPts - 1);
            PacejkaTireModel::TireState state;
            state.slipAngle = sa * 3.14159265f / 180.0f;
            state.normalForce = Fz;
            state.frictionCoefficient = mu;
            state.tirePressure = 26.0f;
            state.tireTemp = 80.0f;
            float Fy = PacejkaTireModel(cmpCoeffs).calculateLateralForce(state);
            cmpLat.append({sa, Fy});
        }

        QVector<QPair<float, float>> cmpLon;
        cmpLon.reserve(k_curvePoints);
        for (int i = 0; i < k_curvePoints; ++i) {
            float sr = (float)i * (float)k_maxLonSlip / (float)(k_curvePoints - 1);
            PacejkaTireModel::TireState state;
            state.slipRatio = sr;
            state.normalForce = Fz;
            state.frictionCoefficient = mu;
            state.tirePressure = 26.0f;
            state.tireTemp = 80.0f;
            float Fx = PacejkaTireModel(cmpCoeffs).calculateLongitudinalForce(state);
            cmpLon.append({sr * 100.0f, Fx});
        }

        m_compareResult = buildResult(cmpLat, cmpLon);
    }
}

TireCurveEditor::CurveResult TireCurveEditor::buildResult(
    const QVector<QPair<float, float>>& lat,
    const QVector<QPair<float, float>>& lon)
{
    CurveResult r;
    r.lateralCurve = lat;
    r.longitudinalCurve = lon;

    // Peak lateral
    for (const auto& pt : lat) {
        double absFy = std::abs(pt.second);
        if (absFy > r.peakLateralForce) {
            r.peakLateralForce = absFy;
            r.peakSlipAngle = std::abs(pt.first);
        }
    }

    // Peak longitudinal
    for (const auto& pt : lon) {
        if (pt.second > r.peakLongitudinalForce) {
            r.peakLongitudinalForce = pt.second;
            r.peakSlipRatio = pt.first;
        }
    }

    // Stiffness = slope near origin
    if (lat.size() > 2) {
        double x0 = lat[0].first;
        double y0 = lat[0].second;
        double x1 = lat[1].first;
        double y1 = lat[1].second;
        double dx = x1 - x0;
        if (dx != 0) {
            r.stiffnessLateral = (y1 - y0) / dx;
        }

        // Use first positive slip region
        for (int i = 0; i < lat.size(); ++i) {
            if (lat[i].first >= 0) {
                if (i > 0 && i < lat.size() - 1) {
                    x0 = lat[i].first;
                    y0 = lat[i].second;
                    x1 = lat[i+1].first;
                    y1 = lat[i+1].second;
                    dx = x1 - x0;
                    if (dx != 0) {
                        r.stiffnessLateral = (y1 - y0) / dx;
                    }
                }
                break;
            }
        }
    }

    if (lon.size() > 1) {
        double x0 = lon[0].first;
        double y0 = lon[0].second;
        double x1 = lon[1].first;
        double y1 = lon[1].second;
        double dx = x1 - x0;
        if (dx != 0) {
            r.stiffnessLongitudinal = (y1 - y0) / dx;
        }
    }

    return r;
}

void TireCurveEditor::computeGripCircle() {
    float Fz = (float)m_normalLoadSpin->value();
    float mu = (float)m_frictionSpin->value();
    int cmpIdx = m_compoundCombo->currentIndex();
    if (cmpIdx < 0 || cmpIdx >= m_presets.size()) return;
    const auto& coeffs = m_presets[cmpIdx].coeffs;
    PacejkaTireModel model(coeffs);

    int saSteps = 36;
    int srSteps = 18;
    float maxSa = (float)k_maxLatSlip;
    float maxSr = (float)k_maxLonSlip;

    QVector<QPointF> cloud;
    QVector<QPointF> envelope;
    QVector<QPointF> latAxis;
    QVector<QPointF> lonAxis;

    // Pure lateral curve (sr = 0): lateral force on X, longitudinal force (0) on Y
    for (int si = 0; si <= saSteps * 2; ++si) {
        float sa = -maxSa + (2.0f * maxSa * si / (saSteps * 2));
        float saRad = sa * 3.14159265f / 180.0f;
        PacejkaTireModel::TireState state;
        state.slipAngle = saRad;
        state.slipRatio = 0.0f;
        state.normalForce = Fz;
        state.frictionCoefficient = mu;
        state.tirePressure = 26.0f;
        state.tireTemp = 80.0f;
        float Fy = model.calculateLateralForce(state);
        latAxis.append({(double)Fy, 0.0});
    }

    // Pure longitudinal curve (sa = 0): lateral force (0) on X, longitudinal on Y
    for (int sj = 0; sj <= srSteps; ++sj) {
        float sr = maxSr * sj / srSteps;
        PacejkaTireModel::TireState state;
        state.slipAngle = 0.0f;
        state.slipRatio = sr;
        state.normalForce = Fz;
        state.frictionCoefficient = mu;
        state.tirePressure = 26.0f;
        state.tireTemp = 80.0f;
        float Fx = model.calculateLongitudinalForce(state);
        lonAxis.append({0.0, (double)Fx});
    }

    // Combined slip grid: compute (Fy, Fx) at many (sa, sr) pairs
    for (int si = 0; si <= saSteps; ++si) {
        float sa = -maxSa + (2.0f * maxSa * si / saSteps);
        float saRad = sa * 3.14159265f / 180.0f;
        for (int sj = 0; sj <= srSteps; ++sj) {
            float sr = maxSr * sj / srSteps;
            PacejkaTireModel::TireState state;
            state.slipAngle = saRad;
            state.slipRatio = sr;
            state.normalForce = Fz;
            state.frictionCoefficient = mu;
            state.tirePressure = 26.0f;
            state.tireTemp = 80.0f;
            auto forces = model.calculateForces(state);
            cloud.append({(double)forces.lateralForce, (double)forces.longitudinalForce});
        }
    }

    // Compute envelope: for each angular bin, find max force magnitude
    int angleSteps = 72;
    QVector<double> envelopeDist(angleSteps, 0.0);
    for (const auto& pt : cloud) {
        double angle = std::atan2(pt.y(), pt.x());
        double dist = std::sqrt(pt.x() * pt.x() + pt.y() * pt.y());
        int idx = (int)((angle + 3.14159) / (2.0 * 3.14159) * (angleSteps - 1) + 0.5);
        idx = std::max(0, std::min(angleSteps - 1, idx));
        if (dist > envelopeDist[idx])
            envelopeDist[idx] = dist;
    }

    // 3-sample smoothing
    QVector<double> smooth(angleSteps, 0.0);
    for (int i = 0; i < angleSteps; ++i) {
        int prev = (i - 1 + angleSteps) % angleSteps;
        int next = (i + 1) % angleSteps;
        smooth[i] = (envelopeDist[prev] + envelopeDist[i] + envelopeDist[next]) / 3.0;
    }

    for (int i = 0; i <= angleSteps; ++i) {
        double angle = (double)i / angleSteps * 2.0 * 3.14159;
        int idx = i % angleSteps;
        double dist = smooth[idx];
        envelope.append({dist * std::cos(angle), dist * std::sin(angle)});
    }

    m_gripCloudSeries->clear();
    m_gripEnvelopeSeries->clear();
    m_gripLatAxis->clear();
    m_gripLonAxis->clear();

    for (const auto& pt : cloud)
        m_gripCloudSeries->append(pt);

    for (const auto& pt : latAxis)
        m_gripLatAxis->append(pt);

    for (const auto& pt : lonAxis)
        m_gripLonAxis->append(pt);

    for (const auto& pt : envelope)
        m_gripEnvelopeSeries->append(pt);
}

void TireCurveEditor::updateGripCircle() {
    computeGripCircle();

    m_gripChart->removeAllSeries();

    m_gripChart->addSeries(m_gripCloudSeries);
    m_gripChart->addSeries(m_gripEnvelopeSeries);
    m_gripChart->addSeries(m_gripLatAxis);
    m_gripChart->addSeries(m_gripLonAxis);

    double maxVal = 0;
    for (const auto* s : {m_gripEnvelopeSeries, m_gripLatAxis, m_gripLonAxis}) {
        for (const auto& pt : s->points())
            maxVal = std::max(maxVal, std::max(std::abs(pt.x()), std::abs(pt.y())));
    }
    maxVal = std::max(maxVal, 100.0);

    clearOldAxes(m_gripChart);
    auto* axisX = new QValueAxis();
    axisX->setTitleText("Lateral Force (N)");
    axisX->setRange(-maxVal * 1.15, maxVal * 1.15);
    m_gripChart->addAxis(axisX, Qt::AlignBottom);

    auto* axisY = new QValueAxis();
    axisY->setTitleText("Longitudinal Force (N)");
    axisY->setRange(-maxVal * 0.1, maxVal * 1.15);
    m_gripChart->addAxis(axisY, Qt::AlignLeft);

    const auto seriesList = m_gripChart->series();
    for (auto* s : seriesList) {
        s->attachAxis(axisX);
        s->attachAxis(axisY);
    }
}

void TireCurveEditor::clearOldAxes(QChart* chart) {
    const auto axes = chart->axes();
    for (auto* ax : axes) {
        chart->removeAxis(ax);
    }
}

void TireCurveEditor::setupAxes(QChart* chart, double xMin, double xMax,
                                double yMin, double yMax,
                                const QString& xTitle, const QString& yTitle)
{
    clearOldAxes(chart);

    auto* axisX = new QValueAxis();
    axisX->setTitleText(xTitle);
    axisX->setRange(xMin, xMax);
    axisX->setLabelFormat("%.0f");
    chart->addAxis(axisX, Qt::AlignBottom);

    auto* axisY = new QValueAxis();
    axisY->setTitleText(yTitle);
    axisY->setRange(yMin, yMax);
    axisY->setLabelFormat("%.0f");
    chart->addAxis(axisY, Qt::AlignLeft);

    const auto seriesList = chart->series();
    for (auto* s : seriesList) {
        s->attachAxis(axisX);
        s->attachAxis(axisY);
    }
}

void TireCurveEditor::updateCharts() {
    // ── Lateral chart ──
    m_latChart->removeAllSeries();
    m_latPrimarySeries->clear();
    m_latCompareSeries->clear();

    for (const auto& pt : m_currentResult.lateralCurve) {
        m_latPrimarySeries->append(pt.first, pt.second);
    }
    m_latChart->addSeries(m_latPrimarySeries);

    if (m_comparisonEnabled) {
        for (const auto& pt : m_compareResult.lateralCurve) {
            m_latCompareSeries->append(pt.first, pt.second);
        }
        m_latChart->addSeries(m_latCompareSeries);
    }

    double latMaxForce = 0;
    for (const auto& pt : m_currentResult.lateralCurve)
        latMaxForce = std::max(latMaxForce, (double)std::abs(pt.second));
    if (m_comparisonEnabled) {
        for (const auto& pt : m_compareResult.lateralCurve)
            latMaxForce = std::max(latMaxForce, (double)std::abs(pt.second));
    }
    setupAxes(m_latChart, -k_maxLatSlip * 1.05, k_maxLatSlip * 1.05,
              -latMaxForce * 1.15, latMaxForce * 1.15,
              "Slip Angle (°)", "Lateral Force (N)");

    // ── Longitudinal chart ──
    m_lonChart->removeAllSeries();
    m_lonPrimarySeries->clear();
    m_lonCompareSeries->clear();

    for (const auto& pt : m_currentResult.longitudinalCurve) {
        m_lonPrimarySeries->append(pt.first, pt.second);
    }
    m_lonChart->addSeries(m_lonPrimarySeries);

    if (m_comparisonEnabled) {
        for (const auto& pt : m_compareResult.longitudinalCurve) {
            m_lonCompareSeries->append(pt.first, pt.second);
        }
        m_lonChart->addSeries(m_lonCompareSeries);
    }

    double lonMaxForce = 0;
    for (const auto& pt : m_currentResult.longitudinalCurve)
        lonMaxForce = std::max(lonMaxForce, (double)pt.second);
    if (m_comparisonEnabled) {
        for (const auto& pt : m_compareResult.longitudinalCurve)
            lonMaxForce = std::max(lonMaxForce, (double)pt.second);
    }
    setupAxes(m_lonChart, 0, k_maxLonSlip * 100.0 * 1.05,
              0, lonMaxForce * 1.15,
              "Slip Ratio (%)", "Longitudinal Force (N)");

    // ── Stats ──
    const auto& r = m_currentResult;
    m_peakLatForceLabel->setText(QString("Peak Lat: %1 N").arg(r.peakLateralForce, 0, 'f', 0));
    m_peakSlipAngleLabel->setText(QString("Peak Angle: %1°").arg(r.peakSlipAngle, 0, 'f', 1));
    m_peakLonForceLabel->setText(QString("Peak Lon: %1 N").arg(r.peakLongitudinalForce, 0, 'f', 0));
    m_peakSlipRatioLabel->setText(QString("Peak Slip: %1%").arg(r.peakSlipRatio, 0, 'f', 1));
    m_latStiffnessLabel->setText(QString("Lat Stiff: %1 N/°").arg(r.stiffnessLateral, 0, 'f', 1));
    m_lonStiffnessLabel->setText(QString("Lon Stiff: %1 N/%").arg(r.stiffnessLongitudinal, 0, 'f', 1));
}

// ── Slots ──

void TireCurveEditor::onCompoundChanged(int index) {
    if (index < 0 || index >= m_presets.size()) return;
    m_currentCompoundIdx = index;
    m_primaryModel.setCoefficients(m_presets[index].coeffs);
    m_normalLoadSpin->setValue(m_presets[index].normalForce);
    m_frictionSpin->setValue(m_presets[index].frictionCoeff);
    applyCompoundToSliders(index);
    emit compoundChanged(m_presets[index].name);
    refreshCurves();
}

void TireCurveEditor::onCompareCompoundChanged(int index) {
    if (index < 0 || index >= m_presets.size()) return;
    m_compareCompoundIdx = index;
    m_compareModel.setCoefficients(m_presets[index].coeffs);
    emit comparisonChanged(m_presets[index].name);
    if (m_comparisonEnabled) refreshCurves();
}

void TireCurveEditor::onParameterChanged() {
    refreshCurves();
}

void TireCurveEditor::onLoadChanged(double) {
    if (m_currentCompoundIdx >= 0 && m_currentCompoundIdx < m_presets.size())
        applyCompoundToSliders(m_currentCompoundIdx);
    refreshCurves();
}

void TireCurveEditor::onFrictionChanged(double) {
    if (m_currentCompoundIdx >= 0 && m_currentCompoundIdx < m_presets.size())
        applyCompoundToSliders(m_currentCompoundIdx);
    refreshCurves();
}

void TireCurveEditor::onCoefficientChanged() {
    if (m_currentCompoundIdx < 0 || m_currentCompoundIdx >= m_presets.size())
        return;

    // Read coefficient spin boxes into the current preset
    auto& c = m_presets[m_currentCompoundIdx].coeffs;
    c.a1  = (float)m_latCoeffSpin[0]->value();
    c.a2  = (float)m_latCoeffSpin[1]->value();
    c.a3  = (float)m_latCoeffSpin[2]->value();
    c.a4  = (float)m_latCoeffSpin[3]->value();
    c.a5  = (float)m_latCoeffSpin[4]->value();
    c.a6  = (float)m_latCoeffSpin[5]->value();
    c.a7  = (float)m_latCoeffSpin[6]->value();
    c.a8  = (float)m_latCoeffSpin[7]->value();
    c.a9  = (float)m_latCoeffSpin[8]->value();
    c.a10 = (float)m_latCoeffSpin[9]->value();
    c.a11 = (float)m_latCoeffSpin[10]->value();
    c.a12 = (float)m_latCoeffSpin[11]->value();
    c.a13 = (float)m_latCoeffSpin[12]->value();

    c.b1 = (float)m_lonCoeffSpin[0]->value();
    c.b2 = (float)m_lonCoeffSpin[1]->value();
    c.b3 = (float)m_lonCoeffSpin[2]->value();
    c.b4 = (float)m_lonCoeffSpin[3]->value();
    c.b5 = (float)m_lonCoeffSpin[4]->value();
    c.b6 = (float)m_lonCoeffSpin[5]->value();
    c.b7 = (float)m_lonCoeffSpin[6]->value();
    c.b8 = (float)m_lonCoeffSpin[7]->value();

    c.c1  = (float)m_alignCoeffSpin[0]->value();
    c.c2  = (float)m_alignCoeffSpin[1]->value();
    c.c3  = (float)m_alignCoeffSpin[2]->value();
    c.c4  = (float)m_alignCoeffSpin[3]->value();
    c.c5  = (float)m_alignCoeffSpin[4]->value();
    c.c6  = (float)m_alignCoeffSpin[5]->value();
    c.c7  = (float)m_alignCoeffSpin[6]->value();
    c.c8  = (float)m_alignCoeffSpin[7]->value();
    c.c9  = (float)m_alignCoeffSpin[8]->value();
    c.c10 = (float)m_alignCoeffSpin[9]->value();
    c.c11 = (float)m_alignCoeffSpin[10]->value();
    c.c12 = (float)m_alignCoeffSpin[11]->value();
    c.c13 = (float)m_alignCoeffSpin[12]->value();

    // Recompute B/C/D/E from the updated coefficients
    float Fz = (float)m_normalLoadSpin->value() / 1000.0f;

    // Block signals on B/C/D/E widgets to avoid double refresh
    m_latBSpin->blockSignals(true); m_latBSlider->blockSignals(true);
    m_latCSpin->blockSignals(true); m_latCSlider->blockSignals(true);
    m_latDSpin->blockSignals(true); m_latDSlider->blockSignals(true);
    m_latESpin->blockSignals(true); m_latESlider->blockSignals(true);
    m_lonBSpin->blockSignals(true); m_lonBSlider->blockSignals(true);
    m_lonCSpin->blockSignals(true); m_lonCSlider->blockSignals(true);
    m_lonDSpin->blockSignals(true); m_lonDSlider->blockSignals(true);
    m_lonESpin->blockSignals(true); m_lonESlider->blockSignals(true);

    // Lateral
    m_latBSpin->setValue(c.a3 * std::sin(2.0f * std::atan(Fz / c.a4)));
    m_latCSpin->setValue(c.a1);
    m_latDSpin->setValue((c.a1 * Fz * Fz + c.a2 * Fz) * (float)m_frictionSpin->value());
    m_latESpin->setValue(c.a6 * Fz + c.a7);
    m_latBSlider->setValue(valueToSlider(m_latBSpin->value(), k_bMin, k_bMax));
    m_latCSlider->setValue(valueToSlider(m_latCSpin->value(), k_cMin, k_cMax));
    m_latDSlider->setValue(valueToSlider(m_latDSpin->value(), k_dMin, k_dMax));
    m_latESlider->setValue(valueToSlider(m_latESpin->value(), k_eMin, k_eMax));

    // Longitudinal
    m_lonBSpin->setValue(c.b3 * Fz * Fz + c.b4 * Fz);
    m_lonCSpin->setValue(1.65);
    m_lonDSpin->setValue((c.b1 * Fz * Fz + c.b2 * Fz) * (float)m_frictionSpin->value());
    m_lonESpin->setValue(c.b5 * Fz * Fz + c.b6 * Fz + c.b7);
    m_lonBSlider->setValue(valueToSlider(m_lonBSpin->value(), k_bMin, k_bMax));
    m_lonCSlider->setValue(valueToSlider(m_lonCSpin->value(), k_cMin, k_cMax));
    m_lonDSlider->setValue(valueToSlider(m_lonDSpin->value(), k_dMin, k_dMax));
    m_lonESlider->setValue(valueToSlider(m_lonESpin->value(), k_eMin, k_eMax));

    m_latBSpin->blockSignals(false); m_latBSlider->blockSignals(false);
    m_latCSpin->blockSignals(false); m_latCSlider->blockSignals(false);
    m_latDSpin->blockSignals(false); m_latDSlider->blockSignals(false);
    m_latESpin->blockSignals(false); m_latESlider->blockSignals(false);
    m_lonBSpin->blockSignals(false); m_lonBSlider->blockSignals(false);
    m_lonCSpin->blockSignals(false); m_lonCSlider->blockSignals(false);
    m_lonDSpin->blockSignals(false); m_lonDSlider->blockSignals(false);
    m_lonESpin->blockSignals(false); m_lonESlider->blockSignals(false);

    refreshCurves();
}

void TireCurveEditor::onComparisonToggled(bool enabled) {
    m_comparisonEnabled = enabled;
    m_compareCombo->setEnabled(enabled);
    m_copyToCompareBtn->setEnabled(enabled);
    refreshCurves();
}

void TireCurveEditor::onCopyToCompare() {
    int idx = m_compoundCombo->currentIndex();
    m_compareCombo->setCurrentIndex(idx);
    m_comparisonToggle->setChecked(true);
}

void TireCurveEditor::resetToDefaults() {
    m_normalLoadSpin->setValue(4000.0);
    m_frictionSpin->setValue(1.0);
    m_compoundCombo->setCurrentIndex(0);
    applyCompoundToSliders(0);
    m_comparisonToggle->setChecked(false);
    refreshCurves();
}

void TireCurveEditor::loadFromIni(const QString& carFolder) {
    // Attempt to load tire coefficients from car's tyres.ini
    QString iniPath = carFolder + "/data/tyres.ini";
    if (!QFile::exists(iniPath))
        iniPath = carFolder + "/tyres.ini";
    if (!QFile::exists(iniPath)) return;

    PacejkaTireModel::TireCoefficients coeffs;
    QFile file(iniPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(';') || line.startsWith('['))
            continue;
        int eq = line.indexOf('=');
        if (eq < 0) continue;
        QString key = line.left(eq).trimmed().toLower();
        float val = line.mid(eq + 1).trimmed().toFloat();

        // Map INI keys to Pacejka coefficients
        if (key == "a1") coeffs.a1 = val;
        else if (key == "a2") coeffs.a2 = val;
        else if (key == "a3") coeffs.a3 = val;
        else if (key == "a4") coeffs.a4 = val;
        else if (key == "a5") coeffs.a5 = val;
        else if (key == "a6") coeffs.a6 = val;
        else if (key == "a7") coeffs.a7 = val;
        else if (key == "a8") coeffs.a8 = val;
        else if (key == "a9") coeffs.a9 = val;
        else if (key == "a10") coeffs.a10 = val;
        else if (key == "a11") coeffs.a11 = val;
        else if (key == "a12") coeffs.a12 = val;
        else if (key == "a13") coeffs.a13 = val;
        else if (key == "b1") coeffs.b1 = val;
        else if (key == "b2") coeffs.b2 = val;
        else if (key == "b3") coeffs.b3 = val;
        else if (key == "b4") coeffs.b4 = val;
        else if (key == "b5") coeffs.b5 = val;
        else if (key == "b6") coeffs.b6 = val;
        else if (key == "b7") coeffs.b7 = val;
        else if (key == "b8") coeffs.b8 = val;
        else if (key == "c1") coeffs.c1 = val;
        else if (key == "c2") coeffs.c2 = val;
        else if (key == "c3") coeffs.c3 = val;
        else if (key == "c4") coeffs.c4 = val;
        else if (key == "c5") coeffs.c5 = val;
        else if (key == "c6") coeffs.c6 = val;
        else if (key == "c7") coeffs.c7 = val;
        else if (key == "c8") coeffs.c8 = val;
        else if (key == "c9") coeffs.c9 = val;
        else if (key == "c10") coeffs.c10 = val;
        else if (key == "c11") coeffs.c11 = val;
        else if (key == "c12") coeffs.c12 = val;
        else if (key == "c13") coeffs.c13 = val;
    }
    file.close();

    // Add a custom preset for this car
    TireCurvePreset custom;
    custom.name = QFileInfo(carFolder).fileName();
    custom.coeffs = coeffs;
    custom.normalForce = m_normalLoadSpin->value();
    custom.frictionCoeff = m_frictionSpin->value();
    m_presets.append(custom);
    m_compoundCombo->addItem(custom.name);

    // Select the custom preset
    m_compoundCombo->setCurrentIndex(m_compoundCombo->count() - 1);
}

void TireCurveEditor::onExportCurve() {
    QString path = QFileDialog::getSaveFileName(this, "Export Tire Curves",
        "tire_curves.csv", "CSV Files (*.csv)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Export Error", "Could not open file for writing.");
        return;
    }

    QTextStream out(&file);
    out << "SlipAngle(deg),LateralForce(N),SlipRatio(%),LongitudinalForce(N)\n";

    int maxPts = std::max(m_currentResult.lateralCurve.size(),
                          m_currentResult.longitudinalCurve.size());
    for (int i = 0; i < maxPts; ++i) {
        double sa = (i < m_currentResult.lateralCurve.size())
            ? m_currentResult.lateralCurve[i].first : 0;
        double fy = (i < m_currentResult.lateralCurve.size())
            ? m_currentResult.lateralCurve[i].second : 0;
        double sr = (i < m_currentResult.longitudinalCurve.size())
            ? m_currentResult.longitudinalCurve[i].first : 0;
        double fx = (i < m_currentResult.longitudinalCurve.size())
            ? m_currentResult.longitudinalCurve[i].second : 0;
        out << sa << "," << fy << "," << sr << "," << fx << "\n";
    }

    file.close();
}

void TireCurveEditor::onImportCurve() {
    QString path = QFileDialog::getOpenFileName(this, "Import Tire Curves",
        "", "CSV Files (*.csv)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Import Error", "Could not open file for reading.");
        return;
    }

    QTextStream in(&file);
    QString header = in.readLine(); // skip header

    // Read points and fit to Pacejka curve (simplified: show imported points)
    QVector<QPair<float, float>> importedLat;
    QVector<QPair<float, float>> importedLon;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;
        QStringList parts = line.split(',');
        if (parts.size() >= 4) {
            float sa = parts[0].toFloat();
            float fy = parts[1].toFloat();
            float sr = parts[2].toFloat();
            float fx = parts[3].toFloat();
            importedLat.append({sa, fy});
            importedLon.append({sr, fx});
        }
    }
    file.close();

    if (importedLat.isEmpty() && importedLon.isEmpty()) {
        QMessageBox::information(this, "Import", "No valid data found in file.");
        return;
    }

    m_currentResult = buildResult(importedLat, importedLon);
    m_latPrimarySeries->clear();
    for (const auto& pt : importedLat)
        m_latPrimarySeries->append(pt.first, pt.second);
    m_lonPrimarySeries->clear();
    for (const auto& pt : importedLon)
        m_lonPrimarySeries->append(pt.first, pt.second);
    updateCharts();
}

} // namespace ks
