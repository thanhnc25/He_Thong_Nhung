#include "spi.h" 

void SPI1_Setup(uint16_t spi_baurate)
{
    SPI_InitTypeDef SPI_InitStruct;    
    GPIO_InitTypeDef GPIO_InitStruct;
    
    // Bat clock cho Alternate Function (AFIO) 
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);    

    // Bat clock cho GPIOA va SPI1
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1, ENABLE);
		
    // --- Cau hinh chan PA5 (SCK), PA6 (MISO), PA7 (MOSI) ---
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7; // chon cac chan SPI1
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;     // Alternate Function Push-Pull
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;   // toc do cao (50MHz)
    GPIO_Init(GPIOA, &GPIO_InitStruct);              // ap dung cau hinh cho GPIOA

    // --- Cau hinh chan PA4 lam NSS/CS (dung phan mem dieu khien) ---
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_4;           // chon chan PA4
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;    // Output Push-Pull
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;   // toc do cao
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIOA->BSRR = GPIO_Pin_4;    // Dat PA4 = 1 (CS = High, khong chon slave)

    // --- Cau hinh SPI1 ---
    SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex;  // SPI full-duplex (MOSI + MISO)
    SPI_InitStruct.SPI_Mode = SPI_Mode_Master;                       // SPI o che do Master
    SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;                   // khung du lieu = 8 bit
    SPI_InitStruct.SPI_CPOL = SPI_CPOL_High;                         // Clock idle o muc cao (CPOL=1)
    SPI_InitStruct.SPI_CPHA = SPI_CPHA_2Edge;                        // Du lieu chot o canh xuong (CPHA=1)
    // => CPOL=1, CPHA=1 tuong ung SPI mode 3
    SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;                           // Quan ly NSS bang phan mem (khong dung NSS cung)
    SPI_InitStruct.SPI_BaudRatePrescaler = spi_baurate;              // Toc do SPI, lay tu tham so dau vao
    SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;                  // Truyen bit cao truoc (MSB first)
    SPI_InitStruct.SPI_CRCPolynomial = 7;                            // Gia tri CRC mac dinh (it dung)

    SPI_Init(SPI1, &SPI_InitStruct);   // Ap dung cau hinh cho SPI1
    SPI_Cmd(SPI1, ENABLE);             // Bat SPI1
}


/**
 * @brief Set the SPI speed by adjusting the baud rate prescaler
 * @param SPIx SPI peripheral (SPI1 or SPI2)
 * @param SPI_BaudRate Baud rate prescaler value
 */
void SPI1_SetSpeed(uint16_t spi_baurate) 
{
	SPI1->CR1 &= 0xFFC7;            		// Clear baud rate bits
	SPI1->CR1 |= spi_baurate;      			// Set new baud rate
	SPI_Cmd(SPI1, ENABLE);          	// Re-enable SPI1
}

// Function to transfer a byte via SPI and return received data
uint8_t SPI1_Transfer(uint8_t data)
{
	// Wait until the transmit buffer is empty
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
	
	// Send the data byte
	SPI_I2S_SendData(SPI1, data);

	// Wait until data is received in the receive buffer
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
	
	// Return the received data byte
	return SPI_I2S_ReceiveData(SPI1);
}
