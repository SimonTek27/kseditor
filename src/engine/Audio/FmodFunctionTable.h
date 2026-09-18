#pragma once

#include <cstdint>

// ============================================================================
// FMOD Core API - Direct declarations (linked via fmod64.lib)
// ============================================================================

// Opaque handle types
struct FMOD_SYSTEM;
struct FMOD_SOUND;
struct FMOD_CHANNEL;
struct FMOD_CHANNELGROUP;
struct FMOD_DSP;
struct FMOD_DSPCONNECTION;
struct FMOD_GEOMETRY;
struct FMOD_REVERB3D;
struct FMOD_SOUNDGROUP;

// FMOD result codes
enum FMOD_RESULT {
    FMOD_OK = 0,
    FMOD_ERR_BADCOMMAND = 1,
    FMOD_ERR_CHANNEL_ALLOC = 2,
    FMOD_ERR_CHANNEL_STOLEN = 3,
    FMOD_ERR_DSP_CONNECTION = 4,
    FMOD_ERR_DSP_FORMAT = 5,
    FMOD_ERR_DSP_INUSE = 6,
    FMOD_ERR_DSP_NOTFOUND = 7,
    FMOD_ERR_DSP_RESERVED = 8,
    FMOD_ERR_DSP_SATURATION = 9,
    FMOD_ERR_DSP_UNKNOWN = 10,
    FMOD_ERR_FILE_BAD = 12,
    FMOD_ERR_FILE_COULDNOTSEEK = 13,
    FMOD_ERR_FILE_EOF = 15,
    FMOD_ERR_FILE_NOTFOUND = 17,
    FMOD_ERR_FORMAT = 18,
    FMOD_ERR_INITIALIZED = 26,
    FMOD_ERR_INITIALIZATION = 25,
    FMOD_ERR_INVALID_HANDLE = 29,
    FMOD_ERR_INVALID_PARAM = 30,
    FMOD_ERR_MEMORY = 37,
    FMOD_ERR_NEEDS3D = 39,
    FMOD_ERR_NEEDS_SOFTWARE = 41,
    FMOD_ERR_OUTPUT = 47,
    FMOD_ERR_OUTPUT_INIT = 51,
    FMOD_ERR_OUTPUT_NO_DRIVERS = 52,
    FMOD_ERR_PLUGIN = 53,
    FMOD_ERR_PLUGIN_MISSING = 54,
    FMOD_ERR_SUBSOUNDS = 61,
    FMOD_ERR_TOOMANYCHANNELS = 65,
    FMOD_ERR_UNINITIALIZED = 68,
    FMOD_ERR_UNSUPPORTED = 69,
    FMOD_ERR_VERSION = 70,
    FMOD_ERR_EVENT_NOTFOUND = 74,
    FMOD_ERR_STUDIO_UNINITIALIZED = 75,
    FMOD_ERR_STUDIO_NOT_LOADED = 76,
};

// FMOD init flags
enum FMOD_INITFLAGS {
    FMOD_INIT_NORMAL = 0x00000000,
    FMOD_INIT_STREAM_FROM_UPDATE = 0x00000001,
    FMOD_INIT_MIX_FROM_UPDATE = 0x00000002,
    FMOD_INIT_3D_RIGHTHANDED = 0x00000004,
    FMOD_INIT_CLIPPING = 0x00000008,
    FMOD_INIT_CHANNEL_LOWPASS = 0x00000100,
    FMOD_INIT_CHANNEL_DISTANCEFILTER = 0x00000200,
    FMOD_INIT_PROFILE_ENABLE = 0x00010000,
    FMOD_INIT_VOL0_BECOMES_VIRTUAL = 0x00020000,
    FMOD_INIT_GEOMETRY_USECULLING = 0x00040000,
    FMOD_INIT_3D_NO_LISTENER = 0x00000010,
};

// FMOD sound mode flags
enum FMOD_MODE {
    FMOD_DEFAULT = 0x00000000,
    FMOD_LOOP_OFF = 0x00000001,
    FMOD_LOOP_NORMAL = 0x00000002,
    FMOD_LOOP_BIDI = 0x00000004,
    FMOD_2D = 0x00000008,
    FMOD_3D = 0x00000010,
    FMOD_CREATESTREAM = 0x00000080,
    FMOD_CREATESAMPLE = 0x00000100,
    FMOD_CREATECOMPRESSEDSAMPLE = 0x00000200,
    FMOD_OPENONLY = 0x00000800,
    FMOD_NONBLOCKING = 0x00010000,
    FMOD_3D_HEADRELATIVE = 0x00040000,
    FMOD_3D_WORLDRELATIVE = 0x00080000,
    FMOD_3D_INVERSEROLLOFF = 0x00100000,
    FMOD_3D_LINEARROLLOFF = 0x00200000,
    FMOD_IGNOREGEOMETRY = 0x40000000,
};

