#pragma once

#include <QObject>
#include <QString>
#include <cstdint>
#include "FmodFunctionTable.h"

namespace ks::engine::audio {

// ============================================================================
// FmodRuntimeWrapper - High-level wrapper for FMOD runtime
// Functions are now linked statically via fmod64.lib / fmodstudio64.lib
// ============================================================================

class FmodRuntimeWrapper : public QObject {
    Q_OBJECT

public:
    static FmodRuntimeWrapper* instance();

    explicit FmodRuntimeWrapper(QObject* parent = nullptr);
    ~FmodRuntimeWrapper();

    // --- Initialization ---
    bool initialize();      // Create + Init system
    void shutdown();        // Close + Release system
    bool isInitialized() const { return m_initialized; }

    // --- Studio initialization ---
    bool initializeStudio();
    void shutdownStudio();
    bool isStudioInitialized() const { return m_studioInitialized; }

    // --- System lifecycle ---
    bool createSystem();
    bool initSystem(int maxChannels = 512, FMOD_INITFLAGS flags = FMOD_INIT_NORMAL);
    bool closeSystem();
    bool updateSystem();
    bool releaseSystem();

    // --- System info ---
    bool getVersion(uint32_t& version, uint32_t& headerVersion) const;
    bool setSoftwareFormat(int sampleRate, int speakermode, int numRawSpeakers);
    bool setDSPBufferSize(int bufferLength, int numBuffers);

    // --- Sound management ---
    FMOD_SOUND* createSound(const char* name, FMOD_MODE mode = FMOD_DEFAULT, void* exinfo = nullptr);
    FMOD_SOUND* createStream(const char* name, FMOD_MODE mode = FMOD_DEFAULT, void* exinfo = nullptr);
    bool releaseSound(FMOD_SOUND* sound);
    bool getSoundLength(FMOD_SOUND* sound, uint32_t& length, int timeunit = 0);

    // --- Playback ---
    FMOD_CHANNEL* playSound(FMOD_SOUND* sound, FMOD_CHANNELGROUP* channelGroup = nullptr, bool paused = false);
    bool channelStop(FMOD_CHANNEL* channel);
    bool channelSetPaused(FMOD_CHANNEL* channel, bool paused);
    bool channelSetVolume(FMOD_CHANNEL* channel, float volume);
    bool channelGetVolume(FMOD_CHANNEL* channel, float& volume);
    bool channelSetFrequency(FMOD_CHANNEL* channel, float frequency);
    bool channelGetFrequency(FMOD_CHANNEL* channel, float& frequency);
    bool channelSetPitch(FMOD_CHANNEL* channel, float pitch);
    bool channelGetPitch(FMOD_CHANNEL* channel, float& pitch);
    bool channelSetPan(FMOD_CHANNEL* channel, float pan);
    bool channelSetPosition(FMOD_CHANNEL* channel, uint32_t position, int timeunit = 0);
    bool channelGetPosition(FMOD_CHANNEL* channel, uint32_t& position, int timeunit = 0);
    bool channelIsPlaying(FMOD_CHANNEL* channel, bool& isPlaying);

    // --- 3D Audio ---
    bool channelSet3DAttributes(FMOD_CHANNEL* channel, const FMOD_VECTOR* pos, const FMOD_VECTOR* vel);
    bool channelSet3DMinMaxDistance(FMOD_CHANNEL* channel, float minDist, float maxDist);
    bool channelSetMode(FMOD_CHANNEL* channel, FMOD_MODE mode);
    bool set3DListenerAttributes(int listener, const FMOD_VECTOR* pos, const FMOD_VECTOR* vel, const FMOD_VECTOR* forward, const FMOD_VECTOR* up);
    bool set3DRolloffFactor(float rolloff);

    // --- Channel Groups ---
    FMOD_CHANNELGROUP* createChannelGroup(const char* name);
    bool releaseChannelGroup(FMOD_CHANNELGROUP* channelGroup);
    bool channelGroupSetVolume(FMOD_CHANNELGROUP* channelGroup, float volume);
    bool channelGroupStop(FMOD_CHANNELGROUP* channelGroup);

    // --- FMOD Studio ---
    bool createStudioSystem();
    bool initStudioSystem(int maxChannels = 512, unsigned int flags = FMOD_INIT_NORMAL);
    bool closeStudioSystem();
    bool updateStudioSystem();
    bool releaseStudioSystem();

    FMOD_STUDIO_BANK* loadBank(const char* filename, unsigned int flags = 0);
    bool releaseBank(FMOD_STUDIO_BANK* bank);
    bool getBankLoadingState(FMOD_STUDIO_BANK* bank, FMOD_STUDIO_LOADING_STATE& state);

    FMOD_STUDIO_EVENTDESCRIPTION* getEventDescription(const char* path);
    FMOD_STUDIO_EVENTINSTANCE* createEventInstance(FMOD_STUDIO_EVENTDESCRIPTION* eventDescription);
    bool releaseEventInstance(FMOD_STUDIO_EVENTINSTANCE* eventInstance);
    bool startEventInstance(FMOD_STUDIO_EVENTINSTANCE* eventInstance);
    bool stopEventInstance(FMOD_STUDIO_EVENTINSTANCE* eventInstance, FMOD_STUDIO_STOP_MODE mode = FMOD_STUDIO_STOP_ALLOWFADEOUT);
    bool eventInstanceSetVolume(FMOD_STUDIO_EVENTINSTANCE* eventInstance, float volume);
    bool eventInstanceSetParameterValue(FMOD_STUDIO_EVENTINSTANCE* eventInstance, const char* name, float value);

    FMOD_STUDIO_BUS* getBus(const char* path);
    bool busSetVolume(FMOD_STUDIO_BUS* bus, float volume);
    bool busGetVolume(FMOD_STUDIO_BUS* bus, float& volume);
    bool busStopAllEvents(FMOD_STUDIO_BUS* bus, FMOD_STUDIO_STOP_MODE mode = FMOD_STUDIO_STOP_ALLOWFADEOUT);

    FMOD_STUDIO_VCA* getVCA(const char* path);
    bool vcaSetVolume(FMOD_STUDIO_VCA* vca, float volume);
    bool vcaGetVolume(FMOD_STUDIO_VCA* vca, float& volume);

    // --- Raw system access ---
    FMOD_SYSTEM* system() const { return m_system; }
    FMOD_STUDIO_SYSTEM* studioSystem() const { return m_studioSystem; }

    // --- String helpers ---
    static const char* resultToString(FMOD_RESULT result);

signals:
    void initialized();
    void shutdown();
    void error(const QString& message);

private:
    bool m_initialized = false;
    bool m_studioInitialized = false;

    FMOD_SYSTEM* m_system = nullptr;
    FMOD_STUDIO_SYSTEM* m_studioSystem = nullptr;
};

} // namespace ks::engine::audio
