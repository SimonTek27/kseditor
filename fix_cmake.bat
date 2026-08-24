@echo off
cd /d E:\Users\Simon\source\repos\kseditor

:: Remove old build cache
if exist build\CMakeCache.txt (
    del /f build\CMakeCache.txt
)

:: Configure CMake
cmake -S . -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.15 -DQT6_PATH="C:/Qt/6.11.1/msvc2022_64" -DCMAKE_PREFIX_PATH="" 2>&1 | find "Error" > NUL

if errorlevel 1 (
    echo CMake configuration failed
    pause
    exit /b 1
)

:: Build
cmake --build build --config Debug 2>&1 | find "error" > NUL

if errorlevel 1 (
    echo Build failed
    pause
    exit /b 1
)

echo Build succeeded
pause