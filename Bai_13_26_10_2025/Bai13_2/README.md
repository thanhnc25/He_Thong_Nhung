# UART1 + FreeRTOS: Vì sao 2 chuỗi bị "đè"/lồng vào nhau?

## Mô tả nhanh
Trong `USER/main.c` có 2 task FreeRTOS (cùng priority) cùng gửi dữ liệu ra UART1:
- TaskA gửi chuỗi: `"1234567890"`
- TaskB gửi chuỗi: `"abcdefghij"`

Mỗi task gửi THEO TỪNG KÝ TỰ, và sau khi gửi 1 ký tự thì `vTaskDelay(1)` để nhường CPU. Không dùng mutex hay bất kỳ cơ chế bảo vệ tài nguyên dùng chung nào.

Kết quả trên terminal thường thấy hai chuỗi bị xen kẽ, ví dụ:
```
1a2b3c4d5e6f7g8h9i0j\r\n
```
Hoặc có lúc dấu xuống dòng (CRLF) của task này chèn vào giữa dòng của task kia.

## Điều gì đang xảy ra?
- UART là tài nguyên dùng chung (shared resource). Khi 2 task cùng truy cập mà không có cơ chế đồng bộ (mutex/semaphore/queue), thứ tự gửi là KHÔNG ĐỊNH TRƯỚC.
- Trong code, mỗi task chỉ gửi 1 byte rồi `vTaskDelay(1)`. Vì 2 task cùng priority, FreeRTOS sẽ luân phiên (time-slicing) giữa chúng ở mỗi tick, dẫn đến 2 chuỗi bị LỒNG THEO KÝ TỰ.
- Ở tốc độ 115200 bps, 1 byte ~ 86 µs. Tick RTOS mặc định thường 1 ms, nên nếu KHÔNG `vTaskDelay`, một task có thể gửi nhiều byte trong 1 tick rồi mới nhường. Nhưng vì ta cố ý `vTaskDelay(1)` sau MỖI KÝ TỰ, nên việc xen kẽ xảy ra đều đặn hơn.

## Có bị "hỏng" byte không?
- Không. Mỗi byte UART được phần cứng truyền nguyên vẹn (có start/stop bit). "Đè" ở đây là ở MỨC CHUỖI — tức là chuỗi A và B bị trộn trật tự, không phải bit trong cùng 1 byte bị lỗi.
- Tuy nhiên, mặt hiển thị sẽ khó đọc vì các ký tự từ 2 chuỗi trộn vào nhau, đặc biệt là khi CR/LF của task này chen giữa chuỗi của task kia.

## Vì sao không dùng mutex lại dẫn đến hiện tượng này?
- Mutex đảm bảo tính loại trừ tương hỗ (mutual exclusion): chỉ 1 task sở hữu UART tại 1 thời điểm. Không có mutex, cả 2 task đều có thể vào hàm gửi gần như bất kỳ lúc nào scheduler cho phép → thứ tự xen kẽ phụ thuộc lịch (schedule) và thời điểm tick.

## Các tham số ảnh hưởng
- `configUSE_TIME_SLICING`: nếu bật (thường = 1), các task cùng priority sẽ được chia thời gian CPU theo tick, tăng khả năng xen kẽ.
- Tốc độ baud: càng cao thì mỗi byte truyền càng nhanh; kết hợp với `vTaskDelay` sẽ quyết định mức độ xen kẽ (theo ký tự hay theo cụm ký tự).
- `vTaskDelay`/`portTICK_PERIOD_MS`: độ dài delay sẽ quyết định tần suất nhường CPU giữa 2 task.

## Cách tái hiện
- Nạp project và mở terminal 115200 8N1.
- Code tham chiếu: `Bai_13_26_10_2025/Bai13_2/USER/main.c`.
- Quan sát 2 task cùng gửi, mỗi ký tự kèm `vTaskDelay(1)` → chuỗi xen kẽ rõ rệt.

## Cách khắc phục (nếu muốn in không bị trộn)
Chọn 1 trong các hướng sau:
- Dùng mutex quanh hàm gửi UART (đơn giản nhất, tiêu tốn ít thay đổi):
	- Ví dụ: `xSemaphoreTake(uartMutex, portMAX_DELAY); ... send ...; xSemaphoreGive(uartMutex);`
- Dùng 1 "UART print task" duy nhất, các task khác gửi message vào queue; print task đọc queue và gửi ra UART tuần tự (kiểu producer-consumer) → trật tự rõ ràng, mở rộng tốt.
- Gửi cả chuỗi trong critical section/priority cao (ít khuyến nghị vì chặn scheduler/ISR lâu với chuỗi dài).
- Sử dụng DMA TX + ring buffer và bảo vệ vùng nạp buffer bằng mutex/queue; phần DMA truyền tuần tự theo FIFO.
- Không `vTaskDelay(1)` sau mỗi ký tự, hoặc tăng priority 1 task để nó gửi hết chuỗi trước; tuy nhiên đây chỉ là mẹo, không giải quyết vấn đề tranh chấp tài nguyên về bản chất.

## Thử nghiệm nhanh
- Đổi `vTaskDelay(1)` thành `vTaskDelay(5)` → xen kẽ thưa hơn, dễ thấy CR/LF chen giữa.
- Bỏ hẳn `vTaskDelay` → xen kẽ theo cụm ký tự (do time-slicing 1ms), ít lộn xộn hơn nhưng vẫn có thể trộn dòng.
- Tăng/giảm baudrate để thấy sự khác biệt giữa thời gian truyền byte và tick của RTOS.

## Kết luận
Hiện tượng 2 chuỗi bị "đè"/lồng vào nhau xuất phát từ việc 2 task cùng truy cập UART mà không có đồng bộ. Với thiết kế hiện tại (gửi từng ký tự + `vTaskDelay(1)`), ta cố ý tạo ra sự xen kẽ theo ký tự để minh hoạ. Nếu muốn output sạch, hãy tuần tự hoá truy cập UART bằng mutex hoặc một print task dùng queue.
