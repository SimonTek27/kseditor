#include "FmodRuntimeWrapper.h"
#include <QDebug>

namespace ks::engine::audio {

// ============================================================================
// Singleton
// ============================================================================

FmodRuntimeWrapper* FmodRuntimeWrapper::instance() {
    static FmodRuntimeWrapper inst;
    return &inst;
}

// ============================================================================
// Construction / Destruction
// ============================================================================

FmodRuntimeWrapper::FmodRuntimeWrapper(QObject* parent)
    : QObject(parent) {
}

FmodRuntimeWrapper::~FmodRuntimeWrapper() {
    shutdown();
}

// ============================================================================
// Initialization
// ============================================================================

bool FmodRuntimeWrapper::initialize() {
    if (m_initialized) return true;

    if (!createSystem()) return false;
    if (!initSystem()) return false;

    m_initialized = true;
    qDebug() << "FmodRuntimeWrapper: Initialized";
    emit initialized();
    return true;
}

void FmodRuntimeWrapper::shutdown() {
    if (!m_initialized) return;

    if (m_studioInitialized) shutdownStudio();
    closeSystem();
    releaseSystem();

    m_initialized = false;
    qDebug() << "FmodRuntimeWrapper: Shutdown";
    emit shutdown();
}

bool FmodRuntimeWrapper::initializeStudio() {
    if (m_studioInitialized) return true;
    if (!m_initialized) {
        if (!initialize()) return false;
    }

    if (!createStudioSystem()) return false;
    if (!initStudioSystem()) return false;

    m_studioInitialized = true;
    qDebug() << "FmodRuntimeWrapper: Studio initialized";
    return true;
}

void FmodRuntimeWrapper::shutdownStudio() {
    if (!m_studioInitialized) return;

    closeStudioSystem();
    releaseStudioSystem();

    m_studioInitialized = false;
    qDebug() << "FmodRuntimeWrapper: Studio shutdown";
}

// ============================================================================
// System lifecycle (direct calls)
// ============================================================================

bool FmodRuntimeWrapper::createSystem() {
    if (m_system) return true;

    FMOD_RESULT result = FMOD_System_Create(&m_system, 0x00020903);
    if (result != FMOD_OK) {
        emit error(QString("FMOD_System_Create failed: %1").arg(resultToString(result)));
        return false;
    }

    qDebug() << "FmodRuntimeWrapper: System created";
    return true;
}

bool FmodRuntimeWrapper::initSystem(int maxChannels, FMOD_INITFLAGS flags) {
    if (!m_system) {
        if (!createSystem()) return false;
    }

    FMOD_RESULT result = FMOD_System_Init(m_system, maxChannels, flags, nullptr);
    if (result != FMOD_OK) {
        emit error(QString("FMOD_System_Init failed: %1").arg(resultToString(result)));
        return false;
    }

    qDebug() << "FmodRuntimeWrapper: System initialized with" << maxChannels << "channels";
    return true;
}

bool FmodRuntimeWrapper::closeSystem() {
    if (!m_system) return true;

    FMOD_RESULT result = FMOD_System_Close(m_system);
    if (result != FMOD_OK) {
        qWarning() << "FmodRuntimeWrapper: FMOD_System_Close failed:" << resultToString(result);
        return false;
    }

    qDebug() << "FmodRuntimeWrapper: System closed";
    return true;
}

bool FmodRuntimeWrapper::updateSystem() {
    if (!m_system) return false;
    return FMOD_System_Update(m_system) == FMOD_OK;
}

bool FmodRuntimeWrapper::releaseSystem() {
    if (!m_system) return true;

    FMOD_RESULT result = FMOD_System_Release(m_system);
    if (result != FMOD_OK) {
        qWarning() << "FmodRuntimeWrapper: FMOD_System_Release failed:" << resultToString(result);
        return false;
    }

    m_system = nullptr;
    qDebug() << "FmodRuntimeWrapper: System released";
    return true;
}

// ============================================================================
// System info
// ============================================================================

bool FmodRuntimeWrapper::getVersion(uint32_t& version, uint32_t& headerVersion) const {
    if (!m_system) return false;
    return FMOD_System_GetVersion(m_system, &version, &headerVersion) == FMOD_OK;
}

bool FmodRuntimeWrapper::setSoftwareFormat(int sampleRate, int speakermode, int numRawSpeakers) {
    if (!m_system) return false;
    return FMOD_System_SetSoftwareFormat(m_system, sampleRate, speakermode, numRawSpeakers) == FMOD_OK;
}

bool FmodRuntimeWrapper::setDSPBufferSize(int bufferLength, int numBuffers) {
    if (!m_system) return false;
    return FMOD_System_SetDSPBufferSize(m_system, bufferLength, numBuffers) == FMOD_OK;
}

// ============================================================================
// Sound management
// ============================================================================

FMOD_SOUND* FmodRuntimeWrapper::createSound(const char* name, FMOD_MODE mode, void* exinfo) {
    if (!m_system) return nullptr;
    FMOD_SOUND* sound = nullptr;
    FMOD_RESULT result = FMOD_System_CreateSound(m_system, name, mode, exinfo, &sound);
    if (result != FMOD_OK) {
        qWarning() << "FmodRuntimeWrapper: createSound failed for" << name << ":" << resultToString(result);
        return nullptr;
    }
    return sound;
}

FMOD_SOUND* FmodRuntimeWrapper::createStream(const char* name, FMOD_MODE mode, void* exinfo) {
    if (!m_system) return nullptr;
    FMOD_SOUND* sound = nullptr;
    FMOD_RESULT result = FMOD_System_CreateStream(m_system, name, mode, exinfo, &sound);
    if (result != FMOD_OK) {
        qWarning() << "FmodRuntimeWrapper: createStream failed for" << name << ":" << resultToString(result);
        return nullptr;
    }
    return sound;
}

bool FmodRuntimeWrapper::releaseSound(FMOD_SOUND* sound) {
    if (!sound) return false;
    return FMOD_Sound_Release(sound) == FMOD_OK;
}

bool FmodRuntimeWrapper::getSoundLength(FMOD_SOUND* sound, uint32_t& length, int timeunit) {
    if (!sound) return false;
    return FMOD_Sound_GetLength(sound, &length, timeunit) == FMOD_OK;
}

// ============================================================================
// Playback
// ============================================================================

FMOD_CHANNEL* FmodRuntimeWrapper::playSound(FMOD_SOUND* sound, FMOD_CHANNELGROUP* channelGroup, bool paused) {
    if (!m_system || !sound) return nullptr;
    FMOD_CHANNEL* channel = nullptr;
    FMOD_RESULT result = FMOD_System_PlaySound(m_system, sound, channelGroup, paused ? 1 : 0, &channel);
    if (result != FMOD_OK) {
        qWarning() << "FmodRuntimeWrapper: playSound failed:" << resultToString(result);
        return nullptr;
    }
    return channel;
}

bool FmodRuntimeWrapper::channelStop(FMOD_CHANNEL* channel) {
    if (!channel) return false;
    return FMOD_Channel_Stop(channel) == FMOD_OK;
}

bool FmodRuntimeWrapper::channelSetPaused(FMOD_CHANNEL* channel, bool paused) {
    if (!channel) return false;
    return FMOD_Channel_SetPaused(channel, paused ? 1 : 0) == FMOD_OK;
}

bool FmodRuntimeWrapper::channelSetVolume(FMOD_CHANNEL* channel, float volume) {
    if (!channel) return false;
    return FMOD_Channel_SetVolume(channel, volume) == FMOD_OK;
}

bool FmodRuntimeWrapper::channelGetVolume(FMOD_CHANNEL* channel, float& volume) {
    if (!channel) return false;
    return FMOD_Channel_GetVolume(channel, &volume) == FMOD_OK;
}

bool FmodRuntimeWrapper::channelSetFrequency(FMOD_CHANNEL* channel, float frequency) {
    if (!channel) return false;
    return FMOD_Channel_SetFrequency(channel, frequency) == FMOD_OK;
}

bool FmodRuntimeWrapper::channelGetFrequency(FMOD_CHANNEL* channel, float& frequency) {
    if (!channel) return false;
    return FMOD_Channel_GetFrequency(channel, &frequency) == FMOD_OK;
}

bool FmodRuntimeWrapper::channelSetPitch(FMOD_CHANNEL* channel, float pitch) {
    if (!channel) return false;
    return FMOD_Channel_SetPitch(channel, pitch) == FMOD_OK;
}

bool FmodRuntimeWrapper::channelGetPitch(FMOD_CHANNEL* channel, float& pitch) {
    if (!channel) return false;
    return FMOD_Channel_GetPitch(channel, &pitch) == FMOD_OK;
}

bool FmodRuntimeWrapper::channelSetPan(FMOD_CHANNEL* channel, float pan) {
    if (!channel) return false;
    return FMOD_Channel_SetPan(channel, pan) == FMOD_OK;
}

bool FmodRuntimeWrapper::channelSetPosition(FMOD_CHANNEL* channel, uint32_t position, int timeunit) {
    if (!channel) return false;
    return FMOD_Channel_SetPosition(channel, position, timeunit) == FMOD_OK;
}

bool FmodRuntimeWrapper::channelGetPosition(FMOD_CHANNEL* channel, uint32_t& position, int timeunit) {
    if (!channel) return false;
    return FMOD_Channel_GetPosition(channel, &position, timeunit) == FMOD_OK;
}

bool FmodRuntimeWrapper::channelIsPlaying(FMOD_CHANNEL* channel, bool& isPlaying) {
    if (!channel) return false;
    int playing = 0;
    FMOD_RESULT result = FMOD_Channel_IsPlaying(channel, &playing);
    isPlaying = (playing != 0);
    return result == FMOD_OK;
}

// ============================================================================
// 3D Audio
// ============================================================================

bool FmodRuntimeWrapper::channelSet3DAttributes(FMOD_CHANNEL* channel, const FMOD_VECTOR* pos, const FMOD_VECTOR* vel) {
    if (!channel) return false;
    return FMOD_Channel_Set3DAttributes(channel, pos, vel) == FMOD_OK;
}

bool FmodRuntimeWrapper::channelSet3DMinMaxDistance(FMOD_CHANNEL* channel, float minDist, float maxDist) {
    if (!channel) return false;
    return FMOD_Channel_Set3DMinMaxDistance(channel, minDist, maxDist) == FMOD_OK;
}

bool FmodRuntimeWrapper::channelSetMode(FMOD_CHANNEL* channel, FMOD_MODE mode) {
    if (!channel) return false;
    return FMOD_Channel_SetMode(channel, mode) == FMOD_OK;
}

bool FmodRuntimeWrapper::set3DListenerAttributes(int listener, const FMOD_VECTOR* pos, const FMOD_VECTOR* vel, const FMOD_VECTOR* forward, const FMOD_VECTOR* up) {
    if (!m_system) return false;
    return FMOD_System_Set3DListenerAttributes(m_system, listener, pos, vel, forward, up) == FMOD_OK;
}

bool FmodRuntimeWrapper::set3DRolloffFactor(float rolloff) {
    if (!m_system) return false;
    return FMOD_System_Set3DRolloffFactor(m_system, rolloff) == FMOD_OK;
}

// ============================================================================
// Channel Groups
// ============================================================================

FMOD_CHANNELGROUP* FmodRuntimeWrapper::createChannelGroup(const char* name) {
    if (!m_system) return nullptr;
    FMOD_CHANNELGROUP* group = nullptr;
    FMOD_RESULT result = FMOD_System_CreateChannelGroup(m_system, name, &group);
    if (result != FMOD_OK) {
        qWarning() << "FmodRuntimeWrapper: createChannelGroup failed for" << name << ":" << resultToString(result);
        return nullptr;
    }
    return group;
}

bool FmodRuntimeWrapper::releaseChannelGroup(FMOD_CHANNELGROUP* channelGroup) {
    if (!channelGroup) return false;
    return FMOD_ChannelGroup_Release(channelGroup) == FMOD_OK;
}

bool FmodRuntimeWrapper::channelGroupSetVolume(FMOD_CHANNELGROUP* channelGroup, float volume) {
    if (!channelGroup) return false;
    return FMOD_ChannelGroup_SetVolume(channelGroup, volume) == FMOD_OK;
}

bool FmodRuntimeWrapper::channelGroupStop(FMOD_CHANNELGROUP* channelGroup) {
    if (!channelGroup) return false;
    return FMOD_ChannelGroup_Stop(channelGroup) == FMOD_OK;
}

// ============================================================================
// FMOD Studio (direct calls)
// ============================================================================

bool FmodRuntimeWrapper::createStudioSystem() {
    if (m_studioSystem) return true;

    FMOD_RESULT result = FMOD_Studio_System_Create(&m_studioSystem, 0x00020906);
    if (result != FMOD_OK) {
        emit error(QString("FMOD_Studio_System_Create failed: %1").arg(resultToString(result)));
        return false;
    }

    qDebug() << "FmodRuntimeWrapper: Studio system created";
    return true;
}

bool FmodRuntimeWrapper::initStudioSystem(int maxChannels, unsigned int flags) {
    if (!m_studioSystem) {
        if (!createStudioSystem()) return false;
    }

    FMOD_RESULT result = FMOD_Studio_System_Initialize(m_studioSystem, maxChannels, flags, nullptr);
    if (result != FMOD_OK) {
        emit error(QString("FMOD_Studio_System_Initialize failed: %1").arg(resultToString(result)));
        return false;
    }

    qDebug() << "FmodRuntimeWrapper: Studio system initialized";
    return true;
}

bool FmodRuntimeWrapper::closeStudioSystem() {
    if (!m_studioSystem) return true;

    FMOD_RESULT result = FMOD_Studio_System_Close(m_studioSystem);
    if (result != FMOD_OK) {
        qWarning() << "FmodRuntimeWrapper: FMOD_Studio_System_Close failed:" << resultToString(result);
        return false;
    }

    return true;
}

bool FmodRuntimeWrapper::updateStudioSystem() {
    if (!m_studioSystem) return false;
    return FMOD_Studio_System_Update(m_studioSystem) == FMOD_OK;
}

bool FmodRuntimeWrapper::releaseStudioSystem() {
    if (!m_studioSystem) return true;

    FMOD_RESULT result = FMOD_Studio_System_Release(m_studioSystem);
    if (result != FMOD_OK) {
        qWarning() << "FmodRuntimeWrapper: FMOD_Studio_System_Release failed:" << resultToString(result);
        return false;
    }

    m_studioSystem = nullptr;
    qDebug() << "FmodRuntimeWrapper: Studio system released";
    return true;
}

// ============================================================================
// FMOD Studio Banks
// ============================================================================

FMOD_STUDIO_BANK* FmodRuntimeWrapper::loadBank(const char* filename, unsigned int flags) {
    if (!m_studioSystem) return nullptr;
    FMOD_STUDIO_BANK* bank = nullptr;
    FMOD_RESULT result = FMOD_Studio_System_LoadBankFile(m_studioSystem, filename, flags, &bank);
    if (result != FMOD_OK) {
        qWarning() << "FmodRuntimeWrapper: loadBank failed for" << filename << ":" << resultToString(result);
        return nullptr;
    }
    qDebug() << "FmodRuntimeWrapper: Loaded bank" << filename;
    return bank;
}

bool FmodRuntimeWrapper::releaseBank(FMOD_STUDIO_BANK* bank) {
    if (!bank) return false;
    return FMOD_Studio_Bank_Release(bank) == FMOD_OK;
}

bool FmodRuntimeWrapper::getBankLoadingState(FMOD_STUDIO_BANK* bank, FMOD_STUDIO_LOADING_STATE& state) {
    if (!bank) return false;
    return FMOD_Studio_Bank_GetLoadingState(bank, &state) == FMOD_OK;
}

// ============================================================================
// FMOD Studio Events
// ============================================================================

FMOD_STUDIO_EVENTDESCRIPTION* FmodRuntimeWrapper::getEventDescription(const char* path) {
    if (!m_studioSystem) return nullptr;
    FMOD_STUDIO_EVENTDESCRIPTION* desc = nullptr;
    FMOD_RESULT result = FMOD_Studio_System_GetEvent(m_studioSystem, path, &desc);
    if (result != FMOD_OK) {
        qWarning() << "FmodRuntimeWrapper: getEventDescription failed for" << path << ":" << resultToString(result);
        return nullptr;
    }
    return desc;
}

FMOD_STUDIO_EVENTINSTANCE* FmodRuntimeWrapper::createEventInstance(FMOD_STUDIO_EVENTDESCRIPTION* eventDescription) {
    if (!eventDescription) return nullptr;
    FMOD_STUDIO_EVENTINSTANCE* instance = nullptr;
    FMOD_RESULT result = FMOD_Studio_EventDescription_CreateInstance(eventDescription, &instance);
    if (result != FMOD_OK) {
        qWarning() << "FmodRuntimeWrapper: createEventInstance failed:" << resultToString(result);
        return nullptr;
    }
    return instance;
}

bool FmodRuntimeWrapper::releaseEventInstance(FMOD_STUDIO_EVENTINSTANCE* eventInstance) {
    if (!eventInstance) return false;
    return FMOD_Studio_EventInstance_Release(eventInstance) == FMOD_OK;
}

bool FmodRuntimeWrapper::startEventInstance(FMOD_STUDIO_EVENTINSTANCE* eventInstance) {
    if (!eventInstance) return false;
    return FMOD_Studio_EventInstance_Start(eventInstance) == FMOD_OK;
}

bool FmodRuntimeWrapper::stopEventInstance(FMOD_STUDIO_EVENTINSTANCE* eventInstance, FMOD_STUDIO_STOP_MODE mode) {
    if (!eventInstance) return false;
    return FMOD_Studio_EventInstance_Stop(eventInstance, mode) == FMOD_OK;
}

bool FmodRuntimeWrapper::eventInstanceSetVolume(FMOD_STUDIO_EVENTINSTANCE* eventInstance, float volume) {
    if (!eventInstance) return false;
    return FMOD_Studio_EventInstance_SetVolume(eventInstance, volume) == FMOD_OK;
}

bool FmodRuntimeWrapper::eventInstanceSetParameterValue(FMOD_STUDIO_EVENTINSTANCE* eventInstance, const char* name, float value) {
    if (!eventInstance) return false;
    return FMOD_Studio_EventInstance_SetParameterValue(eventInstance, name, value) == FMOD_OK;
}

// ============================================================================
// FMOD Studio Buses
// ============================================================================

FMOD_STUDIO_BUS* FmodRuntimeWrapper::getBus(const char* path) {
    if (!m_studioSystem) return nullptr;
    FMOD_STUDIO_BUS* bus = nullptr;
    FMOD_RESULT result = FMOD_Studio_System_GetBus(m_studioSystem, path, &bus);
    if (result != FMOD_OK) {
        qWarning() << "FmodRuntimeWrapper: getBus failed for" << path << ":" << resultToString(result);
        return nullptr;
    }
    return bus;
}

bool FmodRuntimeWrapper::busSetVolume(FMOD_STUDIO_BUS* bus, float volume) {
    if (!bus) return false;
    return FMOD_Studio_Bus_SetFaderLevel(bus, volume) == FMOD_OK;
}

bool FmodRuntimeWrapper::busGetVolume(FMOD_STUDIO_BUS* bus, float& volume) {
    if (!bus) return false;
    return FMOD_Studio_Bus_GetFaderLevel(bus, &volume) == FMOD_OK;
}

bool FmodRuntimeWrapper::busStopAllEvents(FMOD_STUDIO_BUS* bus, FMOD_STUDIO_STOP_MODE mode) {
    if (!bus) return false;
    return FMOD_Studio_Bus_StopAllEvents(bus, mode) == FMOD_OK;
}

// ============================================================================
// FMOD Studio VCAs
// ============================================================================

FMOD_STUDIO_VCA* FmodRuntimeWrapper::getVCA(const char* path) {
    if (!m_studioSystem) return nullptr;
    FMOD_STUDIO_VCA* vca = nullptr;
    FMOD_RESULT result = FMOD_Studio_System_GetVCA(m_studioSystem, path, &vca);
    if (result != FMOD_OK) {
        qWarning() << "FmodRuntimeWrapper: getVCA failed for" << path << ":" << resultToString(result);
        return nullptr;
    }
    return vca;
}

bool FmodRuntimeWrapper::vcaSetVolume(FMOD_STUDIO_VCA* vca, float volume) {
    if (!vca) return false;
    return FMOD_Studio_VCA_SetFaderLevel(vca, volume) == FMOD_OK;
}

bool FmodRuntimeWrapper::vcaGetVolume(FMOD_STUDIO_VCA* vca, float& volume) {
    if (!vca) return false;
    return FMOD_Studio_VCA_GetFaderLevel(vca, &volume) == FMOD_OK;
}

// ============================================================================
// String helpers
// ============================================================================

const char* FmodRuntimeWrapper::resultToString(FMOD_RESULT result) {
    switch (result) {
    case FMOD_OK: return "OK";
    case FMOD_ERR_BADCOMMAND: return "Bad command";
    case FMOD_ERR_CHANNEL_ALLOC: return "Channel allocation error";
    case FMOD_ERR_CHANNEL_STOLEN: return "Channel stolen";
    case FMOD_ERR_DSP_CONNECTION: return "DSP connection error";
    case FMOD_ERR_DSP_FORMAT: return "DSP format error";
    case FMOD_ERR_DSP_INUSE: return "DSP in use";
    case FMOD_ERR_DSP_NOTFOUND: return "DSP not found";
    case FMOD_ERR_FILE_BAD: return "Bad file";
    case FMOD_ERR_FILE_COULDNOTSEEK: return "Could not seek";
    case FMOD_ERR_FILE_EOF: return "End of file";
    case FMOD_ERR_FILE_NOTFOUND: return "File not found";
    case FMOD_ERR_FORMAT: return "Format error";
    case FMOD_ERR_INITIALIZED: return "Already initialized";
    case FMOD_ERR_INITIALIZATION: return "Initialization error";
    case FMOD_ERR_INVALID_HANDLE: return "Invalid handle";
    case FMOD_ERR_INVALID_PARAM: return "Invalid parameter";
    case FMOD_ERR_MEMORY: return "Memory allocation error";
    case FMOD_ERR_NEEDS3D: return "Needs 3D";
    case FMOD_ERR_NEEDS_SOFTWARE: return "Needs software";
    case FMOD_ERR_OUTPUT: return "Output error";
    case FMOD_ERR_OUTPUT_INIT: return "Output initialization error";
    case FMOD_ERR_OUTPUT_NO_DRIVERS: return "No output drivers";
    case FMOD_ERR_PLUGIN: return "Plugin error";
    case FMOD_ERR_PLUGIN_MISSING: return "Plugin missing";
    case FMOD_ERR_SUBSOUNDS: return "Subsounds error";
    case FMOD_ERR_TOOMANYCHANNELS: return "Too many channels";
    case FMOD_ERR_UNINITIALIZED: return "Uninitialized";
    case FMOD_ERR_UNSUPPORTED: return "Unsupported";
    case FMOD_ERR_VERSION: return "Version error";
    case FMOD_ERR_EVENT_NOTFOUND: return "Event not found";
    case FMOD_ERR_STUDIO_UNINITIALIZED: return "Studio uninitialized";
    case FMOD_ERR_STUDIO_NOT_LOADED: return "Studio not loaded";
    default: return "Unknown error";
    }
}

} // namespace ks::engine::audio
