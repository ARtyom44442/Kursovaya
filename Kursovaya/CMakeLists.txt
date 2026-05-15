
cmake_minimum_required(VERSION 3.10)

project(SmartLogistics)

include_directories(include)

file(GLOB SOURCES "src/*.cpp")

add_executable(my_app ${SOURCES})

if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
endif()