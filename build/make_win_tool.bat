@echo off
setlocal enabledelayedexpansion

if not exist build_win_tool ( md build_win_tool )
pushd build_win_tool
if !ERRORLEVEL! NEQ 0 ( exit /b !ERRORLEVEL! )
echo cmake generate build_win_tool
cmake ../.. -G "Visual Studio 17 2022" -A x64 -DNEKOFS_MAKE_TOOLS_LIB=ON
if !ERRORLEVEL! NEQ 0 ( exit /b !ERRORLEVEL! )
popd
echo cmake build build_win_tool
cmake --build build_win_tool --config Release

pause