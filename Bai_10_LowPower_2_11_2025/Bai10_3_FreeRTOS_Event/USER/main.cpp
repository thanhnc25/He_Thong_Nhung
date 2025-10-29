// Demo FreeRTOS Event Groups - 4 Tasks
#include "stm32f10x.h"                  // Device header
#include "stm32f10x_gpio.h"             // Keil::Device:StdPeriph Drivers:GPIO
#include "stm32f10x_rcc.h"              // Keil::Device:StdPeriph Drivers:RCC
#include "stm32f10x_usart.h"            // Keil::Device:StdPeriph Drivers:USART
#include "misc.h"
#include "FreeRTOS.h"                   // ARM.FreeRTOS::RTOS:Core
#include "event_groups.h"               // ARM.FreeRTOS::RTOS:Event Groups
#include "task.h"                       // ARM.FreeRTOS::RTOS:Core
// Thư viện lớp tiện ích GPIO & UART (C++)
#include "gpio.h"
#include "uart.h"

/*
  Bài: FreeRTOS + Event Group (STM32F103C8)
  - Task_Button: đọc nút nhấn PA0 (kéo lên trong), chống dội, phát sự kiện BTN -> Task_Main
  - Task_Main: trung tâm nhận sự kiện BTN, xoay vòng tần số LED (2Hz -> 10Hz -> 50Hz -> 2Hz ...),
			   sau đó phát sự kiện cho Task_Blink (đổi tần số) và Task_UART (gửi thông báo)
  - Task_Blink: nhấp nháy LED PC13 theo tần số hiện tại. Dùng timeout = nửa chu kỳ để vừa chớp vừa bắt được sự kiện đổi tần số
  - Task_UART: khi nhận lệnh từ Main thì in tần số hiện tại ra UART1 (PA9/PA10) ở 115200 8N1
  Lưu ý: Trên BluePill, LED PC13 active-low (mức 0 là sáng). Thời gian delay dùng nửa chu kỳ (bổ đôi) để tạo duty 50%.
*/

// Event bits
#define EV_BTN_PRESS       (1UL << 0)
#define EV_FREQ_2HZ        (1UL << 1)
#define EV_FREQ_10HZ       (1UL << 2)
#define EV_FREQ_50HZ       (1UL << 3)
#define EV_UART_SEND       (1UL << 4)
#define EV_FREQ_MASK       (EV_FREQ_2HZ | EV_FREQ_10HZ | EV_FREQ_50HZ)

// GPIO pins
#define BTN_GPIO           GPIOA
#define BTN_PIN            GPIO_Pin_0
#define LED_GPIO           GPIOC
#define LED_PIN            GPIO_Pin_13

typedef enum {
	BLINK_2HZ = 2,
	BLINK_10HZ = 10,
	BLINK_50HZ = 50
} blink_freq_t;

static volatile blink_freq_t g_currentFreq = BLINK_2HZ; // trạng thái tần số dùng chung
static EventGroupHandle_t g_evt = NULL;                 // Event group trung tâm

// Đối tượng phần cứng (C++) áp dụng từ thư viện đã tạo
// Khởi tạo có cấu hình ngay trong constructor theo yêu cầu
static GpioPin led_pin(LED_GPIO, LED_PIN, GPIO_Mode::OUT_PUSH_PULL);  // LED PC13 (active-low)
static GpioPin btn_pin(BTN_GPIO, BTN_PIN, GPIO_Mode::INPUT_PULL_UP);                      // Button PA0 (kéo lên, nhấn=0)
static Uart    uart1(USART1, 115200);                                                     // UART1 PA9/PA10 @115200

// Khai báo trước các hàm tiện ích và task
static void NVIC_Config(void);
// static void GPIO_Config(void);
// static void USART1_Init(uint32_t baud);
static void USART1_SendString(const char* s);
static inline void LED_Off(void);
static inline void LED_Toggle(void);
static TickType_t HalfPeriodTicksFromHz(uint32_t hz);

static void Task_Button(void* arg);
static void Task_Blink(void* arg);
static void Task_UART(void* arg);
static void Task_Main(void* arg);

int main()
{
	NVIC_Config();
	// GPIO_Config();
	// USART1_Init(115200);

	// Tạo event group
	g_evt = xEventGroupCreate();
	if (g_evt == NULL) {
		while (1) {}
	}

	// Gửi tần số khởi tạo và yêu cầu in ra UART khi vào hệ thống
	xEventGroupSetBits(g_evt, EV_FREQ_2HZ | EV_UART_SEND);

	// Tạo các task
	xTaskCreate(Task_Main,   "Main",   256, NULL, tskIDLE_PRIORITY + 3, NULL);
	xTaskCreate(Task_Button, "Button", 192, NULL, tskIDLE_PRIORITY + 2, NULL);
	xTaskCreate(Task_UART,   "UART",   192, NULL, tskIDLE_PRIORITY + 2, NULL);
	xTaskCreate(Task_Blink,  "Blink",  192, NULL, tskIDLE_PRIORITY + 1, NULL);

	vTaskStartScheduler();
	while (1) {}
}

