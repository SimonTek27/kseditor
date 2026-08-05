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
| **KS3D** | **`.ks3d`** | **`FileFormat/KS3DReader`** | **Import/Export** | **Native binary scene format** |

**Code:** `src/core/FileFormat/` (30 files) — parsers, converters, format detector, CAD type system.

### KS3D Format (`.ks3d`)

**Native binary scene format** for the KSModeler 3D editor. Stores PBR materials, meshes, textures, and scene hierarchy.

**File layout:** `[FileHeader][StringTable][Materials][Meshes][Textures][Nodes]`

| Component | Contents |
|-----------|----------|
| FileHeader | Magic `0x33534B4B`, version, section offsets/counts |
| StringTable | Deduplicated string pool (names, paths) |
| Materials | PBR properties (base color, metallic, roughness, emissive, clearcoat, sheen, transmission, etc.) + texture references |
| Meshes | Interleaved vertex data (position, normal, UV, tangent, bitangent, bone weights) + triangle indices + submeshes |
| Textures | Optional embedded texture data (PNG/JPG/TGA/BMP/DDS/KTX) |
| Nodes | Scene hierarchy with parent indices, transforms (position + quaternion + scale), mesh/material assignments |

**Code:** `src/core/FileFormat/KS3DFormat.h`, `KS3DReader.h/.cpp`, `KS3DWriter.h/.cpp`

## Audio Formats

| Format | Extension | Support | Notes |
|--------|-----------|---------|-------|
| WAV | `.wav` | Import/Export | PCM, floating point |
| MP3 | `.mp3` | Import/Export | |
| OGG | `.ogg` | Import/Export | Vorbis |
| FLAC | `.flac` | Import/Export | Lossless |
| FMOD Studio Project | `.fspro` | Import/Export | FMOD Studio project format (v1.08.12 schema) |
| FMOD Bank | `.bank` | Read/Write/Encrypt | Version-aware: FMOD 1.x (AC1), FMOD 2.x (ACC/ACR/ACE) |
| Wwise SoundBank | — | Import | Wwise-compatible |
| FSB | `.fsb` | Import | FMOD sample bank |
| ksaudio | `.ksaudio` | Native | JSON format, round-trips with `.fspro` |

**Code:** `src/core/Audio/KSBankParser`, `KSBankWriter`, `KSAudioBankGenerator`, `AudioFormatConverter`, `FSPROImporter`.

### ksaudio Format

**Native JSON-based project format** with **100% bidirectional compatibility** with **FMOD Studio `.fspro` version 1.08.12**.

- **Import:** Reads `.fspro` XML (events, parameters, buses, VCAs, banks, snapshots, mixer routing, DSP chains)
- **Export:** Writes `.fspro` XML fully loadable in FMOD Studio 1.08.12+
- **Internal:** `.ksaudio` JSON stores full signal graph, automation, plugin state, and asset references
- **Round-trip:** Lossless conversion between `.ksaudio` ↔ `.fspro` (1.08.12 schema)
- **Content:** Banks, events, mixer busses, DSP chains, volume (dB), pitch (semitones), loop, attenuation, timeline
- **Bank export:** `.ksaudio`/`.fspro` can be compiled to `.bank` for runtime use in Assetto Corsa

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
