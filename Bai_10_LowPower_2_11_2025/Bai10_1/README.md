# Bài 10.1 – Low Power STM32F103C8: Nháy LED PB12, deinit GPIO, vào Sleep bằng WFI/WFE

Tài liệu này giải thích chi tiết mã nguồn trong `Bai10_1/USER/main.c`: nháy LED ở chân PB12, sau đó giải phóng GPIO và đưa MCU vào chế độ Sleep sử dụng hai cơ chế: WFI (Wait For Interrupt) và WFE (Wait For Event).

## Mục tiêu

- Minh hoạ cấu hình GPIOB, chân PB12 làm output để nháy LED.
- Sau khi nháy, tối ưu tiêu thụ bằng cách deinit GPIO và tắt clock port.
- Trình diễn hai cách đưa CPU Cortex-M3 (STM32F103C8) vào Sleep: WFI và WFE.
- Thêm chú thích chuyên sâu về sự khác nhau giữa WFI/WFE, các bit trong SCB->SCR (SLEEPDEEP, SLEEPONEXIT, SEVONPEND), và chuỗi SEV;WFE;WFE.

## Luồng chương trình

1) Bật clock cho GPIOB: `RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);`

2) Cấu hình PB12 là Output Push-Pull, tốc độ 2 MHz.

3) Nháy LED PB12 5 lần bằng delay vòng lặp bận (chỉ để demo, không chính xác tuyệt đối).

4) Tắt LED, gọi `GPIO_DeInit(GPIOB)` và tắt clock của GPIOB để tiết kiệm điện.

5) Đảm bảo KHÔNG ở chế độ deep sleep: `SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;` (Sleep ≠ Stop/Standby).

6) Cấu hình nguồn đánh thức bằng nút nhấn PA0 (đã có kéo lên 3V3 ngoài; nhấn kéo xuống GND):
	- PA0 cấu hình input floating (vì có pull-up ngoài).
	- Map EXTI Line0 -> PA0, kích hoạt ngắt cạnh xuống (falling) và enable NVIC EXTI0.
	- Gọi hàm khởi tạo trước khi vào Sleep.

7) Tuỳ macro `USE_WFI` để chọn:
	 - `USE_WFI = 1`: gọi `__WFI();` (đợi interrupt)
	 - `USE_WFI = 0`: chuỗi `__SEV(); __WFE(); __WFE();` (đợi event)

8) Nếu có nguồn đánh thức phù hợp, sau khi tỉnh dậy CPU tiếp tục chạy vòng `while(1)`.

> Lưu ý quan trọng: Nếu không cấu hình bất kỳ nguồn ngắt (IRQ) hoặc event nào, MCU có thể ngủ vô thời hạn.

## Sleep vs Deep Sleep (Stop/Standby)

- Sleep: CPU dừng; ngoại vi vẫn có thể chạy tuỳ clock còn bật. Điện năng giảm nhưng không thấp nhất.
- Deep Sleep: đặt bit SLEEPDEEP=1; vào các chế độ như Stop/Standby (tuỳ cấu hình). Tiêu thụ thấp hơn nhưng yêu cầu khởi tạo lại clock/phần cứng khi thức dậy. Ví dụ này chỉ dùng Sleep (SLEEPDEEP=0).

## WFI (Wait For Interrupt)

- Cơ chế: CPU dừng cho đến khi có một NGẮT được nhận. Thực tế thường yêu cầu IRQ đã enable trong NVIC để đảm bảo thoát ngủ chắc chắn.
- Hành vi khi ngắt bị mask (ví dụ `PRIMASK=1`): pending interrupt có thể không làm thoát WFI tuỳ bối cảnh. Thực hành tốt là bật ít nhất một IRQ đánh thức trước khi gọi WFI.
- Điển hình dùng với: EXTI (nút nhấn), RTC (định kỳ), USART (RX),…
- Ưu điểm: đơn giản, phổ biến; CPU thức dậy trực tiếp để phục vụ ISR.

## WFE (Wait For Event)

- Cơ chế: CPU dừng đến khi có SỰ KIỆN (event). Các nguồn event:
	- Lệnh phần mềm `__SEV()` (Set Event) phát sự kiện ngay lập tức.
	- Một interrupt chuyển sang trạng thái pending nếu bật `SEVONPEND` (bit trong `SCB->SCR`).
	- Một số event kiến trúc khác (tuỳ dòng Cortex-M).
