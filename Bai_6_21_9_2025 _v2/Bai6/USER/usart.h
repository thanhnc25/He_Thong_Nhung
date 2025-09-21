#ifndef _USART_H_
#define _USART_H_

#include "string.h"
#include "stdio.h"

void uart_Init(void);
void uart_SendChar(char _chr);
void uart_SendStr(char *str);

#endif
