#pragma once

// GPIO tiện ích cho STM32F103 (StdPeriph) viết theo C++ hướng đối tượng
// Mục tiêu: API tiện dụng, dễ nhân bản, hỗ trợ thừa kế; đặt tên theo quy ước:
// - Enum: VIẾT HOA VÀ CÓ GẠCH DƯỚI (VD: GPIO_SPEED_MHZ_50)
// - Hàm: Viết Hoa Chữ Đầu Và Cách Nhau Bằng Gạch Dưới (VD: Configure, As_Input_Pull_Up)
// - Biến: chữ thường, cách nhau bằng gạch dưới (VD: port_, pin_)

#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"

// Kiểu enum dành cho tốc độ và chế độ chân
enum class GPIO_Speed {
	MHZ_2  = GPIO_Speed_2MHz,
	MHZ_10 = GPIO_Speed_10MHz,
	MHZ_50 = GPIO_Speed_50MHz
};

enum class GPIO_Mode {
	INPUT_FLOATING = GPIO_Mode_IN_FLOATING,
	INPUT_PULL_UP   = GPIO_Mode_IPU,
	INPUT_PULL_DOWN = GPIO_Mode_IPD,
	OUT_PUSH_PULL   = GPIO_Mode_Out_PP,
	OUT_OPEN_DRAIN  = GPIO_Mode_Out_OD,
	AF_PUSH_PULL    = GPIO_Mode_AF_PP,
	AF_OPEN_DRAIN   = GPIO_Mode_AF_OD
};

// Lớp đại diện cho 1 chân GPIO cụ thể
class GpioPin {
public:
	// Constructor đơn giản: chỉ định port/pin, chưa cấu hình
	GpioPin(GPIO_TypeDef* port, uint16_t pin);

	// Constructor đầy đủ: nhận port, pin, mode, speed (speed mặc định 50MHz) và cấu hình ngay
	// Giúp khởi tạo tiện dụng tại global/static để dùng ngay
	GpioPin(GPIO_TypeDef* port, uint16_t pin, GPIO_Mode mode, GPIO_Speed speed = GPIO_Speed::MHZ_50);

	// Cấu hình chân theo mode và speed (speed chỉ áp dụng cho Output/AF)
	virtual void Configure(GPIO_Mode mode, GPIO_Speed speed = GPIO_Speed::MHZ_50);

	// Các cấu hình nhanh phổ biến (đặt tên theo quy ước)
	void As_Input_Pull_Up();
	void As_Input_Pull_Down();
	void As_Input_Floating();
	void As_Output_Push_Pull(GPIO_Speed speed = GPIO_Speed::MHZ_50);
	void As_Output_Open_Drain(GPIO_Speed speed = GPIO_Speed::MHZ_50);

	// Điều khiển mức ra và đọc mức
	inline void Set_High()   { GPIO_SetBits(port_, pin_); }
	inline void Set_Low()    { GPIO_ResetBits(port_, pin_); }
	inline void Toggle();
	inline bool Read_Input() const  { return GPIO_ReadInputDataBit(port_, pin_) == Bit_SET; }
	inline bool Read_Output() const { return GPIO_ReadOutputDataBit(port_, pin_) == Bit_SET; }

	// Truy xuất thông tin
	inline GPIO_TypeDef* port() const { return port_; }
	inline uint16_t pin() const { return pin_; }

protected:
	// Cho phép lớp con can thiệp enable clock nếu cần tuỳ biến
	virtual void Enable_Clock();

private:
	static uint32_t Rcc_From_Port(GPIO_TypeDef* port);

	GPIO_TypeDef* port_;
	uint16_t      pin_;
};

