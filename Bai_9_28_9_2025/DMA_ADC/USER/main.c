#include "main.h"
#include "lib_adc.h"
#include "lib_dma.h"
#include "lib_uart.h"

#define ADC_BUFFER_SIZE 100
uint16_t ADC_Buffer[ADC_BUFFER_SIZE];
char msg[64];

void Delay_ms(uint16_t _time);

/* Callback x? lý d? li?u DMA xong */
void My_DMA_Complete_Callback(void)
{
    for (int i = 0; i < ADC_BUFFER_SIZE; i++) {
        sprintf(msg, "ADC_Buffer[%d] = %u\r\n", i, (unsigned int)ADC_Buffer[i]);
        UART1.Print(msg);
    }
}

int main(void)
{
    UART1.Init(115200, NO_REMAP);
    GPIO_ADC_Config();
    ADC_Config();

    DMA_Config_Channel1((uint32_t)&ADC1->DR, (uint32_t)ADC_Buffer, ADC_BUFFER_SIZE);

    // Ðang ký callback khi DMA hoàn t?t
    DMA_SetCallback_Channel1(My_DMA_Complete_Callback);

    while (1)
    {
        // main loop r?nh r?i
        Delay_ms(100);
    }
}

void Delay_ms(uint16_t _time)
{
    uint16_t i,j;
    for(i = 0; i < _time; i++){
        for(j = 0; j < 0x2AFF; j++);
    }
}
