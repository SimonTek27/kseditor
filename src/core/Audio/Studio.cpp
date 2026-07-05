#include "KSAudioCore.h"
#include <QMutexLocker>
#include <QMediaDevices>

namespace ks {
namespace audio {

Studio::Studio(QObject* parent)
    : QObject(parent)
{
}

Studio::~Studio() {
    closeOutput();
    closeInput();
}

bool Studio::openOutput(const QAudioFormat& fmt) {
    QMutexLocker locker(&m_mutex);
    if (m_audioOut) {
        closeOutput();
    }
    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    // No parent: we manage lifetime manually in closeOutput()
    m_audioOut = new QAudioSink(device, fmt);
    if (m_audioOut) {
        m_outputDevice = m_audioOut->start();
        return true;
    }
    return false;
}

void Studio::closeOutput() {
    QMutexLocker locker(&m_mutex);
    if (m_audioOut) {
        m_audioOut->stop();
        delete m_audioOut;
        m_audioOut = nullptr;
        m_outputDevice = nullptr;
    }
}

bool Studio::openInput(const QAudioFormat& fmt) {
    QMutexLocker locker(&m_mutex);
    if (m_audioIn) {
        closeInput();
    }
    QAudioDevice device = QMediaDevices::defaultAudioInput();
    // No parent: we manage lifetime manually in closeInput()
    m_audioIn = new QAudioSource(device, fmt);
    if (m_audioIn) {
        m_inputDevice = m_audioIn->start();
        return true;
    }
    return false;
}

void Studio::closeInput() {
    QMutexLocker locker(&m_mutex);
    if (m_audioIn) {
        m_audioIn->stop();
        delete m_audioIn;
        m_audioIn = nullptr;
        m_inputDevice = nullptr;
    }
}

} // namespace audio
} // namespace ks