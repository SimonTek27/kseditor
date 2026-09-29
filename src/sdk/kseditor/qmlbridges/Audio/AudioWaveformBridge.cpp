#include "AudioWaveformBridge.h"
#include <QFileInfo>

namespace ks {
namespace audio {

AudioWaveformBridge::AudioWaveformBridge(QObject* parent)
    : QObject(parent)
{
}

AudioWaveformBridge::~AudioWaveformBridge() = default;

void AudioWaveformBridge::loadFile(const QString& path)
{
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        m_hasData = false;
        emit loadFailed(path, "File not found");
        return;
    }
    m_hasData = true;
    emit fileLoaded(path);
}

} // namespace audio
} // namespace ks
