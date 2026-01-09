cmake_minimum_required(VERSION 3.16)
project(MillionaireGameClient VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt5 - try multiple methods for Linux compatibility
# First, try to get Qt path from qmake
find_program(QMAKE_EXECUTABLE NAMES qmake qmake5 PATHS
    /usr/bin
    /usr/lib/x86_64-linux-gnu/qt5/bin
    /usr/lib/qt5/bin
    $ENV{QT_DIR}/bin
    /opt/qt5/bin
)

if(QMAKE_EXECUTABLE)
    execute_process(
        COMMAND ${QMAKE_EXECUTABLE} -query QT_INSTALL_PREFIX
        OUTPUT_VARIABLE QT_INSTALL_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    
    execute_process(
        COMMAND ${QMAKE_EXECUTABLE} -query QT_INSTALL_LIBS
        OUTPUT_VARIABLE QT_INSTALL_LIBS
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    
    if(QT_INSTALL_PREFIX)
        # Common locations for Qt5Config.cmake
        set(CMAKE_PREFIX_PATH 
            ${CMAKE_PREFIX_PATH}
            ${QT_INSTALL_PREFIX}
            ${QT_INSTALL_PREFIX}/lib/cmake
            ${QT_INSTALL_PREFIX}/lib/x86_64-linux-gnu/cmake/Qt5
            ${QT_INSTALL_PREFIX}/lib64/cmake
        )
        message(STATUS "Found Qt installation at: ${QT_INSTALL_PREFIX}")
    endif()
    
    if(QT_INSTALL_LIBS)
        set(CMAKE_PREFIX_PATH 
            ${CMAKE_PREFIX_PATH}
            ${QT_INSTALL_LIBS}/cmake
            ${QT_INSTALL_LIBS}/cmake/Qt5
        )
    endif()
endif()

# Add common Linux Qt5 paths
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH}
    /usr/lib/x86_64-linux-gnu
    /usr/lib/x86_64-linux-gnu/cmake/Qt5
    /usr/lib/x86_64-linux-gnu/qt5
    /usr/lib/x86_64-linux-gnu/qt5/lib/cmake/Qt5
    /usr/lib/qt5
    /usr/lib/qt5/lib/cmake/Qt5
    $ENV{QT_DIR}
    $ENV{CMAKE_PREFIX_PATH}
)

# Try to find Qt5
find_package(Qt5 QUIET COMPONENTS Core Widgets Gui)

if(NOT Qt5_FOUND)
    # Last resort: try to find Qt5Config.cmake directly
    message(STATUS "Qt5 not found in standard paths, searching for Qt5Config.cmake...")
    
    file(GLOB_RECURSE QT5_CONFIG_FILE 
        "/usr/lib/*/cmake/Qt5/Qt5Config.cmake"
        "/usr/lib/*/Qt5Config.cmake"
    )
    
    if(QT5_CONFIG_FILE)
        get_filename_component(QT5_CONFIG_DIR ${QT5_CONFIG_FILE} DIRECTORY)
        set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${QT5_CONFIG_DIR}/..)
        message(STATUS "Found Qt5Config.cmake at: ${QT5_CONFIG_FILE}")
        message(STATUS "Adding to CMAKE_PREFIX_PATH: ${QT5_CONFIG_DIR}/..")
    endif()
    
    find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui)
endif()

if(NOT Qt5_FOUND)
    message(FATAL_ERROR "\n"
        "========================================\n"
        "Qt5 not found!\n"
        "========================================\n\n"
        "Please run these commands to find Qt5Config.cmake:\n"
        "  find /usr -name 'Qt5Config.cmake' 2>/dev/null\n\n"
        "Then set CMAKE_PREFIX_PATH:\n"
        "  cmake -DCMAKE_PREFIX_PATH=<path_to_Qt5Config.cmake's_directory>/.. ..\n\n"
        "Or install Qt5 development packages:\n"
        "  sudo apt-get install qtbase5-dev\n")
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
    adminpanelscreen.cpp
    addquestiondialog.cpp
    editquestiondialog.cpp
    friendsscreen.cpp
    leaderboardscreen.cpp
)

set(QT_WIDGETS_HEADERS
    mainwindow.h
    loginscreen.h
    homescreen.h
    gamescreen.h
    resultscreen.h
    adminpanelscreen.h
    addquestiondialog.h
    editquestiondialog.h
    friendsscreen.h
    leaderboardscreen.h
)

# UI files
set(QT_UI_FILES
    loginscreen.ui
    gamescreen.ui
)

# Executable
add_executable(${PROJECT_NAME}
    ${CLIENT_SRC}
    ${CLIENT_HEADERS}
    ${QT_WIDGETS_SRC}
    ${QT_WIDGETS_HEADERS}
    ${QT_UI_FILES}
)

target_link_libraries(${PROJECT_NAME}
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
)

# Linux-specific libraries for sockets
if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} pthread)
endif()

# CRITICAL: Fix Qt5/Qt6 header conflict on macOS
# Qt6 is installed in /usr/local/include via Homebrew, which causes conflicts with Qt5
# We need to ensure Qt5 headers are found BEFORE /usr/local/include
if(APPLE)
    # Get the actual Qt5 include paths from the found Qt5 installation
    get_target_property(QT5_CORE_INCLUDE_DIRS Qt5::Core INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(QT5_WIDGETS_INCLUDE_DIRS Qt5::Widgets INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(QT5_GUI_INCLUDE_DIRS Qt5::Gui INTERFACE_INCLUDE_DIRECTORIES)
    
    # Add Qt5 include paths as SYSTEM includes with higher priority
    # Use -isystem to override default include search order
    if(QT5_CORE_INCLUDE_DIRS)
        target_compile_options(${PROJECT_NAME} PRIVATE
            -isystem ${QT5_CORE_INCLUDE_DIRS}
        )
    endif()
    if(QT5_WIDGETS_INCLUDE_DIRS)
        target_compile_options(${PROJECT_NAME} PRIVATE
            -isystem ${QT5_WIDGETS_INCLUDE_DIRS}
        )
    endif()
    if(QT5_GUI_INCLUDE_DIRS)
        target_compile_options(${PROJECT_NAME} PRIVATE
            -isystem ${QT5_GUI_INCLUDE_DIRS}
        )
    endif()
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Install
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
