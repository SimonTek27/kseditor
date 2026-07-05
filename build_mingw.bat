@echo off
set PATH=C:\Qt\Tools\mingw1310_64\bin;C:\Program Files\CMake\bin;%PATH%
set Qt6_DIR=C:\Qt\6.11.1\mingw_64\lib\cmake\Qt6
set CMAKE_PREFIX_PATH=C:\Qt\6.11.1\mingw_64
cd /d E:\Users\Simon\source\repos\kseditor
rmdir /s /q build 2>nul
mkdir build
cd build
cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=C:\Qt\6.11.1\mingw_64 ..
if exist Makefile (
    mingw32-make -j4
) else (
    echo Makefile not found!
)
Unblock-File -Path "E:\Users\Simon\source\repos\kseditor\bin\ksEditor.exe"