/* ==================== Triển khai tiện ích phần cứng ==================== */
static void NVIC_Config(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
}

// static void GPIO_Config(void)
// {
// 	// Chân đã được cấu hình ngay tại constructor; ở đây chỉ thiết lập trạng thái mặc định nếu cần
// 	// LED tắt mặc định (active-low -> mức 1)
// 	led_pin.Set_High();
// }

// static void USART1_Init(uint32_t baud)
// {
// 	// Đã Begin tại constructor; có thể gọi lại nếu muốn đổi baud lúc chạy
// 	uart1.Begin(baud);
// }

static void USART1_SendChar(char c)
{
	uart1.Write_Char(c);
}

static void USART1_SendString(const char* s)
{
	if (s) uart1.Write(s);
}

static inline void LED_Off(void) { GPIO_SetBits(LED_GPIO, LED_PIN); }
static inline void LED_Toggle(void)
{
	// LED PC13 active-low: mức 1 là tắt
	if (led_pin.Read_Output()) led_pin.Set_Low();
	else led_pin.Set_High();
}

static TickType_t HalfPeriodTicksFromHz(uint32_t hz)
{
	if (hz == 0) hz = 1;
	uint32_t half_ms = 500UL / hz; // delay nửa chu kỳ
	if (half_ms == 0) half_ms = 1;
	return pdMS_TO_TICKS(half_ms);
}

/* ==================== Tasks ==================== */
static void Task_Button(void* arg)
{
	(void)arg;
	uint8_t prev_released = 1; // thả = 1 vì kéo lên

	for(;;) {
	// Đọc nút qua lớp GpioPin: PA0 kéo lên -> nhấn = 0
	uint8_t pressed = (btn_pin.Read_Input() == false);

		if (pressed && prev_released) {
			vTaskDelay(pdMS_TO_TICKS(20)); // debounce
			if (btn_pin.Read_Input() == false) {
				xEventGroupSetBits(g_evt, EV_BTN_PRESS);
				while (btn_pin.Read_Input() == false) {
					vTaskDelay(pdMS_TO_TICKS(10));
				}
				prev_released = 1;
				continue;
			}
		}

		prev_released = !pressed;
		vTaskDelay(pdMS_TO_TICKS(5));
	}
}

static void Task_Blink(void* arg)
{
	(void)arg;
	LED_Off();
	blink_freq_t localFreq = g_currentFreq;
	TickType_t halfTicks = HalfPeriodTicksFromHz((uint32_t)localFreq);

	for(;;) {
		EventBits_t bits = xEventGroupWaitBits(
			g_evt,
			EV_FREQ_MASK,
			pdTRUE,
			pdFALSE,
			halfTicks
		);

		if (bits & EV_FREQ_MASK) {
			if (bits & EV_FREQ_2HZ)       localFreq = BLINK_2HZ;
			else if (bits & EV_FREQ_10HZ) localFreq = BLINK_10HZ;
			else if (bits & EV_FREQ_50HZ) localFreq = BLINK_50HZ;
			g_currentFreq = localFreq;
			halfTicks = HalfPeriodTicksFromHz((uint32_t)localFreq);
			continue; // áp dụng halfTicks mới
		}

		// Hết timeout => toggle LED
		LED_Toggle();
	}
}

static void Task_UART(void* arg)
{
	(void)arg;
	for(;;) {
		xEventGroupWaitBits(g_evt, EV_UART_SEND, pdTRUE, pdFALSE, portMAX_DELAY);
		blink_freq_t f = g_currentFreq;
		const char* label = (f == BLINK_2HZ) ? "2 Hz" : (f == BLINK_10HZ) ? "10 Hz" : "50 Hz";
		USART1_SendString("[INFO] Tan so LED: ");
		USART1_SendString(label);
		USART1_SendString("\r\n");
	}
}

static void Task_Main(void* arg)
{
	(void)arg;
	for(;;) {
		xEventGroupWaitBits(g_evt, EV_BTN_PRESS, pdTRUE, pdFALSE, portMAX_DELAY);

		blink_freq_t next;
		switch (g_currentFreq) {
			case BLINK_2HZ:  next = BLINK_10HZ; break;
			case BLINK_10HZ: next = BLINK_50HZ; break;
			default:         next = BLINK_2HZ;  break;
		}

		EventBits_t setBits = 0;
		if (next == BLINK_2HZ)       setBits = EV_FREQ_2HZ;
		else if (next == BLINK_10HZ) setBits = EV_FREQ_10HZ;
		else                          setBits = EV_FREQ_50HZ;
		xEventGroupSetBits(g_evt, setBits);

		g_currentFreq = next; // cập nhật ngay để UART đọc
		xEventGroupSetBits(g_evt, EV_UART_SEND);
	}
}