
// ================== KHAI BÁO THƯ VIỆN ==================
#include "stm32f10x.h"                  // Thư viện thiết bị STM32F10x
#include "stm32f10x_gpio.h"             // Thư viện điều khiển GPIO
#include "stm32f10x_rcc.h"              // Thư viện điều khiển xung nhịp RCC
#include "stm32f10x_tim.h"              // Thư viện điều khiển Timer
#include "stm32f10x_usart.h"            // Thư viện điều khiển USART
#include "misc.h"                       // Thư viện cho NVIC (ngắt)
#include "stdio.h"                      // Thư viện chuẩn C (dùng cho sprintf nếu cần)
#include "string.h"                     // Thư viện xử lý chuỗi
#include "tim2.h"                       // Header cho Timer2 (tự viết)

// ================== BIẾN TOÀN CỤC & ĐỊNH NGHĨA ==================
// Định nghĩa kích thước bộ đệm nhận UART
#define UART_BUFFER_SIZE 50
// Bộ đệm nhận UART (dùng volatile vì truy cập trong ngắt)
volatile char uart_rx_buffer[UART_BUFFER_SIZE];
// Chỉ số hiện tại trong buffer
volatile uint8_t uart_rx_index = 0;
// Cờ báo hiệu đã nhận xong chuỗi lệnh (gặp '!')
volatile uint8_t uart_command_ready = 0;

void Uart_Init();
void Led_Init();
void Uart_Send_Char(char _chr);
void Uart_Send_Str(char *str);
void Process_UART_Command(void);
void LED_On(void);
void LED_Off(void);


// ================== HÀM MAIN ==================
int main()
{
	uint32_t last_hello_time = 0; // Biến lưu thời điểm gửi "Hello" lần cuối
    
	// --- Khởi tạo các ngoại vi ---
	Timer2_Init();    // Khởi tạo Timer2 (tạo hàm millis, delay)
	Uart_Init();      // Khởi tạo UART1 (giao tiếp nối tiếp)
	Led_Init();       // Khởi tạo LED PC13
    
	// --- Gửi thông báo khởi động qua UART ---
	Uart_Send_Str("STM32F103C8T6 Ready!\r\n");
	Uart_Send_Str("Commands: 'on!' to turn LED on, 'off!' to turn LED off\r\n");
    
	// --- Vòng lặp chính ---
	while(1)
	{
		// Gửi "Hello from STM32!" mỗi 1000ms (1 giây)
		if ((millis() - last_hello_time) >= 1000)
		{
			Uart_Send_Str("Hello from STM32!\r\n");
			last_hello_time = millis();
		}
		// Xử lý lệnh nhận được từ UART (nếu có)
		Process_UART_Command();
	}
}


// ================== KHỞI TẠO UART1 ==================
void Uart_Init()
{
	GPIO_InitTypeDef gpio_typedef;
	USART_InitTypeDef usart_typedef;
	NVIC_InitTypeDef nvic_typedef;
    
	// --- Bật clock cho GPIOA và USART1 ---
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);      // Clock cho port A
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);     // Clock cho USART1
	// --- Cấu hình chân TX (PA9) ---
	gpio_typedef.GPIO_Pin = GPIO_Pin_9;
	gpio_typedef.GPIO_Mode = GPIO_Mode_AF_PP;                  // Chế độ push-pull, chức năng thay thế
	gpio_typedef.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio_typedef);
	// --- Cấu hình chân RX (PA10) ---
	gpio_typedef.GPIO_Pin = GPIO_Pin_10;
	gpio_typedef.GPIO_Mode = GPIO_Mode_IN_FLOATING;            // Chế độ input floating
	gpio_typedef.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio_typedef);
	// --- Cấu hình thông số USART1 ---
	usart_typedef.USART_BaudRate = 9600;                      // Baudrate 9600
	usart_typedef.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	usart_typedef.USART_Mode = USART_Mode_Tx | USART_Mode_Rx; // Cho phép truyền và nhận
	usart_typedef.USART_Parity = USART_Parity_No;              // Không parity
	usart_typedef.USART_StopBits = USART_StopBits_1;           // 1 stop bit
	usart_typedef.USART_WordLength = USART_WordLength_8b;      // 8 bit dữ liệu
	USART_Init(USART1, &usart_typedef);

	// --- Cho phép ngắt nhận dữ liệu UART ---
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    
	// --- Cấu hình NVIC cho ngắt USART1 ---
	nvic_typedef.NVIC_IRQChannel = USART1_IRQn;
	nvic_typedef.NVIC_IRQChannelPreemptionPriority = 0;
	nvic_typedef.NVIC_IRQChannelSubPriority = 0;
	nvic_typedef.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic_typedef);

	// --- Kích hoạt USART1 ---
	USART_Cmd(USART1, ENABLE);
}