// FMOD vector
struct FMOD_VECTOR {
    float x, y, z;
};

// ============================================================================
// FMOD Core C API - Direct function declarations
// ============================================================================

extern "C" {

// System
FMOD_RESULT FMOD_System_Create(FMOD_SYSTEM** system, unsigned int headerversion);
FMOD_RESULT FMOD_System_Release(FMOD_SYSTEM* system);
FMOD_RESULT FMOD_System_Init(FMOD_SYSTEM* system, int maxchannels, FMOD_INITFLAGS flags, void* extradriverdata);
FMOD_RESULT FMOD_System_Close(FMOD_SYSTEM* system);
FMOD_RESULT FMOD_System_Update(FMOD_SYSTEM* system);
FMOD_RESULT FMOD_System_GetVersion(FMOD_SYSTEM* system, unsigned int* version, unsigned int* headerversion);
FMOD_RESULT FMOD_System_SetSoftwareFormat(FMOD_SYSTEM* system, int samplerate, int speakermode, int numrawspeakers);
FMOD_RESULT FMOD_System_GetSoftwareFormat(FMOD_SYSTEM* system, int* samplerate, int* speakermode, int* numrawspeakers);
FMOD_RESULT FMOD_System_SetDSPBufferSize(FMOD_SYSTEM* system, int bufferlength, int numbuffers);
FMOD_RESULT FMOD_System_GetDSPBufferSize(FMOD_SYSTEM* system, int* bufferlength, int* numbuffers);

// Sound
FMOD_RESULT FMOD_System_CreateSound(FMOD_SYSTEM* system, const char* name, FMOD_MODE mode, void* exinfo, FMOD_SOUND** sound);
FMOD_RESULT FMOD_System_CreateStream(FMOD_SYSTEM* system, const char* name, FMOD_MODE mode, void* exinfo, FMOD_SOUND** sound);
FMOD_RESULT FMOD_Sound_Release(FMOD_SOUND* sound);
FMOD_RESULT FMOD_Sound_GetLength(FMOD_SOUND* sound, unsigned int* length, int timeunit);
FMOD_RESULT FMOD_Sound_GetFormat(FMOD_SOUND* sound, int* type, int* format, int* numchannels, int* bits);
FMOD_RESULT FMOD_Sound_GetDefaults(FMOD_SOUND* sound, float* frequency, int* priority);
FMOD_RESULT FMOD_Sound_SetDefaults(FMOD_SOUND* sound, float frequency, int priority);
FMOD_RESULT FMOD_Sound_GetLoopCount(FMOD_SOUND* sound, int* loopcount);
FMOD_RESULT FMOD_Sound_SetLoopCount(FMOD_SOUND* sound, int loopcount);
FMOD_RESULT FMOD_Sound_GetLoopPoints(FMOD_SOUND* sound, unsigned int* loopstart, int loopstarttype, unsigned int* loopend, int loopendtype);
FMOD_RESULT FMOD_Sound_SetLoopPoints(FMOD_SOUND* sound, unsigned int loopstart, int loopstarttype, unsigned int loopend, int loopendtype);

// Channel
FMOD_RESULT FMOD_System_PlaySound(FMOD_SYSTEM* system, FMOD_SOUND* sound, FMOD_CHANNELGROUP* channelgroup, int paused, FMOD_CHANNEL** channel);
FMOD_RESULT FMOD_Channel_Stop(FMOD_CHANNEL* channel);
FMOD_RESULT FMOD_Channel_SetPaused(FMOD_CHANNEL* channel, int paused);
FMOD_RESULT FMOD_Channel_GetPaused(FMOD_CHANNEL* channel, int* paused);
FMOD_RESULT FMOD_Channel_SetVolume(FMOD_CHANNEL* channel, float volume);
FMOD_RESULT FMOD_Channel_GetVolume(FMOD_CHANNEL* channel, float* volume);
FMOD_RESULT FMOD_Channel_SetFrequency(FMOD_CHANNEL* channel, float frequency);
FMOD_RESULT FMOD_Channel_GetFrequency(FMOD_CHANNEL* channel, float* frequency);
FMOD_RESULT FMOD_Channel_SetPitch(FMOD_CHANNEL* channel, float pitch);
FMOD_RESULT FMOD_Channel_GetPitch(FMOD_CHANNEL* channel, float* pitch);
FMOD_RESULT FMOD_Channel_SetPan(FMOD_CHANNEL* channel, float pan);
FMOD_RESULT FMOD_Channel_GetPan(FMOD_CHANNEL* channel, float* pan);
FMOD_RESULT FMOD_Channel_SetPosition(FMOD_CHANNEL* channel, unsigned int position, int timeunit);
FMOD_RESULT FMOD_Channel_GetPosition(FMOD_CHANNEL* channel, unsigned int* position, int timeunit);
FMOD_RESULT FMOD_Channel_GetCurrentSound(FMOD_CHANNEL* channel, FMOD_SOUND** sound);
FMOD_RESULT FMOD_Channel_IsPlaying(FMOD_CHANNEL* channel, int* isplaying);
FMOD_RESULT FMOD_Channel_GetSystemObject(FMOD_CHANNEL* channel, FMOD_SYSTEM** system);

// Channel Group
FMOD_RESULT FMOD_System_CreateChannelGroup(FMOD_SYSTEM* system, const char* name, FMOD_CHANNELGROUP** channelgroup);
FMOD_RESULT FMOD_ChannelGroup_Release(FMOD_CHANNELGROUP* channelgroup);
FMOD_RESULT FMOD_ChannelGroup_Stop(FMOD_CHANNELGROUP* channelgroup);
FMOD_RESULT FMOD_ChannelGroup_SetVolume(FMOD_CHANNELGROUP* channelgroup, float volume);
FMOD_RESULT FMOD_ChannelGroup_GetVolume(FMOD_CHANNELGROUP* channelgroup, float* volume);
FMOD_RESULT FMOD_ChannelGroup_SetPaused(FMOD_CHANNELGROUP* channelgroup, int paused);
FMOD_RESULT FMOD_ChannelGroup_GetPaused(FMOD_CHANNELGROUP* channelgroup, int* paused);
FMOD_RESULT FMOD_ChannelGroup_SetPitch(FMOD_CHANNELGROUP* channelgroup, float pitch);
FMOD_RESULT FMOD_ChannelGroup_GetPitch(FMOD_CHANNELGROUP* channelgroup, float* pitch);
FMOD_RESULT FMOD_ChannelGroup_AddGroup(FMOD_CHANNELGROUP* channelgroup, FMOD_CHANNELGROUP* group, int propagateDSPClock, FMOD_DSPCONNECTION** connection);
FMOD_RESULT FMOD_ChannelGroup_GetNumGroups(FMOD_CHANNELGROUP* channelgroup, int* numgroups);
FMOD_RESULT FMOD_ChannelGroup_GetGroup(FMOD_CHANNELGROUP* channelgroup, int index, FMOD_CHANNELGROUP** group);

// 3D
FMOD_RESULT FMOD_Channel_Set3DAttributes(FMOD_CHANNEL* channel, const FMOD_VECTOR* pos, const FMOD_VECTOR* vel);
FMOD_RESULT FMOD_Channel_Get3DAttributes(FMOD_CHANNEL* channel, FMOD_VECTOR* pos, FMOD_VECTOR* vel);
FMOD_RESULT FMOD_Channel_Set3DMinMaxDistance(FMOD_CHANNEL* channel, float mindistance, float maxdistance);
FMOD_RESULT FMOD_Channel_Get3DMinMaxDistance(FMOD_CHANNEL* channel, float* mindistance, float* maxdistance);
FMOD_RESULT FMOD_Channel_SetMode(FMOD_CHANNEL* channel, FMOD_MODE mode);
FMOD_RESULT FMOD_Channel_GetMode(FMOD_CHANNEL* channel, FMOD_MODE* mode);
FMOD_RESULT FMOD_System_Set3DListenerAttributes(FMOD_SYSTEM* system, int listener, const FMOD_VECTOR* pos, const FMOD_VECTOR* vel, const FMOD_VECTOR* forward, const FMOD_VECTOR* up);
FMOD_RESULT FMOD_System_Get3DListenerAttributes(FMOD_SYSTEM* system, int listener, FMOD_VECTOR* pos, FMOD_VECTOR* vel, FMOD_VECTOR* forward, FMOD_VECTOR* up);
FMOD_RESULT FMOD_System_Set3DRolloffFactor(FMOD_SYSTEM* system, float rolloff);
FMOD_RESULT FMOD_System_Get3DRolloffFactor(FMOD_SYSTEM* system, float* rolloff);

// DSP
FMOD_RESULT FMOD_System_CreateDSPByType(FMOD_SYSTEM* system, int type, FMOD_DSP** dsp);
FMOD_RESULT FMOD_DSP_Release(FMOD_DSP* dsp);
FMOD_RESULT FMOD_DSP_SetParameterFloat(FMOD_DSP* dsp, int index, float value);
FMOD_RESULT FMOD_DSP_GetParameterFloat(FMOD_DSP* dsp, int index, float* value, char* valuestr, int valuestrlen);
FMOD_RESULT FMOD_Channel_AddDSP(FMOD_CHANNEL* channel, int index, FMOD_DSP* dsp);
FMOD_RESULT FMOD_Channel_RemoveDSP(FMOD_CHANNEL* channel, FMOD_DSP* dsp);

// Geometry
FMOD_RESULT FMOD_System_CreateGeometry(FMOD_SYSTEM* system, int maxpolygons, int maxvertices, FMOD_GEOMETRY** geometry);
FMOD_RESULT FMOD_Geometry_Release(FMOD_GEOMETRY* geometry);
FMOD_RESULT FMOD_Geometry_AddPolygon(FMOD_GEOMETRY* geometry, float directocclusion, float reverbocclusion, int doublesided, int numvertices, const FMOD_VECTOR* vertices, float* hole);
FMOD_RESULT FMOD_Geometry_SetPosition(FMOD_GEOMETRY* geometry, float x, float y, float z);
FMOD_RESULT FMOD_Geometry_SetRotation(FMOD_GEOMETRY* geometry, float forwardx, float forwardy, float forwardz, float upx, float upy, float upz);
FMOD_RESULT FMOD_Geometry_SetScale(FMOD_GEOMETRY* geometry, float scalex, float scaley, float scalez);
FMOD_RESULT FMOD_Geometry_SetActive(FMOD_GEOMETRY* geometry, int active);
FMOD_RESULT FMOD_Geometry_GetActive(FMOD_GEOMETRY* geometry, int* active);

// Reverb
FMOD_RESULT FMOD_System_CreateReverb3D(FMOD_SYSTEM* system, FMOD_REVERB3D** reverb);
FMOD_RESULT FMOD_Reverb3D_Release(FMOD_REVERB3D* reverb);
FMOD_RESULT FMOD_Reverb3D_SetProperties(FMOD_REVERB3D* reverb, const void* properties);
FMOD_RESULT FMOD_Reverb3D_GetProperties(FMOD_REVERB3D* reverb, void* properties);
FMOD_RESULT FMOD_Reverb3D_SetPosition(FMOD_REVERB3D* reverb, float x, float y, float z);
FMOD_RESULT FMOD_Reverb3D_GetPosition(FMOD_REVERB3D* reverb, float* x, float* y, float* z);
FMOD_RESULT FMOD_Reverb3D_SetActive(FMOD_REVERB3D* reverb, int active);
FMOD_RESULT FMOD_Reverb3D_GetActive(FMOD_REVERB3D* reverb, int* active);

// Advanced settings
FMOD_RESULT FMOD_System_SetAdvancedSettings(FMOD_SYSTEM* system, void* settings);
FMOD_RESULT FMOD_System_GetAdvancedSettings(FMOD_SYSTEM* system, void* settings);

// Record
FMOD_RESULT FMOD_System_GetRecordNumDrivers(FMOD_SYSTEM* system, int* numdrivers, int* numconnected);
FMOD_RESULT FMOD_System_GetRecordDriverInfo(FMOD_SYSTEM* system, int id, char* name, int namelen, void* guid, int* systemrate, int* speakermode, int* speakermodechannels, unsigned int* drivestate, void* callback);

} // extern "C"

