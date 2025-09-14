#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_exti.h"

volatile uint8_t state_led = 0;       // LED PC13 toggle theo nút
volatile uint32_t msTicks = 0;        // Bo dem SysTick
volatile uint32_t last_press = 0;     // Chong doi nút

void LED_Init(void);
void Button_Init(void);
void EXTI_init(void);
void SysTick_Handler(void);

int main(void)
{
    LED_Init();
    Button_Init();
    EXTI_init();
	
    // Cau hinh SysTick 1ms (SystemCoreClock = 72MHz)
    SysTick_Config(SystemCoreClock / 1000);
		
		GPIO_SetBits(GPIOC, GPIO_Pin_13);
    while (1)
    {
        // LED PB7 nhay 1Hz (500ms on, 500ms off)
        if (msTicks % 1000 < 500)
            GPIO_SetBits(GPIOB, GPIO_Pin_7);
        else
            GPIO_ResetBits(GPIOB, GPIO_Pin_7);
    }
}

void LED_Init(void)
{
    GPIO_InitTypeDef GPIO;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    // PC13 output
    GPIO.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO.GPIO_Pin = GPIO_Pin_13;
    GPIO.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO);

    // PB7 output
    GPIO.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO.GPIO_Pin = GPIO_Pin_7;
    GPIO.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO);
}

void Button_Init(void)
{
    GPIO_InitTypeDef BUTTON;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    BUTTON.GPIO_Pin = GPIO_Pin_12;
    BUTTON.GPIO_Mode = GPIO_Mode_IPD;   // input pull-down
    BUTTON.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &BUTTON);
}

void EXTI_init()
{
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    // Gán PA12 vào EXTI Line12
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource12);

    EXTI_ClearITPendingBit(EXTI_Line12);

    // Config EXTI
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Line = EXTI_Line12;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  // nhan -> suon len
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    // Config NVIC cho Line10~15
    NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

// SysTick 1ms
void SysTick_Handler(void)
{
    msTicks++;
}

// Ngat ngoai nut nhan
void EXTI15_10_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line12) != RESET)
    {
        EXTI_ClearITPendingBit(EXTI_Line12);

        // Chong doi nút: delay 200ms
        if (msTicks - last_press > 200)
        {
            state_led = !state_led;
            GPIO_WriteBit(GPIOC, GPIO_Pin_13,
                          state_led ? Bit_SET : Bit_RESET);

            last_press = msTicks;
        }
    }
}
