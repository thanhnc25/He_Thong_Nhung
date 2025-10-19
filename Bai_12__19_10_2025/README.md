# Điều khiển tần số nháy LED qua Queue trên STM32F103

# Giải thích chi tiết code điều khiển tần số và độ rộng xung LED qua Queue


## 1. Định nghĩa cấu trúc tham số LED
```c
typedef struct {
    uint32_t frequency_hz;         // Tần số nháy (Hz)
    uint32_t duty_cycle_percent;   // Độ rộng xung (0-100%)
} LedParameters_t;
```
Thay vì chỉ truyền tần số, ta truyền cả tần số và độ rộng xung qua queue bằng một struct.

## 2. Khai báo các task và queue
```c
void TaskLedControl(void *pvParameters);      // Task điều khiển LED
void TaskUpdateParameters(void *pvParameters);// Task cập nhật tham số
QueueHandle_t xParameterQueue;                // Queue chứa LedParameters_t
```

## 3. Hàm cấu hình GPIO
```c
void GPIO_Config(void)
{
    // ... cấu hình chân PC13 là output push-pull ...
}
```
Dùng để chuẩn bị chân LED.

## 4. Hàm `main()`
```c
int main(void)
{
    GPIO_Config();
    xParameterQueue = xQueueCreate(1, sizeof(LedParameters_t));
    if (xParameterQueue != NULL)
    {
        xTaskCreate(TaskLedControl, "LedControl", 128, NULL, 1, NULL);
        xTaskCreate(TaskUpdateParameters, "UpdateParams", 128, NULL, 2, NULL);
        vTaskStartScheduler();
    }
    while (1);
}
```
Tạo queue chứa 1 phần tử kiểu struct, tạo 2 task, khởi động scheduler.

## 5. Task điều khiển LED: `TaskLedControl`
```c
void TaskLedControl(void *pvParameters)
{
    LedParameters_t current_params = {2, 50}; // Mặc định: 2Hz, 50%
    while (1)
    {
        // Tính toán chu kỳ, thời gian sáng/tắt dựa trên tần số và duty cycle
        // ...
        // Bật LED, chờ on_time_ticks, lắng nghe queue
        // Nếu nhận được tham số mới thì cập nhật ngay
        // Tắt LED, chờ off_time_ticks, lắng nghe queue
        // Nếu nhận được tham số mới thì cập nhật ngay
    }
}
```
**Điểm nổi bật:**
- Task này điều chỉnh được cả tần số và độ rộng xung (LED sáng bao lâu trong mỗi chu kỳ).
- Sử dụng `xQueueReceive` với timeout cho cả hai pha sáng/tắt, giúp phản ứng tức thì khi có tham số mới.

## 6. Task cập nhật tham số: `TaskUpdateParameters`
```c
void TaskUpdateParameters(void *pvParameters)
{
    const uint32_t parameters[][2] = {
        {2, 10}, {4, 30}, {6, 50}, {8, 70}, {10, 90}
    };
    while (1)
    {
        // Gửi bộ tham số mới vào queue bằng xQueueOverwrite
        // Chờ 5 giây rồi chuyển sang bộ tiếp theo
    }
}
```
**Điểm nổi bật:**
- Task này gửi cả tần số và duty cycle vào queue, giúp LED thay đổi cả tốc độ nháy và độ sáng (tỷ lệ sáng/tắt).
- Dùng `xQueueOverwrite` để luôn đảm bảo giá trị mới nhất được cập nhật.

## 7. Luồng hoạt động tổng thể
1. `main()` cấu hình, tạo queue, tạo 2 task, khởi động scheduler.
2. `TaskUpdateParameters` (ưu tiên cao hơn) gửi bộ tham số đầu tiên vào queue, sau đó chờ 5 giây.
3. `TaskLedControl` nhận tham số, tính toán chu kỳ và duty cycle, điều khiển LED sáng/tắt tương ứng. Trong mỗi pha sáng/tắt đều lắng nghe queue để cập nhật tham số mới ngay khi có.
4. Sau mỗi 5 giây, bộ tham số mới lại được gửi vào queue, LED thay đổi cả tần số và độ rộng xung tức thì.

## 8. Ý nghĩa thực tiễn
- Có thể áp dụng để điều khiển độ sáng LED, động cơ, hoặc các thiết bị cần điều chỉnh cả tốc độ và mức độ hoạt động.
- Mô hình này rất phù hợp cho các ứng dụng nhúng sử dụng FreeRTOS, giúp hệ thống phản ứng nhanh với thay đổi tham số điều khiển.

