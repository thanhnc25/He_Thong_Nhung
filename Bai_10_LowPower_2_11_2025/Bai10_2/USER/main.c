
/*
 * Demo: STM32F103C8 gửi 1 bản tin UART rồi ngủ Standby trong SLEEP_SECONDS giây,
 * chỉ giữ RTC hoạt động để đánh thức qua RTC Alarm (EXTI line 17). Trước khi ngủ
 * sẽ deinit ngoại vi để giảm tiêu thụ. Sau khi thức dậy do Alarm, chip sẽ RESET
 * và chạy lại từ đầu (đặc trưng của chế độ Standby trên STM32F1).
 *
 * Lưu đồ làm việc:
 * 1) Khởi động (power-on hoặc nhấn reset)
 * 2) Dọn cờ (SB/WU/EXTI17) -> Khởi tạo UART -> In log
 * 3) Cấu hình RTC 1 Hz (ưu tiên LSE, fallback LSI), bật Alarm sau SLEEP_SECONDS
 * 4) Deinit trước khi ngủ -> Vào Standby (chỉ RTC/backup domain hoạt động)
 * 5) Sau SLEEP_SECONDS, RTC Alarm kích hoạt EXTI17 -> MCU thoát Standby bằng reset
 * 6) Chạy lại từ đầu và lặp chu trình
 *
 * Lưu ý quan trọng:
 * - NRST (nút reset) KHÔNG reset backup domain (BKP/RTC/BDCR). Để tránh trạng thái
 *   "lỡ cỡ" sau một số lần reset, ta chủ động reset backup domain ở đầu RTC_Config_1Hz().
 * - Để tránh bị wake ngay sau khi vào Standby, phải xóa cả cờ ALR của RTC và pending
 *   EXTI line 17 TRƯỚC KHI đặt Alarm mới và TRƯỚC KHI vào Standby.
 * - WakeUp pin (PA0/WKUP) có thể gây wake ngoài ý muốn nếu ở mức cao -> disable khi không dùng.
 */

#include "stm32f10x.h"          // CMSIS + SPL chung cho STM32F10x
#include "stm32f10x_rcc.h"      // RCC (clock) SPL
#include "stm32f10x_gpio.h"     // GPIO SPL
#include "stm32f10x_exti.h"     // EXTI (RTC Alarm dùng EXTI line 17)
#include "stm32f10x_usart.h"    // USART SPL
#include "stm32f10x_pwr.h"      // PWR (Power control)
#include "stm32f10x_bkp.h"      // BKP (Backup domain)
#include "stm32f10x_rtc.h"      // RTC
//#include "misc.h"               // NVIC (nếu cần)

// Thời gian ngủ (giây). Đổi sang 10 để test nhanh.
#define SLEEP_SECONDS 10


// Hàm delay ms đơn giản bằng vòng lặp bận (không chính xác tuyệt đối nhưng đủ cho demo)
static void delay_ms(uint32_t ms)
{
	// Delay bận (busy-wait) dựa trên vòng lặp và NOP.
	// Giả sử HCLK ~ 72 MHz, hằng số dưới đây cho độ trễ gần đúng ~1ms.
	// Đây KHÔNG phải delay chính xác (phụ thuộc tối ưu hóa, tần số CPU),
	// chỉ dùng cho demo/đợi ngắn như xả UART buffer trước khi ngủ.
	// Nếu cần chính xác, hãy dùng SysTick hoặc Timer.
	for (uint32_t i = 0; i < ms; i++) {
		for (volatile uint32_t j = 0; j < 8000; j++) {
			__NOP();
		}
	}
}