// ============================================================================
// FMOD Studio API - Direct declarations (linked via fmodstudio64.lib)
// ============================================================================

// Opaque handle types
struct FMOD_STUDIO_SYSTEM;
struct FMOD_STUDIO_BANK;
struct FMOD_STUDIO_EVENTDESCRIPTION;
struct FMOD_STUDIO_EVENTINSTANCE;
struct FMOD_STUDIO_BUS;
struct FMOD_STUDIO_VCA;
struct FMOD_STUDIO_COMMANDREPLAY;

// Studio loading state
enum FMOD_STUDIO_LOADING_STATE {
    FMOD_STUDIO_LOADING_STATE_UNLOADED = 0,
    FMOD_STUDIO_LOADING_STATE_LOADING = 1,
    FMOD_STUDIO_LOADING_STATE_LOADED = 2,
    FMOD_STUDIO_LOADING_STATE_ERROR = 3,
};

// Studio stop mode
enum FMOD_STUDIO_STOP_MODE {
    FMOD_STUDIO_STOP_ALLOWFADEOUT = 0,
    FMOD_STUDIO_STOP_IMMEDIATE = 1,
};

extern "C" {

// Studio System
FMOD_RESULT FMOD_Studio_System_Create(FMOD_STUDIO_SYSTEM** system, unsigned int headerversion);
FMOD_RESULT FMOD_Studio_System_Release(FMOD_STUDIO_SYSTEM* system);
FMOD_RESULT FMOD_Studio_System_Initialize(FMOD_STUDIO_SYSTEM* system, int maxchannels, unsigned int flags, void* extradriverdata);
FMOD_RESULT FMOD_Studio_System_Close(FMOD_STUDIO_SYSTEM* system);
FMOD_RESULT FMOD_Studio_System_Update(FMOD_STUDIO_SYSTEM* system);

// Banks
FMOD_RESULT FMOD_Studio_System_LoadBankFile(FMOD_STUDIO_SYSTEM* system, const char* filename, unsigned int flags, FMOD_STUDIO_BANK** bank);
FMOD_RESULT FMOD_Studio_System_LoadBankMemory(FMOD_STUDIO_SYSTEM* system, const char* name, int length, int mode, unsigned int flags, FMOD_STUDIO_BANK** bank);
FMOD_RESULT FMOD_Studio_Bank_Release(FMOD_STUDIO_BANK* bank);
FMOD_RESULT FMOD_Studio_Bank_GetLoadingState(FMOD_STUDIO_BANK* bank, FMOD_STUDIO_LOADING_STATE* state);
FMOD_RESULT FMOD_Studio_Bank_GetSampleLoadingState(FMOD_STUDIO_BANK* bank, FMOD_STUDIO_LOADING_STATE* state);
FMOD_RESULT FMOD_Studio_Bank_LoadSampleData(FMOD_STUDIO_BANK* bank);
FMOD_RESULT FMOD_Studio_Bank_UnloadSampleData(FMOD_STUDIO_BANK* bank);
FMOD_RESULT FMOD_Studio_Bank_GetPath(FMOD_STUDIO_BANK* bank, char* path, int size, int* retrieved);

// Event Descriptions
FMOD_RESULT FMOD_Studio_System_GetEvent(FMOD_STUDIO_SYSTEM* system, const char* path, FMOD_STUDIO_EVENTDESCRIPTION** description);
FMOD_RESULT FMOD_Studio_EventDescription_GetLength(FMOD_STUDIO_EVENTDESCRIPTION* description, int* length);
FMOD_RESULT FMOD_Studio_EventDescription_GetPath(FMOD_STUDIO_EVENTDESCRIPTION* description, char* path, int size, int* retrieved);
FMOD_RESULT FMOD_Studio_EventDescription_GetParameterCount(FMOD_STUDIO_EVENTDESCRIPTION* description, int* count);
FMOD_RESULT FMOD_Studio_EventDescription_CreateInstance(FMOD_STUDIO_EVENTDESCRIPTION* description, FMOD_STUDIO_EVENTINSTANCE** instance);
FMOD_RESULT FMOD_Studio_EventDescription_IsOneshot(FMOD_STUDIO_EVENTDESCRIPTION* description, int* oneshot);
FMOD_RESULT FMOD_Studio_EventDescription_IsStream(FMOD_STUDIO_EVENTDESCRIPTION* description, int* stream);

// Event Instances
FMOD_RESULT FMOD_Studio_EventInstance_Release(FMOD_STUDIO_EVENTINSTANCE* instance);
FMOD_RESULT FMOD_Studio_EventInstance_Start(FMOD_STUDIO_EVENTINSTANCE* instance);
FMOD_RESULT FMOD_Studio_EventInstance_Stop(FMOD_STUDIO_EVENTINSTANCE* instance, FMOD_STUDIO_STOP_MODE mode);
FMOD_RESULT FMOD_Studio_EventInstance_SetPaused(FMOD_STUDIO_EVENTINSTANCE* instance, int paused);
FMOD_RESULT FMOD_Studio_EventInstance_GetPaused(FMOD_STUDIO_EVENTINSTANCE* instance, int* paused);
FMOD_RESULT FMOD_Studio_EventInstance_SetVolume(FMOD_STUDIO_EVENTINSTANCE* instance, float volume);
FMOD_RESULT FMOD_Studio_EventInstance_GetVolume(FMOD_STUDIO_EVENTINSTANCE* instance, float* volume, float* finalvolume);
FMOD_RESULT FMOD_Studio_EventInstance_SetPitch(FMOD_STUDIO_EVENTINSTANCE* instance, float pitch);
FMOD_RESULT FMOD_Studio_EventInstance_GetPitch(FMOD_STUDIO_EVENTINSTANCE* instance, float* pitch);
FMOD_RESULT FMOD_Studio_EventInstance_SetParameterValue(FMOD_STUDIO_EVENTINSTANCE* instance, const char* name, float value);
FMOD_RESULT FMOD_Studio_EventInstance_GetParameterValue(FMOD_STUDIO_EVENTINSTANCE* instance, const char* name, float* value, float* finalvalue);
FMOD_RESULT FMOD_Studio_EventInstance_Set3DAttributes(FMOD_STUDIO_EVENTINSTANCE* instance, void* attributes);
FMOD_RESULT FMOD_Studio_EventInstance_Get3DAttributes(FMOD_STUDIO_EVENTINSTANCE* instance, void* attributes);
FMOD_RESULT FMOD_Studio_EventInstance_GetDescription(FMOD_STUDIO_EVENTINSTANCE* instance, FMOD_STUDIO_EVENTDESCRIPTION** description);
FMOD_RESULT FMOD_Studio_EventInstance_IsPlaying(FMOD_STUDIO_EVENTINSTANCE* instance, int* isplaying);

// Buses
FMOD_RESULT FMOD_Studio_System_GetBus(FMOD_STUDIO_SYSTEM* system, const char* path, FMOD_STUDIO_BUS** bus);
FMOD_RESULT FMOD_Studio_Bus_SetFaderLevel(FMOD_STUDIO_BUS* bus, float volume);
FMOD_RESULT FMOD_Studio_Bus_GetFaderLevel(FMOD_STUDIO_BUS* bus, float* volume);
FMOD_RESULT FMOD_Studio_Bus_SetPaused(FMOD_STUDIO_BUS* bus, int paused);
FMOD_RESULT FMOD_Studio_Bus_GetPaused(FMOD_STUDIO_BUS* bus, int* paused);
FMOD_RESULT FMOD_Studio_Bus_SetMute(FMOD_STUDIO_BUS* bus, int mute);
FMOD_RESULT FMOD_Studio_Bus_GetMute(FMOD_STUDIO_BUS* bus, int* mute);
FMOD_RESULT FMOD_Studio_Bus_StopAllEvents(FMOD_STUDIO_BUS* bus, FMOD_STUDIO_STOP_MODE mode);
FMOD_RESULT FMOD_Studio_Bus_LockChannelGroup(FMOD_STUDIO_BUS* bus);
FMOD_RESULT FMOD_Studio_Bus_UnlockChannelGroup(FMOD_STUDIO_BUS* bus);
FMOD_RESULT FMOD_Studio_Bus_GetID(FMOD_STUDIO_BUS* bus, void* guid);
FMOD_RESULT FMOD_Studio_Bus_GetPath(FMOD_STUDIO_BUS* bus, char* path, int size, int* retrieved);

// VCAs
FMOD_RESULT FMOD_Studio_System_GetVCA(FMOD_STUDIO_SYSTEM* system, const char* path, FMOD_STUDIO_VCA** vca);
FMOD_RESULT FMOD_Studio_VCA_SetFaderLevel(FMOD_STUDIO_VCA* vca, float volume);
FMOD_RESULT FMOD_Studio_VCA_GetFaderLevel(FMOD_STUDIO_VCA* vca, float* volume);
FMOD_RESULT FMOD_Studio_VCA_GetPath(FMOD_STUDIO_VCA* vca, char* path, int size, int* retrieved);

// Command Replay
FMOD_RESULT FMOD_Studio_System_StartCommandCapture(FMOD_STUDIO_SYSTEM* system, const char* filename, unsigned int flags, FMOD_STUDIO_COMMANDREPLAY** replay);
FMOD_RESULT FMOD_Studio_System_StopCommandCapture(FMOD_STUDIO_SYSTEM* system);
FMOD_RESULT FMOD_Studio_CommandReplay_Release(FMOD_STUDIO_COMMANDREPLAY* replay);

} // extern "C"
