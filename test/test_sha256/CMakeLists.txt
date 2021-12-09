cmake_minimum_required (VERSION 3.8)

project(test_sha256)

set(CMAKE_CXX_STANDARD 17)

if (WIN32)
    MESSAGE(STATUS "Now is windows")
    add_definitions("-D_UNICODE" "-DUNICODE")
    remove_definitions("-D_MBCS")
endif ()


add_executable(${PROJECT_NAME}
    main.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE nekofs)
target_include_directories(${PROJECT_NAME} PRIVATE ../../nekofs)
