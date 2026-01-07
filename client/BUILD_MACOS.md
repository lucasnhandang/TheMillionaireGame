# Building Client on macOS

## Prerequisites

### 1. Install Homebrew (if not already installed)

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

### 2. Install Dependencies

```bash
# Install build tools
brew install cmake

# Xcode Command Line Tools (usually already installed, but verify)
xcode-select --install
```

**Note:** On macOS, you don't need to install OpenGL or X11 libraries separately - they come with macOS. The Makefile will automatically use Cocoa and OpenGL frameworks.

## Building

### Step 1: Build GLFW (if not already built)

The Makefile will automatically build GLFW for macOS if needed, but you can also do it manually:

```bash
cd glfw
mkdir -p build
cd build
cmake .. -DGLFW_BUILD_COCOA=ON -DGLFW_BUILD_X11=OFF
make
cd ../..
```

Or use the Makefile helper:
```bash
cd client
make rebuild-glfw
```

### Step 2: Build Client

```bash
cd client
make clean    # Optional: clean old build
make          # Build client
```

The executable will be created at: `client/bin/client`

### Step 3: Run Client

**Important:** The client requires the server to be running.

**Terminal 1 - Start Server:**
```bash
cd server
make
./bin/server
```

**Terminal 2 - Start Client:**
```bash
cd client
./bin/client
```

Or use:
```bash
cd client
make run
```

## Differences from Linux

- **No sudo needed** for installing dependencies (Homebrew installs to user directory)
- **Uses Cocoa** instead of X11 for windowing
- **Uses OpenGL framework** instead of libGL/libGLU
- **Uses clang++** instead of g++ (automatically detected)
- **No X11 libraries** needed

## Troubleshooting

### "cmake: command not found"
```bash
brew install cmake
```

### "GLFW library not found"
```bash
cd client
make rebuild-glfw
```

### "Failed to connect to server"
- Make sure server is running: `cd server && ./bin/server`
- Check if server is listening: `lsof -i :8080`

### Build errors with GLFW
Try rebuilding GLFW specifically for macOS:
```bash
cd glfw
rm -rf build
mkdir -p build
cd build
cmake .. -DGLFW_BUILD_COCOA=ON -DGLFW_BUILD_X11=OFF -DBUILD_SHARED_LIBS=OFF
make
cd ../../client
make clean
make
```

### "NSGL: The targeted version of macOS does not support OpenGL 3.0 or 3.1"
**Error:** `GLFW Error 65543: NSGL: The targeted version of macOS does not support OpenGL 3.0 or 3.1 but may support 3.2 and above`

**Solution:** This has been fixed in the code. The client now automatically uses OpenGL 3.3 on macOS. If you still see this error:
1. Make sure you have the latest version of `main.cpp`
2. Rebuild the client:
   ```bash
   cd client
   make clean
   make
   ```

The fix sets:
- OpenGL 3.3 context (macOS compatible)
- Core profile with forward compatibility (required on macOS)
- GLSL version 330

## Verify Build

```bash
# Check if executable exists
ls -lh client/bin/client

# Check file type (should show Mach-O)
file client/bin/client
```

## Tips

1. **First time setup:** Make sure Xcode Command Line Tools are installed:
   ```bash
   xcode-select --install
   ```

2. **Using Homebrew GLFW (optional):**
   ```bash
   brew install glfw
   ```
   The Makefile will automatically detect and use it via pkg-config.

3. **Clean rebuild:**
   ```bash
   cd client
   make clean
   make
   ```

