# Hướng Dẫn Cài Đặt ImGUI

## Bước 1: Giải nén ImGUI

Sau khi tải file zip ImGUI từ https://github.com/ocornut/imgui, giải nén file zip đó.

## Bước 2: Đặt ImGUI vào đúng vị trí

Bạn có 2 cách:

### Cách 1: Đổi tên thư mục (Đơn giản nhất)

1. Sau khi giải nén, bạn sẽ có một thư mục (thường tên là `imgui-master`, `imgui-1.90.0`, hoặc tương tự)
2. Đổi tên thư mục đó thành `imgui`
3. Di chuyển thư mục `imgui` vào thư mục cha của `client`

**Cấu trúc sau khi đặt đúng:**

```
TheMillionaireGame/
├── client/              (thư mục hiện tại của bạn)
│   ├── socket_client.h
│   ├── gui_app.h
│   └── ...
├── imgui/               ← ĐẶT IMGUI Ở ĐÂY
│   ├── imgui.h
│   ├── imgui.cpp
│   ├── imgui_demo.cpp
│   ├── imgui_draw.cpp
│   ├── imgui_tables.cpp
│   ├── imgui_widgets.cpp
│   ├── backends/
│   │   ├── imgui_impl_glfw.h
│   │   ├── imgui_impl_glfw.cpp
│   │   ├── imgui_impl_opengl3.h
│   │   └── imgui_impl_opengl3.cpp
│   └── ...
├── server/
└── ...
```

### Cách 2: Copy nội dung

1. Tạo thư mục `imgui` trong thư mục `TheMillionaireGame` (cùng cấp với `client`)
2. Copy tất cả các file từ thư mục ImGUI đã giải nén vào thư mục `imgui` vừa tạo

## Bước 3: Kiểm tra cấu trúc

Đảm bảo bạn có các file sau trong thư mục `imgui/`:

```
imgui/
├── imgui.h          ← BẮT BUỘC
├── imgui.cpp        ← BẮT BUỘC
├── imgui_demo.cpp
├── imgui_draw.cpp
├── imgui_tables.cpp
├── imgui_widgets.cpp
└── backends/        ← THƯ MỤC NÀY BẮT BUỘC
    ├── imgui_impl_glfw.h
    ├── imgui_impl_glfw.cpp
    ├── imgui_impl_opengl3.h
    └── imgui_impl_opengl3.cpp
```

## Ví dụ trên Windows:

Nếu bạn đang ở thư mục:
```
C:\Users\ASUS\Downloads\TheMillionaireGame\client\
```

Thì ImGUI cần đặt ở:
```
C:\Users\ASUS\Downloads\TheMillionaireGame\imgui\
```

## Kiểm tra nhanh:

Mở Command Prompt hoặc PowerShell ở thư mục `client` và chạy:
```bash
cd ..
dir imgui
```

Nếu thấy thư mục `imgui` và các file bên trong, bạn đã đặt đúng!

## Sau khi đặt xong:

1. Quay lại thư mục `client`
2. Chạy `make` để build project
3. Nếu thiếu GLFW, bạn cũng cần cài đặt GLFW (xem README_BUILD.md)

