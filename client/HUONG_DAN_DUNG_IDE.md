# Hướng Dẫn Dùng IDE (KHÔNG CẦN .BAT)

## Cách 1: Dùng Code::Blocks (Đơn giản nhất - KHUYẾN NGHỊ)

### Bước 1: Tải và cài Code::Blocks
- Tải từ: http://www.codeblocks.org/
- Chọn bản có MinGW đi kèm (mingw-setup)

### Bước 2: Mở project
1. Mở Code::Blocks
2. File → Open → Chọn file `client/main.cpp`
3. Khi hỏi "Create new project?", chọn **No**
4. Code::Blocks sẽ tự động detect các file liên quan

### Bước 3: Add files vào project
1. Right-click project name trong Projects panel
2. Add files → Chọn tất cả:
   - `socket_client.cpp`
   - `protocol_handler.cpp`  
   - `gui_app.cpp`
   - `../imgui/imgui.cpp`
   - `../imgui/imgui_demo.cpp`
   - `../imgui/imgui_draw.cpp`
   - `../imgui/imgui_tables.cpp`
   - `../imgui/imgui_widgets.cpp`
   - `../imgui/backends/imgui_impl_glfw.cpp`
   - `../imgui/backends/imgui_impl_opengl3.cpp`

### Bước 4: Cấu hình Build Settings
1. Project → Build options
2. **Compiler settings** tab:
   - Other options: Thêm `-std=c++14 -D__MINGW32__ -DNOMINMAX`
   - Search directories → Compiler: Add
     - `../imgui`
     - `../imgui/backends`
     - `../glfw/include`
3. **Linker settings** tab:
   - Link libraries: Add
     - `glfw3`
     - `opengl32`
     - `gdi32`
     - `ws2_32`
   - Search directories → Linker: Add `../glfw/lib`

### Bước 5: Build và Run
- Nhấn **F9** để build
- Nhấn **F10** để run

---

## Cách 2: Dùng Visual Studio Code

### Bước 1: Cài extensions
1. Mở VS Code
2. Extensions (Ctrl+Shift+X)
3. Cài:
   - **C/C++** (Microsoft)
   - **CMake Tools** (Microsoft)

### Bước 2: Mở project
1. File → Open Folder → Chọn thư mục `client`
2. VS Code sẽ tự động detect `CMakeLists.txt`

### Bước 3: Configure CMake
1. Nhấn F1 → Gõ "CMake: Configure"
2. Chọn compiler: MinGW g++
3. VS Code sẽ tự build cấu hình

### Bước 4: Build
1. Nhấn F7 hoặc
2. Bottom bar → [Build] button
3. Run: F5 hoặc [Run] button

---

## Cách 3: Dùng Visual Studio (nếu có)

1. File → New → Project → Visual C++ → Empty Project
2. Add Existing Items → Chọn tất cả .cpp và .h files
3. Project Properties:
   - C/C++ → General → Additional Include Directories:
     - `../imgui`
     - `../imgui/backends`
     - `../glfw/include`
   - Linker → Input → Additional Dependencies:
     - `glfw3.lib;opengl32.lib;gdi32.lib;ws2_32.lib`
   - Linker → General → Additional Library Directories:
     - `../glfw/lib`
4. Build → Build Solution (F7)

---

## Cách 4: Dùng CLion (nếu có)

1. File → Open → Chọn thư mục `client`
2. CLion sẽ tự detect `CMakeLists.txt`
3. Nhấn Run (Shift+F10)

---

## Lưu ý

- **Code::Blocks** là cách đơn giản nhất nếu bạn chưa quen IDE
- **VS Code + CMake** là cách hiện đại nhất, tự động hóa nhiều
- Tất cả đều KHÔNG CẦN file .bat

Bạn muốn tôi hướng dẫn chi tiết cách nào?

