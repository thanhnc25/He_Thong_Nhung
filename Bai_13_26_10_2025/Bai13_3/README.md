# UART + FreeRTOS + Mutex: Chia sẻ UART an toàn giữa 2 task

## Mục tiêu
Hai task FreeRTOS (cùng priority) lần lượt in chuỗi ra UART1 nhưng KHÔNG bị lồng/trộn ký tự. Ta dùng mutex để tuần tự hoá truy cập UART – mỗi lần chỉ 1 task được quyền in trọn vẹn cả chuỗi.

## Ý tưởng chính
- UART là tài nguyên dùng chung. Nếu 2 task cùng in ký tự mà không đồng bộ, output sẽ trộn (interleave).
- Dùng `SemaphoreHandle_t` (mutex) để đảm bảo loại trừ tương hỗ: Task nào lấy được mutex sẽ in toàn bộ chuỗi, sau đó mới nhả mutex cho task khác.
- FreeRTOS mutex có cơ chế Priority Inheritance giúp giảm nguy cơ đảo ưu tiên khi task thấp giữ mutex mà task cao đang chờ.

## Các điểm chính trong code

1) Khai báo và tạo mutex:
- Khai báo toàn cục: `static SemaphoreHandle_t g_uartMutex = NULL;`
- Trong `main()`, trước khi tạo task: `g_uartMutex = xSemaphoreCreateMutex();`
- Nếu tạo mutex thất bại (thiếu heap) thì dừng lại để tránh in lộn xộn.

2) Hàm gửi có bảo vệ:
- `uart1_send_string(const char *s)` sẽ `xSemaphoreTake(g_uartMutex, portMAX_DELAY);` rồi gửi toàn bộ chuỗi và `CRLF`, sau đó `xSemaphoreGive(g_uartMutex);`.
- Không chèn `vTaskDelay` bên trong đoạn giữ mutex để tránh khoá tài nguyên lâu.

3) TaskA/TaskB sử dụng API mới:
- Mỗi task chỉ việc gọi `uart1_send_string("...")` và `vTaskDelay(3000);` giữa 2 lần in.
- Nhờ mutex, mỗi chuỗi sẽ được in liền mạch, không còn xen kẽ ký tự với task kia.

## Hành vi khi chạy
- Trước (không mutex, gửi từng ký tự + `vTaskDelay(1)`): output thường bị lồng như `1a2b3c...`.
- Nay (có mutex, gửi cả chuỗi trong 1 lần giữ mutex): output theo từng dòng hoàn chỉnh, ví dụ:
	- `1234567890\r\n`
	- `abcdefghij\r\n`
	- Lặp lại theo chu kỳ delay của mỗi task.

## Lưu ý thực hành tốt
- Không `vTaskDelay` trong khi đang giữ mutex. Nếu cần chờ, hãy nhả mutex trước rồi delay.
- Giữ mutex càng ngắn càng tốt (chỉ bọc đoạn gửi). Với chuỗi dài hoặc log nhiều, cân nhắc mô hình "print task" + queue để giảm blocking.
- Đảm bảo tạo mutex trước khi tạo task để tránh race condition lúc khởi động.

## Kết luận
Mutex giúp tuần tự hoá quyền truy cập UART giữa các task, loại bỏ hiện tượng chuỗi bị trộn. Đây là cách đơn giản, dễ tích hợp; với hệ thống log phức tạp, cân nhắc chuyển sang mô hình queue + print task để đạt hiệu năng và khả năng mở rộng tốt hơn.
