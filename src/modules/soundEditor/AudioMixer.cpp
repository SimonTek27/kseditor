#include "AudioMixer.h"
#include <QVBoxLayout>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QUuid>

namespace ks { namespace audio {

// ============================================================================
// VCAMixerSystem
// ============================================================================

void VCAMixerSystem::createVCA(const QString& name)
{
    QString id = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    VCA vca;
    vca.id = id;
    vca.name = name;
    vca.volume = 1.0f;
    vca.muted = false;
    vca.solo = false;
    m_vcas[id] = vca;
    emit vcaCreated(id);
}

void VCAMixerSystem::deleteVCA(const QString& vcaId)
{
    if (m_vcas.contains(vcaId)) {
        m_vcas.remove(vcaId);
        emit vcaDeleted(vcaId);
    }
}

VCAMixerSystem::VCA VCAMixerSystem::getVCA(const QString& vcaId) const
{
    return m_vcas.value(vcaId);
}

void VCAMixerSystem::assignToVCA(const QString& channelId, const QString& vcaId)
{
    if (m_vcas.contains(vcaId)) {
        if (!m_vcas[vcaId].assignedChannels.contains(channelId)) {
            m_vcas[vcaId].assignedChannels.append(channelId);
        }
    }
}

void VCAMixerSystem::unassignFromVCA(const QString& channelId, const QString& vcaId)
{
    if (m_vcas.contains(vcaId)) {
        m_vcas[vcaId].assignedChannels.removeAll(channelId);
    }
}

void VCAMixerSystem::setChannelVolume(const QString& channelId, float volume)
{
    if (!m_channels.contains(channelId)) {
        MixerChannel ch;
        ch.id = channelId;
        ch.name = channelId;
        ch.volume = 1.0f;
        ch.pan = 0.0f;
        ch.muted = false;
        ch.solo = false;
        m_channels[channelId] = ch;
    }
    m_channels[channelId].volume = qBound(0.0f, volume, 1.0f);
    emit volumeChanged(channelId, m_channels[channelId].volume);
}

float VCAMixerSystem::getChannelVolume(const QString& channelId) const
{
    if (m_channels.contains(channelId)) {
        return m_channels[channelId].volume;
    }
    return 1.0f;
}

void VCAMixerSystem::setChannelPan(const QString& channelId, float pan)
{
    if (m_channels.contains(channelId)) {
        m_channels[channelId].pan = qBound(-1.0f, pan, 1.0f);
    }
}

void VCAMixerSystem::setChannelMute(const QString& channelId, bool mute)
{
    if (m_channels.contains(channelId)) {
        m_channels[channelId].muted = mute;
    }
}

void VCAMixerSystem::setChannelSolo(const QString& channelId, bool solo)
{
    if (m_channels.contains(channelId)) {
        m_channels[channelId].solo = solo;
    }
}

void VCAMixerSystem::saveSnapshot(const QString& name)
{
    MixerSnapshot snapshot;
    snapshot.id = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    snapshot.name = name;
    snapshot.fadeTime = 0;

    for (const auto& ch : m_channels) {
        snapshot.channelVolumes[ch.id] = ch.volume;
        snapshot.mutes[ch.id] = ch.muted;
        snapshot.solos[ch.id] = ch.solo;
    }
    for (const auto& vca : m_vcas) {
        snapshot.vcaVolumes[vca.id] = vca.volume;
    }

    m_snapshots.append(snapshot);
    emit snapshotCreated(snapshot.id);
}

void VCAMixerSystem::loadSnapshot(const QString& snapshotId)
{
    for (const auto& snapshot : m_snapshots) {
        if (snapshot.id == snapshotId) {
            for (auto it = snapshot.channelVolumes.constBegin(); it != snapshot.channelVolumes.constEnd(); ++it) {
                if (m_channels.contains(it.key())) {
                    m_channels[it.key()].volume = it.value();
                }
            }
            for (auto it = snapshot.mutes.constBegin(); it != snapshot.mutes.constEnd(); ++it) {
                if (m_channels.contains(it.key())) {
                    m_channels[it.key()].muted = it.value();
                }
            }
            for (auto it = snapshot.solos.constBegin(); it != snapshot.solos.constEnd(); ++it) {
                if (m_channels.contains(it.key())) {
                    m_channels[it.key()].solo = it.value();
                }
            }
            for (auto it = snapshot.vcaVolumes.constBegin(); it != snapshot.vcaVolumes.constEnd(); ++it) {
                if (m_vcas.contains(it.key())) {
                    m_vcas[it.key()].volume = it.value();
                }
            }
            emit snapshotLoaded(snapshotId);
            return;
        }
    }
}

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
