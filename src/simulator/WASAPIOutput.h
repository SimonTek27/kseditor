#pragma once

#include <memory>
#include <functional>

namespace ks::sim {

struct AudioFormat {
    int sampleRate = 44100;
    int channels = 2;
    int bufferFrames = 0;
};

using AudioRenderCallback = std::function<void(float* output, int frames, const AudioFormat& fmt)>;

class WASAPIOutput {
public:
    WASAPIOutput();
    ~WASAPIOutput();

    WASAPIOutput(const WASAPIOutput&) = delete;
    WASAPIOutput& operator=(const WASAPIOutput&) = delete;

    bool initialize(int sampleRate = 0, int channels = 2, int bufferMs = 20);
    void shutdown();
    void start();
    void stop();

    void setRenderCallback(AudioRenderCallback cb);

    bool isInitialized() const;
    AudioFormat format() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace ks::sim
