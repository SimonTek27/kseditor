#pragma once

// QMediaDevices / QAudioDevice (Qt Multimedia) replacement: WASAPI endpoint
// enumeration. id strings feed WASAPIInput::initialize(deviceId); description
// is the friendly device name the Qt code printed.

#include <string>
#include <vector>

namespace ks::audio {

struct AudioDeviceInfo {
    std::string id;          // QAudioDevice::id() - WASAPI endpoint ID
    std::string description; // QAudioDevice::description() - friendly name
    bool isDefault = false;

    // QAudioDevice::isNull(): true when no endpoint was found
    bool isNull() const { return id.empty(); }
};

// QMediaDevices::audioInputs() / QMediaDevices::audioOutputs()
std::vector<AudioDeviceInfo> inputDevices();
std::vector<AudioDeviceInfo> outputDevices();

// QMediaDevices::defaultAudioInput() / QMediaDevices::defaultAudioOutput()
AudioDeviceInfo defaultInputDevice();
AudioDeviceInfo defaultOutputDevice();

} // namespace ks::audio
