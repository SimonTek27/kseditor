#include "AudioMixer.h"
#include <QDebug>

namespace ks { namespace audio {

Mixer::Mixer(KSAudioVSTManager* manager, QObject* parent)
    : QObject(parent), m_manager(manager) {
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &Mixer::onTimer);
}

Mixer::~Mixer() {
    stop();
}

void Mixer::start(QAudioSink* output, const QAudioFormat& fmt, int blockSize) {
    m_output = output;
    m_format = fmt;
    m_blockSize = blockSize > 0 ? blockSize : 512;
    m_outBuffer.resize(m_blockSize * fmt.channelCount() * 4);
    m_running.store(true);
    if (m_timer) m_timer->start(10);
    qDebug() << "Mixer: started" << fmt.sampleRate() << "Hz" << fmt.channelCount() << "ch";
}

void Mixer::stop() {
    m_running.store(false);
    if (m_timer) m_timer->stop();
    m_output = nullptr;
    m_audioDevice = nullptr;
}

void Mixer::onTimer() {
    if (!m_running.load() || !m_output) return;
}

}} // namespace ks::audio
