@echo off
setlocal

REM One-shot build script for Windows (requires CMake 3.24+ and MSVC).
REM Usage: build_windows.bat [build_dir]

set "BUILD_DIR=%~1"
if "%BUILD_DIR%"=="" set "BUILD_DIR=build"

echo Configuring...
cmake -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1

echo Building...
cmake --build "%BUILD_DIR%" --config Release --parallel
if errorlevel 1 exit /b 1

echo.
echo Build complete. Artefacts in %BUILD_DIR%\SfxrVsti_artefacts\Release
echo.
echo To install the VST3, copy it to:
echo   C:\Program Files\Common Files\VST3\
echo (requires Administrator privileges)

endlocal
