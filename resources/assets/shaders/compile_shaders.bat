@echo off
REM Compile GLSL shaders to SPIR-V using glslc (part of Vulkan SDK)
REM Run this script before building ksEditor if you use Vulkan rendering

set SHADER_SRC=..\..\src\core\Graphics\shaders
set SHADER_OUT=%~dp0

echo Compiling shaders...
glslc %SHADER_SRC%\ksPerPixel.vert     -o %SHADER_OUT%ksPerPixel.vert.spv
glslc %SHADER_SRC%\ksPerPixel.frag     -o %SHADER_OUT%ksPerPixel.frag.spv
glslc %SHADER_SRC%\ksPerPixelAT.vert   -o %SHADER_OUT%ksPerPixelAT.vert.spv
glslc %SHADER_SRC%\ksPerPixelAT.frag   -o %SHADER_OUT%ksPerPixelAT.frag.spv
glslc %SHADER_SRC%\ksMultilayer.vert   -o %SHADER_OUT%ksMultilayer.vert.spv
glslc %SHADER_SRC%\ksMultilayer.frag   -o %SHADER_OUT%ksMultilayer.frag.spv
glslc %SHADER_SRC%\ksPostProcess.vert  -o %SHADER_OUT%ksPostProcess.vert.spv
glslc %SHADER_SRC%\ksPostProcess.frag  -o %SHADER_OUT%ksPostProcess.frag.spv
glslc %SHADER_SRC%\ksSky.vert          -o %SHADER_OUT%ksSky.vert.spv
glslc %SHADER_SRC%\ksSky.frag          -o %SHADER_OUT%ksSky.frag.spv
glslc %SHADER_SRC%\ksShadow.vert       -o %SHADER_OUT%ksShadow.vert.spv
glslc %SHADER_SRC%\ksShadow.frag       -o %SHADER_OUT%ksShadow.frag.spv
echo Done.
