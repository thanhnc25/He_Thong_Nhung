//#include "main.h"
//#include "lib_adc.h"
//#include "lib_dma.h"
//#include "lib_uart.h"

//#define ADC_BUFFER_SIZE 100
//uint16_t ADC_Buffer[ADC_BUFFER_SIZE];
//char msg[64];

//void Delay_ms(uint16_t _time);

//void My_DMA_Complete_Callback(void)
//{
//    for (int i = 0; i < ADC_BUFFER_SIZE; i++) {
//        sprintf(msg, "ADC_Buffer[%d] = %u\r\n", i, (unsigned int)ADC_Buffer[i]);
//        UART1.Print(msg);
//    }
//}

//int main(void)
//{
//    UART1.Init(115200, NO_REMAP);
//    GPIO_ADC_Config();
//    ADC_Config();

//    DMA_Config_Channel1((uint32_t)&ADC1->DR, (uint32_t)ADC_Buffer, ADC_BUFFER_SIZE);

//   
//    DMA_SetCallback_Channel1(My_DMA_Complete_Callback);

//    while (1)
//    {
//   
//        Delay_ms(100);
//    }
//}

//void Delay_ms(uint16_t _time)
//{
//    uint16_t i,j;
//    for(i = 0; i < _time; i++){
//        for(j = 0; j < 0x2AFF; j++);
//    }
//}

#include "stm32f10x_gpio.h"             // Keil::Device:StdPeriph Drivers:GPIO
#include "stm32f10x_rcc.h"              // Keil::Device:StdPeriph Drivers:RCC

void GPIO_Config(void)
{
	GPIO_InitTypeDef GPIO;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	
	GPIO.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_2;
	GPIO.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO.GPIO_Speed = GPIO_Speed_50MHz;
	
	GPIO_Init(GPIOC, &GPIO);
}

void delay_1ms(uint16_t time_ms)
{
	for (int i = 0; i < time_ms; i++)
	{
		for (int j = 0; j < 0x2AFF; j++)
		{
		}
	}
}

int main()
{
	while(1)
	{
		GPIO_SetBits(GPIOC, GPIO_Pin_13); // Xuat tin hieu 1
		delay_1ms(1000);
		GPIO_ResetBits(GPIOC, GPIO_Pin_13); // Xuat tin hieu 0
		delay_1ms(1000);
	}
}



































