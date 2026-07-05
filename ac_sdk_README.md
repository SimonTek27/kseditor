# Assetto Corsa SDK for ksEditor

Comprehensive SDK for Assetto Corsa game content editing and simulation.

## Files Overview

### Core
- `SDKBackend.h/cpp` - Main SDK class with car/track loading
- `ac_constants.h` - Physics constants and math helpers
- `ac_shaders.h` - Shader definitions
- `ac_ui.h` - UI themes and telemetry display
- `ac_audio.h` - FMOD audio integration

### Data Formats
- `ac_kn5.h` - KN5 3D model format
- `ac_ini.h` - INI configuration parser
- `ac_track.h` - Track geometry and waypoints
- `ac_script.h` - Lua/Python script generators

### Features
- `ac_config.h` - Game configuration management
- `ac_setup.h` - Car setup save/load/compare
- `ac_weather.h` - Weather system
- `ac_telemetry.h` - Telemetry recording/analyzer
- `ac_network.h` - Multiplayer server/client
- `ac_ai.h` - AI driver simulation
- `ac_replay.h` - Replay file handling
- `ac_util.h` - Utilities (launch, backup, crash reporter)
- `ac_physics.h` - Physics simulation

### Example
- `SDKExample.cpp` - Complete usage examples

## Quick Start

```cpp
#include "SDKBackend.h"

// Initialize
ks::SDKBackend* sdk = ks::SDKBackend::instance();
sdk->initialize();

// Load cars
QStringList cars = ks::SDKBackend::getCarList();

// Load car spec
ks::ACCarSpec spec;
ks::SDKBackend::loadCarSpec("ks_nissan_gtr", spec);

// Load track
float length = ks::SDKBackend::getTrackLength("monza");
```

## Features

### Content Management
- List and load cars/tracks
- Parse car.ini, tyres.ini, engine.ini
- Track geometry and waypoints

### Physics
- Full vehicle dynamics simulation
- Suspension, brakes, aero
- Lap time estimation

### Telemetry
- Record and analyze driving data
- Export to CSV
- Lap analysis

### AI
- Driver profiles
- Rubber-banding
- Pathfinding

### Setup
- Save/load car setups
- Compare setups
- Optimize for conditions

## Requirements

- Qt 6.11.1+
- C++17

## License

MIT License - see [LICENSE.txt](LICENSE.txt) for details