# ksEditor

A comprehensive, professional-grade modding toolkit for Assetto Corsa and other Kunos/Steam racing games. ksEditor provides a unified environment for editing game audio, 3D models, physics, telemetry, and more.

## Overview

ksEditor is a Qt6-based desktop application designed to provide modders with professional editing tools for racing game content. It combines multiple specialized editors into a single, cohesive environment with native performance and modern UI.

The editor supports the full Assetto Corsa modding pipeline, from audio synthesis to 3D modeling and physics simulation. It is compatible with FMOD Studio 1.08.12 project formats used by the game.

## Features

### Audio Editor (KSWaveEditor)
Professional audio editing and processing module for car sounds, engine notes, and environmental audio.

**Panels:**
- AudioMixerPanel - Multi-track mixer with channel strips, VU meters, master bus
- EffectsRackPanel - Effects chain: EQ, Compressor, Gate, Reverb, Delay, Limiter
- AudioAnalyzerPanel - VU meters, spectrum analyzer, phase scope
- LoudnessMeterPanel - EBU R128 loudness measurement (LUFS, True Peak, LRA)
- SidechainCompressorPanel - Dynamics processing with external sidechain
- AudioAutomationPanel - Volume/Pan/Filter automation with point editing
- SurroundSoundMixerPanel - 5.1/7.1/7.1.2/7.1.4 surround mixing
- MarkerRegionEditor - Timeline markers and regions
- MultibandCompressorPanel - 4-band dynamic EQ with crossover visualization
- ConvolutionReverbPanel - Impulse response reverb with presets
- StereoEnhancerPanel - Stereo width and mid/side processing
- TapeEmulatorPanel - Analog tape simulation (2"/1"/Cassette/VHS/DAT)
- DitherShapingPanel - Bit depth conversion and noise shaping
- VocalProcessorPanel - De-esser, compressor, noise gate, pitch correction
- GuitarAmpSimulatorPanel - Amplifier and cabinet simulation
- HarmonicGeneratorPanel - Even/odd/sub harmonic generation
- TransientDesignerPanel - Attack/sustain/release shaping
- FrequencyAnalyzerPanel - FFT/1/3 OCT spectrum analysis
- OscilloscopePanel - Real-time waveform display

**Dialogs:**
- Import/Export wizards, Equalizer, Tone generator, Effects chains
- Dynamics processing, BPM detection, Recording dialog
- Audio settings, Preset manager, Keyboard shortcuts
- Fade curves, Metadata editor

### 3D Modeler (KSModeler)
Vulkan-based 3D modeling module for creating and editing cars, tracks, and characters.

**Features:**
- Mesh Primitives: Cube, Sphere, Cylinder, Cone, Torus, Plane, Grid
- UV Mapping: Planar, Cylindrical, Spherical, Box projection; Unwrap; Pack; Auto UVs
- Skeleton/Rigging: Humanoid/Quadruped skeleton creation, Bone tools, Weight painting, IK solvers
- Material/Shader: PBR material properties, Shader editor
- Animation: Keyframes, Tracks, Playback controls
- File Format Converters: KN5, FBX, GLB, OBJ

**Editors:**
- CarEditor - Vehicle model editing, livery painter, tire/engine rig tools
- TrackEditor - Track geometry, terrain editing, waypoints
- CharacterEditor - Driver/character model editing

**Panels:**
- Project Explorer, Properties Inspector, Tools Options
- Layers, History, Python Console, Shader Graph Editor
- Material Preset Library, Curve Editor, Render Settings

### Physics Editor (ksPhysicsEditor)
Physics simulation and vehicle dynamics editor for tuning car handling.

**Features:**
- Suspension setup, brake tuning, aero configuration
- Tire physics modeling, engine/gearbox simulation
- Lap time estimation, performance analysis

### Additional Modules

| Module | Description |
|--------|-------------|
| **ksDisplayEditor** | UI and display customization |
| **ksFontEditor** | Custom font generation for in-game UI |
| **ksSetupEditor** | Car setup save/load/compare tools |
| **ksTelemetryViewer** | Telemetry recording and analysis with lap timing |
| **ksAIEditor** | AI driver behavior and rubber-banding configuration |
| **ksPPFiltersEditor** | Post-processing filter management |
| **ksWorkshop** | Community mod sharing and management |
| **ksModManager** | Content organization and installation |
| **ksLicensePlates** | Custom license plate creation |
| **AssetsLibrary** | Asset browser and organization |

## Technical Stack

- **UI Framework:** Qt6 / QML
- **3D Rendering:** Vulkan (via QVulkanWindow), optional Qt3D
- **Audio Processing:** Qt6::Multimedia
- **Build System:** CMake (C++17)
- **Audio Engine:** FMOD Studio 1.08.12 compatible

## Audio Engine: ksAudioStudio

ksAudioStudio is the internal audio workstation integrated into ksEditor, serving as a replacement/alternative to FMOD Studio. It provides:

- Complete FMOD Studio 1.08.12 project compatibility - import and export existing .bank files
- Internal audio format "KSaudio" for native project storage
- Full audio production pipeline: recording, synthesis, effects, mixing
- VST plugin hosting support for third-party audio effects
- Multi-channel surround mixing (up to 7.1.4)
- Real-time audio analysis and visualization

ksEditor uses ksAudioStudio as its audio backend for all sound design work within the application.

## Project Structure

```
kseditor/
├── CMakeLists.txt           # Build configuration
├── src/
│   ├── main.cpp             # Application entry point
│   ├── SDKBackend.cpp/h     # Game SDK integration
│   ├── core/                # Core utilities (Audio, Graphics, Config)
│   ├── common/             # Shared utilities, Vulkan integration
│   ├── modules/             # Editor modules
│   │   ├── ksAudioEditor/  # Audio editing module
│   │   ├── ksmodeler/      # 3D modeling module
│   │   ├── ksphysicseditor/
│   │   ├── kstelemetryviewer/
│   │   └── ...
│   └── plugins/            # Game plugin system
├── resources/
│   ├── ui/                 # Qt UI resources, Ribbon UI
│   └── qml/                # QML UI components
├── tests/                  # Unit tests
├── i18n/                   # Localization (en, de, ja, it)
└── docs/                   # Documentation
```

## Building

**Requirements:**
- Qt 6.x
- CMake 3.16+
- C++17 compatible compiler
- Vulkan SDK (optional, for 3D features)

**Build on Windows:**
```powershell
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

The compiled executable will be in the `bin/` directory.

**Build Scripts:**
- `compile.bat` / `compile.sh` - Basic compilation
- `build.bat` - Windows build with deployment

## SDK Integration

ksEditor includes a comprehensive SDK (`SDKBackend`) for Assetto Corsa content:

```cpp
#include "SDKBackend.h"

// Initialize
ks::SDKBackend* sdk = ks::SDKBackend::instance();
sdk->initialize();

// List content
QStringList cars = ks::SDKBackend::getCarList();
QStringList tracks = ks::SDKBackend::getTrackList();

// Load car/track specs
ks::ACCarSpec spec;
ks::SDKBackend::loadCarSpec("ks_nissan_gtr", spec);

// Physics calculations
float downforce = ks::SDKBackend::calculateDownforce(speed, aoa, cl);
float cornerG = ks::SDKBackend::calculateCornerG(speedMs, radius);
```

## License

MIT License - see [LICENSE.txt](LICENSE.txt) for details

## Version

Current SDK Version: 1.4

---

For support and contributions, see CONTRIBUTING.md