#include "gpio.h"

// Constructor đơn giản (chưa cấu hình)
GpioPin::GpioPin(GPIO_TypeDef* port, uint16_t pin)
	: port_(port), pin_(pin) {}

// Constructor đầy đủ: cấu hình ngay theo mode/speed (mặc định 50MHz)
GpioPin::GpioPin(GPIO_TypeDef* port, uint16_t pin, GPIO_Mode mode, GPIO_Speed speed)
	: port_(port), pin_(pin)
{
	Configure(mode, speed);
}

void GpioPin::Enable_Clock()
{
	uint32_t rcc = Rcc_From_Port(port_);
	if (rcc) RCC_APB2PeriphClockCmd(rcc, ENABLE);
}

uint32_t GpioPin::Rcc_From_Port(GPIO_TypeDef* port)
{
	if (port == GPIOA) return RCC_APB2Periph_GPIOA;
	if (port == GPIOB) return RCC_APB2Periph_GPIOB;
	if (port == GPIOC) return RCC_APB2Periph_GPIOC;
	if (port == GPIOD) return RCC_APB2Periph_GPIOD;
	if (port == GPIOE) return RCC_APB2Periph_GPIOE;
	if (port == GPIOF) return RCC_APB2Periph_GPIOF;
	if (port == GPIOG) return RCC_APB2Periph_GPIOG;
	// Mặc định an toàn, dù trên F103 thường chỉ A..G
	return 0;
}

void GpioPin::Configure(GPIO_Mode mode, GPIO_Speed speed)
{
	Enable_Clock();
	// AFIO thường cần khi dùng alternate function; bật sẵn khi cần
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	GPIO_InitTypeDef init = {};
	init.GPIO_Pin   = pin_;
	init.GPIO_Mode  = static_cast<GPIOMode_TypeDef>(mode);
	init.GPIO_Speed = static_cast<GPIOSpeed_TypeDef>(speed);
	GPIO_Init(port_, &init);
}

void GpioPin::As_Input_Pull_Up()
{
	Configure(GPIO_Mode::INPUT_PULL_UP);
}

void GpioPin::As_Input_Pull_Down()
{
	Configure(GPIO_Mode::INPUT_PULL_DOWN);
}

void GpioPin::As_Input_Floating()
{
	Configure(GPIO_Mode::INPUT_FLOATING);
}

void GpioPin::As_Output_Push_Pull(GPIO_Speed speed)
{
	Configure(GPIO_Mode::OUT_PUSH_PULL, speed);
}

void GpioPin::As_Output_Open_Drain(GPIO_Speed speed)
{
	Configure(GPIO_Mode::OUT_OPEN_DRAIN, speed);
}

void GpioPin::Toggle()
{
	if (Read_Output()) {
		Set_Low();
	} else {
		Set_High();
	}
}



