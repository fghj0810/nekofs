cmake_minimum_required (VERSION 3.8)

project(test_write)

set(CMAKE_CXX_STANDARD 17)

if (WIN32)
    add_definitions("-D_UNICODE" "-DUNICODE")
    remove_definitions("-D_MBCS")
    add_definitions("-DNOMINMAX")
endif ()


add_executable(${PROJECT_NAME}
    main.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE nekofs)