- Vì thanh ghi “event register” có thể đang ở trạng thái đã có event từ trước, ARM khuyến cáo trình tự:
	- `__SEV(); __WFE(); __WFE();`
	- Trong đó: `__SEV()` đặt cờ event; lần `__WFE()` đầu XOÁ cờ đó (quay ra ngay); lần `__WFE()` thứ hai mới thực sự NGỦ và chờ event mới.
- Ưu điểm: hữu ích khi bạn muốn ngủ ngay cả khi tạm thời mask ngắt; kết hợp `SEVONPEND` để pending IRQ cũng đánh thức WFE.

### WFE và EXTI (PA0)

- Để WFE chắc chắn được đánh thức bởi IRQ pending (ví dụ EXTI0 khi nhấn nút), mã ví dụ bật bit `SEVONPEND` trước khi vào WFE:

	```c
	SCB->SCR |= SCB_SCR_SEVONPEND_Msk;
	__SEV(); __WFE(); __WFE();
	```

- Với WFI thì không cần `SEVONPEND` vì ngắt sẽ trực tiếp đánh thức CPU để vào ISR.

## Các bit trong SCB->SCR hữu ích

- `SLEEPDEEP` (bit):
	- `0`: vào Sleep bình thường (ví dụ này dùng).
	- `1`: vào Deep-sleep (Stop/Standby tuỳ cấu hình clock).
- `SLEEPONEXIT` (bit):
	- Nếu `1`: Sau khi thoát khỏi ISR, CPU tự động quay về Sleep mà không trở về main loop. Hữu ích cho ứng dụng “tickless” siêu tiết kiệm.
- `SEVONPEND` (bit):
	- Nếu `1`: Bất kỳ IRQ chuyển sang pending sẽ phát sinh EVENT. Điều này cho phép `__WFE()` thức dậy bởi pending IRQ, kể cả khi ngắt đang bị mask tạm thời.

Trong mã ví dụ, hai bit `SLEEPONEXIT` và `SEVONPEND` được để ở dạng dòng comment minh hoạ; bạn có thể bật nếu phù hợp ứng dụng.

## Ghi chú về GPIO và tiết kiệm điện

- `GPIO_DeInit(GPIOB)` đưa các chân về trạng thái reset (Input floating). Trạng thái này có thể tăng chút rò rỉ nếu chân hở.
- Trong hệ thực, để tối ưu hơn nữa:
	- Cấu hình chân ở “Analog input” hoặc kéo lên/kéo xuống (pull-up/down) phù hợp trước khi tắt clock.
	- Đảm bảo không để các chân ở trạng thái dao động hoặc mấp mé mức.

### Riêng với PA0 (nút nhấn)

- Vì đã có điện trở kéo lên 3V3 bên ngoài, cấu hình `Input floating` là phù hợp, tránh double pull.
- Ngắt cạnh xuống (falling) trùng với hành vi “nhấn = kéo về GND”.

## Tuỳ chọn trong mã

- Chọn WFI hay WFE: sửa đầu file `main.c`:

	```c
	#define USE_WFI 1   // =1 dùng WFI (mặc định); =0 dùng WFE
	```

- Tuỳ chọn nâng cao (đang comment sẵn trong mã):

	```c
	// SCB->SCR |= SCB_SCR_SLEEPONEXIT_Msk;   // Tự động ngủ lại sau ISR
	// SCB->SCR |= SCB_SCR_SEVONPEND_Msk;     // Cho phép WFE dậy bởi IRQ pending
	```

## Đánh thức MCU – Gợi ý cấu hình

- Nếu muốn quan sát MCU THỨC DẬY thực tế, bạn cần một nguồn đánh thức:
	- EXTI (nút nhấn) cấu hình “rising/falling edge”.
	- RTC (chu kỳ 1s hoặc định kỳ khác).
	- USART RX (khi nhận dữ liệu).
- Với WFI: đảm bảo IRQ tương ứng đã enable trong NVIC.
- Với WFE: có thể dùng `SEVONPEND=1` để pending IRQ tạo event; hoặc tự `__SEV()` từ một ngữ cảnh khác.

## Kết luận

Ví dụ minh hoạ đầy đủ: nháy LED, giải phóng GPIO để tiết kiệm, và đưa CPU vào Sleep với cả hai lựa chọn WFI/WFE. Các chú thích đã nêu rõ cơ chế, bit cấu hình quan trọng, và khuyến cáo thực tế khi triển khai low-power trên STM32F1.

