# File Formats

ksEditor supports a wide range of file formats across 3D models, audio, configuration, and simulation data.

## 3D Model Formats

| Format | Extension | Parser | Direction | Notes |
|--------|-----------|--------|-----------|-------|
| Wavefront OBJ | `.obj` | `FileFormat/CADOBJParser` | Import/Export | CAD converter wrapper |
| GLTF/GLB | `.glb`, `.gltf` | `FileFormat/GLBParser` | Import/Export | Binary + text |
| STL | `.stl` | `FileFormat/STLParser` | Import/Export | ASCII + binary |
| FBX | `.fbx` | `FileFormat/FBXParser` | Import | Autodesk FBX |
| KN5 | `.kn5` | `plugins/.../KN5Parser` | Import | Encrypted AC format |
| ACD | `.acd` | `plugins/.../ACDParser` | Import | AC data format |

**Code:** `src/core/FileFormat/` (30 files) — parsers, converters, format detector, CAD type system.

## Audio Formats

| Format | Extension | Support | Notes |
|--------|-----------|---------|-------|
| WAV | `.wav` | Import/Export | PCM, floating point |
| MP3 | `.mp3` | Import/Export | |
| OGG | `.ogg` | Import/Export | Vorbis |
| FLAC | `.flac` | Import/Export | Lossless |
| FMOD Bank | `.bank` | Read/Write/Encrypt | Full FMOD bank compatibility |
| Wwise SoundBank | — | Import | Wwise-compatible |
| FSB | `.fsb` | Import | FMOD sample bank |
| ksaudio | `.ksaudio` | Native | Custom JSON format |

**Code:** `src/core/Audio/KSBankParser`, `KSBankWriter`, `KSAudioBankGenerator`, `AudioFormatConverter`.

### ksaudio Format

Custom JSON-based project format for the audio editor:
- Banks, events, mixer busses, DSP chains
- FMOD template compatible
- Volume (dB), pitch (semitones), loop, attenuation, timeline

## Config & Data Formats

| Format | Extension | Parser | Usage |
|--------|-----------|--------|-------|
| JSON | `.json` | `FileFormat/JSONParser` | Project files, settings |
| INI | `.ini` | `FileFormat/INIParser`, `KsIni` | AC config files |
| Lua | `.lua` | `core/Scripting/luaScript` | Scripting |
| Python | `.py` | `core/Scripting/python` | Scripting |
| Setup | `.setup` | `core/FileFormat` | Car setup data |
| Replay | `.replay` | `FileFormat/ReplayParser` | Telemetry replay |
| CSP Config | — | `core/Config` | Custom Shaders Patch configs |

## Format Detection

`FileFormat/CADFormatDetector` auto-detects 3D model formats. `FormatConverter` handles inter-format conversion.
