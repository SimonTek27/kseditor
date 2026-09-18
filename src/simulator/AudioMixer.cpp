#include "AudioMixer.h"
#include <algorithm>
#include <cmath>

namespace ks::sim {

static constexpr float PI = 3.14159265f;

AudioMixer::AudioMixer() = default;

int AudioMixer::addChannel(const std::string& name)
{
    if (static_cast<int>(m_channels.size()) >= MAX_CHANNELS)
        return -1;

    int id = static_cast<int>(m_channels.size());
    Channel ch;
    ch.name = name;
    ch.buffer.resize(m_maxFrames * 2, 0.0f);
    m_channels.push_back(std::move(ch));
    return id;
}

void AudioMixer::removeChannel(int id)
{
    if (!validId(id)) return;
    m_channels.erase(m_channels.begin() + id);
}

void AudioMixer::clearChannels()
{
    m_channels.clear();
}

void AudioMixer::setVolume(int id, float volume)
{
    if (validId(id)) m_channels[id].volume = std::max(0.0f, std::min(2.0f, volume));
}

void AudioMixer::setPitch(int id, float pitch)
{
    if (validId(id)) m_channels[id].pitch = std::max(0.1f, std::min(4.0f, pitch));
}

void AudioMixer::setPan(int id, float pan)
{
    if (validId(id)) m_channels[id].pan = std::max(-1.0f, std::min(1.0f, pan));
}

void AudioMixer::setBypassed(int id, bool bypassed)
{
    if (validId(id)) m_channels[id].bypassed = bypassed;
}

float AudioMixer::volume(int id) const
{
    return validId(id) ? m_channels[id].volume : 0.0f;
}

float AudioMixer::pitch(int id) const
{
    return validId(id) ? m_channels[id].pitch : 1.0f;
}

float AudioMixer::pan(int id) const
{
    return validId(id) ? m_channels[id].pan : 0.0f;
}

bool AudioMixer::isBypassed(int id) const
{
    return validId(id) ? m_channels[id].bypassed : true;
}

int AudioMixer::channelCount() const
{
    return static_cast<int>(m_channels.size());
}

float* AudioMixer::channelBuffer(int id)
{
    return validId(id) ? m_channels[id].buffer.data() : nullptr;
}

int AudioMixer::channelBufferFrames() const
{
    return m_maxFrames;
}

void AudioMixer::setMaxFrames(int frames)
{
    m_maxFrames = frames;
    for (auto& ch : m_channels) {
        ch.buffer.resize(m_maxFrames * 2, 0.0f);
    }
}

void AudioMixer::mix(float* output, int frames, int outputChannels, int outputSampleRate)
{
    for (int i = 0; i < frames * outputChannels; ++i)
        output[i] = 0.0f;

    for (const auto& ch : m_channels) {
        if (ch.bypassed) continue;

        float vol = ch.volume;
        if (vol < 0.0001f) continue;

        float panAngle = (ch.pan + 1.0f) * PI * 0.25f;
        float gainL = std::cos(panAngle) * vol;
        float gainR = std::sin(panAngle) * vol;

        if (outputChannels == 2) {
            for (int i = 0; i < frames; ++i) {
                float sampleL = ch.buffer[i * 2];
                float sampleR = ch.buffer[i * 2 + 1];
                output[i * 2]     += sampleL * gainL + sampleR * gainR * 0.1f;
                output[i * 2 + 1] += sampleR * gainR + sampleL * gainL * 0.1f;
            }
        } else {
            for (int i = 0; i < frames; ++i) {
                float sample = ch.buffer[i * 2] * 0.5f + ch.buffer[i * 2 + 1] * 0.5f;
                float mixed = sample * vol;
                for (int ch_idx = 0; ch_idx < outputChannels; ++ch_idx)
                    output[i * outputChannels + ch_idx] += mixed;
            }
        }
    }

    for (int i = 0; i < frames * outputChannels; ++i)
        output[i] = std::max(-1.0f, std::min(1.0f, output[i]));
}

} // namespace ks::sim
