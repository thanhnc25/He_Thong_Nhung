# STM32F103C8T6 UART Giao Tiếp & Điều Khiển LED

## Mục đích

Chương trình này chạy trên vi điều khiển STM32F103C8T6, sử dụng UART1 để giao tiếp với máy tính hoặc thiết bị khác. Mỗi giây, vi điều khiển sẽ gửi chuỗi "Hello from STM32!" lên UART. Ngoài ra, chương trình còn nhận lệnh từ UART để điều khiển LED trên chân PC13:
- Gửi `on!` để bật LED
- Gửi `off!` để tắt LED
- Gửi chuỗi khác (kết thúc bằng `!`) sẽ được gửi lại (echo) lên UART

## Cấu hình phần cứng
- **UART1 TX**: PA9 (kết nối tới RX của thiết bị khác)
- **UART1 RX**: PA10 (kết nối tới TX của thiết bị khác)
- **LED**: PC13 (trên board Blue Pill hoặc custom)

## Cấu hình phần mềm

### 1. Khởi tạo UART1
- **Bật clock** cho GPIOA và USART1
- **PA9** cấu hình là Alternate Function Push-Pull (TX)
- **PA10** cấu hình là Input Floating (RX)
- **USART1** cấu hình:
  - Baudrate: 9600
  - 8 data bits, 1 stop bit, không parity
  - Cho phép truyền và nhận
- **Bật ngắt nhận UART** (RXNE interrupt)
- **Cấu hình NVIC** cho ngắt USART1

### 2. Khởi tạo LED PC13
- Bật clock cho GPIOC
- PC13 cấu hình là Output Push-Pull, tốc độ 2MHz
- Mặc định tắt LED (PC13 mức cao)

### 3. Khởi tạo Timer2
- Sử dụng Timer2 để tạo hàm `millis()` (đếm thời gian ms)
- Dùng để gửi chuỗi mỗi 1 giây

## Gửi dữ liệu UART
- Hàm `Uart_Send_Char(char c)`: Gửi 1 ký tự qua UART1
- Hàm `Uart_Send_Str(char *str)`: Gửi chuỗi ký tự (kết thúc bằng '\0') qua UART1
- Trong hàm main, mỗi 1 giây sẽ gửi "Hello from STM32!" lên UART

## Nhận dữ liệu UART bằng ngắt
- **Ngắt USART1_IRQHandler** được gọi mỗi khi có ký tự mới nhận về
- Ký tự nhận được sẽ lưu vào buffer `uart_rx_buffer`
- Khi gặp ký tự '!' sẽ kết thúc chuỗi, đặt cờ `uart_command_ready`
- Trong vòng lặp chính, hàm `Process_UART_Command()` sẽ kiểm tra cờ này và xử lý lệnh:
  - Nếu chuỗi là "on" thì bật LED, gửi "LED ON"
  - Nếu chuỗi là "off" thì tắt LED, gửi "LED OFF"
  - Nếu chuỗi khác thì gửi lại chuỗi đó (echo)

## Sơ đồ luồng chương trình
1. Khởi tạo ngoại vi (UART, Timer, LED)
2. Gửi thông báo khởi động qua UART
3. Vòng lặp chính:
   - Gửi "Hello from STM32!" mỗi 1 giây
   - Kiểm tra và xử lý lệnh nhận được từ UART
4. Ngắt UART nhận từng ký tự, phát hiện kết thúc chuỗi bằng '!'

## Lưu ý
- Buffer nhận UART tối đa 49 ký tự (ký tự thứ 50 là '\0')
- Lệnh gửi lên UART phải kết thúc bằng dấu chấm than `!`
- LED PC13 trên Blue Pill là active low (Reset bit để bật, Set bit để tắt)

## Ví dụ sử dụng
- Gửi: `on!` → LED bật, nhận lại "LED ON"
- Gửi: `off!` → LED tắt, nhận lại "LED OFF"
- Gửi: `abc!` → Nhận lại "Echo: abc"

---

