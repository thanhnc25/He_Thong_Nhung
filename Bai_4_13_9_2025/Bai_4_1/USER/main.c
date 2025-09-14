#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_tim.h"

volatile char SttBlink = 0;
int Stt_Blink = 0;

void Timer2_init() // Tao timer 1ms
{
	TIM_TimeBaseInitTypeDef timer;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	timer.TIM_CounterMode = TIM_CounterMode_Up;
	timer.TIM_Period = 65535; // Cau hinh timer 1ms
	timer.TIM_Prescaler = 2 - 1;
	timer.TIM_ClockDivision = 0;
	timer.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM2, &timer);
	TIM_SetCounter(TIM2, 0);

	// Cau hinh ngat
	TIM_ClearFlag(TIM2, TIM_FLAG_Update);
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	NVIC_EnableIRQ(TIM2_IRQn);

	TIM_Cmd(TIM2, ENABLE);
}

void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
	{
		SttBlink = !SttBlink;
		Stt_Blink = ~Stt_Blink;
	}
	TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
}

void delay_1ms(void)
{
	Timer2_init();
	while (TIM_GetCounter(TIM2) < 36000)
		; // lap den khi gia tri bo dem = 36000 => thoat khoi vong while v� ngat timer
	TIM_Cmd(TIM2, DISABLE);
}

void Delay(int time)
{
	while (time--)
	{
		delay_1ms();
	}
}

void LED_PC_13_Init()
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
	LED_PC_13_Init();
	// Timer2_init();
	while (1)
	{
		GPIO_ResetBits(GPIOC, GPIO_Pin_13);
		Delay(1000);
		GPIO_SetBits(GPIOC, GPIO_Pin_13);
		Delay(1000);
	}
}
