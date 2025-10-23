#include "stm32f10x.h"                  // Device header
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_usart.h"
#include "FreeRTOS.h"                   // ARM.FreeRTOS::RTOS:Core
#include "task.h"
#include "semphr.h"

// Mutex dùng để bảo vệ UART khỏi truy cập đồng thời
static SemaphoreHandle_t g_uartMutex = NULL;

// ------------------ UART1 (PA9 TX, PA10 RX) ------------------
static void USART1_Init(uint32_t baudrate)
{
	// Enable clocks for GPIOA and USART1
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

	// Configure PA9 as Alternate Function Push-Pull (TX)
	GPIO_InitTypeDef gpio;
	GPIO_StructInit(&gpio);
	gpio.GPIO_Pin = GPIO_Pin_9;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &gpio);

	// Configure PA10 as Input Floating (RX)
	gpio.GPIO_Pin = GPIO_Pin_10;
	gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &gpio);

	// USART1 configuration
	USART_InitTypeDef usart;
	USART_StructInit(&usart);
	usart.USART_BaudRate = baudrate;
	usart.USART_WordLength = USART_WordLength_8b;
	usart.USART_StopBits = USART_StopBits_1;
	usart.USART_Parity = USART_Parity_No;
	usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	usart.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_Init(USART1, &usart);

	USART_Cmd(USART1, ENABLE);
}

static inline void uart1_send_char(char c)
{
	// Wait until the transmit data register is empty
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET) {
	}
	USART_SendData(USART1, (uint16_t)c);
}

static void uart1_send_crlf(void)
{
	uart1_send_char('\r');
	uart1_send_char('\n');
}

static void uart1_send_string(const char *s)
{
	if (g_uartMutex != NULL)
	{
		// Chờ đến khi lấy được mutex (ưu tiên kế thừa giúp tránh đảo ưu tiên)
		xSemaphoreTake(g_uartMutex, portMAX_DELAY);
	}

	for (const char *p = s; *p; ++p)
	{
		uart1_send_char(*p);
	}
	uart1_send_crlf();

	if (g_uartMutex != NULL)
	{
		xSemaphoreGive(g_uartMutex);
	}
}

// ------------------ FreeRTOS Tasks ------------------
static void TaskA(void *pvParameters)
{
	const char *s1 = "1234567890"; // 10 chars
	(void)pvParameters;
	for (;;)
	{
		// Nhờ mutex, cả chuỗi sẽ được in liền mạch, không trộn với task khác
		uart1_send_string(s1);
		vTaskDelay(3000);
	}
}

static void TaskB(void *pvParameters)
{
	const char *s2 = "abcdefghij"; // 10 chars
	(void)pvParameters;
	for (;;)
	{
		// Nhờ mutex, cả chuỗi sẽ được in liền mạch, không trộn với task khác
		uart1_send_string(s2);
		vTaskDelay(3000);
	}
}

int main(void)
{
	// SystemInit() is called by startup code before main on STM32F1.

	// Initialize UART1 at 115200 8N1
	USART1_Init(115200);

	// Tạo mutex để bảo vệ UART trước khi tạo task
	g_uartMutex = xSemaphoreCreateMutex();
	if (g_uartMutex == NULL)
	{
		// Không đủ heap để tạo mutex: dừng tại đây để tránh in lộn xộn
		while (1) { }
	}

	// Create two tasks at the same priority to demonstrate interleaving without mutex
	xTaskCreate(TaskA, "TaskA", 128, NULL, tskIDLE_PRIORITY + 1, NULL);
	xTaskCreate(TaskB, "TaskB", 128, NULL, tskIDLE_PRIORITY + 1, NULL);

	// Start the scheduler (does not return if successful)
	vTaskStartScheduler();

	// If we get here, there was not enough heap for the idle/Timer tasks
	while (1)
	{
	}
}