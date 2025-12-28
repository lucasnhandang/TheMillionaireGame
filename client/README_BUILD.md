# Client GUI Build Instructions

## Prerequisites

### Required Libraries

1. **ImGUI** (Dear ImGui)
   - Download from: https://github.com/ocornut/imgui
   - Extract to `../imgui/` directory (sibling to client folder)
   - Required files:
     - `imgui.h`, `imgui.cpp`
     - `imgui_demo.cpp`, `imgui_draw.cpp`, `imgui_tables.cpp`, `imgui_widgets.cpp`
     - `backends/imgui_impl_glfw.cpp`, `backends/imgui_impl_opengl3.cpp`
     - `backends/imgui_impl_glfw.h`, `backends/imgui_impl_opengl3.h`

2. **GLFW** (OpenGL Framework)
   - Download from: https://www.glfw.org/
   - Or install via package manager

3. **OpenGL**
   - Usually comes with graphics drivers

### Platform-Specific Setup

#### Linux (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install build-essential libglfw3-dev libgl1-mesa-dev libglu1-mesa-dev
```

#### Linux (Arch)
```bash
sudo pacman -S base-devel glfw glew
```

#### macOS
```bash
brew install glfw
```

#### Windows (MinGW/MSYS2)
```bash
pacman -S mingw-w64-x86_64-glfw mingw-w64-x86_64-glbinding
```

Or download GLFW precompiled binaries and extract to `../glfw/`

## Building

### 1. Clone/Download ImGUI
```bash
cd ..
git clone https://github.com/ocornut/imgui.git
cd client
```

### 2. Build the Client
```bash
make
```

### 3. Run the Client
```bash
./client
```

Or on Windows:
```bash
./client.exe
```

## Directory Structure

```
TheMillionaireGame/
├── client/
│   ├── main.cpp
│   ├── socket_client.h/cpp
│   ├── protocol_handler.h/cpp
│   ├── gui_app.h/cpp
│   ├── Makefile
│   └── README_BUILD.md
├── imgui/          (Download here)
│   ├── imgui.h
│   ├── imgui.cpp
│   ├── backends/
│   └── ...
└── glfw/           (Optional, if not using system package)
    ├── include/
    └── lib/
```

## Configuration

The client connects to `localhost:8080` by default. To change this, modify the `server_host_` and `server_port_` variables in `gui_app.cpp` constructor, or add command-line arguments.

## Troubleshooting

### "Cannot find imgui.h"
- Make sure ImGUI is extracted to `../imgui/` directory
- Check that `imgui.h` exists at `../imgui/imgui.h`

### "Cannot find GLFW"
- On Linux: Install `libglfw3-dev` package
- On Windows: Make sure GLFW is in `../glfw/` or in system PATH
- On macOS: Install via `brew install glfw`

### "Undefined reference to OpenGL functions"
- Install OpenGL development libraries
- Linux: `libgl1-mesa-dev` or `libglu1-mesa-dev`
- Windows: Usually comes with MinGW or Visual Studio

### Connection errors
- Make sure the server is running
- Check server host and port in `gui_app.cpp`
- Verify firewall settings

## Features

- **Login/Register**: Authenticate with server
- **Main Menu**: Navigate to different features
- **Game Screen**: Play the quiz game with:
  - Question display
  - Answer buttons
  - Timer
  - Lifelines (50/50, Phone a Friend, Ask the Audience)
  - Prize display
- **Leaderboard**: View global and friend rankings
- **Friends**: Manage friend list and requests
- **Profile**: View user statistics and game history
- **Admin Panel**: (If logged in as admin) Manage questions and users

## Notes

- The client uses traditional TCP sockets (not WebSocket)
- Messages are JSON-formatted with newline delimiter
- The GUI is built with ImGUI for a lightweight, fast interface
- OpenGL 3.3+ is required for rendering

