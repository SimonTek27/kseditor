#include "TireThermalWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QSlider>

#if HAS_TIRE_3D

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

    m_chart3D = new Q3DScatter();
    m_chart3D->activeTheme()->setType(Q3DTheme::ThemeChocolate);
    m_chart3D->scene()->activeCamera()->setCameraPreset(Q3DScene::CameraPresetIsometricRight);

    QString tireNames[4] = {"FL", "FR", "RL", "RR"};
    QColor tireColors[4] = {Qt::red, Qt::blue, Qt::green, Qt::magenta};

    for (int i = 0; i < 4; i++) {
        m_tireSeries[i] = new QScatter3DSeries();
        m_tireSeries[i]->setItemSize(0.5f);
        m_tireSeries[i]->setBaseColor(tireColors[i]);
        m_chart3D->addSeries(m_tireSeries[i]);
    }

    auto* chartContainer = QWidget::createWindowContainer(m_chart3D);
    chartContainer->setMinimumHeight(300);

    auto* paramsGroup = new QGroupBox("Tire Parameters", this);
    auto* paramsLayout = new QFormLayout(paramsGroup);

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

        connect(coreIn, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this, i, coreIn]() {
            m_tireData[i].coreTemp = coreIn->value();
            updateTireVisualization();
        });
        connect(surfIn, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this, i, surfIn]() {
            m_tireData[i].surfaceTemp = surfIn->value();
            updateTireVisualization();
        });
        connect(wearIn, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this, i, wearIn]() {
            m_tireData[i].wearLevel = wearIn->value();
            updateTireVisualization();
        });

        tireLayout->addRow("Core Temp:", coreIn);
        tireLayout->addRow("Surf Temp:", surfIn);
        tireLayout->addRow("Wear:", wearIn);

        paramsLayout->addRow(tireGroup);
    }

    auto* posGroup = new QGroupBox("Tire Position", this);
    auto* posLayout = new QFormLayout(posGroup);

    QDoubleSpinBox* xInput = new QDoubleSpinBox(this);
    xInput->setRange(-5, 5);
    xInput->setValue(0.8);
    connect(xInput, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this, xInput]() {
        m_tireData[0].position.setX(xInput->value());
        updateTireVisualization();
    });

    QDoubleSpinBox* zInput = new QDoubleSpinBox(this);
    zInput->setRange(-5, 5);
    zInput->setValue(0.0);
    connect(zInput, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this, zInput]() {
        m_tireData[0].position.setZ(zInput->value());
        updateTireVisualization();
    });

    posLayout->addRow("X Position (m):", xInput);
    posLayout->addRow("Z Position (m):", zInput);

    m_statusLabel = new QLabel("No data", this);

    mainLayout->addWidget(chartContainer, 1);
    mainLayout->addWidget(paramsGroup);
    mainLayout->addWidget(posGroup);
    mainLayout->addWidget(m_statusLabel);

    for (int i = 0; i < 4; i++) {
        m_tireData[i].position = QVector3D(
            i < 2 ? 0.8 : -0.8,
            0,
            i % 2 == 0 ? 0.6 : -0.6
        );
        m_tireData[i].dimensions = QVector3D(0.3, 0.1, 0.9);
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
    m_statusLabel->setText(QString("Tire: Core %1°C, Surf %2°C, Wear %3%")
        .arg(m_tireData[0].coreTemp, 0, 'f', 1)
        .arg(m_tireData[0].surfaceTemp, 0, 'f', 1)
        .arg(m_tireData[0].wearLevel, 0, 'f', 1));

    for (int i = 0; i < 4; i++) {
        QList<QScatterDataItem> items;

        int resolution = 10;
        for (int x = 0; x <= resolution; x++) {
            for (int z = 0; z <= resolution; z++) {
                float u = float(x) / float(resolution);
                float w = float(z) / float(resolution);

                float core = m_tireData[i].coreTemp;
                float surf = m_tireData[i].surfaceTemp;
                float wear = m_tireData[i].wearLevel;

                float tempVariation = 10.0f * (0.5f - (float(x) - 5.0f) / 10.0f * (float(z) - 5.0f) / 10.0f);
                float localTemp = surf + tempVariation;

                float xPos = m_tireData[i].position.x() + (u - 0.5f) * m_tireData[i].dimensions.x();
                float yPos = localTemp;
                float zPos = m_tireData[i].position.z() + (w - 0.5f) * m_tireData[i].dimensions.z();

                items.append(QScatterDataItem(QVector3D(xPos, yPos, zPos)));
            }
        }

        m_tireSeries[i]->dataProxy()->addRows(items);
    }
}

} // namespace ks

#else

// Constructor and destructor are inline in the header when HAS_TIRE_3D is 0

#endif
