# STM32F103 - LED Toggle bằng Ngắt Ngoài & SysTick

## Mô tả
Chương trình thực hiện hai chức năng song song:
1. **Ngắt ngoài với nút nhấn (PA12):**  
   - Khi nhấn nút, LED **PC13** sẽ đổi trạng thái (toggle).  
   - Có cơ chế **chống dội phím (debounce)** bằng SysTick (200ms).  

2. **LED nhấp nháy định kỳ:**  
   - LED **PB7** nhấp nháy với chu kỳ **1Hz** (500ms sáng, 500ms tắt).  
   - Sử dụng **SysTick timer** với chu kỳ 1ms để đếm thời gian.  

## Cấu hình phần cứng
- **LED1:** PC13 (toggle theo nút nhấn PA12).  
- **LED2:** PB7 (nhấp nháy tự động 1Hz).  
- **Button:** PA12, cấu hình **Input Pull-Down**.  

## Cấu hình chính
- **SysTick Timer:**  
  - Tần số: 1ms (SystemCoreClock / 1000 với clock = 72MHz).  
  - Dùng để tạo bộ đếm ms và chống dội phím.  

- **Ngắt ngoài EXTI Line12:**  
  - Gắn với chân PA12.  
  - Kích hoạt khi có cạnh lên (**Rising edge**).  
  - Ưu tiên NVIC ở mức cao nhất (preemption = 0, sub = 0).  

## Luồng hoạt động
1. **Khởi tạo:**
   - Cấu hình SysTick 1ms.  
   - Cấu hình LED PC13, PB7 làm output.  
   - Cấu hình PA12 làm input, kích hoạt EXTI Line12.  

2. **Trong vòng lặp chính (main loop):**
   - LED PB7 nhấp nháy theo `msTicks` với chu kỳ 1Hz.  

3. **Trong ISR:**
   - **SysTick_Handler:** tăng biến `msTicks` mỗi 1ms.  
   - **EXTI15_10_IRQHandler:** kiểm tra debounce, nếu hợp lệ → đảo trạng thái LED PC13.  

## Kết quả
- **LED PB7** nhấp nháy định kỳ 1Hz.  
- **LED PC13** đổi trạng thái mỗi lần nhấn nút PA12 (chống dội 200ms).  
