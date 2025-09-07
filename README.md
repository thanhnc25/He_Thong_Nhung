# Bài 2- Yêu cầu 1: STM32 LED Blinking (PC13 và PB7)

## Mô tả chương trình
Chương trình Blink Led với 2 led được kết nối tại các chân PC13 và PB7.

- Cả hai chân được cấu hình ở chế độ **Output Push-Pull**.  
- LED sẽ nhấp nháy với chu kỳ 1000ms (1 giây):
  - LED sáng trong 500ms
  - LED tắt trong 500ms

Kết quả là hai LED sáng tắt đồng bộ với chu kỳ 1 giây.


## Cấu hình phần cứng
- LED1 nối với chân PC13.  
- LED2 nối với chân PB7.  
- Hai chân được cấu hình ở chế độ Output Push-Pull với tốc độ 50MHz.  
- Thời gian trễ được tạo bởi hàm `delay_1ms()` sử dụng vòng lặp for (phụ thuộc vào xung nhịp hệ thống, không chính xác tuyệt đối).


## Cấu trúc chương trình
1. **Hàm main()**
   - Gọi hàm `LED_Init()` để cấu hình GPIO.  
   - Trong vòng lặp vô hạn `while(1)`:  
     - Bật LED tại PC13 và PB7.  
     - Delay 500ms.  
     - Tắt LED tại PC13 và PB7.  
     - Delay 500ms.  

2. **Hàm LED_Init()**
   - Kích hoạt clock cho GPIOC và GPIOB.  
   - Cấu hình PC13 ở chế độ Output Push-Pull.  
   - Cấu hình PB7 ở chế độ Output Push-Pull.  

3. **Hàm delay_1ms(unsigned int time)**
   - Tạo độ trễ theo đơn vị ms thông qua vòng lặp for.  
   - Dùng để tạo thời gian sáng và tắt cho LED.  


## Chu kỳ nhấp nháy
- Thời gian sáng: 500ms  
- Thời gian tắt: 500ms  
- Chu kỳ: 500ms + 500ms = 1000ms (1 giây)  
- Tần số nhấp nháy: 1Hz  


## Kết quả
Khi nạp chương trình lên vi điều khiển STM32F103:  
- LED nối với PC13 và LED nối với PB7 sẽ cùng sáng trong 500ms.  
- Sau đó cả hai sẽ cùng tắt trong 500ms.  
- Quá trình lặp lại liên tục với chu kỳ 1 giây. 
---
# STM32 LED Toggle with Button (PC13 và PA12)

## Mô tả chương trình
Chương trình này được viết cho vi điều khiển STM32F10x với mục đích điều khiển LED bằng nút nhấn:

- LED được nối tại chân **PC13**, cấu hình ở chế độ Output Push-Pull.  
- Nút nhấn được nối tại chân **PA12**, cấu hình ở chế độ Input Pull-Down.  
- Mỗi lần người dùng nhấn nút, LED sẽ **đổi trạng thái**:
  - Nếu LED đang tắt thì sẽ bật.
  - Nếu LED đang bật thì sẽ tắt.


## Cấu hình phần cứng
- LED: nối với chân **PC13**, xuất tín hiệu ở chế độ Output Push-Pull, tốc độ 50MHz.  
- Nút nhấn: nối với chân **PA12**, ở chế độ Input Pull-Down để đảm bảo mức logic ổn định khi chưa nhấn.  


## Cấu trúc chương trình
1. **Hàm main()**
   - Gọi hàm `LED_PC_13_Init()` để cấu hình LED.  
   - Gọi hàm `Button_Init()` để cấu hình nút nhấn.  
   - Trong vòng lặp `while(1)`:  
     - Đọc trạng thái nút nhấn.  
     - So sánh trạng thái trước và sau để phát hiện cạnh nhấn (từ 0 → 1).  
     - Nếu phát hiện nhấn nút, tăng biến đếm `cnt_btn_press`.  
     - Dựa vào `cnt_btn_press` để xác định LED bật hay tắt:
       - Nếu `cnt_btn_press` là số chẵn → LED tắt.  
       - Nếu `cnt_btn_press` là số lẻ → LED bật.  

2. **Hàm LED_PC_13_Init()**
   - Bật clock cho GPIOC.  
   - Cấu hình PC13 làm Output Push-Pull.  

3. **Hàm Button_Init()**
   - Bật clock cho GPIOA.  
   - Cấu hình PA12 làm Input Pull-Down.  

4. **Hàm delay_1ms(unsigned int time)**
   - Tạo độ trễ bằng vòng lặp for.  
   - Dùng để chống dội phím (debounce) trong khi đọc nút nhấn.  


## Cơ chế hoạt động
- Khi nút nhấn chưa được bấm, đầu vào PA12 có giá trị logic 0 (do pull-down).  
- Khi nút nhấn được bấm, PA12 nhận giá trị logic 1.  
- Chương trình kiểm tra sự thay đổi từ 0 → 1 để xác định thao tác nhấn nút hợp lệ.  
- Mỗi lần nhấn nút, LED thay đổi trạng thái ngược lại với trạng thái trước đó.  


## Kết quả 
- Khi nạp chương trình lên vi điều khiển STM32F103:  
  - LED trên chân PC13 sẽ **bật hoặc tắt luân phiên** mỗi lần người dùng nhấn nút PA12.  
  - Nếu người dùng nhấn nút nhiều lần liên tiếp, LED sẽ thay đổi trạng thái theo từng lần nhấn.  
