#include "stm32f10x.h"
#include "stm32f10x_GPIO.h"
#include "stm32f10x_rcc.h"

void delay_1ms(unsigned int time);
void LED_Init();

int main()
{
	LED_Init();
	
	while(1){
		GPIO_SetBits(GPIOC, GPIO_Pin_13);
		GPIO_SetBits(GPIOB, GPIO_Pin_7);
		delay_1ms(500);
		GPIO_ResetBits(GPIOC, GPIO_Pin_13);
		GPIO_ResetBits(GPIOB, GPIO_Pin_7);
		delay_1ms(500);
	}
}

void LED_Init()
{
	GPIO_InitTypeDef GPIO;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO.GPIO_Pin = GPIO_Pin_13;
	GPIO.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO);
	
	GPIO.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO.GPIO_Pin = GPIO_Pin_7;
	GPIO.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO);
}
	
void delay_1ms(unsigned int time)
{
	unsigned int i, j;
	for (i = 0; i < time; i++)
	{
		for (j = 0; j < 0x2aff; j++)
		{
		}
	}
}
