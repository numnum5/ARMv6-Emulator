cmake_minimum_required(VERSION 3.15)

project(emulator)

set(CMAKE_CXX_STANDARD 20)

find_package(Catch2 REQUIRED)

# ----------------------------
# Shared emulator sources
# ----------------------------
set(EMULATOR_SOURCES
    src/cpu.cpp
    src/emulator.cpp
    src/compiler.cpp
    src/registers.cpp
    src/system.cpp
    src/organised.cpp
)

# ----------------------------
# Main emulator executable
# ----------------------------
add_executable(emulator
    src/main.cpp
    ${EMULATOR_SOURCES}
)

target_include_directories(emulator
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

set_target_properties(emulator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# ----------------------------
# Tests executable
# ----------------------------
add_executable(tests
    test/str.cpp
    test/ldr.cpp
    ${EMULATOR_SOURCES}
)

target_include_directories(tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(tests
    PRIVATE
        Catch2::Catch2WithMain
)