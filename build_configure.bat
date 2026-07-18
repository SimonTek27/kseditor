cd E:\Users\Simon\source\repos\kseditor
rmdir /s /q build 2>nul
mkdir build
cd build
cmake -G "Visual Studio 18 2026" -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.11.1/msvc2022_64" -S .. -B .