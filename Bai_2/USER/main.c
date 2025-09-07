#include "stm32f10x.h"
#include "stm32f10x_GPIO.h"
#include "stm32f10x_rcc.h"

void delay_1ms(unsigned int time);
void LED_PC_13_Init();

int main()
{
	LED_PC_13_Init();
	
	while(1){
		GPIO_SetBits(GPIOC, GPIO_Pin_13);
		delay_1ms(500);
		GPIO_ResetBits(GPIOC, GPIO_Pin_13);
		delay_1ms(500);
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
