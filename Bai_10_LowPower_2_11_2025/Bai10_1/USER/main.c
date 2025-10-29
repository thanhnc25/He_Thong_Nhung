// Ví dụ: Nháy LED chân PB12, sau đó giải phóng (deinit) GPIO và vào chế độ ngủ (Sleep)
// Minh hoạ hai cách vào Sleep: WFI (Wait For Interrupt) và WFE (Wait For Event)
//
// Tóm tắt nhanh về Sleep, WFI, WFE trên Cortex-M3 (STM32F103):
// - Sleep: CPU dừng, ngoại vi (tuỳ clock) vẫn có thể chạy. Đây KHÔNG phải deep sleep (Stop/Standby).
// - __WFI(): CPU đợi ĐẾN KHI có NGẮT (interrupt) được nhận; thường dùng khi đã enable một IRQ đánh thức.
// - __WFE(): CPU đợi ĐẾN KHI có SỰ KIỆN (event); có thể là event từ __SEV(), hoặc sự kiện do IRQ pending tạo ra
//            (nếu set bit SEVONPEND). Cần chuỗi SEV;WFE;WFE để loại sự kiện tồn đọng.
// - Các bit hữu ích trong thanh ghi SCB->SCR:
//     + SLEEPDEEP: =0 để vào Sleep; =1 để vào Deep-sleep (Stop/Standby tuỳ cấu hình clock). Ở ví dụ này đặt =0.
//     + SLEEPONEXIT: nếu =1, sau khi thoát khỏi ISR, CPU tự quay về chế độ Sleep (thuận tiện cho ứng dụng siêu tiết kiệm).
//     + SEVONPEND: nếu =1, bất kỳ IRQ nào chuyển sang trạng thái pending cũng phát sinh EVENT (hỗ trợ WFE đánh thức).
// - Cảnh báo: Nếu không cấu hình NGUỒN ĐÁNH THỨC (ngắt/event), MCU có thể ngủ vô thời hạn.

#include "stm32f10x.h"          // CMSIS + SPL chung cho STM32F10x
#include "stm32f10x_rcc.h"      // RCC (clock) SPL
#include "stm32f10x_gpio.h"     // GPIO SPL
#include "stm32f10x_exti.h"     // EXTI (ngoại vi ngắt ngoài)

// Định nghĩa chọn phương thức vào Sleep
// Đặt USE_WFI = 1 để dùng __WFI();
// Đặt USE_WFI = 0 để dùng __WFE();
#ifndef USE_WFI
#define USE_WFI 1   // =1 dùng WFI (mặc định); =0 dùng WFE
#endif

static void Wakeup_PA0_EXTI_Falling_Init(void);git 

// Hàm delay ms đơn giản bằng vòng lặp bận (không chính xác tuyệt đối nhưng đủ cho demo)
static void delay_ms(uint32_t ms)
{
	// Giả sử HCLK ~ 72 MHz, hằng số dưới đây cho độ trễ gần đúng ~1ms
	// Điều chỉnh nếu thấy nháy quá nhanh/chậm
	for (uint32_t i = 0; i < ms; i++) {
		for (volatile uint32_t j = 0; j < 8000; j++) {
			__NOP();
		}
	}
}

int main(void)
{
	// 1) Bật clock cho GPIOB
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	// 2) Cấu hình PB12 là Output Push-Pull
	GPIO_InitTypeDef gpio;
	GPIO_StructInit(&gpio);
	gpio.GPIO_Pin   = GPIO_Pin_12;            // Chân PB12
	gpio.GPIO_Speed = GPIO_Speed_2MHz;        // Tốc độ 2MHz là đủ cho LED
	gpio.GPIO_Mode  = GPIO_Mode_Out_PP;       // Push-Pull output
	GPIO_Init(GPIOB, &gpio);

	// 3) Nháy LED vài lần để quan sát
	for (int k = 0; k < 5; k++) {
		// Tuỳ vào cách đấu LED: nếu LED nối về GND qua điện trở, đặt chân ở mức cao để sáng
		GPIO_SetBits(GPIOB, GPIO_Pin_12);  // LED ON
		delay_ms(300);
		GPIO_ResetBits(GPIOB, GPIO_Pin_12); // LED OFF
		delay_ms(300);
	}

	// 4) Tắt LED, giải phóng GPIO để giảm tiêu thụ
	GPIO_ResetBits(GPIOB, GPIO_Pin_12);
	GPIO_DeInit(GPIOB);

	// Có thể tắt luôn clock cho GPIOB để tiết kiệm hơn
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, DISABLE);

	// 5) Chuẩn bị vào chế độ ngủ (Sleep)
	// Lưu ý:
	// - Sleep khác với Stop/Standby (deep sleep). Ở đây KHÔNG bật SLEEPDEEP.
	// - Việc thoát khỏi Sleep cần có ngắt (với WFI) hoặc event (với WFE).
	//   Nếu dự án không cấu hình bất kỳ nguồn ngắt/event nào, MCU có thể ngủ vô thời hạn.
	// - Sau khi GPIO_DeInit(), các chân quay về trạng thái reset (input floating). Để tiết kiệm hơn nữa
	//   trong hệ thực, có thể cấu hình chân ở Analog input hoặc kéo lên/kéo xuống trước khi tắt clock.

	// Đảm bảo không ở chế độ Deep Sleep
	SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;  // đảm bảo chỉ là Sleep
 
	// Tuỳ chọn nâng cao (đang tắt bằng comment):
	// SCB->SCR |= SCB_SCR_SLEEPONEXIT_Msk;   // Khi thoát ISR sẽ vào lại sleep ngay (hữu ích cho ứng dụng tickless)
	// SCB->SCR |= SCB_SCR_SEVONPEND_Msk;     // Cho phép WFE thức dậy khi có IRQ pending (kể cả khi bị mask tạm thời)

	// Tuỳ chọn: tắt SysTick nếu đã bật trước đó để giảm tiêu thụ (không bắt buộc)
	// SysTick->CTRL = 0;

	// 5b) Cấu hình nguồn đánh thức: PA0 (nút nhấn kéo xuống GND) qua EXTI0 cạnh xuống
	// Wakeup_PA0_EXTI_Falling_Init();

