#include "WASAPIInput.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <thread>

#include <Audioclient.h>
#include <mmdeviceapi.h>

#pragma comment(lib, "ole32.lib")

namespace ks::audio {

namespace {

std::wstring toWide(const std::string& utf8)
{
    if (utf8.empty()) return std::wstring();
    const int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()),
                                        nullptr, 0);
    if (len <= 0) return std::wstring();
    std::wstring out(static_cast<std::size_t>(len), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), out.data(), len);
    return out;
}

} // namespace

struct WASAPIInput::Impl {
    IMMDeviceEnumerator* enumerator = nullptr;
    IMMDevice* device = nullptr;
    IAudioClient* audioClient = nullptr;
    IAudioCaptureClient* captureClient = nullptr;
    WAVEFORMATEX* waveFormat = nullptr;
    UINT32 bufferFrameCount = 0;
    HANDLE hEvent = nullptr;
    std::atomic<bool> running{false};
    std::thread captureThread;
    AudioCaptureCallback callback;
    AudioFormat format;
    int bufferMs = 20;

    ~Impl()
    {
        stop();
        if (audioClient) audioClient->Stop();
        if (hEvent) CloseHandle(hEvent);
        if (captureClient) captureClient->Release();
        if (audioClient) audioClient->Release();
        if (waveFormat) CoTaskMemFree(waveFormat);
        if (device) device->Release();
        if (enumerator) enumerator->Release();
    }

    void stop()
    {
        running = false;
        if (captureThread.joinable()) captureThread.join();
    }
};

WASAPIInput::WASAPIInput() : m_impl(std::make_unique<Impl>()) {}
WASAPIInput::~WASAPIInput() { shutdown(); }

bool WASAPIInput::initialize(int sampleRate, int channels, int bufferMs,
                             const std::string& deviceId)
{
    if (m_impl->audioClient) return true;

    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                  __uuidof(IMMDeviceEnumerator),
                                  (void**)&m_impl->enumerator);
    if (FAILED(hr)) {
        printf("WASAPIInput: Failed to create device enumerator\n");
        return false;
    }

    if (deviceId.empty()) {
        hr = m_impl->enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &m_impl->device);
    } else {
        hr = m_impl->enumerator->GetDevice(toWide(deviceId).c_str(), &m_impl->device);
    }
    if (FAILED(hr)) {
        printf("WASAPIInput: Failed to get capture endpoint\n");
        return false;
    }

    hr = m_impl->device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                                  (void**)&m_impl->audioClient);
    if (FAILED(hr)) {
        printf("WASAPIInput: Failed to activate audio client\n");
        return false;
    }

    WAVEFORMATEX* mixFormat = nullptr;
    hr = m_impl->audioClient->GetMixFormat(&mixFormat);
    if (FAILED(hr)) {
        printf("WASAPIInput: Failed to get mix format\n");
        return false;
    }

    // The shared-mode mix format is always float32; optional overrides mirror
    // what the Qt code did with QAudioFormat before QAudioSource::start().
    if (sampleRate > 0) {
        mixFormat->nSamplesPerSec = static_cast<WORD>(sampleRate);
    }
    if (channels > 0) {
        mixFormat->nChannels = static_cast<WORD>(channels);
        mixFormat->nBlockAlign = mixFormat->nChannels * (mixFormat->wBitsPerSample / 8);
        mixFormat->nAvgBytesPerSec = mixFormat->nSamplesPerSec * mixFormat->nBlockAlign;
    }

    m_impl->bufferMs = bufferMs;
    const REFERENCE_TIME bufferDuration = static_cast<REFERENCE_TIME>(bufferMs * 10000.0);
    // AUTOCONVERTPCM lets the shared engine convert the requested rate/channel
    // layout to the device mix format, which is what QAudioSource did
    // internally for formats the capture device did not provide directly.
#ifndef AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM
#define AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM 0x08000000
#endif
    hr = m_impl->audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                         AUDCLNT_STREAMFLAGS_EVENTCALLBACK |
                                             AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM,
                                         bufferDuration, bufferDuration, mixFormat, nullptr);
    if (FAILED(hr)) {
        printf("WASAPIInput: Failed to init audio client (hr=0x%08lx)\n", hr);
        CoTaskMemFree(mixFormat);
        return false;
    }

    m_impl->waveFormat = mixFormat;
    m_impl->audioClient->GetBufferSize(&m_impl->bufferFrameCount);

    hr = m_impl->audioClient->GetService(__uuidof(IAudioCaptureClient),
                                         (void**)&m_impl->captureClient);
    if (FAILED(hr)) {
        printf("WASAPIInput: Failed to get capture client\n");
        CoTaskMemFree(mixFormat);
        return false;
    }

    m_impl->hEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    m_impl->audioClient->SetEventHandle(m_impl->hEvent);

    m_impl->format.rate = mixFormat->nSamplesPerSec;
    m_impl->format.channels = mixFormat->nChannels;
    m_impl->format.bufferFrames = m_impl->bufferFrameCount;
    m_impl->format.format = AudioFormat::Float;

    printf("WASAPIInput: %d Hz %d ch, %d frames (%d ms)\n", m_impl->format.rate,
           m_impl->format.channels, m_impl->format.bufferFrames, bufferMs);
    return true;
}

void WASAPIInput::shutdown()
{
    if (m_impl) {
        m_impl->stop();
        if (m_impl->audioClient) m_impl->audioClient->Stop();
    }
    m_impl = std::make_unique<Impl>();
}

void WASAPIInput::start()
{
    if (!m_impl->audioClient || m_impl->running) return;

    m_impl->running = true;
    m_impl->captureThread = std::thread([this]() {
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        const int channels = m_impl->format.channels;
        while (m_impl->running) {
            const DWORD waitResult =
                WaitForSingleObject(m_impl->hEvent, static_cast<DWORD>(m_impl->bufferMs * 4));
            if (waitResult != WAIT_OBJECT_0) continue;

            UINT32 packet = 0;
            while (m_impl->captureClient->GetNextPacketSize(&packet) == S_OK && packet > 0) {
                BYTE* data = nullptr;
                UINT32 frames = 0;
                DWORD flags = 0;
                const HRESULT hr =
                    m_impl->captureClient->GetBuffer(&data, &frames, &flags, nullptr, nullptr);
                if (FAILED(hr)) break;

                float* input = reinterpret_cast<float*>(data);
                if ((flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0 && input) {
                    std::fill(input, input + static_cast<std::size_t>(frames) * channels, 0.0f);
                }
                if (m_impl->callback && input) {
                    m_impl->callback(input, static_cast<int>(frames), m_impl->format);
                }
                m_impl->captureClient->ReleaseBuffer(frames);
            }
        }
        CoUninitialize();
    });

    m_impl->audioClient->Start();
    printf("WASAPIInput: Started\n");
}

void WASAPIInput::stop()
{
    if (!m_impl->running) return;
    m_impl->stop();
    if (m_impl->audioClient) m_impl->audioClient->Stop();
    printf("WASAPIInput: Stopped\n");
}

void WASAPIInput::setCaptureCallback(AudioCaptureCallback cb)
{
    m_impl->callback = std::move(cb);
}

bool WASAPIInput::isInitialized() const
{
    return m_impl->audioClient != nullptr;
}

bool WASAPIInput::isRunning() const
{
    return m_impl->running.load();
}

AudioFormat WASAPIInput::format() const
{
    return m_impl->format;
}

} // namespace ks::audio
