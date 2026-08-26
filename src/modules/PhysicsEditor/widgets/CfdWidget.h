#pragma once

#include <QWidget>
#include <QChart>
#include <QChartView>
#include <Q3DBubbleSeries>
#include <Q3DScatterTheme>

namespace ks {

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

    QChart3D* m_chart3D = nullptr;
    QScatterSeries* m_pressureSeries = nullptr;
    QAbstract3DItemModel* m_model = nullptr;

    QComboBox* m_resolutionCombo = nullptr;
    QComboBox* m_previewModeCombo = nullptr;
    QLabel* m_statusLabel = nullptr;

    int m_gridResolution = 64;
    QVector<float> m_pressures;
};

} // namespace ks