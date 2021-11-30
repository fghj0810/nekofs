cmake_minimum_required (VERSION 3.8)

project(test)

set(CMAKE_CXX_STANDARD 17)

if (ANDROID_ABI AND ("${ANDROID_ABI}" STREQUAL "x86" OR "${ANDROID_ABI}" STREQUAL "armeabi-v7a"))
    add_definitions("-D_FILE_OFFSET_BITS=64")
endif ()

if (WIN32)
    MESSAGE(STATUS "Now is windows")
    add_definitions("-D_UNICODE" "-DUNICODE")
    remove_definitions("-D_MBCS")
endif ()


add_executable(${PROJECT_NAME}
    main.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE nekofs)
target_include_directories(${PROJECT_NAME} PRIVATE ../nekofs)
add_dependencies(${PROJECT_NAME} nekofs)
