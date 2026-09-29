#pragma once

// Qt-free capture counterpart of WASAPIOutput: replaces the QAudioSource /
// QAudioDevice (Qt Multimedia) capture layer used by AudioCore, AudioTools,
// RPMRecorder and Studio. The Qt code pulled PCM from a QIODevice readyRead
// signal; here the WASAPI capture thread pushes frames into a callback.
// Data is always float32 (WASAPI shared-mode mix format).

#include "AudioFormat.h"

#include <functional>
#include <memory>
#include <string>

namespace ks::audio {

using AudioCaptureCallback =
    std::function<void(const float* input, int frames, const AudioFormat& fmt)>;

class WASAPIInput {
public:
    WASAPIInput();
    ~WASAPIInput();

    WASAPIInput(const WASAPIInput&) = delete;
    WASAPIInput& operator=(const WASAPIInput&) = delete;

    // deviceId empty selects the default capture endpoint
    // (QMediaDevices::defaultAudioInput()); otherwise the AudioDeviceInfo.id.
    // sampleRate/channels > 0 override the mix format, like the Qt code did
    // with QAudioFormat; 0 keeps the device mix format.
    bool initialize(int sampleRate = 0, int channels = 0, int bufferMs = 20,
                    const std::string& deviceId = {});
    void shutdown();
    void start();
    void stop();

    void setCaptureCallback(AudioCaptureCallback cb);

    bool isInitialized() const;
    bool isRunning() const;
    AudioFormat format() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace ks::audio
