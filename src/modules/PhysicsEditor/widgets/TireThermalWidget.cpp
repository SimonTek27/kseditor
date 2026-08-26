#include "TireThermalWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QSlider>

namespace ks {

TireThermalWidget::TireThermalWidget(QWidget* parent)
    : QWidget(parent)
{
    buildUI();
}

TireThermalWidget::~TireThermalWidget() {
}

void TireThermalWidget::buildUI() {
    auto* mainLayout = new QVBoxLayout(this);

    // 3D chart for tire thermal visualization
    m_chart3D = new QChart3D(this);
    m_chart3D->setTitle("Tire Thermal Mesh");
    m_chart3D->setTheme(Q3DTheme::Theme::Chocolate);

    // Create scatter series for each tire
    QString tireNames[4] = {"FL", "FR", "RL", "RR"};
    QColor tireColors[4] = {Qt::red, Qt::blue, Qt::green, Qt::magenta};

    for (int i = 0; i < 4; i++) {
        m_tireSeries[i] = new QScatterSeries();
        m_tireSeries[i]->setName(tireNames[i]);
        m_tireSeries[i]->setItemSize(0.5f);
        m_tireSeries[i]->setColor(tireColors[i]);
        m_chart3D->addSeries(m_tireSeries[i]);
    }

    // Set up axes
    m_chart3D->axisX()->setTitle("X (m)");
    m_chart3D->axisX()->setRange(-1.5, 1.5);
    m_chart3D->axisY()->setTitle("Temperature (°C)");
    m_chart3D->axisY()->setRange(0, 130);
    m_chart3D->axisZ()->setTitle("Z (m)");
    m_chart3D->axisZ()->setRange(-1.5, 1.5);

    // Tire parameter controls
    auto* paramsGroup = new QGroupBox("Tire Parameters", this);
    auto* paramsLayout = new QFormLayout(paramsGroup);

    // Temperature displays
    for (int i = 0; i < 4; i++) {
        QGroupBox* tireGroup = new QGroupBox(tireNames[i], this);
        auto* tireLayout = new QFormLayout(tireGroup);

        QDoubleSpinBox* coreIn = new QDoubleSpinBox(this);
        coreIn->setRange(-50, 200);
        coreIn->setValue(80);
        coreIn->setSuffix("°C");

        QDoubleSpinBox* surfIn = new QDoubleSpinBox(this);
        surfIn->setRange(-50, 200);
        surfIn->setValue(100);
        surfIn->setSuffix("°C");

        QDoubleSpinBox* wearIn = new QDoubleSpinBox(this);
        wearIn->setRange(0.0, 1.0);
        wearIn->setSingleStep(0.1);
        wearIn->setValue(0.0);

        connect(coreIn, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this, i]() {
            m_tireData[i].coreTemp = coreIn->value();
            updateTireVisualization();
        });
        connect(surfIn, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this, i]() {
            m_tireData[i].surfaceTemp = surfIn->value();
            updateTireVisualization();
        });
        connect(wearIn, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this, i]() {
            m_tireData[i].wearLevel = wearIn->value();
            updateTireVisualization();
        });

        tireLayout->addRow("Core Temp:", coreIn);
        tireLayout->addRow("Surf Temp:", surfIn);
        tireLayout->addRow("Wear:", wearIn);

        // Store references for later update
        // (In a full impl, would use QMap or array)
    }

    // Simplified - just use the first tire's controls as example
    QDoubleSpinBox* coreIn = new QDoubleSpinBox(this);
    coreIn->setRange(-50, 200);
    coreIn->setValue(80);
    coreIn->setSuffix("°C");
    connect(coreIn, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this]() {
        m_tireData[0].coreTemp = coreIn->value();
        updateTireVisualization();
    });

    QDoubleSpinBox* surfIn = new QDoubleSpinBox(this);
    surfIn->setRange(-50, 200);
    surfIn->setValue(100);
    surfIn->setSuffix("°C");
    connect(surfIn, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this]() {
        m_tireData[0].surfaceTemp = surfIn->value();
        updateTireVisualization();
    });

    QDoubleSpinBox* wearIn = new QDoubleSpinBox(this);
    wearIn->setRange(0.0, 1.0);
    wearIn->setSingleStep(0.1);
    wearIn->setValue(0.0);
    connect(wearIn, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this]() {
        m_tireData[0].wearLevel = wearIn->value();
        updateTireVisualization();
    });

    paramsLayout->addRow("Front Left Core Temp:", coreIn);
    paramsLayout->addRow("Front Left Surf Temp:", surfIn);
    paramsLayout->addRow("Front Left Wear:", wearIn);

    // Controls for tire position
    auto* posGroup = new QGroupBox("Tire Position", this);
    auto* posLayout = new QFormLayout(posGroup);

    QDoubleSpinBox* xInput = new QDoubleSpinBox(this);
    xInput->setRange(-5, 5);
    xInput->setValue(0.8);  // typical front tire position
    connect(xInput, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this]() {
        m_tireData[0].position.setX(xInput->value());
        updateTireVisualization();
    });

    QDoubleSpinBox* zInput = new QDoubleSpinBox(this);
    zInput->setRange(-5, 5);
    zInput->setValue(0.0);
    connect(zInput, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this]() {
        m_tireData[0].position.setZ(zInput->value());
        updateTireVisualization();
    });

    posLayout->addRow("X Position (m):", xInput);
    posLayout->addRow("Z Position (m):", zInput);

    // Add widgets to main layout
    mainLayout->addWidget(m_chart3D, 1);
    mainLayout->addWidget(paramsGroup);
    mainLayout->addWidget(posGroup);
    mainLayout->addWidget(m_statusLabel);

    // Initialize with default data
    for (int i = 0; i < 4; i++) {
        m_tireData[i].position = QVector3D(
            i < 2 ? 0.8 : -0.8,  // FL/FR vs RL/RR
            0,
            i % 2 == 0 ? 0.6 : -0.6  // Left/Right
        );
        m_tireData[i].dimensions = QVector3D(0.3, 0.1, 0.9);  // width, height, depth
        updateTireVisualization();
    }
}

