cmake_minimum_required (VERSION 3.8)

project(nekofs_cli)

set(CMAKE_CXX_STANDARD 17)

if (WIN32)
    MESSAGE(STATUS "Now is windows")
    add_definitions("-D_UNICODE" "-DUNICODE")
    remove_definitions("-D_MBCS")
endif ()
ADD_DEFINITIONS("-DNEKOFS_TOOLS")

set (NEKOFS_CLI_SRC
    common.h
    main.cpp
    prepare.h
    prepare.cpp
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${NEKOFS})

add_executable(${PROJECT_NAME} ${NEKOFS_CLI_SRC})

target_link_libraries(${PROJECT_NAME} PRIVATE nekofs)
target_link_libraries(${PROJECT_NAME} PRIVATE gflags::gflags)
