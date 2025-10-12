# Giải thích chi tiết về FreeRTOS trong code Basic

## 1. FreeRTOS là gì?
FreeRTOS là một hệ điều hành thời gian thực (RTOS) mã nguồn mở, dùng cho vi điều khiển. FreeRTOS giúp quản lý đa nhiệm, cho phép nhiều tác vụ (task) chạy song song, chia sẻ tài nguyên và xử lý các sự kiện thời gian thực.

## 2. Cách FreeRTOS hoạt động trong code Basic

### a. Khởi tạo và cấu hình
- **SystemInit()**: Khởi tạo hệ thống vi điều khiển STM32.
- **GPIO_Config()**: Cấu hình các chân GPIOB12, GPIOB13, GPIOB14 làm đầu ra để điều khiển LED.

### b. Tạo các Task
- Sử dụng hàm **xTaskCreate()** để tạo 3 task:
  - `Task_LED_B12_3Hz`: Nháy LED B12 với tần số 3Hz
  - `Task_LED_B13_10Hz`: Nháy LED B13 với tần số 10Hz
  - `Task_LED_B14_25Hz`: Nháy LED B14 với tần số 25Hz
- Mỗi task là một hàm riêng, thực hiện đảo trạng thái chân GPIO tương ứng và delay theo tần số yêu cầu.

#### Giải thích các tham số của hàm xTaskCreate:
```
xTaskCreate(TaskFunction, Name, StackSize, Parameters, Priority, Handle);
```
- **TaskFunction**: Con trỏ tới hàm thực thi task.
- **Name**: Tên task (dùng cho debug).
- **StackSize**: Kích thước stack cho task (đơn vị là word, không phải byte).
- **Parameters**: Tham số truyền vào cho task (thường là NULL nếu không cần).
- **Priority**: Độ ưu tiên của task (số càng lớn càng ưu tiên).
- **Handle**: Con trỏ lưu lại handle của task (thường NULL nếu không cần quản lý).

### c. Scheduler
- **vTaskStartScheduler()**: Khởi động bộ lập lịch của FreeRTOS. Từ đây, các task sẽ được quản lý và thực thi luân phiên dựa trên độ ưu tiên và trạng thái của chúng.

### d. Task hoạt động như thế nào?
- Mỗi task là một vòng lặp vô hạn:
  - Đảo trạng thái chân GPIO (bật/tắt LED)
  - Gọi `vTaskDelay(pdMS_TO_TICKS(x))` để tạm dừng task trong một khoảng thời gian nhất định (tính bằng ms), giúp tạo tần số nháy LED mong muốn.
- FreeRTOS sẽ tự động chuyển đổi giữa các task, đảm bảo các LED nháy độc lập và đúng tần số.

## 3. Ưu điểm khi dùng FreeRTOS
- Quản lý đa nhiệm dễ dàng, mỗi LED là một task riêng biệt.
- Độ chính xác thời gian cao nhờ bộ lập lịch của FreeRTOS.
- Code rõ ràng, dễ mở rộng (thêm task mới chỉ cần thêm hàm và gọi xTaskCreate).

## 4. Tổng kết
- FreeRTOS giúp tách biệt logic điều khiển từng LED thành các task độc lập.
- Scheduler của FreeRTOS đảm bảo các task được thực thi đúng thời gian, không bị xung đột.
- Đây là cách tiếp cận chuyên nghiệp cho các ứng dụng nhúng cần xử lý nhiều tác vụ đồng thời.

---

## FreeRTOS trong code Advanced

- Sử dụng **xTaskCreate()** để tạo các task, nhưng chỉ dùng một hàm generic `Task_LED_Generic` cho tất cả các LED.
- Tham số truyền vào cho mỗi task là một struct chứa thông tin về GPIO và tần số nháy.

### Giải thích chi tiết về struct và cách truyền tham số
- Struct `LED_Config_t` được định nghĩa như sau:
  ```c
  typedef struct {
      GPIO_TypeDef* GPIOx;      // Con trỏ tới port GPIO (ví dụ GPIOB)
      uint16_t GPIO_Pin;        // Số hiệu chân GPIO (ví dụ GPIO_Pin_12)
      uint16_t frequency_hz;    // Tần số nháy LED (Hz)
  } LED_Config_t;
  ```
- Khi tạo task, ta khai báo các biến struct (ví dụ `led1`, `led2`, `led3`) chứa thông tin cho từng LED.
- Truyền địa chỉ của biến struct vào hàm `xTaskCreate` ở tham số thứ 4 (Parameters):
  ```c
  static LED_Config_t led1 = {GPIOB, GPIO_Pin_12, 3};
  xTaskCreate(Task_LED_Generic, "LED_B12_3Hz", 128, &led1, 1, NULL);
  ```
- Khi task được thực thi, tham số truyền vào (`pvParameters`) chính là con trỏ tới struct này. Trong hàm task, ta ép kiểu lại:
  ```c
  void Task_LED_Generic(void *pvParameters) {
      LED_Config_t* ledConfig = (LED_Config_t*)pvParameters;
      // ... sử dụng ledConfig->GPIOx, ledConfig->GPIO_Pin, ledConfig->frequency_hz ...
  }
  ```
- Như vậy, mỗi task sẽ truy cập đúng thông tin LED của mình thông qua con trỏ struct đã truyền vào.

- FreeRTOS quản lý các task giống như phiên bản basic, nhưng code ngắn gọn hơn, dễ mở rộng hơn.
- Scheduler của FreeRTOS vẫn đảm bảo các task được thực thi luân phiên, các LED nháy độc lập với tần số truyền vào.
- Việc truyền tham số cho task giúp tái sử dụng hàm, chỉ cần thay đổi struct là có thể tạo thêm task mới cho LED khác hoặc tần số khác.
