# Hướng Dẫn Nhanh - Chạy GUI Client

## Cách Đơn Giản Nhất - Dùng Code::Blocks (KHUYẾN NGHỊ)

1. **Tải Code::Blocks** (miễn phí): http://www.codeblocks.org/
2. **Tạo project mới:**
   - File → New → Project → Console Application
   - Chọn C++
   - Đặt tên project: `MillionaireClient`
3. **Add files vào project:**
   - Right-click project → Add files
   - Chọn tất cả file: `main.cpp`, `socket_client.h/cpp`, `protocol_handler.h/cpp`, `gui_app.h/cpp`
4. **Cấu hình compiler:**
   - Project → Build options → Compiler settings → Other options
   - Thêm: `-I../imgui -I../imgui/backends -I../glfw/include -D__MINGW32__ -DNOMINMAX`
   - Linker settings → Link libraries: Thêm `glfw3`, `opengl32`, `gdi32`, `ws2_32`
   - Search directories → Compiler: Thêm `../imgui`, `../imgui/backends`, `../glfw/include`
   - Search directories → Linker: Thêm `../glfw/lib`
5. **Add ImGUI files:**
   - Add `../imgui/imgui.cpp`, `imgui_demo.cpp`, `imgui_draw.cpp`, `imgui_tables.cpp`, `imgui_widgets.cpp`
   - Add `../imgui/backends/imgui_impl_glfw.cpp`, `imgui_impl_opengl3.cpp`
6. **Build và Run:**
   - F9 để build, F10 để run

## Hoặc Dùng Visual Studio Code với CMake

1. Cài extension: C/C++, CMake Tools
2. Tạo file `CMakeLists.txt` (tôi có thể tạo cho bạn)
3. Mở folder project trong VS Code
4. CMake sẽ tự động configure
5. Build bằng Ctrl+Shift+P → CMake: Build

## Hoặc Dùng Visual Studio

1. Tạo Win32 Console Application
2. Add tất cả source files
3. Cấu hình include paths và libraries tương tự

Bạn muốn tôi tạo file CMakeLists.txt để dùng với VS Code không? Đó sẽ là cách đơn giản nhất!