// ===================== UART1 (PA9-TX, PA10-RX) =====================
static void UART1_Init(uint32_t baud)
{
	// 1) Bật clock cho GPIOA, AFIO (chức năng thay thế), và USART1
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO | RCC_APB2Periph_USART1, ENABLE);

	// 2) Cấu hình chân UART: PA9 (TX) ở chế độ Alternate Function Push-Pull,
	//    PA10 (RX) ở chế độ Input Floating.
	GPIO_InitTypeDef gpio;
	GPIO_StructInit(&gpio);

	gpio.GPIO_Pin = GPIO_Pin_9;               // TX
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &gpio);

	gpio.GPIO_Pin = GPIO_Pin_10;              // RX
	gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &gpio);

	// 3) Cấu hình thông số USART1 (8N1, no flow control), bật cả TX/RX
	USART_InitTypeDef us;
	USART_StructInit(&us);
	us.USART_BaudRate = baud;
	us.USART_WordLength = USART_WordLength_8b;
	us.USART_StopBits = USART_StopBits_1;
	us.USART_Parity = USART_Parity_No;
	us.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	us.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_Init(USART1, &us);

	// 4) Bật USART1
	USART_Cmd(USART1, ENABLE);
}

static void UART1_DeInit_PinsLowLeak(void)
{
	// Mục tiêu: đưa ngoại vi và chân GPIO về trạng thái tiêu thụ thấp trước khi ngủ.
	// 1) Tắt USART1 và trả peripheral về reset default
	USART_Cmd(USART1, DISABLE);
	USART_DeInit(USART1);

	GPIO_InitTypeDef gpio;
	GPIO_StructInit(&gpio);
	gpio.GPIO_Speed = GPIO_Speed_2MHz;  // tốc độ không quan trọng khi Analog In
	gpio.GPIO_Mode = GPIO_Mode_AIN;     // Analog input giúp giảm dòng rò ở input

	gpio.GPIO_Pin = GPIO_Pin_9; // TX
	GPIO_Init(GPIOA, &gpio);
	gpio.GPIO_Pin = GPIO_Pin_10; // RX
	GPIO_Init(GPIOA, &gpio);

	// 2) Tắt clock AFIO/USART1. GPIOA có thể tắt nếu chắc chắn không dùng chân khác.
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, DISABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, DISABLE);
	// Lưu ý: Có thể không tắt GPIOA nếu còn dùng chân khác; ở đây demo tắt để giảm tiêu thụ
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, DISABLE);
}

static void UART1_SendString(const char *s)
{
	// Gửi từng byte, chờ TXE trước khi nạp, và chờ TC ở cuối để đảm bảo byte cuối đã phát xong.
	while (*s) {
		while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET) {}
		USART_SendData(USART1, (uint16_t)*s++);
	}
	// Chờ truyền xong byte cuối cùng
	while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {}
}

// ===================== RTC + Standby =====================

// Xóa pending do RTC Alarm: cần xóa cả cờ ALR của RTC và pending EXTI line 17
static void RTC_AlarmPending_ClearAll(void)
{
	// Vì Standby có thể để lại pending của Alarm, cần xóa đủ cả hai nơi:
	// - Cờ ALR trong RTC
	// - Pending bit EXTI line 17 (đường Alarm)
	// Làm bước này trước khi đặt Alarm mới và trước khi vào Standby.
	// Xóa cờ ALR của RTC (nếu còn)
	RTC_ClearFlag(RTC_FLAG_ALR);
	RTC_WaitForLastTask();

	// Xóa pending bit của EXTI line 17 (RTC Alarm)
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	EXTI_ClearITPendingBit(EXTI_Line17);
	// Có thể tắt lại AFIO nếu muốn tiết kiệm
	// RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, DISABLE);
}

