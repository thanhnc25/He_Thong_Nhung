# Giải thích mã nguồn FreeRTOS với Ngắt ngoài (EXTI) trên STM32F103

Đây là một dự án mẫu minh họa cách sử dụng ngắt ngoài để giao tiếp với một tác vụ (task) trong FreeRTOS một cách an toàn. Cơ chế được sử dụng là **Binary Semaphore**.

## Chức năng chính

Chương trình thực hiện hai công việc đồng thời:

1.  **Nhấp nháy LED (Task 1):** Một đèn LED kết nối với chân `PC13` sẽ nhấp nháy liên tục với chu kỳ 2 giây (1 giây sáng, 1 giây tắt).
2.  **Xử lý sự kiện nút nhấn (Task 2 & Ngắt):**
    *   Một nút nhấn được kết nối với chân `PA0`.
    *   Khi nút nhấn được nhấn, nó sẽ tạo ra một ngắt ngoài (EXTI line 0).
    *   Trình phục vụ ngắt (ISR) sẽ "giải phóng" một semaphore.
    *   Task 2, vốn đang ở trạng thái chờ (Blocked), sẽ được đánh thức bởi semaphore này.
    *   Sau khi được đánh thức, Task 2 sẽ bật một đèn LED khác ở chân `PB12` trong 1 giây rồi tắt. Sau đó, nó quay lại trạng thái chờ semaphore.

## Sơ đồ hoạt động

```
+----------------+      (Nhấn nút)      +-----------------+      (Give Semaphore)      +----------------+
|   Nút nhấn     | -------------------> |   Ngắt EXTI0    | -------------------------> |     Task 2     |
|     (PA0)      |                      | (EXTI0_IRQHandler)|                          |   (vTaskEvent) |
+----------------+                      +-----------------+      (Take Semaphore)      +----------------+
                                                                                            |
                                                                                            | 1. Bật LED (PB12)
                                                                                            | 2. Chờ 1 giây
                                                                                            | 3. Tắt LED (PB12)
                                                                                            | 4. Quay lại chờ
                                                                                            V
                                                                                    (Trạng thái Blocked)
```

Trong khi đó, Task 1 (`vTaskBlink`) chạy độc lập và liên tục nhấp nháy LED ở chân `PC13`.

## Phân tích mã nguồn (`main.c`)

### 1. `main()`

-   **`SystemInit()`**: Khởi tạo hệ thống cơ bản cho vi điều khiển.
-   **`NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4)`**: Cấu hình nhóm ưu tiên ngắt. Đây là bước **bắt buộc** khi dùng FreeRTOS trên ARM Cortex-M để đảm bảo hệ thống quản lý ưu tiên ngắt tương thích với RTOS.
-   **`GPIO_Init_Custom()`**: Khởi tạo các chân GPIO:
    -   `PC13`: Output Push-Pull cho Task nhấp nháy LED.
    -   `PB12`: Output Push-Pull cho Task sự kiện.
    -   `PA0`: Input Pull-up cho nút nhấn.
-   **`EXTI0_Config_Custom()`**: Cấu hình ngắt ngoài trên chân `PA0`.
-   **`xButtonSem = xSemaphoreCreateBinary()`**: Tạo một binary semaphore. Semaphore này hoạt động như một cờ hiệu, dùng để báo cho Task 2 biết rằng nút nhấn đã được nhấn.
-   **`xTaskCreate(...)`**: Tạo hai tác vụ:
    -   `vTaskBlink`: Ưu tiên `tskIDLE_PRIORITY + 1`.
    -   `vTaskEvent`: Ưu tiên `tskIDLE_PRIORITY + 2` (cao hơn `vTaskBlink`).
-   **`vTaskStartScheduler()`**: Bắt đầu bộ lập lịch của FreeRTOS. Kể từ đây, các tác vụ sẽ được thực thi theo sự quản lý của hệ điều hành.

### 2. `vTaskBlink` (Task 1)

-   Đây là một tác vụ đơn giản, chạy trong một vòng lặp vô tận (`for(;;)`).
-   Trong mỗi chu kỳ, nó đảo trạng thái của `PC13` và sau đó tạm dừng (`vTaskDelay`) trong 1000ms. `vTaskDelay` sẽ đưa tác vụ vào trạng thái Blocked, nhường CPU cho các tác vụ khác.

### 3. `vTaskEvent` (Task 2)

-   Tác vụ này cũng chạy trong một vòng lặp vô tận.
-   **`xSemaphoreTake(xButtonSem, portMAX_DELAY)`**: Đây là điểm mấu chốt. Lệnh này cố gắng "lấy" semaphore.
    -   Nếu semaphore chưa có sẵn (chưa ai "give"), tác vụ sẽ bị **khóa** tại đây vô thời hạn (`portMAX_DELAY`), không tiêu tốn CPU.
    -   Khi semaphore có sẵn (do ISR "give"), lệnh này thực thi thành công và tác vụ tiếp tục chạy.
-   Sau khi lấy được semaphore, tác vụ sẽ bật LED `PB12`, chờ 1 giây, tắt LED `PB12`, và sau đó quay lại đầu vòng lặp để tiếp tục chờ semaphore.

### 4. `EXTI0_IRQHandler` (Trình phục vụ ngắt)

-   Đây là hàm sẽ được CPU tự động gọi khi có ngắt trên đường EXTI0 (do nhấn nút `PA0`).
-   **`EXTI_ClearITPendingBit(EXTI_Line0)`**: Xóa cờ ngắt để báo cho vi điều khiển biết ngắt đã được xử lý, tránh việc hàm này bị gọi lại liên tục.
-   **`xSemaphoreGiveFromISR(xButtonSem, &xHigherPriorityTaskWoken)`**: Đây là hàm **an toàn cho ngắt** (ISR-safe) của FreeRTOS.
    -   Nó "give" (giải phóng) semaphore `xButtonSem`. Hành động này sẽ đánh thức `vTaskEvent` đang chờ semaphore đó.
    -   `&xHigherPriorityTaskWoken` là một tham số đầu ra. Nếu việc "give" semaphore làm cho một tác vụ có ưu tiên cao hơn tác vụ hiện tại sẵn sàng chạy, biến này sẽ được set thành `pdTRUE`.
-   **`portYIELD_FROM_ISR(xHigherPriorityTaskWoken)`**: Nếu `xHigherPriorityTaskWoken` là `pdTRUE`, lệnh này sẽ yêu cầu hệ điều hành thực hiện chuyển đổi ngữ cảnh ngay lập tức khi thoát khỏi ISR, để tác vụ có ưu tiên cao hơn (trong trường hợp này là `vTaskEvent`) được chạy ngay.

## Tại sao phải dùng Semaphore?

-   **An toàn và đồng bộ**: Không nên gọi các hàm như `vTaskDelay` hoặc thực hiện các xử lý dài trong ISR. ISR cần được thực thi nhanh nhất có thể.
-   **Tách biệt xử lý**: ISR chỉ làm nhiệm vụ tối thiểu là thông báo sự kiện đã xảy ra. Việc xử lý sự kiện (bật/tắt LED, chờ đợi) được "ủy quyền" cho một tác vụ riêng.
-   **Tối ưu CPU**: Khi không có sự kiện, `vTaskEvent` ở trạng thái Blocked và không tốn tài nguyên CPU, khác với việc dùng vòng lặp kiểm tra (polling) trạng thái nút nhấn.
-   Đây là mẫu thiết kế (design pattern) phổ biến và hiệu quả trong các hệ thống nhúng sử dụng RTOS.
