cmake_minimum_required(VERSION 3.10)
project(SmartLogistics)


include_directories(
    include
    datareader
    main
    Transport
    logs
    "types of transport"
)

file(GLOB SOURCES 
    "datareader/*.cpp"
    "logs/*.cpp"
    "main/*.cpp"
    "Transport/*.cpp"
    "types of transport/*.cpp"
)

add_executable(my_app ${SOURCES})

if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
endif()
