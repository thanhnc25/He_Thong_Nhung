#ifndef _USART_H_
#define _USART_H_

#include "string.h"
#include "stdio.h"
#include "stdint.h"

// Khai báo các hàm UART
void uart_Init(void);                                      // Khởi tạo UART1 (9600 baud)
void uart_SendChar(char _chr);                            // Gửi 1 ký tự qua UART
void uart_SendStr(char *str);                             // Gửi chuỗi ký tự qua UART
void uart_SendNumber(uint32_t number);                    // Gửi số nguyên qua UART
void uart_SendFloat(float number, uint8_t decimal_places); // Gửi số thực với số chữ số thập phân tùy chỉnh

#endif
