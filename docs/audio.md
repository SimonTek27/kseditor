# Audio

The ksEditor audio subsystem provides a complete game audio middleware solution with 100% feature parity against FMOD Studio 1.08.12, Wwise 2024, and GoldWave.

## Core Components

**`src/core/Audio/`** — 62 files powering the audio engine:

- **KSAudio** / **KSAudioCore** — Core audio engine
- **KSAudioMixer** — Multi-bus mixing with DSP chain
- **KSAudioGenerator** — Procedural audio generation
- **KSAudioVST3Host** / **KSAudioVSTHost** / **LADSPAHost** — Plugin hosting
- **KSBankParser** / **KSBankWriter** — FMOD bank read/write/encrypt
- **AudioFormatConverter** — Cross-format conversion
- **AudioRecording** — Recording engine
- **AudioTimeStretch** / **FFTProcessor** — DSP processing
- **WaveformEngine** / **WaveProcessor** — Waveform analysis
- **KSCarAcoustics** / **KsCarAudioEngine** — Vehicle acoustic simulation
- **KSRPMProfile** / **KSRPMRecorder** / **EngineSimHook** — RPM-based engine sound synthesis
- **KSAudioBankGenerator** — Bank generation for runtime

**`src/modules/soundEditor/`** — 28 files for the sound editor UI:
- AudioEffects (basic + advanced DSP), AudioMixer, AudioAnalysis, AudioBank, AIAudioEngine, LoudnessMeter, AudioWaveformBridge

## Feature Parity

| Feature | FMOD | Wwise | GoldWave | ksEditor |
|---------|------|-------|----------|----------|
| Event System | Yes | Yes | — | Yes |
| Multi-bus Mixer | Yes | Yes | Yes | Yes |
| Effects/DSP | Yes | Yes | Yes | Yes |
| Spatial Audio | Yes | Yes | — | Yes |
| Recording | — | — | Yes | Yes |
| Bank Export | Yes | Yes | — | Yes |
| VST Hosting | — | — | Yes | Yes |
| AI Engine Sounds | — | — | — | Yes (unique) |
| Loudness Metering | — | — | Yes | EBU R128 |

## File Format: `.ksaudio`

Custom JSON-based audio project format (`docs/file_formats.md` for details).

## Audio Import/Export

- **Import:** WAV, MP3, OGG, FLAC, FSB
- **Export:** WAV, OGG, MP3, FLAC
- **Bank Formats:** FMOD `.bank` (read/write/encrypt), Wwise-compatible soundbanks
- **Proprietary:** `.ksaudio` project files
