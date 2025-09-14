# Task 1: STM32F103 Timer2 Delay & LED Blink 

## Cấu hình Timer2
- **Clock hệ thống (SYSCLK):** 72 MHz  
- **Prescaler (PSC):** `2 - 1 = 1`  
  → Tần số Timer: `72 MHz / 2 = 36 MHz`  
- **Auto Reload Register (ARR):** `65535`  
  → Bộ đếm có thể đếm từ 0 → 65535  

Trong chương trình:  
- Hàm `delay_1ms()` kiểm tra giá trị bộ đếm Timer.  
- Khi `TIM_GetCounter(TIM2) >= 36000` → đúng bằng 1ms (vì `36 MHz / 36000 = 1 kHz = 1ms`).  

---

## Cấu trúc chương trình

### 1. `Timer2_init()`
- Bật clock cho Timer2.  
- Cấu hình chế độ đếm lên.  
- Đặt Prescaler và Period.  
- Cho phép ngắt `Update`.  
- Enable NVIC cho TIM2.  
- Bật Timer2.

### 2. `TIM2_IRQHandler()`
- Hàm phục vụ ngắt Timer2.  
- Khi có ngắt:
  - `SttBlink` thay đổi trạng thái (0 ↔ 1).  
  - `Stt_Blink` đảo bit.  
- Xóa cờ ngắt bằng `TIM_ClearITPendingBit`.

### 3. `delay_1ms()`
- Khởi tạo Timer2.  
- Vòng lặp chờ cho đến khi `Counter >= 36000` (tương ứng 1ms).  
- Dừng Timer sau khi đạt điều kiện.  

### 4. `Delay(int time)`
- Lặp lại hàm `delay_1ms()` `time` lần.  
- Ví dụ: `Delay(1000)` = 1000ms = 1 giây.

### 5. `LED_PC_13_Init()`
- Cấu hình chân **PC13** làm ngõ ra dạng Push-Pull.  
- Đây là LED tích hợp sẵn trên board **Blue Pill**.

### 6. `main()`
- Gọi `LED_PC_13_Init()`.  
- Trong vòng lặp chính:
  - Bật LED (Reset PC13) trong 1000ms.  
  - Tắt LED (Set PC13) trong 1000ms.  

Kết quả: LED nhấp nháy với chu kỳ **2 giây** (1 giây sáng, 1 giây tắt).

---

# Task 2: STM32F103 - Blink LED với Timer2 và Ngắt

## Mô tả
- Cấu hình một timer cơ bản để tạo ngắt mỗi **500ms**.  
- Trong ISR của Timer2, chương trình sẽ **đảo trạng thái LED PC13**.  
- Kết quả: LED nhấp nháy với **chu kỳ 1000ms** (500ms sáng, 500ms tắt).  

## Cấu hình Timer2
- **Clock APB1 (TIM2):** 72 MHz  
- **Prescaler (PSC):** `7200 - 1` → chia clock xuống còn 10 kHz  
- **Auto-reload (ARR):** `4999` → tràn sau 500 ms  


## Các hàm chính trong code
- `LED_PC13_Init()`  
  Cấu hình chân PC13 làm **Output Push-Pull**.  

- `Timer2_init()`  
  Cấu hình Timer2 với chu kỳ ngắt 500ms, cho phép ngắt và bật Timer2.  

- `TIM2_IRQHandler()`  
  Hàm phục vụ ngắt của Timer2, đảo trạng thái LED mỗi khi Timer tràn.  

## Kết quả chạy chương trình
- LED nối với chân **PC13** sẽ nhấp nháy đều đặn.  
- Chu kỳ nhấp nháy: **1 giây** (500ms sáng, 500ms tắt).  