[//]: # (============================)

# 4. Giải thích chi tiết code `main.c` (bổ sung: điều khiển cả tần số và độ rộng xung)

Phiên bản code mới trong `main.c` không chỉ điều khiển tần số nháy LED mà còn điều chỉnh được độ rộng xung (duty cycle) của LED. Dưới đây là giải thích chi tiết từng phần:

## a. Định nghĩa cấu trúc tham số LED
```c
typedef struct {
    uint32_t frequency_hz;         // Tần số nháy (Hz)
    uint32_t duty_cycle_percent;   // Độ rộng xung (0-100%)
} LedParameters_t;
```
Thay vì chỉ truyền tần số, ta truyền cả tần số và độ rộng xung qua queue bằng một struct.

## b. Khai báo các task và queue
```c
void TaskLedControl(void *pvParameters);      // Task điều khiển LED
void TaskUpdateParameters(void *pvParameters);// Task cập nhật tham số
QueueHandle_t xParameterQueue;                // Queue chứa LedParameters_t
```

## c. Hàm cấu hình GPIO
```c
void GPIO_Config(void)
{
    // ... cấu hình chân PC13 là output push-pull ...
}
```


## d. Hàm `main()`
```c
int main(void)
{
    GPIO_Config();
    xParameterQueue = xQueueCreate(1, sizeof(LedParameters_t));
    if (xParameterQueue != NULL)
    {
        xTaskCreate(TaskLedControl, "LedControl", 128, NULL, 1, NULL);
        xTaskCreate(TaskUpdateParameters, "UpdateParams", 128, NULL, 2, NULL);
        vTaskStartScheduler();
    }
    while (1);
}
```
Tạo queue chứa 1 phần tử kiểu struct, tạo 2 task, khởi động scheduler.

## e. Task điều khiển LED: `TaskLedControl`
```c
void TaskLedControl(void *pvParameters)
{
    LedParameters_t current_params = {2, 50}; // Mặc định: 2Hz, 50%
    while (1)
    {
        // Tính toán chu kỳ, thời gian sáng/tắt dựa trên tần số và duty cycle
        // ...
        // Bật LED, chờ on_time_ticks, lắng nghe queue
        // Nếu nhận được tham số mới thì cập nhật ngay
        // Tắt LED, chờ off_time_ticks, lắng nghe queue
        // Nếu nhận được tham số mới thì cập nhật ngay
    }
}
```
**Điểm nổi bật:**
- Task này không chỉ nháy LED với tần số thay đổi mà còn điều chỉnh được độ rộng xung (LED sáng bao lâu trong mỗi chu kỳ).
- Sử dụng `xQueueReceive` với timeout cho cả hai pha sáng/tắt, giúp phản ứng tức thì khi có tham số mới.

## f. Task cập nhật tham số: `TaskUpdateParameters`
```c
void TaskUpdateParameters(void *pvParameters)
{
    const uint32_t parameters[][2] = {
        {2, 10}, {4, 30}, {6, 50}, {8, 70}, {10, 90}
    };
    while (1)
    {
        // Gửi bộ tham số mới vào queue bằng xQueueOverwrite
        // Chờ 5 giây rồi chuyển sang bộ tiếp theo
    }
}
```
**Điểm nổi bật:**
- Task này gửi cả tần số và duty cycle vào queue, giúp LED thay đổi cả tốc độ nháy và độ sáng (tỷ lệ sáng/tắt).
- Dùng `xQueueOverwrite` để luôn đảm bảo giá trị mới nhất được cập nhật.

## g. Luồng hoạt động tổng thể
1. `main()` cấu hình, tạo queue, tạo 2 task, khởi động scheduler.
2. `TaskUpdateParameters` (ưu tiên cao hơn) gửi bộ tham số đầu tiên vào queue, sau đó chờ 5 giây.
3. `TaskLedControl` nhận tham số, tính toán chu kỳ và duty cycle, điều khiển LED sáng/tắt tương ứng. Trong mỗi pha sáng/tắt đều lắng nghe queue để cập nhật tham số mới ngay khi có.
4. Sau mỗi 5 giây, bộ tham số mới lại được gửi vào queue, LED thay đổi cả tần số và độ rộng xung tức thì.

## h. So sánh với ví dụ cũ
- Ví dụ cũ chỉ điều khiển tần số nháy LED.
- Phiên bản mới điều khiển cả tần số và độ rộng xung (giúp LED có thể sáng ngắn/dài trong mỗi chu kỳ, tạo hiệu ứng PWM đơn giản).
- Cách dùng queue và task vẫn giữ nguyên, chỉ mở rộng thêm kiểu dữ liệu truyền qua queue.

## i. Ý nghĩa thực tiễn
- Có thể áp dụng để điều khiển độ sáng LED, động cơ, hoặc các thiết bị cần điều chỉnh cả tốc độ và mức độ hoạt động.
- Mô hình này rất phù hợp cho các ứng dụng nhúng sử dụng FreeRTOS, giúp hệ thống phản ứng nhanh với thay đổi tham số điều khiển.

[//]: # (============================)
