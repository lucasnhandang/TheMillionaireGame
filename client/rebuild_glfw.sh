#!/bin/bash
# Script to rebuild GLFW for Linux

echo "=== Rebuilding GLFW for Linux ==="

cd ../glfw

# Clean old build
echo "Cleaning old build..."
rm -rf build lib/libglfw3.a

# Create build directory
mkdir -p build
cd build

# Configure for Linux/X11
echo "Configuring GLFW for Linux..."
cmake .. \
    -DGLFW_BUILD_X11=ON \
    -DGLFW_BUILD_WAYLAND=OFF \
    -DGLFW_BUILD_WIN32=OFF \
    -DGLFW_BUILD_COCOA=OFF \
    -DBUILD_SHARED_LIBS=OFF \
    -DGLFW_BUILD_EXAMPLES=OFF \
    -DGLFW_BUILD_TESTS=OFF \
    -DGLFW_BUILD_DOCS=OFF

# Build
echo "Building GLFW..."
make -j$(nproc)

# Copy library to lib directory
if [ -f src/libglfw3.a ]; then
    echo "Copying library..."
    mkdir -p ../lib
    cp src/libglfw3.a ../lib/
    echo "GLFW rebuild complete! Library: ../lib/libglfw3.a"
else
    echo "Error: GLFW library not found after build!"
    exit 1
fi

cd ../..

