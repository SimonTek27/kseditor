# Contributing to ksEditor

## Development Setup

### Prerequisites
- Qt 6.2+ (Core, Gui, Qml, Quick, Widgets, Network, Multimedia, OpenGL, Sql)
- CMake 3.16+
- Visual Studio 2022 or GCC 10+
- FMOD Studio SDK (for audio)
- Vulkan SDK (for 3D rendering)

### Building
```bash
# Clone the repository
git clone https://github.com/yourrepo/kseditor.git
cd kseditor

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --config Release
```

### Running
```bash
# From build directory
./kseditor.exe
```

## Code Style

### QML Files
- Use 4-space indentation
- Follow established prefix naming: `modeler_`, `audio_`, `phys_`, `collab_`
- Use color scheme: Background `#1e1e1e`, Primary `#4fc3f7`

### C++ Files
- Follow Qt naming conventions
- Use `Q_OBJECT` macro for QML-exposed classes
- Document public APIs with Doxygen-style comments

## Adding New Features

### New Editor (QML)
1. Create in `qml/modules/{module}/`
2. Follow naming: `{prefix}_{FeatureName}.qml`
3. Use existing color scheme
4. Test in development build

### New Module (C++)
1. Create header in `src/modules/{module}/`
2. Add to `SOURCES` in CMakeLists.txt
3. Register in module manager

## Testing
```bash
cd tests
mkdir build
cd build
cmake ..
cmake --build .
ctest
```

## Reporting Issues
- Include CMake output
- Qt version
- Platform (Windows/Linux)
- Error messages

## License
Public Domain
