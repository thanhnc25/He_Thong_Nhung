#include "lib_define.h"
#include "uart.h"
#include "spi_sdcard/sdcard.h"

int main(void)
{	
	  UART1.Init(115200, NO_REMAP); 
    SPI1_Setup(SPI_BaudRatePrescaler_256);  // SPI for SD card with prescaler 8

    UART1.Print("[MAIN]: SD Card Init...\n");
	
    // Initialize SD card
    if (SD_Init() == 0)
    {
        UART1.Print("[MAIN]: SD Card Init SUCCESS!\n");
        
        FATFS fs;
        FIL file;
        UINT written;
        f_mount(0, &fs);
        f_open(&file, "Nhom_1.txt", FA_CREATE_ALWAYS | FA_WRITE);
				f_lseek(&file, 0);
				char data[] = "Bai tap 7 Ngay 21/7/2025";
        f_write(&file, data, strlen(data),  &written);
        f_close(&file);
        f_mount(1, &fs);
    }
    else
    {
        UART1.Print("[MAIN]: SD Card Init ERROR!\n");
    }
		UART1.Print("Done!!!\n");

    while (1)
    {
      
    }
}

