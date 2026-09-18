#pragma once

#include <vector>
#include <string>
#include <cmath>

namespace ks::sim {

class AudioMixer {
public:
    static constexpr int MAX_CHANNELS = 32;

    AudioMixer();

    int addChannel(const std::string& name = "");
    void removeChannel(int id);
    void clearChannels();

    void setVolume(int id, float volume);
    void setPitch(int id, float pitch);
    void setPan(int id, float pan);
    void setBypassed(int id, bool bypassed);

    float volume(int id) const;
    float pitch(int id) const;
    float pan(int id) const;
    bool isBypassed(int id) const;
    int channelCount() const;

    void mix(float* output, int frames, int outputChannels, int outputSampleRate);

    float* channelBuffer(int id);
    int channelBufferFrames() const;

    void setMaxFrames(int frames);

private:
    struct Channel {
        std::string name;
        float volume = 1.0f;
        float pitch = 1.0f;
        float pan = 0.0f;
        bool bypassed = false;
        std::vector<float> buffer;
    };

    bool validId(int id) const { return id >= 0 && id < static_cast<int>(m_channels.size()); }

    std::vector<Channel> m_channels;
    int m_maxFrames = 2048;
};

} // namespace ks::sim
