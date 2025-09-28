#include "lib_adc.h"

void GPIO_ADC_Config(void)
{
	GPIO_InitTypeDef GPIO_ADC;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_ADC.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_ADC.GPIO_Pin = GPIO_Pin_0;
	GPIO_ADC.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_ADC);
}

void ADC_Config(void)
{
    ADC_InitTypeDef ADC_1;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    ADC_1.ADC_Mode = ADC_Mode_Independent;
    ADC_1.ADC_ScanConvMode = DISABLE;
    ADC_1.ADC_ContinuousConvMode = ENABLE; // b?t continuous mode
    ADC_1.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_1.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_1.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_1);

    ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_55Cycles5);

		ADC_DMACmd(ADC1, ENABLE); // Bat DMA cho ADC
    ADC_Cmd(ADC1, ENABLE);

    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));

    ADC_SoftwareStartConvCmd(ADC1, ENABLE); // ch? g?i 1 l?n
}


unsigned int ADCx_Read(void){
	return(ADC_GetConversionValue(ADC1));
}

int Map(int x, int in_min, int in_max, int out_min, int out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}