// Trả về 1 nếu đã cấu hình RTC thành công; 0 nếu thất bại (không có LSE/LSI)
static int RTC_Config_1Hz(void)
{
	// Bật clock PWR/BKP để truy cập backup domain
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
	PWR_BackupAccessCmd(ENABLE);

	// NRST không reset backup domain -> có thể để lại trạng thái "lỡ cỡ" của RTC/BDCR
	// (ví dụ RSF/ALR/đồng bộ). Để chắc chắn, reset backup domain mỗi lần khởi động
	// trong demo này. Lưu ý: thao tác này sẽ xóa nội dung backup registers và RTC time.
	RCC_BackupResetCmd(ENABLE);
	RCC_BackupResetCmd(DISABLE);

	// Nếu RTC chưa được cấu hình lần nào, cấu hình mới
	if (BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5) {
		// 1) Thử LSE trước (32.768 kHz) vì chính xác cho RTC
		RCC_LSEConfig(RCC_LSE_ON);

		uint32_t tout = 0x400000; // timeout tránh kẹt
		while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET && tout--) {}

		if (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == SET) {
			RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
		} else {
			// 2) Nếu LSE không sẵn có, fallback sang LSI (~40 kHz), kém chính xác hơn
			RCC_LSICmd(ENABLE);
			tout = 0x400000;
			while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET && tout--) {}
			if (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET) {
				return 0; // Không có nguồn RTC -> không thể đặt Alarm để wake
			}
			RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
		}

		// 3) Bật RTCCLK và chờ đồng bộ với APB (RTC_WaitForSynchro)
		RCC_RTCCLKCmd(ENABLE);
		RTC_WaitForSynchro();

		// 4) Đặt prescaler để có tick 1 Hz:
		//    - LSE 32768 Hz -> prescaler = 32767
		//    - LSI ~40000 Hz -> prescaler ~ 39999 (độ chính xác phụ thuộc chip)
		uint32_t prescaler = (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == SET) ? 32767 : 39999;
		RTC_WaitForLastTask();
		RTC_SetPrescaler(prescaler);
		RTC_WaitForLastTask();

		// 5) (Tùy chọn) đặt counter = 0 làm mốc
		RTC_SetCounter(0);
		RTC_WaitForLastTask();

		// 6) Đánh dấu đã init để các lần sau có thể bỏ qua cấu hình nặng
		BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);
	} else {
		// RTC đã có sẵn (sau reset/standby), chỉ cần đồng bộ lại
		RCC_RTCCLKCmd(ENABLE); // Đảm bảo RTCCLK được bật tới RTC
		RTC_WaitForSynchro();
	}

	// 7) Xóa các cờ cũ và bật RTC_IT_ALR: dù không cần NVIC để wake Standby,
	//    nhưng nên enable để đảm bảo sự kiện được tạo đúng.
	RTC_ClearFlag(RTC_FLAG_ALR | RTC_FLAG_OW | RTC_FLAG_SEC);
	RTC_WaitForLastTask();
	// Đồng thời xóa pending EXTI line 17 để tránh wake ngay lập tức ở lần sau
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	EXTI_ClearITPendingBit(EXTI_Line17);
	// (Tùy chọn) cấu hình EXTI line 17 ở chế độ Event + cạnh lên để hỗ trợ wake từ Standby
	EXTI_InitTypeDef ex;
	ex.EXTI_Line = EXTI_Line17;
	ex.EXTI_Mode = EXTI_Mode_Event;
	ex.EXTI_Trigger = EXTI_Trigger_Rising;
	ex.EXTI_LineCmd = ENABLE;
	EXTI_Init(&ex);
	RTC_ITConfig(RTC_IT_ALR, ENABLE);
	RTC_WaitForLastTask();

	return 1;
}

static void RTC_SetAlarm_AfterSeconds(uint32_t seconds)
{
	// Đặt RTC Alarm sau "seconds" giây kể từ thời điểm hiện tại (counter tính bằng giây)
	uint32_t now = RTC_GetCounter();
	RTC_WaitForLastTask();
	RTC_SetAlarm(now + seconds);
	RTC_WaitForLastTask();
}

static void Deinit_Before_Standby(void)
{
	// 1) Tắt SysTick để tránh đánh thức/tiêu thụ không cần thiết khi ngủ sâu
	SysTick->CTRL = 0;

	// 2) Deinit UART và đưa chân về trạng thái tiêu thụ thấp (Analog In)
	UART1_DeInit_PinsLowLeak();

	// 3) (Tùy chọn) đưa các GPIO không dùng về Analog In để giảm rò
	//    Ví dụ: toàn bộ PortB làm Analog In (cân nhắc phần cứng thực tế!)
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitTypeDef gpio;
	GPIO_StructInit(&gpio);
	gpio.GPIO_Mode = GPIO_Mode_AIN;
	gpio.GPIO_Speed = GPIO_Speed_2MHz;
	gpio.GPIO_Pin = GPIO_Pin_All;
	GPIO_Init(GPIOB, &gpio);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, DISABLE);

	// 4) Giữ clock cho PWR/BKP/RTC, các phần khác có thể tắt nếu muốn
}

