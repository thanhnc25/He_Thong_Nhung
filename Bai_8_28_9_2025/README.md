# Bài 8: Đọc ADC trên STM32F103C8T6

## Mô tả dự án

Dự án này minh họa cách sử dụng **ADC (Analog-to-Digital Converter)** trên vi điều khiển STM32F103C8T6 để đọc tín hiệu analog từ chân PA0 và gửi kết quả qua UART.

## Tính năng chính

- ✅ Đọc tín hiệu analog từ chân **PA0** (ADC1 Channel 0)
- ✅ Chuyển đổi giá trị ADC 12-bit (0-4095) sang điện áp (0-3.3V)
- ✅ Gửi dữ liệu qua **UART** với tốc độ 9600 baud
- ✅ Cập nhật dữ liệu mỗi **500ms**
- ✅ Hiển thị cả giá trị ADC thô và điện áp chuyển đổi

## Cấu hình phần cứng

### Sơ đồ kết nối
```
STM32F103C8T6:
- PA0  → Tín hiệu analog đầu vào (0-3.3V)
- PA9  → UART TX (gửi dữ liệu)
- PA10 → UART RX (nhận dữ liệu)
- VCC  → 3.3V
- GND  → Ground
```

### ADC Configuration
- **ADC Module**: ADC1
- **Channel**: Channel 0 (PA0)
- **Resolution**: 12-bit (0-4095)
- **Sample Time**: 55.5 cycles
- **Conversion Time**: ~1μs (68 cycles tại 72MHz)
- **Reference Voltage**: 3.3V (VCC)

## Chi tiết kỹ thuật ADC

### 1. Khởi tạo ADC (`Adc_Init()`)

```c
void Adc_Init()
{
    // Bật clock cho GPIOA và ADC1
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    
    // Cấu hình PA0 làm analog input
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AIN;
    
    // Cấu hình ADC
    ADC_InitStruct.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStruct.ADC_ScanConvMode = DISABLE;
    ADC_InitStruct.ADC_ContinuousConvMode = DISABLE;
    // ... các cấu hình khác
}
```

**Các thông số quan trọng:**
- **Independent Mode**: ADC hoạt động độc lập, không dùng dual mode
- **Single Conversion**: Chỉ thực hiện 1 lần chuyển đổi khi được yêu cầu
- **Right Alignment**: Dữ liệu căn phải trong thanh ghi 16-bit
- **No External Trigger**: Khởi động chuyển đổi bằng software

### 2. Đọc giá trị ADC (`ADC_Read()`)

```c
uint16_t ADC_Read()
{
    // Bắt đầu chuyển đổi
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    
    // Chờ chuyển đổi hoàn thành
    while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
    
    // Đọc kết quả
    return ADC_GetConversionValue(ADC1);
}
```

**Quá trình chuyển đổi:**
1. **Start**: Khởi động chuyển đổi bằng software trigger
2. **Sample**: Lấy mẫu tín hiệu trong 55.5 clock cycles
3. **Convert**: Chuyển đổi trong 12.5 clock cycles
4. **EOC Flag**: Set flag báo hiệu chuyển đổi hoàn thành
5. **Read**: Đọc kết quả từ data register

### 3. Chuyển đổi sang điện áp (`Convert_To_Voltage()`)

```c
float Convert_To_Voltage(uint16_t adc_value)
{
    return (float)adc_value * 3.3f / 4095.0f;
}
```

**Công thức chuyển đổi:**
```
Voltage = (ADC_Value / ADC_Max) × Vref
Voltage = (ADC_Value / 4095) × 3.3V
```

**Ví dụ:**
- ADC = 0     → Voltage = 0.000V
- ADC = 2048  → Voltage = 1.650V  
- ADC = 4095  → Voltage = 3.300V

## Đặc tính kỹ thuật ADC STM32F103

| Thông số | Giá trị |
|----------|---------|
| **Resolution** | 12-bit (4096 levels) |
| **Input Range** | 0V - 3.3V |
| **Conversion Time** | 1μs (tối thiểu) |
| **Sample Rate** | Tối đa 1 MSPS |
| **Accuracy** | ±2 LSB |
| **Input Impedance** | 1MΩ (điển hình) |
| **Channels** | 16 channels (ADC1) |

## Output Format qua UART

```
ADC Reading Started
ADC Value: 0, Voltage: 0.00000 V
ADC Value: 1024, Voltage: 0.82500 V
ADC Value: 2048, Voltage: 1.65000 V
ADC Value: 3072, Voltage: 2.47500 V
ADC Value: 4095, Voltage: 3.30000 V
```

## Cấu trúc thư mục

```
Bai8/
├── USER/
│   ├── main.c          # Code chính chứa ADC functions
│   ├── usart.c         # UART communication functions
│   ├── usart.h         # UART header file
│   ├── tim2.c          # Timer2 for delay functions
│   └── tim2.h          # Timer2 header file
├── MDK/                # Keil MDK project files
└── RTE/                # Runtime Environment files
```

## Cách sử dụng

1. **Kết nối phần cứng** theo sơ đồ trên
2. **Mở dự án** trong Keil MDK
3. **Build** và **Flash** code vào STM32F103C8T6
4. **Kết nối UART** (9600 baud, 8N1) để xem kết quả
5. **Thay đổi điện áp** tại chân PA0 và quan sát kết quả

## Lưu ý quan trọng

⚠️ **Giới hạn điện áp đầu vào**: 0V - 3.3V
- Không được vượt quá 3.3V có thể làm hỏng vi điều khiển
- Sử dụng voltage divider nếu cần đo điện áp cao hơn

⚠️ **Độ chính xác**: 
- Độ phân giải: 3.3V/4095 = 0.8mV per LSB
- Nhiễu có thể ảnh hưởng đến kết quả, nên lấy trung bình nhiều lần đọc

⚠️ **Tần số ADC Clock**:
- ADC Clock = PCLK2/6 = 72MHz/6 = 12MHz (tối đa 14MHz)

## Mở rộng

Có thể mở rộng dự án với:
- [ ] Đọc nhiều channel ADC
- [ ] Sử dụng DMA để đọc ADC liên tục
- [ ] Thêm filter để giảm nhiễu
- [ ] Hiệu chuẩn ADC với điện áp tham chiếu chính xác
- [ ] Gửi dữ liệu qua other interfaces (SPI, I2C)

## Tác giả

**Môn học**: Hệ Thống Nhúng  
**Học kỳ**: Năm 4 - Kỳ 1  
**Ngày**: 28/09/2025