# Hướng dẫn Cài đặt Cursor trên Linux

## Cách 1: Tải từ Website (Khuyến nghị)

1. **Truy cập website:**
   - Vào https://cursor.sh/
   - Click "Download" hoặc "Get Cursor"

2. **Chọn Linux version:**
   - Chọn file `.AppImage` (universal, không cần cài đặt)
   - Hoặc `.deb` cho Ubuntu/Debian
   - Hoặc `.rpm` cho Fedora/RHEL

## Cách 2: Cài đặt từ .deb package (Ubuntu/Debian)

### Tải file .deb:

```bash
# Tải file .deb từ website
wget https://downloader.cursor.sh/linux/appImage/x64 -O cursor.deb

# Hoặc tải từ GitHub releases
# Kiểm tra link mới nhất tại: https://github.com/getcursor/cursor/releases
```

### Cài đặt:

```bash
# Cài đặt
sudo dpkg -i cursor.deb

# Nếu thiếu dependencies
sudo apt-get install -f
```

## Cách 3: Dùng AppImage (Universal)

### Tải AppImage:

```bash
# Tạo thư mục
mkdir -p ~/Applications
cd ~/Applications

# Tải AppImage (kiểm tra link mới nhất tại website)
wget https://downloader.cursor.sh/linux/appImage/x64 -O Cursor.AppImage

# Cho phép execute
chmod +x Cursor.AppImage
```

### Chạy:

```bash
# Chạy trực tiếp
./Cursor.AppImage

# Hoặc tạo desktop entry
cat > ~/.local/share/applications/cursor.desktop << EOF
[Desktop Entry]
Name=Cursor
Exec=$HOME/Applications/Cursor.AppImage
Icon=cursor
Type=Application
Categories=Development;IDE;
EOF
```

## Cách 4: Dùng Snap (nếu có)

```bash
sudo snap install cursor
```

## Cách 5: Build từ Source (Advanced)

```bash
# Clone repository
git clone https://github.com/getcursor/cursor.git
cd cursor

# Follow build instructions in README
```

## Kiểm tra sau khi cài đặt

```bash
# Kiểm tra version
cursor --version

# Hoặc chạy từ terminal
cursor
```

## Troubleshooting

### Lỗi "Permission denied" với AppImage:

```bash
chmod +x Cursor.AppImage
```

### Lỗi dependencies với .deb:

```bash
sudo apt-get update
sudo apt-get install -f
```

### Không tìm thấy cursor command:

```bash
# Thêm vào PATH (nếu dùng AppImage)
export PATH=$PATH:$HOME/Applications

# Hoặc tạo symlink
sudo ln -s $HOME/Applications/Cursor.AppImage /usr/local/bin/cursor
```

### Lỗi khi chạy trên Wayland:

```bash
# Chạy với X11
GDK_BACKEND=x11 ./Cursor.AppImage
```

## Update Cursor

### Với .deb:

```bash
# Tải file .deb mới và cài đặt lại
sudo dpkg -i cursor-new.deb
```

### Với AppImage:

```bash
# Tải file AppImage mới và thay thế file cũ
wget https://downloader.cursor.sh/linux/appImage/x64 -O Cursor.AppImage
chmod +x Cursor.AppImage
```

## Gỡ cài đặt

### Với .deb:

```bash
sudo apt-get remove cursor
```

### Với AppImage:

```bash
# Xóa file
rm ~/Applications/Cursor.AppImage
rm ~/.local/share/applications/cursor.desktop
```

## Lưu ý

- Cursor yêu cầu **64-bit Linux**
- Cần **glibc 2.28+** hoặc tương đương
- Kiểm tra system requirements tại: https://cursor.sh/docs

## Link tải chính thức

- **Website:** https://cursor.sh/
- **GitHub Releases:** https://github.com/getcursor/cursor/releases
- **Documentation:** https://cursor.sh/docs