void TireThermalWidget::updateTireData(int wheelIndex, float coreTemp, float surfaceTemp, float wearLevel,
                                       const QVector3D& position, const QVector3D& dimension) {
    if (wheelIndex >= 0 && wheelIndex < 4) {
        m_tireData[wheelIndex].coreTemp = coreTemp;
        m_tireData[wheelIndex].surfaceTemp = surfaceTemp;
        m_tireData[wheelIndex].wearLevel = wearLevel;
        m_tireData[wheelIndex].position = position;
        m_tireData[wheelIndex].dimensions = dimension;
        emit tireDataUpdated(wheelIndex);
    }
}

void TireThermalWidget::updateAllTires(float flCore, float flSurface, float flWear,
                                      float frCore, float frSurface, float frWear,
                                      float rlCore, float rlSurface, float rlWear,
                                      float rrCore, float rrSurface, float rrWear) {
    updateTireData(0, flCore, flSurface, flWear, m_tireData[0].position, m_tireData[0].dimensions);
    updateTireData(1, frCore, frSurface, frWear, m_tireData[1].position, m_tireData[1].dimensions);
    updateTireData(2, rlCore, rlSurface, rlWear, m_tireData[2].position, m_tireData[2].dimensions);
    updateTireData(3, rrCore, rrSurface, rrWear, m_tireData[3].position, m_tireData[3].dimensions);
}

void TireThermalWidget::updateTireVisualization() {
    // Update the 3D scatter series with tire temperature data
    // Plot points based on temperature across the tire surface

    // For each tire, plot a grid of points colored by temperature
    // This is a simplified visualization - real impl would use actual mesh data

    m_statusLabel->setText(QString("Tire: Core %1°C, Surf %2°C, Wear %3%")
        .arg(m_tireData[0].coreTemp, 0, 'f', 1)
        .arg(m_tireData[0].surfaceTemp, 0, 'f', 1)
        .arg(m_tireData[0].wearLevel, 0, 'f', 1));

    // Clear and rebuild series data
    for (int i = 0; i < 4; i++) {
        m_tireSeries[i]->clear();

        // Generate temperature points across the tire surface
        int resolution = 10;
        for (int x = 0; x <= resolution; x++) {
            for (int z = 0; z <= resolution; z++) {
                // Sample position on tire surface
                float u = float(x) / float(resolution);
                float w = float(z) / float(resolution);

                // Calculate temperature at this point (simplified interpolation)
                float core = m_tireData[i].coreTemp;
                float surf = m_tireData[i].surfaceTemp;
                float wear = m_tireData[i].wearLevel;

                // Temperature varies across tire - inner warmer, outer cooler
                float tempVariation = 10.0 * (0.5 - (float(x) - 5.0) / 10.0 * (float(z) - 5.0) / 10.0);
                float localTemp = surf + tempVariation;

                // Position on tire surface
                float xPos = m_tireData[i].position.x() + (u - 0.5) * m_tireData[i].dimensions.x();
                float yPos = localTemp;  // Temperature as height
                float zPos = m_tireData[i].position.z() + (w - 0.5) * m_tireData[i].dimensions.z();

                m_tireSeries[i]->append(QVector3D(xPos, yPos, zPos), localTemp);
            }
        }
    }
}

} // namespace ks