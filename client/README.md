# Who Wants to be a Millionaire - Client (C++)

Client application for the Who Wants to be a Millionaire game, written in C++ with ImGui GUI.

## Requirements

- C++11 compatible compiler (GCC or Clang)
- OpenGL libraries
- X11 development libraries
- CMake (for building GLFW)
- Linux/Ubuntu environment

## Dependencies

The client uses:
- **ImGui**: Immediate mode GUI library (included in project)
- **GLFW**: Window and input handling (included in project)
- **OpenGL**: Graphics rendering

## Installation

### 1. Install System Dependencies

```bash
sudo apt-get update
sudo apt-get install -y build-essential g++ make cmake
sudo apt-get install -y libgl1-mesa-dev libglu1-mesa-dev
sudo apt-get install -y libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
```

### 2. Build GLFW (if not already built)

```bash
cd glfw
cmake .
make
cd ..
```

### 3. Build Client

```bash
cd client
make clean
make
```

The executable will be in `bin/client`.

## Running the Client

### Basic Usage

```bash
cd client
./bin/client
```

### With Custom Server Address

```bash
cd client
./bin/client 192.168.1.100 8080
```

### Using Make

```bash
cd client
make run
```

## Features

### Authentication
- User registration with password validation
- User login
- Session management with authentication tokens

### Game Features
- Start new game
- Resume saved game
- Answer questions with timer (30 seconds per question)
- Three lifelines:
  - 50/50: Remove two wrong answers
  - Phone a Friend: Get a suggestion
  - Ask the Audience: See audience poll results
- Give up option (take current prize)
- Leave game (auto-save)

### UI Features
- Prize ladder display
- Real-time timer countdown
- Question and answer display
- Score tracking
- Lifeline buttons
- Game state management

## Project Structure

```
client/
├── main.cpp                 # Main entry point with ImGui GUI
├── socket_client.h/cpp      # TCP socket communication
├── protocol_handler.h/cpp   # Protocol request/response handling
├── json_utils.h/cpp         # JSON parsing utilities
├── Makefile                 # Build configuration
└── README.md                # This file
```

## Protocol Communication

The client communicates with the server using:
- **Protocol**: TCP sockets
- **Format**: JSON messages
- **Delimiter**: Newline character (`\n`)

All requests follow the format:
```json
{
  "requestType": "REQUEST_TYPE",
  "data": { ... }
}
```

All responses follow the format:
```json
{
  "responseCode": 200,
  "data": { ... }
}
```

See `../PROTOCOL.md` for detailed protocol specification.

## Building

### Manual Build

```bash
cd client
g++ -std=c++11 -Wall -Wextra -g \
    -I. -I../imgui -I../imgui/backends -I../glfw/include \
    main.cpp socket_client.cpp protocol_handler.cpp json_utils.cpp \
    ../imgui/imgui.cpp ../imgui/imgui_draw.cpp ../imgui/imgui_tables.cpp \
    ../imgui/imgui_widgets.cpp \
    ../imgui/backends/imgui_impl_glfw.cpp ../imgui/backends/imgui_impl_opengl3.cpp \
    -L../glfw/lib -lglfw3 \
    -pthread -lGL -lGLU -lX11 -lXrandr -lXinerama -lXcursor -ldl -lrt \
    -o bin/client
```

### Using Makefile

```bash
cd client
make          # Build
make clean   # Clean build artifacts
make run     # Build and run
```

## Troubleshooting

### Build Errors

**"GL/gl.h: No such file or directory"**
```bash
sudo apt-get install -y libgl1-mesa-dev libglu1-mesa-dev
```

**"X11/Xlib.h: No such file or directory"**
```bash
sudo apt-get install -y libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
```

**"GLFW library not found"**
```bash
cd glfw
cmake .
make
cd ../client
make
```

### Runtime Errors

**"Failed to connect to server"**
- Make sure server is running
- Check server address and port
- Check firewall settings

**"Segmentation fault"**
- Make sure all dependencies are installed
- Check if GLFW was built correctly
- Run with gdb for debugging: `gdb ./bin/client`

## Development

### Code Structure

- **socket_client.h/cpp**: Low-level socket communication
- **protocol_handler.h/cpp**: High-level protocol API
- **json_utils.h/cpp**: Simple JSON parsing utilities
- **main.cpp**: ImGui GUI and application logic

### Adding Features

To add new features:

1. Add protocol methods in `protocol_handler.h/cpp`
2. Add UI components in `main.cpp` using ImGui
3. Wire up events in the main loop

## License

Part of the Who Wants to be a Millionaire project for Network Programming course.
