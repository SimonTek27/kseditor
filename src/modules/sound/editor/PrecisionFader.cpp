#include "PrecisionFader.h"
#include <QVBoxLayout>

namespace ks { namespace audio {

// ============================================================================
// PrecisionFader
// ============================================================================

void PrecisionFader::setupUI()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(2);

    m_labelWidget = new QLabel(this);
    layout->addWidget(m_labelWidget);

    m_slider = new QSlider(Qt::Vertical, this);
    m_slider->setRange(0, 1000);
    m_slider->setValue(500);
    layout->addWidget(m_slider);

    m_spinBox = new QDoubleSpinBox(this);
    m_spinBox->setRange(m_min, m_max);
    m_spinBox->setSingleStep(m_step);
    m_spinBox->setDecimals(m_decimals);
    m_spinBox->setSuffix(m_suffix);
    layout->addWidget(m_spinBox);

    connect(m_spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PrecisionFader::onSpinBoxChanged);
    connect(m_slider, &QSlider::valueChanged, this, &PrecisionFader::onSliderChanged);
}

void PrecisionFader::setRange(double min, double max)
{
    m_min = min;
    m_max = max;
    if (m_spinBox) {
        m_spinBox->setRange(min, max);
    }
}

void PrecisionFader::setValue(double value)
{
    m_value = value;
    if (m_spinBox) {
        m_spinBox->blockSignals(true);
        m_spinBox->setValue(value);
        m_spinBox->blockSignals(false);
    }
    if (m_slider) {
        m_slider->blockSignals(true);
        int sliderVal = static_cast<int>((value - m_min) / (m_max - m_min) * 1000);
        m_slider->setValue(qBound(0, sliderVal, 1000));
        m_slider->blockSignals(false);
    }
}

void PrecisionFader::setDecimals(int decimals)
{
    m_decimals = decimals;
    if (m_spinBox) m_spinBox->setDecimals(decimals);
}

void PrecisionFader::setStep(double step)
{
    m_step = step;
    if (m_spinBox) m_spinBox->setSingleStep(step);
}

void PrecisionFader::setLabel(const QString& label)
{
    if (m_labelWidget) m_labelWidget->setText(label);
}

void PrecisionFader::setSuffix(const QString& suffix)
{
    m_suffix = suffix;
    if (m_spinBox) m_spinBox->setSuffix(suffix);
}

void PrecisionFader::updateFromSpinBox()
{
    onSpinBoxChanged(m_spinBox ? m_spinBox->value() : m_value);
}

void PrecisionFader::updateFromSlider()
{
    onSliderChanged(m_slider ? m_slider->value() : 500);
}

void PrecisionFader::onSpinBoxChanged(double value)
{
    m_value = value;
    if (m_slider) {
        m_slider->blockSignals(true);
        int sliderVal = static_cast<int>((value - m_min) / (m_max - m_min) * 1000);
        m_slider->setValue(qBound(0, sliderVal, 1000));
        m_slider->blockSignals(false);
    }
    emit valueChanged(value);
}

void PrecisionFader::onSliderChanged(int value)
{
    double val = m_min + (m_max - m_min) * value / 1000.0;
    m_value = val;
    if (m_spinBox) {
        m_spinBox->blockSignals(true);
        m_spinBox->setValue(val);
        m_spinBox->blockSignals(false);
    }
    emit valueChanged(val);
}

}} // ks::audio
