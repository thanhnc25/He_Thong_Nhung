#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "gpio.h"

// Giao diện đơn giản cho writer/reader để dễ kế thừa về sau
struct IByteWriter {
	virtual ~IByteWriter() {}
	virtual size_t Write(const uint8_t* data, size_t len) = 0;
	size_t Write(const char* s) { return Write(reinterpret_cast<const uint8_t*>(s), s ? strlen(s) : 0); }
};

struct IByteReader {
	virtual ~IByteReader() {}
	virtual int    Read() = 0;       // -1 nếu không có dữ liệu
	virtual int    Available() = 0;  // số byte sẵn sàng (ước lượng)
};

// Lớp UART cơ bản (polling, blocking). Có thể kế thừa để thêm interrupt/DMA.
class Uart : public IByteWriter, public IByteReader {
public:
	// instance: USART1/2/3; tự cấu hình chân mặc định tương ứng
	explicit Uart(USART_TypeDef* instance, uint32_t baud = 9600); // khởi tạo và Begin ngay (mặc định 9600)

	// Khởi tạo: baud, 8N1 mặc định
	// StdPeriph định nghĩa Parity/StopBits dạng uint16_t macro (không có typedef riêng)
	virtual void Begin(uint32_t baud, uint16_t parity = USART_Parity_No, uint16_t stopBits = USART_StopBits_1);
	virtual void End();

	// IByteWriter
	virtual size_t Write(const uint8_t* data, size_t len) override;
	size_t         Write(const char* s) { return IByteWriter::Write(s); }
	void           Write_Char(char c);

	// IByteReader
	virtual int Read() override;      // trả về byte (0..255) hoặc -1
	virtual int Available() override; // 0 hoặc 1 (polling)

	// Truy cập thấp
	inline USART_TypeDef* raw() const { return usart_; }

protected:
	// Cho phép override để map chân khác (ví dụ remap)
	virtual void Enable_Clocks();
	virtual void Setup_Default_Pins();
	virtual void Setup_Uart(uint32_t baud, uint16_t parity, uint16_t stopBits);

	// Trả về thông tin clock tương ứng mỗi UART
	uint32_t Usart_Clock_APB() const;          // RCC_APB1Periph_* hoặc RCC_APB2Periph_*
	bool     Is_APB2() const;                  // USART1 trên APB2, còn lại APB1

protected:
	USART_TypeDef* usart_;
	// TX/RX pin theo mặc định của từng UART (có thể override)
	GpioPin tx_;
	GpioPin rx_;
};

