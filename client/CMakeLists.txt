cmake_minimum_required(VERSION 3.10)
project(MillionaireClient)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ImGUI sources
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/../imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# Client sources
set(CLIENT_SOURCES
    main.cpp
    socket_client.cpp
    protocol_handler.cpp
    gui_app.cpp
)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${CMAKE_SOURCE_DIR}/../glfw/include
)

# GLFW library path
if(WIN32)
    set(GLFW_LIB_DIR ${CMAKE_SOURCE_DIR}/../glfw/lib)
    link_directories(${GLFW_LIB_DIR})
    set(GLFW_LIBS glfw3 opengl32 gdi32 ws2_32)
else()
    find_package(glfw3 REQUIRED)
    set(GLFW_LIBS glfw)
endif()

# Add executable
add_executable(${PROJECT_NAME} ${CLIENT_SOURCES} ${IMGUI_SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} ${GLFW_LIBS})

# Compiler definitions for Windows
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE 
        __MINGW32__
        NOMINMAX
        _WIN32_WINNT=0x0601
    )
endif()

