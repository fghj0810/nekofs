@echo off
setlocal enabledelayedexpansion

if "%ANDROID_NDK%" EQU "" (
    echo ANDROID_NDK not set
    exit 1
)

if not exist build_android_unitylib_v7a ( md build_android_unitylib_v7a )
pushd build_android_unitylib_v7a
if !ERRORLEVEL! NEQ 0 ( exit /b !ERRORLEVEL! )
echo cmake generate build_android_unitylib_v7a
cmake ../.. "-GNinja" -DANDROID_ABI=armeabi-v7a -DANDROID_NDK=%ANDROID_NDK% -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=%ANDROID_NDK%\build\cmake\android.toolchain.cmake -DANDROID_TOOLCHAIN=clang
if !ERRORLEVEL! NEQ 0 ( exit /b !ERRORLEVEL! )
popd
echo cmake build build_android_unitylib_v7a
cmake --build build_android_unitylib_v7a --config Release
if !ERRORLEVEL! NEQ 0 ( exit /b !ERRORLEVEL! )
if not exist plugin_unity\Android\Libs\armeabi-v7a ( md plugin_unity\Android\Libs\armeabi-v7a )
copy /Y build_android_unitylib_v7a\nekofs\libnekofs.so plugin_unity\Android\Libs\armeabi-v7a\libnekofs.so

if not exist build_android_unitylib_v8a ( md build_android_unitylib_v8a )
pushd build_android_unitylib_v8a
if !ERRORLEVEL! NEQ 0 ( exit /b !ERRORLEVEL! )
echo cmake generate build_android_unitylib_v8a
cmake ../.. "-GNinja" -DANDROID_ABI=arm64-v8a -DANDROID_NDK=%ANDROID_NDK% -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=%ANDROID_NDK%\build\cmake\android.toolchain.cmake -DANDROID_TOOLCHAIN=clang
if !ERRORLEVEL! NEQ 0 ( exit /b !ERRORLEVEL! )
popd
echo cmake build build_android_unitylib_v8a
cmake --build build_android_unitylib_v8a --config Release
if !ERRORLEVEL! NEQ 0 ( exit /b !ERRORLEVEL! )
if not exist plugin_unity\Android\Libs\arm64-v8a ( md plugin_unity\Android\Libs\arm64-v8a )
copy /Y build_android_unitylib_v8a\nekofs\libnekofs.so plugin_unity\Android\Libs\arm64-v8a\libnekofs.so

if not exist build_android_unitylib_x86 ( md build_android_unitylib_x86 )
pushd build_android_unitylib_x86
if !ERRORLEVEL! NEQ 0 ( exit /b !ERRORLEVEL! )
echo cmake generate build_android_unitylib_x86
cmake ../.. "-GNinja" -DANDROID_ABI=x86 -DANDROID_NDK=%ANDROID_NDK% -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=%ANDROID_NDK%\build\cmake\android.toolchain.cmake -DANDROID_TOOLCHAIN=clang
if !ERRORLEVEL! NEQ 0 ( exit /b !ERRORLEVEL! )
popd
echo cmake build build_android_unitylib_x86
cmake --build build_android_unitylib_x86 --config Release
if !ERRORLEVEL! NEQ 0 ( exit /b !ERRORLEVEL! )
if not exist plugin_unity\Android\Libs\x86 ( md plugin_unity\Android\Libs\x86 )
copy /Y build_android_unitylib_x86\nekofs\libnekofs.so plugin_unity\Android\Libs\x86\libnekofs.so

if not exist build_android_unitylib_x86_64 ( md build_android_unitylib_x86_64 )
pushd build_android_unitylib_x86_64
if !ERRORLEVEL! NEQ 0 ( exit /b !ERRORLEVEL! )
echo cmake generate build_android_unitylib_x86_64
cmake ../.. "-GNinja" -DANDROID_ABI=x86_64 -DANDROID_NDK=%ANDROID_NDK% -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=%ANDROID_NDK%\build\cmake\android.toolchain.cmake -DANDROID_TOOLCHAIN=clang
if !ERRORLEVEL! NEQ 0 ( exit /b !ERRORLEVEL! )
popd
echo cmake build build_android_unitylib_x86_64
cmake --build build_android_unitylib_x86_64 --config Release
if !ERRORLEVEL! NEQ 0 ( exit /b !ERRORLEVEL! )
if not exist plugin_unity\Android\Libs\x86_64 ( md plugin_unity\Android\Libs\x86_64 )
copy /Y build_android_unitylib_x86_64\nekofs\libnekofs.so plugin_unity\Android\Libs\x86_64\libnekofs.so

echo "compile success"
pause