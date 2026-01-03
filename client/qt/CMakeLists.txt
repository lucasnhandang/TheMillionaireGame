cmake_minimum_required(VERSION 3.16)
project(MillionaireGameQtClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 (or Qt5 if Qt6 not available)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Network)

# Enable automatic MOC, UIC, RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Include directories
include_directories(
    ../src
    ../include
)

# Source files from existing client
set(CLIENT_SOURCES
    ../src/client_core.cpp
    ../src/protocol_handler.cpp
    ../src/utils/json_parser.cpp
)

# Qt GUI source files
set(QT_SOURCES
    main_qt.cpp
    login_register_page.cpp
    landing_page.cpp
    ingame_page.cpp
    result_page.cpp
    instruction_dialog.cpp
    network_thread.cpp
    walkaway_dialog.cpp
)

# Qt header files
set(QT_HEADERS
    login_register_page.h
    landing_page.h
    ingame_page.h
    result_page.h
    instruction_dialog.h
    network_thread.h
    walkaway_dialog.h
)

# UI files (if using Qt Designer)
set(QT_UI_FILES
    # login_register_page.ui
    # landing_page.ui
    # ingame_page.ui
)

# Resources (images, icons, etc.)
set(QT_RESOURCES
    resources.qrc
)

# Create executable
add_executable(client_qt
    ${CLIENT_SOURCES}
    ${QT_SOURCES}
    ${QT_HEADERS}
    ${QT_UI_FILES}
    ${QT_RESOURCES}
)

# Link Qt libraries
target_link_libraries(client_qt
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    pthread
)

# Set output directory
set_target_properties(client_qt PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

