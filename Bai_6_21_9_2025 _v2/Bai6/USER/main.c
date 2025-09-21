#include "stm32f10x.h"
#include "mcu-i2c.h"
#include "sensor_ds1307.h"
#include "usart.h"
#include "tim2.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define UART_RX_BUF_SIZE 64

volatile char uart_rx_buf[UART_RX_BUF_SIZE];
volatile uint8_t uart_rx_idx = 0;
volatile uint8_t uart_rx_ready = 0;

void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        char c = USART_ReceiveData(USART1);
        if (c == '\n' || c == '\r')
        {
            uart_rx_buf[uart_rx_idx] = 0;
            uart_rx_ready = 1;
            uart_rx_idx = 0;
        }
        else
        {
            if (uart_rx_idx < UART_RX_BUF_SIZE - 1)
                uart_rx_buf[uart_rx_idx++] = c;
        }
    }
}

void uart1_interrupt_init(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void parse_and_set_time(const char *cmd)
{
    // Format: SET hh:mm:ss dd/mm/yy
    if (strncmp(cmd, "SET", 3) != 0)
        return;
    int hh, mm, ss, dd, MM, yy;
    if (sscanf(cmd, "SET %2d:%2d:%2d %2d/%2d/%2d", &hh, &mm, &ss, &dd, &MM, &yy) == 6)
    {
        DS1307_Time_t t;
        t.hours = hh;
        t.minutes = mm;
        t.seconds = ss;
        t.date = dd;
        t.month = MM;
        t.year = yy;
        // Lấy thứ tự động (giả sử thứ 2 là ngày đầu tuần)
        t.day = 1;
        DS1307_Set_Time_7reg(&t);
        uart_SendStr("Time updated!\r\n");
    }
    else
    {
        uart_SendStr("Invalid format. Use: SET hh:mm:ss dd/mm/yy\r\n");
    }
}

int main(void)
{
    Timer2_Init();
    uart_Init();
    uart1_interrupt_init();

    // Khởi tạo I2C1 cho DS1307
    I2Cx_Init(I2C1, I2C1_B67, 100000);

    uart_SendStr("STM32 DS1307 UART Demo\r\n");
    uart_SendStr("Send: SET hh:mm:ss dd/mm/yy\r\n");

    DS1307_Time_t now;
    uint32_t last_read = 0;
    uint8_t last_second = 0xFF; // Giá trị không hợp lệ để đảm bảo lần đầu sẽ gửi

    while (1)
    {
        // Đọc thời gian mỗi 100ms
        if (millis() - last_read >= 100)
        {
            last_read = millis();
            DS1307_Get_Time_7reg(&now);

            // Nếu giá trị giây thay đổi thì gửi UART
            if (now.seconds != last_second)
            {
                char buf[64];
                snprintf(buf, sizeof(buf), "Time: %02d:%02d:%02d %02d/%02d/20%02d\r\n",
                         now.hours, now.minutes, now.seconds, now.date, now.month, now.year);
                uart_SendStr(buf);
                last_second = now.seconds;
            }
        }

        // Nếu nhận được lệnh UART
        if (uart_rx_ready)
        {
            uart_rx_ready = 0;
            parse_and_set_time((char *)uart_rx_buf);
        }
    }

    // oke
}