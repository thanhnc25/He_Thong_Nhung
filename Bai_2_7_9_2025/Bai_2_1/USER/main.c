#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stdio.h"

uint8_t btn_state_old = 0; 
uint8_t btn_state_new = 0;
uint32_t cnt_btn_press = 0; 

void delay_1ms(unsigned int time);
void LED_PC_13_Init(void);
void Button_Init(void);

int main(void)
{
    LED_PC_13_Init();
    Button_Init();
    
    while(1)
    {
        btn_state_old = btn_state_new;
        btn_state_new = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_12);

        if (btn_state_old == 0 && btn_state_new == 1)
        {
            cnt_btn_press++;
            delay_1ms(100); 
        }
        
        if (cnt_btn_press % 2 == 0)
        {
            GPIO_ResetBits(GPIOC, GPIO_Pin_13); 
        }
        else 
        {
            GPIO_SetBits(GPIOC, GPIO_Pin_13);  
        }
    }
}

void LED_PC_13_Init(void)
{
    GPIO_InitTypeDef GPIO;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    GPIO.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO.GPIO_Pin = GPIO_Pin_13;
    GPIO.GPIO_Speed = GPIO_Speed_50MHz;
    
    GPIO_Init(GPIOC, &GPIO);
}

void Button_Init(void)
{
    GPIO_InitTypeDef BUTTON;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    BUTTON.GPIO_Pin = GPIO_Pin_12;
    BUTTON.GPIO_Mode = GPIO_Mode_IPD;   // Input pull-down
    BUTTON.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &BUTTON);
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
