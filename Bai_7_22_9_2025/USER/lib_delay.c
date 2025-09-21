#include "lib_delay.h"

static uint16_t count_clock = 0;

void Delay_Setup(void)
{
	SystemCoreClockUpdate();
	SysTick_Config(SystemCoreClock/1000);
}

void SysTick_Handler(void)
{
	count_clock--;
}

void Delay_ms(uint16_t time_delay)
{
	count_clock = time_delay;
	while(count_clock != 0){}
}
