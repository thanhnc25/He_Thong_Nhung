#include "stm32f10x.h"                  // Device header
#include "stm32f10x_gpio.h"             // Keil::Device:StdPeriph Drivers:GPIO
#include "stm32f10x_rcc.h"              // Keil::Device:StdPeriph Drivers:RCC
#include "stm32f10x_adc.h"              // Keil::Device:StdPeriph Drivers:ADC
#include "usart.h"
#include "tim2.h"

// Function prototypes - Khai báo nguyên mẫu hàm
void Adc_Init();                        // Hàm khởi tạo ADC
uint16_t ADC_Read();                    // Hàm đọc giá trị ADC
float Convert_To_Voltage(uint16_t adc_value); // Hàm chuyển đổi giá trị ADC sang điện áp

int main(void)
{
    uint16_t adc_value;    // Biến lưu giá trị ADC (0-4095)
    float voltage;         // Biến lưu giá trị điện áp chuyển đổi (0-3.3V)
    char buffer[50];       // Buffer dự phòng (hiện tại chưa sử dụng)
    
    // Initialize peripherals - Khởi tạo các ngoại vi
    uart_Init();           // Khởi tạo UART để giao tiếp
    Timer2_Init();         // Khởi tạo Timer2 cho delay
    Adc_Init();            // Khởi tạo ADC để đọc analog
    
    uart_SendStr("ADC Reading Started\r\n");  // Gửi thông báo bắt đầu đọc ADC
    
    while(1)  // Vòng lặp chính
    {
        // Read ADC value - Đọc giá trị từ ADC channel 0 (chân PA0)
        adc_value = ADC_Read();
        
        // Convert to voltage - Chuyển đổi giá trị ADC sang điện áp thực tế
        voltage = Convert_To_Voltage(adc_value);
        
        // Send ADC value and voltage via UART - Gửi giá trị ADC và điện áp qua UART
        uart_SendStr("ADC Value: ");
        uart_SendNumber(adc_value);        // Gửi giá trị ADC thô (0-4095)
        uart_SendStr(", Voltage: ");
        uart_SendFloat(voltage, 5);        // Gửi điện áp với 3 chữ số thập phân
        uart_SendStr(" V\r\n");
        
        // Wait 500ms - Chờ 500ms trước lần đọc tiếp theo
        Delay_ms(500);
    }
}

/*
 * Hàm khởi tạo ADC
 * Cấu hình ADC1 để đọc tín hiệu analog từ chân PA0 (channel 0)
 * ADC hoạt động ở chế độ single conversion với độ phân giải 12-bit
 */
void Adc_Init()
{
    GPIO_InitTypeDef GPIO_InitStruct;   // Cấu trúc cấu hình GPIO
    ADC_InitTypeDef ADC_InitStruct;     // Cấu trúc cấu hình ADC
    
    // Enable clocks - Bật clock cho GPIO và ADC
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);  // Bật clock GPIOA
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);   // Bật clock ADC1
    
    // Configure PA0 as analog input - Cấu hình chân PA0 làm đầu vào analog
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;     // Chọn chân PA0
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AIN; // Chế độ analog input (không cần pull-up/pull-down)
    GPIO_Init(GPIOA, &GPIO_InitStruct);        // Áp dụng cấu hình cho GPIOA
    
    // ADC configuration - Cấu hình các thông số cho ADC
    ADC_InitStruct.ADC_Mode = ADC_Mode_Independent;           // Chế độ độc lập (không dùng dual mode)
    ADC_InitStruct.ADC_ScanConvMode = DISABLE;                // Tắt chế độ quét nhiều channel
    ADC_InitStruct.ADC_ContinuousConvMode = DISABLE;          // Tắt chế độ chuyển đổi liên tục
    ADC_InitStruct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None; // Không dùng trigger ngoài
    ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right;       // Căn phải dữ liệu (bit 0-11)
    ADC_InitStruct.ADC_NbrOfChannel = 1;                      // Số channel sử dụng: 1
    ADC_Init(ADC1, &ADC_InitStruct);                          // Áp dụng cấu hình cho ADC1
    
    // Configure ADC1 Channel 0 - Cấu hình channel 0 của ADC1
    // Thời gian lấy mẫu 55.5 cycles (tổng cộng 55.5 + 12.5 = 68 cycles cho 1 lần chuyển đổi)
    ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_55Cycles5);
    
    // Enable ADC1 - Bật ADC1
    ADC_Cmd(ADC1, ENABLE);
    
    // ADC1 calibration - Hiệu chuẩn ADC1 để có độ chính xác cao
    ADC_ResetCalibration(ADC1);                    // Reset quá trình hiệu chuẩn
    while(ADC_GetResetCalibrationStatus(ADC1));    // Chờ reset hoàn thành
    ADC_StartCalibration(ADC1);                    // Bắt đầu hiệu chuẩn
    while(ADC_GetCalibrationStatus(ADC1));         // Chờ hiệu chuẩn hoàn thành
}

/*
 * Hàm đọc giá trị từ ADC
 * Thực hiện một lần chuyển đổi và trả về giá trị 12-bit (0-4095)
 * Thời gian chuyển đổi: 68 cycles (khoảng 1μs với ADC clock 72MHz)
 */
uint16_t ADC_Read()
{
    // Start conversion - Bắt đầu quá trình chuyển đổi ADC
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    
    // Wait for conversion to complete - Chờ chuyển đổi hoàn thành
    // Flag EOC (End Of Conversion) sẽ được set khi chuyển đổi xong
    while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
    
    // Read conversion result - Đọc kết quả chuyển đổi từ thanh ghi DR
    return ADC_GetConversionValue(ADC1);
}

/*
 * Hàm chuyển đổi giá trị ADC sang điện áp
 * Input: adc_value - giá trị ADC từ 0 đến 4095 (12-bit)
 * Output: điện áp tương ứng từ 0V đến 3.3V
 * 
 * Công thức: Voltage = (ADC_Value / ADC_Max) * Vref
 * Trong đó:
 * - ADC_Value: giá trị đọc được từ ADC (0-4095)
 * - ADC_Max: giá trị ADC maximum = 2^12 - 1 = 4095
 * - Vref: điện áp tham chiếu = 3.3V (VCC của STM32F103)
 */
float Convert_To_Voltage(uint16_t adc_value)
{
    // Vref = 3.3V, ADC resolution = 12 bits (0-4095)
    // Chuyển đổi: (adc_value / 4095) * 3.3V
    return (float)adc_value * 3.3f / 4095.0f;
}