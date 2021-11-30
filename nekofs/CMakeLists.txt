cmake_minimum_required (VERSION 3.8)

project(nekofs)

set(CMAKE_CXX_STANDARD 17)

if (ANDROID_ABI AND ("${ANDROID_ABI}" STREQUAL "x86" OR "${ANDROID_ABI}" STREQUAL "armeabi-v7a"))
    add_definitions("-D_FILE_OFFSET_BITS=64")
endif ()

if (WIN32)
    MESSAGE(STATUS "Now is windows")
    add_definitions("-D_UNICODE" "-DUNICODE")
    remove_definitions("-D_MBCS")
endif ()

set(NEKOFS_NATIVE
    native_win/nativefile.h
    native_win/nativefile.cpp
    native_win/nativefileblock.h
    native_win/nativefileblock.cpp
    native_win/nativefilehandle.h
    native_win/nativefilehandle.cpp
    native_win/nativefilesystem.h
    native_win/nativefilesystem.cpp
    native_win/nativefileistream.h
    native_win/nativefileistream.cpp
    native_win/nativefileostream.h
    native_win/nativefileostream.cpp
)

set(NEKOFS_COMMON
    common/typedef.h
    common/noncopyable.h
    common/nonmovable.h
    common/utils.h
    common/utils.cpp
    common/env.h
    common/env.cpp
)

set(NEKOFS
    nekofs.cpp
    nekofs.h
    typedef.h
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${NEKOFS_NATIVE})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${NEKOFS_COMMON})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${NEKOFS})

    add_library(${PROJECT_NAME} SHARED
        ${NEKOFS}
        ${NEKOFS_COMMON}
        ${NEKOFS_NATIVE}
    )
