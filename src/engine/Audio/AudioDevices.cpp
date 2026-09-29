#include "AudioDevices.h"

#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>

#pragma comment(lib, "ole32.lib")

namespace ks::audio {

namespace {

std::string toUtf8(const wchar_t* wide)
{
    if (!wide || !*wide) return std::string();
    const int len =
        WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 1) return std::string();
    std::string out(static_cast<std::size_t>(len), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, out.data(), len, nullptr, nullptr);
    out.resize(static_cast<std::size_t>(len - 1));
    return out;
}

AudioDeviceInfo endpointInfo(IMMDevice* device)
{
    AudioDeviceInfo info;
    if (!device) return info;

    LPWSTR id = nullptr;
    if (SUCCEEDED(device->GetId(&id)) && id) {
        info.id = toUtf8(id);
        CoTaskMemFree(id);
    }

    IPropertyStore* props = nullptr;
    if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, &props)) && props) {
        PROPVARIANT var;
        PropVariantInit(&var);
        if (SUCCEEDED(props->GetValue(PKEY_Device_FriendlyName, &var)) &&
            var.vt == VT_LPWSTR && var.pwszVal) {
            info.description = toUtf8(var.pwszVal);
        }
        PropVariantClear(&var);
        props->Release();
    }
    return info;
}

struct Enumerator {
    IMMDeviceEnumerator* e = nullptr;
    Enumerator()
    {
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                         __uuidof(IMMDeviceEnumerator), (void**)&e);
    }
    ~Enumerator()
    {
        if (e) e->Release();
        CoUninitialize();
    }
};

std::string defaultId(Enumerator& en, EDataFlow flow)
{
    IMMDevice* dev = nullptr;
    std::string id;
    if (en.e && SUCCEEDED(en.e->GetDefaultAudioEndpoint(flow, eMultimedia, &dev)) && dev) {
        id = endpointInfo(dev).id;
        dev->Release();
    }
    return id;
}

std::vector<AudioDeviceInfo> devicesFor(EDataFlow flow)
{
    std::vector<AudioDeviceInfo> out;
    Enumerator en;
    if (!en.e) return out;

    const std::string defaultIdStr = defaultId(en, flow);

    IMMDeviceCollection* collection = nullptr;
    if (FAILED(en.e->EnumAudioEndpoints(flow, DEVICE_STATE_ACTIVE, &collection)) ||
        !collection) {
        return out;
    }

    UINT count = 0;
    collection->GetCount(&count);
    for (UINT i = 0; i < count; ++i) {
        IMMDevice* dev = nullptr;
        if (FAILED(collection->Item(i, &dev)) || !dev) continue;
        AudioDeviceInfo info = endpointInfo(dev);
        info.isDefault = !info.id.empty() && info.id == defaultIdStr;
        dev->Release();
        if (!info.id.empty()) out.push_back(std::move(info));
    }
    collection->Release();
    return out;
}

} // namespace

std::vector<AudioDeviceInfo> inputDevices() { return devicesFor(eCapture); }

std::vector<AudioDeviceInfo> outputDevices() { return devicesFor(eRender); }

AudioDeviceInfo defaultInputDevice()
{
    Enumerator en;
    AudioDeviceInfo info;
    if (!en.e) return info;
    IMMDevice* dev = nullptr;
    if (SUCCEEDED(en.e->GetDefaultAudioEndpoint(eCapture, eMultimedia, &dev)) && dev) {
        info = endpointInfo(dev);
        dev->Release();
        info.isDefault = true;
    }
    return info;
}

AudioDeviceInfo defaultOutputDevice()
{
    Enumerator en;
    AudioDeviceInfo info;
    if (!en.e) return info;
    IMMDevice* dev = nullptr;
    if (SUCCEEDED(en.e->GetDefaultAudioEndpoint(eRender, eMultimedia, &dev)) && dev) {
        info = endpointInfo(dev);
        dev->Release();
        info.isDefault = true;
    }
    return info;
}

} // namespace ks::audio