// ================== GỬI 1 KÝ TỰ QUA UART ==================
void Uart_Send_Char(char _chr)
{
	USART_SendData(USART1, _chr); // Gửi ký tự qua thanh ghi dữ liệu
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET); // Đợi gửi xong
}


// ================== GỬI CHUỖI QUA UART ==================
void Uart_Send_Str(char *str)
{
	while (*str != NULL)
	{
		Uart_Send_Char(*str++); // Gửi từng ký tự cho đến khi gặp ký tự kết thúc chuỗi
	}
}


// ================== KHỞI TẠO LED PC13 ==================
void Led_Init()
{
	GPIO_InitTypeDef gpio_typedef;
	// Bật clock cho port C
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	// Cấu hình chân PC13 là output push-pull
	gpio_typedef.GPIO_Pin = GPIO_Pin_13;
	gpio_typedef.GPIO_Mode = GPIO_Mode_Out_PP;
	gpio_typedef.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOC, &gpio_typedef);
	// Tắt LED ban đầu (PC13 active low)
	GPIO_SetBits(GPIOC, GPIO_Pin_13);
}


// ================== HÀM NGẮT NHẬN UART1 ==================
void USART1_IRQHandler(void)
{
	// Kiểm tra cờ nhận dữ liệu RXNE
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		char received_char = USART_ReceiveData(USART1); // Đọc ký tự nhận được
        
		if (received_char == '!') // Nếu gặp ký tự kết thúc chuỗi
		{
			uart_rx_buffer[uart_rx_index] = '\0'; // Kết thúc chuỗi bằng null
			uart_command_ready = 1;                // Báo hiệu đã nhận xong lệnh
			uart_rx_index = 0;                     // Reset chỉ số cho lần nhận tiếp theo
		}
		else if (uart_rx_index < UART_BUFFER_SIZE - 1)
		{
			uart_rx_buffer[uart_rx_index++] = received_char; // Lưu ký tự vào buffer
		}
		else
		{
			uart_rx_index = 0; // Nếu tràn bộ đệm thì reset lại
		}
		// Xóa cờ ngắt
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
	}
}


// ================== HÀM ĐIỀU KHIỂN LED ==================
void LED_On(void)
{
	GPIO_ResetBits(GPIOC, GPIO_Pin_13); // PC13 active low: reset bit để bật LED
}

void LED_Off(void)
{
	GPIO_SetBits(GPIOC, GPIO_Pin_13);   // PC13 active low: set bit để tắt LED
}


// ================== XỬ LÝ LỆNH UART NHẬN ĐƯỢC ==================
void Process_UART_Command(void)
{
	if (uart_command_ready) // Nếu đã nhận xong chuỗi lệnh (gặp '!')
	{
		if (strcmp((char*)uart_rx_buffer, "on") == 0) // Nếu lệnh là "on"
		{
			LED_On();
			Uart_Send_Str("LED ON\r\n"); // Phản hồi đã bật LED
		}
		else if (strcmp((char*)uart_rx_buffer, "off") == 0) // Nếu lệnh là "off"
		{
			LED_Off();
			Uart_Send_Str("LED OFF\r\n"); // Phản hồi đã tắt LED
		}
		else // Nếu là chuỗi khác
		{
			Uart_Send_Str("Echo: ");
			Uart_Send_Str((char*)uart_rx_buffer); // Gửi lại chính chuỗi đó
			Uart_Send_Str("\r\n");
		}
		// Reset cờ báo hiệu đã xử lý xong lệnh
		uart_command_ready = 0;
	}
}