static void Enter_Standby_With_RTC(void)
{
	// 1) Đảm bảo có quyền truy cập backup domain (ghi BKP/RTC)
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
	PWR_BackupAccessCmd(ENABLE);

	// 2) Vô hiệu WakeUp pin để tránh wake ngoài ý muốn bởi PA0 (WKUP)
	PWR_WakeUpPinCmd(DISABLE);

	// 3) Xóa cờ Wakeup (WU) và Standby (SB) trước khi vào Standby
	PWR_ClearFlag(PWR_FLAG_WU);
	PWR_ClearFlag(PWR_FLAG_SB);

	// 4) Xóa mọi pending của RTC Alarm/EXTI trước khi ngủ, tránh wake ngay
	RTC_AlarmPending_ClearAll();

	// 5) Gửi SEV và WFE để xả sự kiện treo (workaround một số trường hợp),
	//    sau đó vào Standby. Ở Standby, toàn bộ VCORE tắt, chỉ giữ Vbat/backup domain.
	__SEV();
	__WFE();

	// 6) Vào Standby: chỉ còn lại RTC/Backup domain hoạt động.
	//    Khi có sự kiện wake hợp lệ (RTC Alarm hoặc WKUP), MCU sẽ reset và chạy lại từ 0.
	PWR_EnterSTANDBYMode();

	// Không bao giờ chạy xuống đây, vì Standby sẽ reset khi thức dậy
}

int main(void)
{
	// Ghi chú tổng quan (VN):
	// - Mục tiêu: Gửi 1 bản tin qua UART, sau đó ngủ 1 phút ở chế độ Standby.
	// - Trong thời gian ngủ chỉ dùng RTC để hẹn giờ (Alarm) đánh thức.
	// - Trước khi ngủ, deinit ngoại vi để giảm tiêu thụ.

	// 0) Chuẩn bị: bật clock PWR và dọn cờ sớm nhất có thể
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
	if (PWR_GetFlagStatus(PWR_FLAG_SB) != RESET) {
		// Nếu vừa thức dậy từ Standby, SB=1 -> cần xóa đi
		PWR_ClearFlag(PWR_FLAG_SB);
	}
	// Luôn xóa cờ WakeUp và pending EXTI17 thật sớm sau reset để trạng thái sạch
	PWR_ClearFlag(PWR_FLAG_WU);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	EXTI_ClearITPendingBit(EXTI_Line17);

	// 1) Khởi tạo UART và gửi bản tin (đổi SLEEP_SECONDS để thay thời gian ngủ)
	UART1_Init(115200);
	UART1_SendString("Standby 10s, chi giu RTC...\r\n");

	// 2) Cấu hình RTC 1 Hz và đặt Alarm sau SLEEP_SECONDS giây
	if (!RTC_Config_1Hz()) {
		UART1_SendString("Khong khoi tao duoc RTC (LSE/LSI)!\r\n");
		// Không thể đảm bảo wake-up -> dừng tại đây (demo)
		while (1) {
			// Nháy nhẹ hoặc chờ
			delay_ms(500);
		}
	}
	// Bảo đảm xóa pending cũ (nếu vừa thức dậy từ Alarm trước đó)
	RTC_AlarmPending_ClearAll();
	RTC_SetAlarm_AfterSeconds(SLEEP_SECONDS);

	UART1_SendString("Da dat RTC Alarm sau 10s. Tien hanh deinit va vao Standby...\r\n");

	// 3) Deinit ngoại vi trước khi vào Standby để giảm tiêu thụ
	Deinit_Before_Standby();

	// Nhỏ: đợi một chút cho bản tin UART ra hết (nếu dùng bộ đệm/cáp TTL chậm)
	delay_ms(50);

	// 4) Vào Standby (chỉ RTC hoạt động). Sau khi Alarm, chip reset và chạy lại từ đầu
	Enter_Standby_With_RTC();

	// Không bao giờ tới đây
	while (1) {}
}
