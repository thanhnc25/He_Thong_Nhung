#include "mcu-i2c.h"

GPIO_InitTypeDef GPIO_I2C_InitStruct;

// void Config_Timer(void)
// {
// 	TIM_TimeBaseInitTypeDef TIMER;
// 	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
// 	TIMER.TIM_CounterMode = TIM_CounterMode_Up;
// 	TIMER.TIM_Period = 65535;
// 	TIMER.TIM_Prescaler = 1;
// 	TIMER.TIM_RepetitionCounter = 0;
// 	TIM_TimeBaseInit(TIM2, &TIMER);
// }

// void Delay_1ms(void)
// {
// 	Config_Timer();
// 	TIM_Cmd(TIM2, ENABLE);
// 	TIM_SetCounter(TIM2, 0);
// 	while (TIM_GetCounter(TIM2) < 36000)
// 		;
// 	TIM_Cmd(TIM2, DISABLE);
// }

// void Delay_ms(unsigned int time_ms)
// {
// 	while (time_ms--)
// 	{
// 		Delay_1ms();
// 	}
// }

// void Delay_1us(void)
// {
// 	Config_Timer();
// 	TIM_Cmd(TIM2, ENABLE);
// 	TIM_SetCounter(TIM2, 0);
// 	while (TIM_GetCounter(TIM2) < 36)
// 		;
// 	TIM_Cmd(TIM2, DISABLE);
// }

// void Delay_us(unsigned int time_us)
// {
// 	while (time_us--)
// 	{
// 		Delay_1us();
// 	}
// }
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

void I2Cx_PinOut(uint32_t Pinout)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	if (Pinout == I2C1_B67)
	{
		//                      					SCL          SDA
		GPIO_I2C_InitStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	}
	else if (Pinout == I2C1_B89)
	{
		//                      					SCL          SDA
		GPIO_I2C_InitStruct.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
		GPIO_PinRemapConfig(GPIO_Remap_I2C1, ENABLE);
	}
	else if (Pinout == I2C2_B1011)
	{
		//                      					SCL          SDA
		GPIO_I2C_InitStruct.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
	}
	GPIO_I2C_InitStruct.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_I2C_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_I2C_InitStruct);
}

void I2Cx_Init(I2C_TypeDef *I2Cx, uint32_t Pinout, uint32_t ClockSpeed)
{
	I2C_InitTypeDef I2C_InitStruct;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	if (I2Cx == I2C1)
	{
		/* Enable clock */
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
		/* Enable pins */
		I2Cx_PinOut(Pinout);
		/* I2C1 Reset */

		RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1, ENABLE);
		RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1, DISABLE);

		/* Set values */
		I2C_InitStruct.I2C_ClockSpeed = ClockSpeed;
		I2C_InitStruct.I2C_AcknowledgedAddress = I2C1_ACKNOWLEDGED_ADDRESS;
		I2C_InitStruct.I2C_Mode = I2C1_MODE;
		I2C_InitStruct.I2C_OwnAddress1 = I2C1_OWN_ADDRESS;
		I2C_InitStruct.I2C_Ack = I2C1_ACK;
		I2C_InitStruct.I2C_DutyCycle = I2C1_DUTY_CYCLE;
	}
	else if (I2Cx == I2C2)
	{
		/* Enable clock */
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
		/* Enable pins */
		I2Cx_PinOut(Pinout);

		RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C2, ENABLE);
		RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C2, DISABLE);

		/* Set values */
		I2C_InitStruct.I2C_ClockSpeed = ClockSpeed;
		I2C_InitStruct.I2C_AcknowledgedAddress = I2C2_ACKNOWLEDGED_ADDRESS;
		I2C_InitStruct.I2C_Mode = I2C2_MODE;
		I2C_InitStruct.I2C_OwnAddress1 = I2C2_OWN_ADDRESS;
		I2C_InitStruct.I2C_Ack = I2C2_ACK;
		I2C_InitStruct.I2C_DutyCycle = I2C2_DUTY_CYCLE;
	}

	/* Disable I2C first */
	I2C_Cmd(I2Cx, DISABLE);
	/* Initialize I2C */
	I2C_Init(I2Cx, &I2C_InitStruct);
	/* Enable I2C */
	I2C_Cmd(I2Cx, ENABLE);
}

void I2Cx_WriteByte(I2C_TypeDef *I2Cx, uint8_t address, uint8_t data)
{
	/* Send START condition */
	I2C_GenerateSTART(I2Cx, ENABLE);
	/* Test on EV5 and clear it */
	while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT))
		;
	/* Send PCF8574A address for write */
	I2C_Send7bitAddress(I2Cx, address, I2C_Direction_Transmitter);
	/* Test on EV6 and clear it */
	while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
		;
	/* Send the data byte to be written */
	I2C_SendData(I2Cx, data);
	/* Test on EV8 and clear it */
	while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
		;
	/* Send STOP condition */
	I2C_GenerateSTOP(I2Cx, ENABLE);
}

void I2Cx_WriteMultiByte(I2C_TypeDef *I2Cx, const uint8_t *buff, uint32_t nbyte, uint8_t address)
{
	if (nbyte)
	{
		I2C_GenerateSTART(I2Cx, ENABLE);
		while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT))
			;

		I2C_Send7bitAddress(I2Cx, address, I2C_Direction_Transmitter);
		while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
			;

		I2C_SendData(I2Cx, address);
		while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
			;

		I2C_SendData(I2Cx, *buff++);
		while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
			;

		while (nbyte--)
		{
			I2C_SendData(I2Cx, *buff++);
			while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
				;
		}
		I2C_GenerateSTOP(I2Cx, ENABLE);
	}
}

uint8_t I2Cx_ReadByte(I2C_TypeDef *I2Cx, uint8_t address)
{
	uint8_t msb = 0;
	uint8_t lsb = 0;
	I2C_AcknowledgeConfig(I2Cx, ENABLE);
	I2C_GenerateSTART(I2Cx, ENABLE);
	while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT))
		;
	I2C_Send7bitAddress(I2Cx, address, I2C_Direction_Transmitter);
	while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
		;

	I2C_SendData(I2Cx, address);
	while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
		;

	I2C_GenerateSTART(I2C1, ENABLE);
	while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT))
		;

	I2C_Send7bitAddress(I2Cx, address, I2C_Direction_Receiver);
	while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
		;

	while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED))
		;
	lsb = I2C_ReceiveData(I2Cx);

	while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED))
		;
	msb = I2C_ReceiveData(I2Cx);

	I2C_GenerateSTOP(I2Cx, ENABLE);
	I2C_AcknowledgeConfig(I2Cx, DISABLE);

	return (msb << 8) | lsb;
}
