#include "CfdWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QDebug>

namespace ks {

#if HAS_3D

CfdWidget::CfdWidget(QWidget* parent)
    : QWidget(parent)
{
    buildUI();
}

CfdWidget::~CfdWidget() {
}

void CfdWidget::buildUI() {
    auto* mainLayout = new QVBoxLayout(this);

    // 3D scatter view for CFD visualization
    m_chart3D = new Q3DScatter();
    m_chart3D->activeTheme()->setType(Q3DTheme::ThemeBrownSand);

    // Create scatter series for pressure field
    m_pressureSeries = new QScatter3DSeries();
    m_pressureSeries->setItemSize(0.1f);
    m_pressureSeries->setBaseColor(Qt::blue);
    m_chart3D->addSeries(m_pressureSeries);

    // Axes
    m_chart3D->axisX()->setTitle("X (m)");
    m_chart3D->axisX()->setRange(-50.0, 50.0);
    m_chart3D->axisY()->setTitle("Y (m)");
    m_chart3D->axisY()->setRange(-5.0, 20.0);
    m_chart3D->axisZ()->setTitle("Z (m)");
    m_chart3D->axisZ()->setRange(-50.0, 50.0);

    auto* chartContainer = QWidget::createWindowContainer(m_chart3D, this);

    // UI controls
    auto* controlsGroup = new QGroupBox("CFD Parameters", this);
    auto* controlsLayout = new QFormLayout(controlsGroup);

    m_resolutionCombo = new QComboBox(this);
    m_resolutionCombo->addItem("32^3", 32);
    m_resolutionCombo->addItem("64^3", 64);  // default
    m_resolutionCombo->addItem("128^3", 128);
    connect(m_resolutionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CfdWidget::onResolutionChanged);

    m_previewModeCombo = new QComboBox(this);
    m_previewModeCombo->addItem("Pressure Contours", 0);
    m_previewModeCombo->addItem("Velocity Vectors", 1);
    m_previewModeCombo->addItem("Vorticity Magnitude", 2);
    connect(m_previewModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CfdWidget::onPreviewModeChanged);

    controlsLayout->addRow("Grid Resolution:", m_resolutionCombo);
    controlsLayout->addRow("Preview Mode:", m_previewModeCombo);

    // Car parameter controls
    auto* carGroup = new QGroupBox("Car Parameters", this);
    auto* carLayout = new QFormLayout(carGroup);

    // Speed slider/box
    m_speedInput = new QDoubleSpinBox(this);
    m_speedInput->setRange(0.0, 300.0);
    m_speedInput->setValue(150.0);
    m_speedInput->setSuffix(" km/h");
    connect(m_speedInput, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this]() {
        emit carParametersChanged(QVector3D(), m_speedInput->value(), m_slipAngleInput->value());
    });

    m_slipAngleInput = new QDoubleSpinBox(this);
    m_slipAngleInput->setRange(-30.0, 30.0);
    m_slipAngleInput->setValue(0.0);
    m_slipAngleInput->setSuffix(" deg");
    connect(m_slipAngleInput, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this]() {
        emit carParametersChanged(QVector3D(), m_speedInput->value(), m_slipAngleInput->value());
    });

    // Position would be handled with proper 3D input widgets

    carLayout->addRow("Speed (km/h):", m_speedInput);
    carLayout->addRow("Slip Angle (deg):", m_slipAngleInput);

    // Wind parameter controls
    auto* windGroup = new QGroupBox("Wind Tunnel", this);
    auto* windLayout = new QFormLayout(windGroup);

    // Wind direction would be handled with proper 3D input widgets

    windLayout->addRow("Wind Speed (m/s):", new QDoubleSpinBox(this));
    windLayout->addRow("Air Density:", new QDoubleSpinBox(this));

    // Buttons
    auto* btnLayout = new QHBoxLayout;
    m_runBtn = new QPushButton("Run Simulation", this);
    m_resetBtn = new QPushButton("Reset", this);
    btnLayout->addWidget(m_runBtn);
    btnLayout->addWidget(m_resetBtn);

    // Status
    m_statusLabel = new QLabel("Ready", this);
    m_statusLabel->setStyleSheet("font-style: italic;");

    // Add widgets to main layout
    mainLayout->addWidget(chartContainer, 1);
    mainLayout->addWidget(controlsGroup);
    mainLayout->addWidget(carGroup);
    mainLayout->addWidget(windGroup);
    mainLayout->addLayout(btnLayout);
    mainLayout->addWidget(m_statusLabel);

    // Initialize with default data
    m_pressures.resize(64 * 64 * 64, 1.0f);
    setCfdData(m_pressures, 64);
}

void CfdWidget::setCfdData(const QVector<float>& pressures, int gridSize) {
    m_pressures = pressures;
    m_gridResolution = gridSize;

    // Update the 3D series
    if (m_pressureSeries && gridSize > 0) {
        m_pressureSeries->clear();
        int count = 0;
        for (int k = 0; k < gridSize; k++) {
            for (int j = 0; j < gridSize; j++) {
                for (int i = 0; i < gridSize; i++) {
                    float p = pressures[k * gridSize * gridSize + j * gridSize + i];
                    if (qAbs(p - 1.0f) > 0.01f) {  // Only show non-uniform pressure
                        float x = float(i) / float(gridSize - 1) * 100.0f - 50.0f;
                        float y = float(k) / float(gridSize - 1) * 20.0f - 5.0f;
                        float z = float(j) / float(gridSize - 1) * 100.0f - 50.0f;
                        m_pressureSeries->append(QVector3D(x, y, z), p);
                        count++;
                    }
                }
            }
        }
        qDebug() << "CFD: Added" << count << "pressure points out of" << gridSize * gridSize * gridSize;
    }
}

void CfdWidget::setCarParameters(const QVector3D& position, float speed, float slipAngle) {
    m_carPosition = position;
    m_speed = speed;
    m_slipAngle = slipAngle;
    m_statusLabel->setText(QString("CFD: Car at %1, Speed %2 km/h, Slip %3°")
        .arg(position.toString())
        .arg(speed, 0, 'f', 1)
        .arg(slipAngle, 0, 'f', 1));
}

void CfdWidget::setWindParameters(const QVector3D& direction, float speed, float density) {
    m_windDirection = direction;
    m_windSpeed = speed;
    m_airDensity = density;
}

void CfdWidget::onResolutionChanged(int index) {
    int size = m_resolutionCombo->itemData(index).toInt();
    m_statusLabel->setText(QString("Resolution changed to %1^3").arg(size));
    // Would trigger recompute here
}

void CfdWidget::onPreviewModeChanged(int index) {
    m_statusLabel->setText(QString("Preview mode: %1").arg(index == 0 ? "Pressure" : index == 1 ? "Velocity" : "Vorticity"));
}

#endif // HAS_3D

} // namespace ks