#pragma once

#include <QWidget>
#include <QVector3D>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>

#if __has_include(<Q3DScatter>)
#include <Q3DScatter>
#include <QScatter3DSeries>
#include <Q3DTheme>
#include <Q3DCamera>
#define HAS_3D 1
#else
#define HAS_3D 0
#endif

namespace ks {

#if HAS_3D
class CfdWidget : public QWidget {
    Q_OBJECT
public:
    explicit CfdWidget(QWidget* parent = nullptr);
    ~CfdWidget();

    void setCfdData(const QVector<float>& pressures, int gridSize);
    void setCarParameters(const QVector3D& position, float speed, float slipAngle);
    void setWindParameters(const QVector3D& direction, float speed, float density);

signals:
    void carParametersChanged(const QVector3D& position, float speed, float slipAngle);
    void windParametersChanged(const QVector3D& direction, float speed, float density);

private slots:
    void onResolutionChanged(int index);
    void onPreviewModeChanged(int index);

private:
    void buildUI();

    Q3DScatter* m_chart3D = nullptr;
    QScatter3DSeries* m_pressureSeries = nullptr;

    QComboBox* m_resolutionCombo = nullptr;
    QComboBox* m_previewModeCombo = nullptr;
    QDoubleSpinBox* m_speedInput = nullptr;
    QDoubleSpinBox* m_slipAngleInput = nullptr;
    QPushButton* m_runBtn = nullptr;
    QPushButton* m_resetBtn = nullptr;
    QLabel* m_statusLabel = nullptr;

    QVector3D m_carPosition;
    QVector3D m_windDirection;
    float m_speed = 150.0f;
    float m_slipAngle = 0.0f;
    float m_windSpeed = 0.0f;
    float m_airDensity = 1.225f;

    int m_gridResolution = 64;
    QVector<float> m_pressures;
};
#else
class CfdWidget : public QWidget {
    Q_OBJECT
public:
    explicit CfdWidget(QWidget* parent = nullptr) : QWidget(parent) {}
    ~CfdWidget() override = default;
    void setCfdData(const QVector<float>&, int) {}
    void setCarParameters(const QVector3D&, float, float) {}
    void setWindParameters(const QVector3D&, float, float) {}
};
#endif

} // namespace ks