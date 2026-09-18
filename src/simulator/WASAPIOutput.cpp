#include "WASAPIOutput.h"
#include <cstdio>
#include <thread>
#include <atomic>
#include <mmdeviceapi.h>
#include <Audioclient.h>

#pragma comment(lib, "ole32.lib")

namespace ks::sim {

struct WASAPIOutput::Impl {
    IMMDeviceEnumerator* enumerator = nullptr;
    IMMDevice* device = nullptr;
    IAudioClient* audioClient = nullptr;
    IAudioRenderClient* renderClient = nullptr;
    WAVEFORMATEX* waveFormat = nullptr;
    UINT32 bufferFrameCount = 0;
    HANDLE hEvent = nullptr;
    std::atomic<bool> running{false};
    std::thread renderThread;
    AudioRenderCallback callback;
    AudioFormat format;
    int bufferMs = 20;

    ~Impl() {
        stop();
        if (audioClient) audioClient->Stop();
        if (hEvent) CloseHandle(hEvent);
        if (renderClient) renderClient->Release();
        if (audioClient) audioClient->Release();
        if (waveFormat) CoTaskMemFree(waveFormat);
        if (device) device->Release();
        if (enumerator) enumerator->Release();
    }

    void stop() {
        running = false;
        if (renderThread.joinable()) renderThread.join();
    }
};

WASAPIOutput::WASAPIOutput() : m_impl(std::make_unique<Impl>()) {}
WASAPIOutput::~WASAPIOutput() { shutdown(); }

bool WASAPIOutput::initialize(int sampleRate, int channels, int bufferMs)
{
    if (m_impl->audioClient) return true;

    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    HRESULT hr;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                          __uuidof(IMMDeviceEnumerator), (void**)&m_impl->enumerator);
    if (FAILED(hr)) { printf("WASAPIOutput: Failed to create device enumerator\n"); return false; }

    hr = m_impl->enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_impl->device);
    if (FAILED(hr)) { printf("WASAPIOutput: Failed to get default audio endpoint\n"); return false; }

    hr = m_impl->device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&m_impl->audioClient);
    if (FAILED(hr)) { printf("WASAPIOutput: Failed to activate audio client\n"); return false; }

    WAVEFORMATEX* mixFormat = nullptr;
    hr = m_impl->audioClient->GetMixFormat(&mixFormat);
    if (FAILED(hr)) { printf("WASAPIOutput: Failed to get mix format\n"); return false; }

    if (sampleRate > 0) {
        mixFormat->nSamplesPerSec = static_cast<WORD>(sampleRate);
    }
    if (channels > 0) {
        mixFormat->nChannels = static_cast<WORD>(channels);
        mixFormat->nBlockAlign = mixFormat->nChannels * (mixFormat->wBitsPerSample / 8);
        mixFormat->nAvgBytesPerSec = mixFormat->nSamplesPerSec * mixFormat->nBlockAlign;
    }

    m_impl->bufferMs = bufferMs;
    REFERENCE_TIME bufferDuration = (REFERENCE_TIME)(bufferMs * 10000.0);
    hr = m_impl->audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                                          bufferDuration, bufferDuration, mixFormat, nullptr);
    if (FAILED(hr)) {
        printf("WASAPIOutput: Failed to init audio client (hr=0x%08lx)\n", hr);
        CoTaskMemFree(mixFormat);
        return false;
    }

    m_impl->waveFormat = mixFormat;
    m_impl->audioClient->GetBufferSize(&m_impl->bufferFrameCount);

    hr = m_impl->audioClient->GetService(__uuidof(IAudioRenderClient), (void**)&m_impl->renderClient);
    if (FAILED(hr)) {
        printf("WASAPIOutput: Failed to get render client\n");
        CoTaskMemFree(mixFormat);
        return false;
    }

    m_impl->hEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    m_impl->audioClient->SetEventHandle(m_impl->hEvent);

    m_impl->format.sampleRate = mixFormat->nSamplesPerSec;
    m_impl->format.channels = mixFormat->nChannels;
    m_impl->format.bufferFrames = m_impl->bufferFrameCount;

    printf("WASAPIOutput: %d Hz %d ch, %d frames (%d ms)\n",
           m_impl->format.sampleRate, m_impl->format.channels,
           m_impl->format.bufferFrames, bufferMs);

    return true;
}

void WASAPIOutput::shutdown()
{
    if (m_impl) {
        m_impl->stop();
        if (m_impl->audioClient) m_impl->audioClient->Stop();
    }
    m_impl = std::make_unique<Impl>();
}

void WASAPIOutput::start()
{
    if (!m_impl->audioClient || m_impl->running) return;

    m_impl->running = true;
    m_impl->renderThread = std::thread([this]() {
        BYTE* pData;
        while (m_impl->running) {
            DWORD waitResult = WaitForSingleObject(m_impl->hEvent, m_impl->bufferMs * 2);
            if (waitResult != WAIT_OBJECT_0) continue;

            UINT32 numFramesPadding = 0;
            m_impl->audioClient->GetCurrentPadding(&numFramesPadding);
            UINT32 numFramesAvailable = m_impl->bufferFrameCount - numFramesPadding;
            if (numFramesAvailable == 0) continue;

            HRESULT hr = m_impl->renderClient->GetBuffer(numFramesAvailable, &pData);
            if (FAILED(hr)) continue;

            float* output = reinterpret_cast<float*>(pData);

            if (m_impl->callback) {
                m_impl->callback(output, numFramesAvailable, m_impl->format);
            } else {
                for (UINT32 i = 0; i < numFramesAvailable * m_impl->format.channels; ++i)
                    output[i] = 0;
            }

            m_impl->renderClient->ReleaseBuffer(numFramesAvailable, 0);
        }
    });

    m_impl->audioClient->Start();
    printf("WASAPIOutput: Started\n");
}

void WASAPIOutput::stop()
{
    if (!m_impl->running) return;
    m_impl->stop();
    if (m_impl->audioClient) m_impl->audioClient->Stop();
    printf("WASAPIOutput: Stopped\n");
}

void WASAPIOutput::setRenderCallback(AudioRenderCallback cb)
{
    m_impl->callback = std::move(cb);
}

bool WASAPIOutput::isInitialized() const
{
    return m_impl->audioClient != nullptr;
}

AudioFormat WASAPIOutput::format() const
{
    return m_impl->format;
}

} // namespace ks::sim
