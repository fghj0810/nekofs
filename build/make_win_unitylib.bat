@echo off
setlocal enabledelayedexpansion

if not exist build_win64_unitylib ( md build_win64_unitylib )
pushd build_win64_unitylib
if !ERRORLEVEL! NEQ 0 ( exit /b !ERRORLEVEL! )
echo cmake generate build_win64_unitylib
cmake ../.. -G "Visual Studio 17 2022" -A x64
if !ERRORLEVEL! NEQ 0 ( exit /b !ERRORLEVEL! )
popd
echo cmake build build_win64_unitylib
cmake --build build_win64_unitylib --config Release
if !ERRORLEVEL! NEQ 0 ( exit /b !ERRORLEVEL! )
if not exist plugin_unity\x86_64 ( md plugin_unity\x86_64 )
copy /Y build_win64_unitylib\nekofs\Release\nekofs.dll plugin_unity\x86_64\nekofs.dll

pause