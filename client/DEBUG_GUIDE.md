# Hướng dẫn Debug Client-Server Connection

## Vấn đề hiện tại

Client không thể gửi REGISTER request đến server, hoặc server không nhận được request.

## Đã thêm Debug Logging

Tôi đã thêm debug logging vào client để xem chính xác điều gì đang xảy ra:

1. **Trong `main.cpp`**: Log khi nhận CONNECTION notification
2. **Trong `protocol_handler.cpp`**: Log khi gửi/nhận request/response
3. **Trong `client_core.cpp`**: Log khi gửi message

## Cách sử dụng Debug

### Bước 1: Rebuild client với debug logging

```bash
cd ~/TheMillionaireGame/client
make clean
make
```

### Bước 2: Chạy client và xem debug output

```bash
./bin/client
```

Bạn sẽ thấy các dòng debug như:
- `[DEBUG] Sending request: ...`
- `[DEBUG] Sent X bytes to server`
- `[DEBUG] Received message: ...`
- `[DEBUG] This is a response/notification`

### Bước 3: Kiểm tra server log

Trong terminal server, bạn sẽ thấy:
- Client connected
- Request received (nếu có)
- Response sent (nếu có)

## Các trường hợp có thể xảy ra

### Trường hợp 1: Client gửi được nhưng server không nhận

**Dấu hiệu:**
- Client log: `[DEBUG] Sent X bytes`
- Server log: Không có request nào

**Nguyên nhân có thể:**
- Message bị mất trên đường truyền
- Server đang đọc CONNECTION notification thay vì REGISTER request

### Trường hợp 2: Server nhận được nhưng client không nhận response

**Dấu hiệu:**
- Server log: Có request và response
- Client log: `[DEBUG] Empty message received`

**Nguyên nhân có thể:**
- Client đang đọc notification thay vì response
- Timeout quá ngắn

### Trường hợp 3: Client không gửi được

**Dấu hiệu:**
- Client log: `[DEBUG] Cannot send: not connected` hoặc `Error sending message`

**Nguyên nhân có thể:**
- Connection bị đóng
- Socket error

## Cách sửa

Sau khi xem debug log, chúng ta sẽ biết chính xác vấn đề ở đâu và sửa tương ứng.

## Tắt Debug Logging

Sau khi fix xong, có thể tắt debug bằng cách:
1. Xóa hoặc comment các dòng `cerr << "[DEBUG] ..."`
2. Hoặc thêm flag `#ifdef DEBUG` để có thể bật/tắt dễ dàng

