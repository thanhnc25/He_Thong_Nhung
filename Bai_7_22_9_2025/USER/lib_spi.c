#include "lib_spi.h" 

void SPI_Setup(SPI_TypeDef* SPIx, uint16_t spi_baurate)
{
    SPI_InitTypeDef SPI_InitStruct;    
    GPIO_InitTypeDef GPIO_InitStruct;
    
    // Enable clock for Alternate Function I/O (AFIO)
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);    

    if (SPIx == SPI1) 
		{
        // Enable clock for GPIOA and SPI1 peripherals
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1, ENABLE);
        
        // Configure PA5 as Alternate Function Push-Pull (SCK for SPI1)
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5;           					// Select PA5 pin
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;     // Set mode to Alternate Function Push-Pull
        GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;   // Set GPIO speed to 50MHz
        GPIO_Init(GPIOA, &GPIO_InitStruct);             								// Apply configuration to GPIOA

        // Configure PA6 as Input Floating (MISO for SPI1)
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6;           									// Select PA6 pin
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING; 	// Set mode to Input Floating
        GPIO_Init(GPIOA, &GPIO_InitStruct);              											// Apply configuration to GPIOA

        // Configure PA7 as Alternate Function Push-Pull (MOSI for SPI1)
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_7;           					// Select PA7 pin
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;     // Set mode to Alternate Function Push-Pull
        GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz; 	// Set GPIO speed to 50MHz
        GPIO_Init(GPIOA, &GPIO_InitStruct);              							// Apply configuration to GPIOA
    }

    if (SPIx == SPI2)
		{
        // Enable clock for GPIOB and SPI2 peripherals
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
        
        // Configure PB13 as Alternate Function Push-Pull (SCK for SPI2)
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_13;          					// Select PB13 pin
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;     // Set mode to Alternate Function Push-Pull
        GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;   // Set GPIO speed to 50MHz
        GPIO_Init(GPIOB, &GPIO_InitStruct);             								// Apply configuration to GPIOB

        // Configure PB14 as Input Floating (MISO for SPI2)
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_14;          								// Select PB14 pin
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING; // Set mode to Input Floating
        GPIO_Init(GPIOB, &GPIO_InitStruct);              										// Apply configuration to GPIOB

        // Configure PB15 as Alternate Function Push-Pull (MOSI for SPI2)
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_15;          					// Select PB15 pin
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;     // Set mode to Alternate Function Push-Pull
        GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;   // Set GPIO speed to 50MHz
        GPIO_Init(GPIOB, &GPIO_InitStruct);              							// Apply configuration to GPIOB
    }
    
    // Configure SPI peripheral settings
    SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex; 	// Set SPI to full-duplex, 2-line mode
    SPI_InitStruct.SPI_Mode = SPI_Mode_Master;                      			// Set SPI to Master mode
    SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;                 	 			// Set data size to 8 bits
    SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;                         			// Clock polarity: idle low
    SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;                       		// Clock phase: data captured on first edge
    SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;                          					// NSS (Slave Select) managed by software
    SPI_InitStruct.SPI_BaudRatePrescaler = spi_baurate;             			// Set baud rate prescaler from input parameter
    SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;                 					// Data transmission starts with Most Significant Bit
    SPI_InitStruct.SPI_CRCPolynomial = 7;                           						// CRC polynomial value (default is 7)
    SPI_Init(SPIx, &SPI_InitStruct);                                									// Initialize SPI peripheral with settings
    SPI_Cmd(SPIx, ENABLE);                                          								// Enable the SPI peripheral
}

/**
 * @brief Set the SPI speed by adjusting the baud rate prescaler
 * @param SPIx SPI peripheral (SPI1 or SPI2)
 * @param SPI_BaudRate Baud rate prescaler value
 */
void SPIx_SetSpeed(SPI_TypeDef* SPIx, uint16_t spi_baurate) 
{
    assert_param(IS_SPI_BAUDRATE_PRESCALER(spi_baurate));
    if (SPIx == SPI1)
		{
        SPI1->CR1 &= 0xFFC7;            		// Clear baud rate bits
        SPI1->CR1 |= spi_baurate;      			// Set new baud rate
        SPI_Cmd(SPI1, ENABLE);          	// Re-enable SPI1
    } 
		else if (SPIx == SPI2) 
		{
        SPI2->CR1 &= 0xFFC7;            		// Clear baud rate bits
        SPI2->CR1 |= spi_baurate;      			// Set new baud rate
        SPI_Cmd(SPI2, ENABLE);          	// Re-enable SPI2
    }
}

// Function to transfer a byte via SPI and return received data
uint8_t SPIx_Transfer(SPI_TypeDef* SPIx, uint8_t data)
{
    // Wait until the transmit buffer is empty
    while (SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_TXE) == RESET);
    
    // Send the data byte
    SPI_I2S_SendData(SPIx, data);

    // Wait until data is received in the receive buffer
    while (SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_RXNE) == RESET);
    
    // Return the received data byte
    return SPI_I2S_ReceiveData(SPIx);
}
