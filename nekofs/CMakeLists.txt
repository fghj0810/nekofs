cmake_minimum_required (VERSION 3.8)

project(nekofs)

set(CMAKE_CXX_STANDARD 17)

if (ANDROID_ABI AND ("${ANDROID_ABI}" STREQUAL "x86" OR "${ANDROID_ABI}" STREQUAL "armeabi-v7a"))
    add_definitions("-D_FILE_OFFSET_BITS=64")
endif ()

if (WIN32)
    add_definitions("-D_UNICODE" "-DUNICODE")
    remove_definitions("-D_MBCS")
    add_definitions("-DNOMINMAX")

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
else ()
    set(NEKOFS_NATIVE
        native_posix/nativefile.h
        native_posix/nativefile.cpp
        native_posix/nativefileblock.h
        native_posix/nativefileblock.cpp
        native_posix/nativefilehandle.h
        native_posix/nativefilehandle.cpp
        native_posix/nativefilesystem.h
        native_posix/nativefilesystem.cpp
        native_posix/nativefileistream.h
        native_posix/nativefileistream.cpp
        native_posix/nativefileostream.h
        native_posix/nativefileostream.cpp
    )
endif ()

set(NEKOFS_COMMON
    common/typedef.h
    common/noncopyable.h
    common/nonmovable.h
    common/utils.h
    common/utils.cpp
    common/env.h
    common/env.cpp
    common/error.h
    common/error.cpp
    common/sha256.h
    common/sha256.cpp
    common/rapidjson.h
    common/rapidjson.cpp
    common/lz4.h
)

set(NEKOFS_LAYER
    layer/overlayfilesystem.h
    layer/overlayfilesystem.cpp
    layer/layerfilesystem.h
    layer/layerfilesystem.cpp
    layer/layerfilesmeta.h
    layer/layerfilesmeta.cpp
    layer/layerversionmeta.h
    layer/layerversionmeta.cpp
)

set(NEKOFS_NEKODATA
    nekodata/util.h
    nekodata/nekodatafilemeta.h
    nekodata/nekodatafilemeta.cpp
    nekodata/nekodataostream.h
    nekodata/nekodataostream.cpp
    nekodata/nekodataarchiver.h
    nekodata/nekodataarchiver.cpp
    nekodata/nekodatafilesystem.h
    nekodata/nekodatafilesystem.cpp
    nekodata/nekodatafile.h
    nekodata/nekodatafile.cpp
    nekodata/nekodataistream.h
    nekodata/nekodataistream.cpp
)

set(NEKOFS_LZ4
    ../thirdparty/lz4/lib/lz4.h
    ../thirdparty/lz4/lib/lz4.c
    ../thirdparty/lz4/lib/lz4hc.h
    ../thirdparty/lz4/lib/lz4hc.c
)

set(NEKOFS
    nekofs.cpp
    include/nekofs/nekofs.h
    include/nekofs/typedef.h
)

if (NEKOFS_MAKE_TOOLS_LIB STREQUAL "ON")
    set(NEKOFS ${NEKOFS}
        tools/prepare.cpp
        tools/prepare.h
        tools/pack.cpp
        tools/pack.h
        tools/unpack.cpp
        tools/unpack.h
        tools/mkdiff.cpp
        tools/mkdiff.h
    )
    ADD_DEFINITIONS("-DNEKOFS_TOOLS")
endif ()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${NEKOFS_NATIVE})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${NEKOFS_COMMON})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${NEKOFS_LAYER})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${NEKOFS_NEKODATA})
source_group(lz4 FILES ${NEKOFS_LZ4})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${NEKOFS})

if (NEKOFS_MAKE_TOOLS_LIB STREQUAL "ON")
    add_library(${PROJECT_NAME} STATIC
        ${NEKOFS_NATIVE}
        ${NEKOFS_COMMON}
        ${NEKOFS_LAYER}
        ${NEKOFS_NEKODATA}
        ${NEKOFS_LZ4}
        ${NEKOFS}
    )
else ()
    add_library(${PROJECT_NAME} SHARED
        ${NEKOFS_NATIVE}
        ${NEKOFS_COMMON}
        ${NEKOFS_LAYER}
        ${NEKOFS_NEKODATA}
        ${NEKOFS_LZ4}
        ${NEKOFS}
    )
endif ()

add_definitions("-DRAPIDJSON_NAMESPACE=nekofs::rapidjson" "-DRAPIDJSON_NOMEMBERITERATORCLASS=1" "-DRAPIDJSON_HAS_STDSTRING=1")
target_include_directories(${PROJECT_NAME} PRIVATE ../thirdparty/rapidjson/include)
target_include_directories(${PROJECT_NAME} PUBLIC include)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)
