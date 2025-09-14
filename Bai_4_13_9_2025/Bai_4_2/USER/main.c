#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_tim.h"

volatile uint8_t SttBlink = 0;   // Trang thai LED

void Timer2_init(void) //Tao timer 500ms 
{
    TIM_TimeBaseInitTypeDef timer;
//    NVIC_InitTypeDef nvic;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    timer.TIM_CounterMode = TIM_CounterMode_Up;
    timer.TIM_Period = 4999;               
    timer.TIM_Prescaler = 7200 - 1;        
    timer.TIM_ClockDivision = 0;
    timer.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &timer);
    TIM_SetCounter(TIM2, 0);

    // Cau hình ngat
    TIM_ClearFlag(TIM2, TIM_FLAG_Update);
		TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
		NVIC_EnableIRQ(TIM2_IRQn);

    TIM_Cmd(TIM2, ENABLE);
}

void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        SttBlink = !SttBlink;   // Dao trang thái LED
        GPIO_WriteBit(GPIOC, GPIO_Pin_13, SttBlink);
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}

void LED_PC13_Init(void)
{
    GPIO_InitTypeDef GPIO;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    GPIO.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO.GPIO_Pin = GPIO_Pin_13;
    GPIO.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO);
}

int main(void)
{
    LED_PC13_Init();
    Timer2_init();

    while (1)
    {
			
    }
}
