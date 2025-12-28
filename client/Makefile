CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2
INCLUDES = -I. -I../imgui -I../imgui/backends -I../glfw/include
LIBS = -lglfw -lGL -lX11 -lpthread -ldl

# Windows specific
ifeq ($(OS),Windows_NT)
    CXX = g++
    INCLUDES = -I. -I../imgui -I../imgui/backends -I../glfw/include -I/usr/include
    LIBS = -L../glfw/lib -lglfw3 -lopengl32 -lgdi32 -lws2_32 -lwsock32
    TARGET = client.exe
else
    # Linux
    TARGET = client
    LIBS += -lrt
endif

# ImGUI sources
IMGUI_DIR = ../imgui
IMGUI_SOURCES = $(IMGUI_DIR)/imgui.cpp \
                $(IMGUI_DIR)/imgui_demo.cpp \
                $(IMGUI_DIR)/imgui_draw.cpp \
                $(IMGUI_DIR)/imgui_tables.cpp \
                $(IMGUI_DIR)/imgui_widgets.cpp \
                $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp \
                $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp

IMGUI_OBJECTS = $(IMGUI_SOURCES:.cpp=.o)

# Client sources
CLIENT_SOURCES = main.cpp \
                 socket_client.cpp \
                 protocol_handler.cpp \
                 gui_app.cpp

CLIENT_OBJECTS = $(CLIENT_SOURCES:.cpp=.o)

ALL_OBJECTS = $(CLIENT_OBJECTS) $(IMGUI_OBJECTS)

.PHONY: all clean install-deps

all: $(TARGET)

$(TARGET): $(ALL_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(ALL_OBJECTS) $(TARGET) *.exe

install-deps:
	@echo "Installing dependencies..."
	@echo "Note: You need to install:"
	@echo "  1. ImGUI - Download from https://github.com/ocornut/imgui"
	@echo "  2. GLFW - Install via package manager or download from https://www.glfw.org/"
	@echo ""
	@echo "On Ubuntu/Debian:"
	@echo "  sudo apt-get install libglfw3-dev libgl1-mesa-dev"
	@echo ""
	@echo "On Windows (MinGW):"
	@echo "  Download GLFW and extract to ../glfw/"
	@echo "  Download ImGUI and extract to ../imgui/"

