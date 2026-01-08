cmake_minimum_required(VERSION 3.16)
project(MillionaireGameClient VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt5 - try multiple methods for Linux compatibility
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} $ENV{QT_DIR} /usr/lib/x86_64-linux-gnu/cmake/Qt5 /usr/lib/qt5)

# Try to find Qt5 using different methods
find_package(Qt5 QUIET COMPONENTS Core Widgets Gui)

if(NOT Qt5_FOUND)
    # If Qt5 not found, try alternative paths
    message(STATUS "Qt5 not found in standard paths, trying alternatives...")
    
    # Check if qmake is available and get Qt path from it
    find_program(QMAKE_EXECUTABLE NAMES qmake qmake5 PATHS
        /usr/bin
        /usr/lib/qt5/bin
        $ENV{QT_DIR}/bin
        /opt/qt5/bin
    )
    
    if(QMAKE_EXECUTABLE)
        execute_process(
            COMMAND ${QMAKE_EXECUTABLE} -query QT_INSTALL_PREFIX
            OUTPUT_VARIABLE QT_INSTALL_PREFIX
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${QT_INSTALL_PREFIX})
        message(STATUS "Found Qt at: ${QT_INSTALL_PREFIX}")
    endif()
    
    find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui)
endif()

if(NOT Qt5_FOUND)
    message(FATAL_ERROR "Qt5 not found! Please install Qt5 development packages:\n"
        "  sudo apt-get install qtbase5-dev qt5-qmake\n"
        "  sudo apt-get install libqt5widgets5 libqt5core5a libqt5gui5")
endif()

message(STATUS "Found Qt5 version: ${Qt5_VERSION}")
message(STATUS "Qt5 Core: ${Qt5Core_DIR}")
message(STATUS "Qt5 Widgets: ${Qt5Widgets_DIR}")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Source files
set(CLIENT_SRC
    main.cpp
    socket_client.cpp
    protocol_handler.cpp
    json_utils.cpp
    qt_texture_loader.cpp
    gamestate.cpp
)

set(CLIENT_HEADERS
    socket_client.h
    protocol_handler.h
    json_utils.h
    game_event.h
    qt_texture_loader.h
    gamestate.h
)

set(QT_WIDGETS_SRC
    mainwindow.cpp
    loginscreen.cpp
    homescreen.cpp
    gamescreen.cpp
    resultscreen.cpp
)

set(QT_WIDGETS_HEADERS
    mainwindow.h
    loginscreen.h
    homescreen.h
    gamescreen.h
    resultscreen.h
)

# Executable
add_executable(${PROJECT_NAME}
    ${CLIENT_SRC}
    ${CLIENT_HEADERS}
    ${QT_WIDGETS_SRC}
    ${QT_WIDGETS_HEADERS}
)

target_link_libraries(${PROJECT_NAME}
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
    Qt5::Network
)

# Linux-specific libraries for sockets
if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} pthread)
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Install
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
