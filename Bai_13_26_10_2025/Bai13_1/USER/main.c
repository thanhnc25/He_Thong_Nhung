#include "stm32f10x.h"                  // Device header
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_exti.h"
#include "FreeRTOS.h"                   // ARM.FreeRTOS::RTOS:Core
#include "task.h"
#include "semphr.h"

// Task handles
// static TaskHandle_t xTaskBlinkHandle = NULL;
static TaskHandle_t xTaskEventHandle = NULL;

// Binary semaphore to signal button press from ISR to Task2
static SemaphoreHandle_t xButtonSem = NULL;

// Forward declarations
void vTaskBlink(void *pvParameters);
void vTaskEvent(void *pvParameters);
static void GPIO_Init_Custom(void);
static void EXTI0_Config_Custom(void);

int main(void)
{
	// Init hardware
	SystemInit();
	// Set NVIC priority grouping compatible with FreeRTOS (all preemption bits)
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	GPIO_Init_Custom();
	EXTI0_Config_Custom();

	// Create binary semaphore (initially empty)
	xButtonSem = xSemaphoreCreateBinary();

	// Create tasks
	xTaskCreate(vTaskBlink, "BlinkPC13", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
	xTaskCreate(vTaskEvent, "EventPB12", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, &xTaskEventHandle);

	// Start scheduler
	vTaskStartScheduler();

	// Should never reach here
	for(;;);
}

// Task1: Blink PC13 every 1000 ms
void vTaskBlink(void *pvParameters)
{
	const TickType_t xDelay = pdMS_TO_TICKS(1000);
	for(;;)
	{
		// Toggle PC13 (use StdPeriph helpers for readability)
		if(GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_13) == Bit_SET)
		{
			GPIO_ResetBits(GPIOC, GPIO_Pin_13);
		}
		else
		{
			GPIO_SetBits(GPIOC, GPIO_Pin_13);
		}
		vTaskDelay(xDelay);
	}
}

// Task2: Wait blocked on semaphore; when given, set PB12 low for 1s then release PB12 high and go back to blocking
void vTaskEvent(void *pvParameters)
{
	const TickType_t xActiveTime = pdMS_TO_TICKS(1000);
	for(;;)
	{
		// Block until button press (semaphore taken)
		if(xSemaphoreTake(xButtonSem, portMAX_DELAY) == pdTRUE)
		{
			// Drive PB12 low (active low LED per requirement)
			GPIO_ResetBits(GPIOB, GPIO_Pin_12);
			vTaskDelay(xActiveTime);
			// Turn PB12 off (set high)
			GPIO_SetBits(GPIOB, GPIO_Pin_12);
		}
	}
}

// Initialize GPIOC PC13 as push-pull output (blink), PB12 as push-pull output (LED active low), PA0 as input pull-up
static void GPIO_Init_Custom(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	// Enable clocks: APB2 for GPIOA, GPIOB, GPIOC (no AFIO needed if using PA0 default mapping)
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);

	// Configure PC13 as General purpose output push-pull, max speed 2 MHz
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	// Configure PB12 as General purpose output push-pull, max speed 2 MHz
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	// Configure PA0 as input with pull-up
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // input pull-up
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// Ensure initial LED states: PC13 high (LED off on many boards), PB12 high (LED off since active low)
	GPIO_SetBits(GPIOC, GPIO_Pin_13);
	GPIO_SetBits(GPIOB, GPIO_Pin_12);
}

// Configure EXTI0 to trigger on falling edge (button active low) and enable NVIC
static void EXTI0_Config_Custom(void)
{
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	// Use default mapping: PA0 is already connected to EXTI0 after reset; no AFIO configuration required

	// Configure EXTI line 0: falling edge trigger
	EXTI_InitStructure.EXTI_Line = EXTI_Line0;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	// Clear any pending
	EXTI_ClearITPendingBit(EXTI_Line0);

	// Configure and enable EXTI0 IRQ in NVIC
	NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
	// IMPORTANT: Set a FreeRTOS-safe priority (numerically >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY)
	// Adjust as needed to match your FreeRTOSConfig.h; using 10 here is typically safe on F1 (0..15 valid)
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 10;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

// EXTI0 ISR
// Hàm xử lý ngắt EXTI0 (kích hoạt khi có tín hiệu ngắt từ chân PA0)
void EXTI0_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE; // Biến để kiểm tra xem có cần chuyển đổi ngữ cảnh hay không

    // Kiểm tra xem ngắt EXTI0 có đang chờ xử lý không
    if(EXTI_GetITStatus(EXTI_Line0) != RESET)
    {
        // Xóa cờ báo ngắt để tránh ngắt lặp lại
        EXTI_ClearITPendingBit(EXTI_Line0);

        // Kiểm tra semaphore có tồn tại không, nếu có thì "give" semaphore
        // Điều này sẽ đánh thức Task2 đang chờ semaphore này
        if(xButtonSem != NULL)
        {
            // Sử dụng API FreeRTOS an toàn trong ISR để "give" semaphore
            xSemaphoreGiveFromISR(xButtonSem, &xHigherPriorityTaskWoken);
        }
    }

    // Nếu xHigherPriorityTaskWoken được đặt thành true, điều này có nghĩa là
    // một task có độ ưu tiên cao hơn đã sẵn sàng chạy. Gọi portYIELD_FROM_ISR
    // để yêu cầu chuyển đổi ngữ cảnh ngay lập tức (nếu cần).
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
