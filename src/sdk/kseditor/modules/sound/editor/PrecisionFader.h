#pragma once

#include <QWidget>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QLabel>

namespace ks { namespace audio {

class PrecisionFader : public QWidget
{
    Q_OBJECT
public:
    explicit PrecisionFader(QWidget* parent = nullptr) : QWidget(parent) { setupUI(); }
    ~PrecisionFader() {}

    void setRange(double min, double max);
    void setValue(double value);
    double value() const { return m_value; }
    void setDecimals(int decimals);
    void setStep(double step);
    void setLabel(const QString& label);
    void setSuffix(const QString& suffix);

signals:
    void valueChanged(double value);

private slots:
    void onSpinBoxChanged(double value);
    void onSliderChanged(int value);

private:
    void setupUI();
    void updateFromSpinBox();
    void updateFromSlider();

    double m_value = 0.0;
    double m_min = -96.0;
    double m_max = 6.0;
    double m_step = 0.01;
    int m_decimals = 2;
    QString m_suffix = " dB";

    QSlider* m_slider = nullptr;
    QDoubleSpinBox* m_spinBox = nullptr;
    QLabel* m_labelWidget = nullptr;
};

}} // namespace ks
