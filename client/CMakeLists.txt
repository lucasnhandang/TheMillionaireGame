cmake_minimum_required(VERSION 3.16)
project(MillionaireGameClient VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt5
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui Network)

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
