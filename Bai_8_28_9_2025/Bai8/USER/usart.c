#include "usart.h"
#include "stm32f10x_usart.h"

// struct __FILE {
//     int dummy;
// };
// FILE __stdout;

// int fputc(int ch, FILE *f) {
//
//     uart_SendChar(ch);
//
//     return ch;
// }

void uart_Init(void)
{
	GPIO_InitTypeDef gpio_typedef;
	USART_InitTypeDef usart_typedef;
	// enable clock
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	// congifgure pin Tx - A9;
	gpio_typedef.GPIO_Pin = GPIO_Pin_9;
	gpio_typedef.GPIO_Mode = GPIO_Mode_AF_PP;
	gpio_typedef.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio_typedef);
	// configure pin Rx - A10;
	gpio_typedef.GPIO_Pin = GPIO_Pin_10;
	gpio_typedef.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	gpio_typedef.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio_typedef);
	// usart configure
	usart_typedef.USART_BaudRate = 9600;
	usart_typedef.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	usart_typedef.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	usart_typedef.USART_Parity = USART_Parity_No;
	usart_typedef.USART_StopBits = USART_StopBits_1;
	usart_typedef.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART1, &usart_typedef);

	USART_Cmd(USART1, ENABLE);
}

void uart_SendChar(char _chr)
{
	USART_SendData(USART1, _chr);
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
		;
}

void uart_SendStr(char *str)
{
	while (*str != NULL)
	{
		uart_SendChar(*str++);
	}
}

void uart_SendNumber(uint32_t number)
{
	char str[12]; // Enough for 32-bit unsigned integer
	int i = 0;
	
	// Handle special case of 0
	if (number == 0)
	{
		uart_SendChar('0');
		return;
	}
	
	// Convert number to string (reverse order)
	while (number > 0)
	{
		str[i++] = (number % 10) + '0';
		number /= 10;
	}
	
	// Send string in correct order
	while (i > 0)
	{
		uart_SendChar(str[--i]);
	}
}

void uart_SendFloat(float number, uint8_t decimal_places)
{
	// Handle negative numbers
	if (number < 0)
	{
		uart_SendChar('-');
		number = -number;
	}
	
	// Send integer part
	uint32_t integer_part = (uint32_t)number;
	uart_SendNumber(integer_part);
	
	// Send decimal point
	uart_SendChar('.');
	
	// Send fractional part
	float fractional_part = number - integer_part;
	for (uint8_t i = 0; i < decimal_places; i++)
	{
		fractional_part *= 10;
		uint8_t digit = (uint8_t)fractional_part;
		uart_SendChar(digit + '0');
		fractional_part -= digit;
	}
}