#if USE_WFI
	// 6A) Vào Sleep bằng WFI (Wait For Interrupt)
	// - Cơ chế: CPU dừng cho đến khi có một NGẮT được nhận (thường là IRQ đã enable trong NVIC).
	// - Khi IRQ xảy ra, CPU thức dậy và phục vụ ISR (nếu đang enable). Nếu IRQ đang bị mask (PRIMASK=1),
	//   hành vi thức dậy phụ thuộc cấu hình; thông thường nên đảm bảo ngắt đánh thức đã enable để WFI chắc chắn thoát.
	// - Ưu điểm: đơn giản, phổ biến khi có nguồn ngắt định kỳ (RTC, SysTick) hoặc sự kiện bên ngoài (EXTI nút nhấn).
	__WFI();
#else
	// 6B) Vào Sleep bằng WFE (Wait For Event)
	// - Cơ chế: CPU dừng cho đến khi có SỰ KIỆN. Event có thể đến từ:
	//   + Lệnh __SEV() (phần mềm tự phát sự kiện),
	//   + Một interrupt chuyển sang trạng thái pending (nếu set SCB->SCR.SEVONPEND=1),
	//   + Các event kiến trúc khác (tuỳ dòng Cortex-M).
	// - Vì event register có thể đang ở trạng thái "đã có event" từ trước, theo khuyến cáo ARM cần chuỗi:
	//     __SEV(); __WFE(); __WFE();
	//   Trong đó, __SEV() đặt cờ event; lần __WFE() đầu sẽ XOÁ cờ (và không ngủ lâu), lần __WFE() thứ hai mới thật sự chờ event mới.
	// - Ưu điểm: có thể dùng trong bối cảnh đã tạm thời mask ngắt; kết hợp SEVONPEND giúp pending IRQ cũng đánh thức được WFE.
	// Đặt SEVONPEND để pending IRQ (ví dụ EXTI0) cũng tạo EVENT đánh thức WFE
	SCB->SCR |= SCB_SCR_SEVONPEND_Msk;

	__SEV();    // Phát 1 sự kiện để "xả" cờ event tồn đọng (nếu có)
	__WFE();    // Bỏ qua event vừa phát, xoá cờ event
	__WFE();    // Bây giờ mới chờ event thực sự (từ IRQ pending hoặc __SEV() khác)
#endif

	// Nếu có nguồn ngắt/event đánh thức, chương trình tiếp tục từ đây sau khi tỉnh dậy
	while (1) {
		// Vòng lặp trống sau khi thức dậy. Tuỳ ứng dụng, có thể:
		// - Tái cấu hình GPIO/clock, làm việc ngắn, rồi lại gọi WFI/WFE để tiết kiệm.
		// - Kết hợp SLEEPONEXIT để tự động quay về sleep sau ISR.
		__NOP();
	}
}

// Cấu hình đánh thức bằng nút nhấn trên PA0 (kéo lên ngoài 3V3, nhấn kéo xuống GND)
// - PA0 input floating (do đã có pull-up ngoài)
// - EXTI Line0 -> PA0, kích hoạt ngắt cạnh xuống (falling)
// - NVIC enable EXTI0_IRQn để WFI có thể thoát ngủ; với WFE cần kết hợp SEVONPEND
static void Wakeup_PA0_EXTI_Falling_Init(void)
{
	// Bật clock cho GPIOA và AFIO (để map EXTI)
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);

	// PA0 là input (floating vì có pull-up ngoài)
	GPIO_InitTypeDef gpio;
	GPIO_StructInit(&gpio);
	gpio.GPIO_Pin  = GPIO_Pin_0;
	gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &gpio);

	// Map EXTI Line0 tới Port A, Pin 0
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);

	// Cấu hình EXTI Line0: interrupt, cạnh xuống, enable
	EXTI_InitTypeDef exti;
	EXTI_StructInit(&exti);
	exti.EXTI_Line = EXTI_Line0;
	exti.EXTI_Mode = EXTI_Mode_Interrupt;
	exti.EXTI_Trigger = EXTI_Trigger_Falling;  // nhấn -> kéo xuống GND
	exti.EXTI_LineCmd = ENABLE;
	EXTI_Init(&exti);

	// Xoá pending trước khi vào sleep để tránh đánh thức giả
	EXTI_ClearITPendingBit(EXTI_Line0);

	// Bật NVIC cho EXTI0
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	NVIC_InitTypeDef nvic;
	nvic.NVIC_IRQChannel = EXTI0_IRQn;
	nvic.NVIC_IRQChannelPreemptionPriority = 2;
	nvic.NVIC_IRQChannelSubPriority = 0;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic);
}

// Trình phục vụ ngắt EXTI line 0 (PA0)
void EXTI0_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line0) != RESET) {
		// Xoá cờ pending để tránh lặp ngắt
		EXTI_ClearITPendingBit(EXTI_Line0);
		// Tuỳ chọn: có thể đặt một cờ toàn cục để main biết đã thức dậy
	}
}