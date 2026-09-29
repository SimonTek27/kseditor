#pragma once

// Qt-free replacement for QAudioFormat (Qt Multimedia). Plain value type.
// Call sites keep the Qt spellings: AudioFormat::Float reads exactly like the
// old QAudioFormat::Float, sampleRate()/channelCount()/sampleFormat() keep
// their method names, and the public fields (rate/channels/bufferFrames) are
// what the WASAPI backend consumes.

namespace ks::audio {

struct AudioFormat {
    // Mirrors QAudioFormat::SampleFormat; unscoped so call sites write
    // AudioFormat::Float / AudioFormat::Int16 like the old QAudioFormat::Float.
    enum SampleFormat { UInt8, Int16, Int32, Float };

    int rate = 44100;
    int channels = 2;
    int bufferFrames = 0;
    SampleFormat format = Float;

    int sampleRate() const { return rate; }
    void setSampleRate(int v) { rate = v; }
    int channelCount() const { return channels; }
    void setChannelCount(int v) { channels = v; }
    SampleFormat sampleFormat() const { return format; }
    void setSampleFormat(SampleFormat v) { format = v; }

    int bytesPerSample() const
    {
        switch (format) {
        case UInt8: return 1;
        case Int16: return 2;
        case Float:
        case Int32: return 4;
        }
        return 4;
    }
    int bytesPerFrame() const { return bytesPerSample() * channels; }
    int bytesForDuration(long long usecs) const
    {
        return static_cast<int>((usecs * rate / 1000000LL) * bytesPerFrame());
    }
    long long durationForBytes(long long bytes) const
    {
        const int frameSize = bytesPerFrame();
        if (frameSize <= 0 || rate <= 0) return 0;
        return (bytes / frameSize) * 1000000LL / rate;
    }

    bool isValid() const { return rate > 0 && channels > 0; }

    bool operator==(const AudioFormat& o) const
    {
        return rate == o.rate && channels == o.channels &&
               bufferFrames == o.bufferFrames && format == o.format;
    }
    bool operator!=(const AudioFormat& o) const { return !(*this == o); }
};

} // namespace ks::audio
