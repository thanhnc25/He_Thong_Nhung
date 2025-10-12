#define USE_ADVANCED_VERSION 0

#if USE_ADVANCED_VERSION == 0

#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
void Task_LED_B12_3Hz(void *pvParameters);
void Task_LED_B13_10Hz(void *pvParameters);
void Task_LED_B14_25Hz(void *pvParameters);
void GPIO_Config(void);

int main(void)
{
    SystemInit();      
    GPIO_Config();   

    xTaskCreate(Task_LED_B12_3Hz,  "LED_B12_3Hz",  128, NULL, 1, NULL);
    xTaskCreate(Task_LED_B13_10Hz, "LED_B13_10Hz", 128, NULL, 1, NULL);
    xTaskCreate(Task_LED_B14_25Hz, "LED_B14_25Hz", 128, NULL, 1, NULL);
    vTaskStartScheduler();
    while(1);
}


void GPIO_Config(void)
{
    // Bật clock cho GPIOB
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    
    // Cấu hình pin B12, B13, B14 làm đầu ra push-pull
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}
// Task nháy LED B12 với tần số 3Hz (chu kỳ = 333ms, nửa chu kỳ = 167ms)
void Task_LED_B12_3Hz(void *pvParameters)
{
    while(1)
    {
        GPIOB->ODR ^= GPIO_Pin_12;  
        vTaskDelay(pdMS_TO_TICKS(167)); // 1000ms / (2 * 3Hz) = 167ms
    }
}

// Task nháy LED B13 với tần số 10Hz (chu kỳ = 100ms, nửa chu kỳ = 50ms) 
void Task_LED_B13_10Hz(void *pvParameters)
{
    while(1)
    {
        GPIOB->ODR ^= GPIO_Pin_13;    
        vTaskDelay(pdMS_TO_TICKS(50)); // 1000ms / (2 * 10Hz) = 50ms
    }
}

// Task nháy LED B14 với tần số 25Hz (chu kỳ = 40ms, nửa chu kỳ = 20ms)
void Task_LED_B14_25Hz(void *pvParameters)
{
    while(1)
    {
        GPIOB->ODR ^= GPIO_Pin_14;    
        vTaskDelay(pdMS_TO_TICKS(20)); // 1000ms / (2 * 25Hz) = 20ms
    }
}

#else
#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"

typedef struct {
    GPIO_TypeDef* GPIOx;
    uint16_t GPIO_Pin;
    uint16_t frequency_hz;
} LED_Config_t;

void Task_LED_Generic(void *pvParameters);
void GPIO_Config_Advanced(void);

int main()
{
    SystemInit();      
    GPIO_Config_Advanced();   

    static LED_Config_t led1 = {GPIOB, GPIO_Pin_12, 3};
    static LED_Config_t led2 = {GPIOB, GPIO_Pin_13, 10};
    static LED_Config_t led3 = {GPIOB, GPIO_Pin_14, 25};

    xTaskCreate(Task_LED_Generic,  "LED_B12_3Hz",  128, &led1, 1, NULL);
    xTaskCreate(Task_LED_Generic, "LED_B13_10Hz", 128, &led2, 1, NULL);
    xTaskCreate(Task_LED_Generic, "LED_B14_25Hz", 128, &led3, 1, NULL);
    vTaskStartScheduler();
    while(1);
}
void GPIO_Config_Advanced(void)
{
    // Bật clock cho GPIOB
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    
    // Cấu hình pin B12, B13, B14 làm đầu ra push-pull
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}
void Task_LED_Generic(void *pvParameters)
{
    LED_Config_t* ledConfig = (LED_Config_t*)pvParameters;
    uint16_t half_period_ms = 1000 / (2 * ledConfig->frequency_hz);

    while(1)
    {
        ledConfig->GPIOx->ODR ^= ledConfig->GPIO_Pin;  
        vTaskDelay(pdMS_TO_TICKS(half_period_ms));
    }
}

#endif