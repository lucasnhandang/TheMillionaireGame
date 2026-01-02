cmake_minimum_required(VERSION 3.10)
project(MillionaireGameClient)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2 -pthread")

# Source files
set(SOURCES
    src/main.cpp
    src/client_core.cpp
    src/protocol_handler.cpp
    src/gui/login_window.cpp
    src/gui/main_window.cpp
    src/gui/game_window.cpp
    src/gui/leaderboard_window.cpp
    src/utils/json_parser.cpp
)

# Include directories
include_directories(
    src
    include
)

# Create executable
add_executable(client ${SOURCES})

# Link pthread for threading
target_link_libraries(client pthread)

# Set output directory
set_target_properties(client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Installation (optional)
install(TARGETS client DESTINATION bin)

