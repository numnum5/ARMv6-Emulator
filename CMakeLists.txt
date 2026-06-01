cmake_minimum_required(VERSION 3.15)

project(emulator)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Catch2 REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

# ----------------------------
# Glaze
# ----------------------------
include(FetchContent)

FetchContent_Declare(
    glaze
    GIT_REPOSITORY https://github.com/stephenberry/glaze.git
    GIT_TAG main
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(glaze)

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
# API sources
# ----------------------------
set(API_SOURCES
    api/server.cpp
)

# ----------------------------
# Emulator executable
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
# WebSocket server executable
# ----------------------------
add_executable(server
    ${API_SOURCES}
    ${EMULATOR_SOURCES}
)

target_include_directories(server
    PRIVATE
        ${CMAKE_SOURCE_DIR}/api
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(server
    PRIVATE
        Boost::system
        glaze::glaze
)

set_target_properties(server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# ----------------------------
# Tests executable
# ----------------------------
add_executable(tests
    test/str.cpp
    test/ldr.cpp
    test/misc.cpp
    test/alu.cpp
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