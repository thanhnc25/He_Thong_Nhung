# STM32 SPI SD Card Example  

## Giới thiệu  
Chương trình minh họa cách **cấu hình STM32 ở chế độ Master SPI** để giao tiếp với một module ngoại vi đơn giản – ở đây là **SD Card**.  
Code thực hiện:  
- Khởi tạo UART để in log ra màn hình.  
- Cấu hình SPI1 để giao tiếp với thẻ SD.  
- Khởi tạo SD Card, tạo file mới, ghi dữ liệu, sau đó đóng file.  

---

## Yêu cầu phần cứng  
- **Vi điều khiển**: STM32 (ví dụ: STM32F103C8T6, STM32F103RB, …)  
- **Module SD Card** giao tiếp chuẩn SPI  
- **UART TTL to USB** để quan sát log từ STM32 trên máy tính  
- **Kết nối SPI**:  

| STM32 (SPI1) | SD Card Module |
|--------------|----------------|
| PA5 (SCK)    | SCK            |
| PA6 (MISO)   | MISO           |
| PA7 (MOSI)   | MOSI           |
| PA4 (NSS)    | CS             |
| GND          | GND            |
| 3.3V         | VCC (3.3V)     |

> ⚠️ Lưu ý: SD Card chỉ hỗ trợ 3.3V, không cấp 5V trực tiếp.  

---

## Yêu cầu phần mềm  
- **IDE**: STM32CubeIDE hoặc Keil uVision  
- **Thư viện sử dụng**:  
  - `uart.h`: cấu hình UART1  
  - `spi_sdcard/sdcard.h`: driver giao tiếp SD Card qua SPI  
  - `lib_define.h`: các định nghĩa chung  
  - **FatFs**: hệ thống quản lý file (FATFS, FIL, f_mount, f_open, f_write, f_close, …)  

---

## Luồng hoạt động của chương trình  
1. **Khởi tạo UART1** ở baudrate 115200 để in log.  
2. **Cấu hình SPI1** với prescaler = 256, hoạt động ở chế độ **SPI Master**.  
3. In ra thông báo bắt đầu khởi tạo SD Card.  
4. Gọi `SD_Init()`:  
   - Nếu **thành công**:  
     - Mount hệ thống file (`f_mount`)  
     - Mở/tạo mới file `Nhom_1.txt`  
     - Ghi chuỗi `"Bai tap 7 Ngay 21/7/2025"` vào file  
     - Đóng file (`f_close`)  
     - Unmount hệ thống file  
     - In ra `"SD Card Init SUCCESS!"`  
   - Nếu **thất bại**:  
     - In ra `"SD Card Init ERROR!"`  
5. In ra `"Done!!!"`.  

---

