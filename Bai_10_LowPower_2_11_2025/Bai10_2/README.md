## Bai10_2 — Low Power Standby với RTC Alarm (STM32F103C8)

### Mục tiêu

- Gửi 1 bản tin qua UART1, sau đó đưa MCU vào chế độ Standby trong SLEEP_SECONDS giây (mặc định 60s).
- Trong thời gian ngủ chỉ giữ RTC hoạt động và đánh thức bằng RTC Alarm (EXTI line 17).
- Trước khi ngủ, deinit ngoại vi (UART, GPIO không dùng) để giảm tiêu thụ.
- Sau khi thức dậy do RTC Alarm, MCU reset và chạy lại từ đầu (đặc trưng của Standby).

File chính: `USER/main.c`

Tham số thời gian ngủ: `#define SLEEP_SECONDS 60` (ở đầu file `main.c`).

UART log (115200 8N1):
- "Standby 60s, chi giu RTC..."
- "Da dat RTC Alarm sau 60s. Tien hanh deinit va vao Standby..."

Sau đó MCU ngủ, hết thời gian thì reset và lặp lại.

---

## Luồng hoạt động chi tiết

1) Khởi động (power-on hoặc nhấn reset):
- Bật clock PWR để có thể thao tác cờ SB/WU.
- Xóa cờ SB (nếu vừa thức dậy từ Standby), xóa cờ WU và pending EXTI17 càng sớm càng tốt để trạng thái sạch.
- Khởi tạo UART1 và in dòng log đầu.

2) Cấu hình RTC 1 Hz qua `RTC_Config_1Hz()`:
- Reset backup domain (BKP/RTC/BDCR) để tránh trạng thái “lỡ cỡ” sau khi nhấn nút reset (NRST không reset backup domain).
- Ưu tiên chọn LSE 32.768 kHz (chính xác), nếu không có thì fallback LSI (~40 kHz, sai số lớn hơn).
- Bật RTCCLK, chờ đồng bộ `RTC_WaitForSynchro()`.
- Đặt prescaler 1 Hz: LSE → 32767; LSI → 39999 (xấp xỉ).
- Bật RTC_IT_ALR và dọn cờ cũ; xóa pending EXTI line 17 để tránh wake ngay.
- Cấu hình EXTI line 17 ở chế độ Event + cạnh lên.

3) Đặt Alarm sau `SLEEP_SECONDS` giây: `RTC_SetAlarm_AfterSeconds()`.

4) Trước khi ngủ:
- Tắt SysTick.
- Deinit USART1 và đưa các chân về Analog In để giảm dòng rò.
- Tùy chọn: đưa các GPIO không dùng (ví dụ toàn bộ PortB) về Analog In.
- Disable WakeUp pin (PA0) nếu không dùng, tránh wake ngoài ý muốn.
- Xóa cờ WU/SB và pending RTC Alarm/EXTI (phòng wake ngay).

5) Vào Standby: `PWR_EnterSTANDBYMode()`.
- Ở Standby, VCORE tắt, chỉ còn lại backup domain/RTC.
- Khi RTC Alarm xảy ra, EXTI17 tạo sự kiện wake và MCU reset.

6) Sau khi thức dậy: quay lại bước (1) và lặp.

---

## Giải thích các thành phần chính

### Standby là gì?

- Là chế độ tiêu thụ thấp nhất trên STM32F1: tắt VCORE, chỉ giữ backup domain (RTC, BKP).
- Thức dậy bằng RTC Alarm hoặc WakeUp pin (PA0). Khi thức dậy, MCU reset và chạy từ địa chỉ 0.
- Không giống Sleep/Stop: Sleep giữ CPU clock (tiêu thụ cao hơn), Stop dừng PLL/HSE nhưng giữ VCORE (thức dậy không reset), Standby thì reset hoàn toàn.

### Vì sao cần dọn cờ/pending trước khi ngủ?

- RTC Alarm sử dụng EXTI line 17. Nếu cờ ALR của RTC hoặc pending EXTI17 còn sót, lần vào Standby tiếp theo có thể bị wake ngay lập tức (trông như "không ngủ được").
- Cần xóa: `RTC_ClearFlag(RTC_FLAG_ALR)` và `EXTI_ClearITPendingBit(EXTI_Line17)` trước khi đặt Alarm mới và trước khi vào Standby.

### LSE vs LSI

- LSE (32.768 kHz) cho RTC chính xác. Cần thạch anh ngoài và tụ phù hợp.
- LSI (~40 kHz nội) tiện lợi nhưng sai số cao. Với 60 giây thì vẫn hoạt động, nhưng thời gian thực tế có thể lệch.

---

## Build và môi trường

- Dự án Keil MDK: mở `MDK/low.uvprojx` (trong cùng thư mục `Bai10_2/MDK`).
- VS Code có thể báo lỗi `cannot open source file ...` do includePath chưa trỏ tới RTE/SPL của Keil. Có thể bỏ qua khi build bằng Keil.
- Nếu muốn cấu hình VS Code cho sạch squiggles, thêm các đường dẫn include:
	- `Bai10_2/MDK/RTE/Device/STM32F103C8`
	- `Device/StdPeriph_Driver/inc` (tùy cấu trúc RTE)

---

## Tham khảo nhanh API (SPL)

- PWR: `PWR_ClearFlag(PWR_FLAG_WU | PWR_FLAG_SB)`, `PWR_WakeUpPinCmd(DISABLE)`, `PWR_EnterSTANDBYMode()`
- RTC: `RCC_RTCCLKConfig(...)`, `RTC_WaitForSynchro()`, `RTC_SetPrescaler()`, `RTC_SetCounter()`, `RTC_SetAlarm()`, `RTC_ClearFlag(RTC_FLAG_ALR)`
- EXTI: `EXTI_ClearITPendingBit(EXTI_Line17)`, `EXTI_Init()` với `EXTI_Mode_Event`, `EXTI_Trigger_Rising`
- UART: `USART_Init()`, `USART_SendData()`, chờ `TXE`/`TC`
