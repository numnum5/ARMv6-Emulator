cmake_minimum_required(VERSION 3.15)

project(emulator)

add_executable(emulator
    src/main.cpp
    src/cpu.cpp
    src/emulator.cpp
    src/compiler.cpp
)

target_include_directories(emulator
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)