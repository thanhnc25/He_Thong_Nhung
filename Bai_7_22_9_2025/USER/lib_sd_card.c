#include "lib_sd_card.h"

SD_CardInfo sd_card_info;
static SPI_TypeDef* active_spi = NULL;

static void SD_CS_Low(SPI_TypeDef* SPIx);
static void SD_CS_High(SPI_TypeDef* SPIx);
static uint8_t SD_WaitReady(SPI_TypeDef* SPIx);
static uint8_t SD_SendCmd(SPI_TypeDef* SPIx, uint8_t cmd, uint32_t arg, uint8_t crc);
static uint8_t SD_RecvData(SPI_TypeDef* SPIx, uint8_t* buf, uint16_t len);
static uint8_t SD_SendBlock(SPI_TypeDef* SPIx, uint8_t* buf, uint8_t cmd);

static void SD_CS_Low(SPI_TypeDef* SPIx)
{
    if (SPIx == SPI1)
		{
					GPIO_ResetBits(SD_CS_PORT_SPI1, SD_CS_PIN_SPI1);
		}
    else if (SPIx == SPI2) 
		{
					GPIO_ResetBits(SD_CS_PORT_SPI2, SD_CS_PIN_SPI2);
		}
}

static void SD_CS_High(SPI_TypeDef* SPIx) 
{
    if (SPIx == SPI1)
		{
					GPIO_SetBits(SD_CS_PORT_SPI1, SD_CS_PIN_SPI1);
		}
    else if (SPIx == SPI2) 
		{
					GPIO_SetBits(SD_CS_PORT_SPI2, SD_CS_PIN_SPI2);
		}
}

static uint8_t SD_WaitReady(SPI_TypeDef* SPIx) 
{
    uint32_t t = 0;
    do 
		{
        if (SPIx_Transfer(SPIx, 0xFF) == 0xFF) return 0;
        t++;
    } 
		while (t < 0xFFFF);
    return 1;
}

static uint8_t SD_SendCmd(SPI_TypeDef* SPIx, uint8_t cmd, uint32_t arg, uint8_t crc) 
{
    uint8_t r1, retry = 0x1F;
    SD_CS_High(SPIx);
    SPIx_Transfer(SPIx, 0xFF);
    SD_CS_Low(SPIx);
    if (SD_WaitReady(SPIx)) 
		{
				return 0xFF;
		}

    SPIx_Transfer(SPIx, cmd | 0x40);
    SPIx_Transfer(SPIx, arg >> 24);
    SPIx_Transfer(SPIx, arg >> 16);
    SPIx_Transfer(SPIx, arg >> 8);
    SPIx_Transfer(SPIx, arg);
    SPIx_Transfer(SPIx, crc);

    do 
		{
        r1 = SPIx_Transfer(SPIx, 0xFF);
    } 
		while ((r1 & 0x80) && retry--);

    return r1;
}

static uint8_t SD_RecvData(SPI_TypeDef* SPIx, uint8_t* buf, uint16_t len) 
{
    uint16_t t = 0xFFFF;
    while (SPIx_Transfer(SPIx, 0xFF) != 0xFE && t--);
    if (t == 0) 
		{
				return 1;
		}

    while (len--) 
		{
				*buf++ = SPIx_Transfer(SPIx, 0xFF);
		}
		
    SPIx_Transfer(SPIx, 0xFF); 
    SPIx_Transfer(SPIx, 0xFF);
    return 0;
}

static uint8_t SD_SendBlock(SPI_TypeDef* SPIx, uint8_t* buf, uint8_t cmd) 
{
    if (SD_WaitReady(SPIx)) 
		{
				return 1;
		}
		
    SPIx_Transfer(SPIx, cmd);
    if (cmd != 0xFD) 
		{
        for (uint16_t t = 0; t < SD_BLOCK_SIZE; t++) 
				{
						SPIx_Transfer(SPIx, buf[t]);
				}
				
        SPIx_Transfer(SPIx, 0xFF); 
        SPIx_Transfer(SPIx, 0xFF);
        uint8_t resp = SPIx_Transfer(SPIx, 0xFF);
        if ((resp & 0x1F) != 0x05) 
				{
						return 2;
				}
    }
    return 0;
}

char SD_Init(SPI_TypeDef* SPIx) 
{
    active_spi = SPIx;
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (SPIx == SPI1) 
		{
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
        GPIO_InitStruct.GPIO_Pin = SD_CS_PIN_SPI1;
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
        GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_Init(SD_CS_PORT_SPI1, &GPIO_InitStruct);
    } 
		else if (SPIx == SPI2) 
		{
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
        GPIO_InitStruct.GPIO_Pin = SD_CS_PIN_SPI2;
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
        GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_Init(SD_CS_PORT_SPI2, &GPIO_InitStruct);
    }

    SD_CS_High(SPIx);
    for (uint8_t i = 0; i < 10; i++) 
		{
				SPIx_Transfer(SPIx, 0xFF);
		}

    uint8_t retry = 0, r1;
    do 
		{
        r1 = SD_SendCmd(SPIx, CMD0, 0, 0x95);
    } 
		while (r1 != MSD_IN_IDLE_STATE && retry++ < 200);

    if (retry >= 200) return 1;

    r1 = SD_SendCmd(SPIx, CMD8, 0x1AA, 0x87);
    if (r1 == MSD_IN_IDLE_STATE) 
		{
        sd_card_info.type = SD_TYPE_V2;
        uint8_t ocr[4];
        for (uint8_t i = 0; i < 4; i++) 
				{
						ocr[i] = SPIx_Transfer(SPIx, 0xFF);
				}
				
        if (ocr[2] == 0x01 && ocr[3] == 0xAA) 
				{
            retry = 0;
            do 
						{
                SD_SendCmd(SPIx, CMD55, 0, 0x01);
                r1 = SD_SendCmd(SPIx, CMD41, 0x40000000, 0x01);
            } 
						while (r1 != 0x00 && retry++ < 200);
						
            if (retry >= 200) 
						{
								return 2;
						}

            r1 = SD_SendCmd(SPIx, CMD58, 0, 0x01);
            if (r1 == 0x00) 
						{
                for (uint8_t i = 0; i < 4; i++) ocr[i] = SPIx_Transfer(SPIx, 0xFF);
                if (ocr[0] & 0x40) sd_card_info.type = SD_TYPE_V2HC;
            }
        }
    } 
		else 
		{
        retry = 0;
        do 
				{
            r1 = SD_SendCmd(SPIx, CMD1, 0, 0x01);
        } 
				while (r1 != 0x00 && retry++ < 200);
				
        if (retry >= 200) 
				{
						return 3;
				}
        sd_card_info.type = SD_TYPE_V1;
    }

    SD_CS_High(SPIx);
    SPIx_Transfer(SPIx, 0xFF);
    return 0;
}

uint32_t SD_GetSectorCount(SPI_TypeDef* SPIx) 
{
    uint8_t csd[16];
    if (SD_SendCmd(SPIx, CMD9, 0, 0x01) != 0 || SD_RecvData(SPIx, csd, 16) != 0) 
		{
				return 0;
		}

    uint32_t capacity;
    if ((csd[0] & 0xC0) == 0x40) 
		{ 
				// SDHC
        uint16_t csize = csd[9] + ((uint16_t)csd[8] << 8) + 1;
        capacity = (uint32_t)csize << 10;
    } 
		else 
		{ 
				// SDSC
        uint8_t n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
        uint16_t csize = (csd[8] >> 6) + ((uint16_t)csd[7] << 2) + ((uint16_t)(csd[6] & 3) << 10) + 1;
        capacity = (uint32_t)csize << (n - 9);
    }
		
    sd_card_info.capacity = capacity;
    SD_CS_High(SPIx);
    return capacity;
}

uint8_t SD_ReadSector(SPI_TypeDef* SPIx, uint32_t sector, uint8_t* buffer, uint8_t cnt) 
{
    uint8_t r1;
    if (sd_card_info.type != SD_TYPE_V2HC) sector <<= 9;

    if (cnt == 1) 
		{
        r1 = SD_SendCmd(SPIx, CMD17, sector, 0x01);
        if (r1 == 0) 
				{
						r1 = SD_RecvData(SPIx, buffer, SD_BLOCK_SIZE);
				}
    } 
		else 
		{
        r1 = SD_SendCmd(SPIx, CMD18, sector, 0x01);
        do 
				{
            r1 = SD_RecvData(SPIx, buffer, SD_BLOCK_SIZE);
            buffer += SD_BLOCK_SIZE;
        } 
				while (--cnt && r1 == 0);
				
        SD_SendCmd(SPIx, CMD12, 0, 0x01);
    }
		
    SD_CS_High(SPIx);
    return r1;
}

uint8_t SD_WriteSector(SPI_TypeDef* SPIx, uint32_t sector, uint8_t* buffer, uint8_t cnt) 
{
    uint8_t r1;
    if (sd_card_info.type != SD_TYPE_V2HC) 
		{
				sector <<= 9;
		}

    if (cnt == 1) 
		{
        r1 = SD_SendCmd(SPIx, CMD24, sector, 0x01);
        if (r1 == 0) 
				{
						r1 = SD_SendBlock(SPIx, buffer, 0xFE);
				}
    } 
		else 
		{
        if (sd_card_info.type != SD_TYPE_MMC) 
				{
            SD_SendCmd(SPIx, CMD55, 0, 0x01);
            SD_SendCmd(SPIx, CMD23, cnt, 0x01);
        }
				
        r1 = SD_SendCmd(SPIx, CMD25, sector, 0x01);
        if (r1 == 0) 
				{
            do 
						{
                r1 = SD_SendBlock(SPIx, buffer, 0xFC);
                buffer += SD_BLOCK_SIZE;
            } 
						while (--cnt && r1 == 0);
            r1 = SD_SendBlock(SPIx, 0, 0xFD);
        }
    }
    SD_CS_High(SPIx);
    return r1;
}

char SD_Mount(FATFS* fs_ptr)
{
    return f_mount(0, fs_ptr) == FR_OK ? 0 : 1;  
}

char SD_Unmount(void) 
{
    return f_mount(0, NULL) == FR_OK ? 0 : 1;   
}

char SD_WriteFile(const char* filename, const char* data) 
{
    FIL file;
    UINT bw;
    FRESULT res = f_open(&file, filename, FA_WRITE | FA_CREATE_ALWAYS);
    if (res != FR_OK) return 1;

    res = f_write(&file, data, strlen(data), &bw);
    f_close(&file);
    return res == FR_OK ? 0 : 1;
}

char SD_ReadFile(const char* filename, unsigned char* buffer, UINT* bytesRead) 
{  
    FIL file;
    FRESULT res = f_open(&file, filename, FA_READ);
    if (res != FR_OK) return 1;

    res = f_read(&file, buffer, SD_BLOCK_SIZE, bytesRead);
    f_close(&file);
    return res == FR_OK ? 0 : 1;
}

DSTATUS SD_disk_initialize(BYTE pdrv) 
{
    return SD_Init(active_spi) != 0 ? STA_NOINIT : 0; 
}

DSTATUS SD_disk_status(BYTE pdrv) 
{
    return sd_card_info.type == SD_TYPE_ERR ? STA_NOINIT : 0;
}

DRESULT SD_disk_read(BYTE pdrv, BYTE* buff, DWORD sector, UINT count) 
{
    return SD_ReadSector(active_spi, sector, buff, count) == 0 ? RES_OK : RES_ERROR;
}

DRESULT SD_disk_write(BYTE pdrv, const BYTE* buff, DWORD sector, UINT count) 
{
    return SD_WriteSector(active_spi, sector, (uint8_t*)buff, count) == 0 ? RES_OK : RES_ERROR;
}

DRESULT SD_disk_ioctl(BYTE pdrv, BYTE cmd, void* buff) 
{
    switch (cmd) 
		{
        case CTRL_SYNC:
            return RES_OK;
				
        case GET_SECTOR_COUNT:
            *(DWORD*)buff = SD_GetSectorCount(active_spi);
            return RES_OK;
				
        case GET_SECTOR_SIZE:
            *(WORD*)buff = SD_BLOCK_SIZE;
            return RES_OK;
				
        case GET_BLOCK_SIZE:
            *(DWORD*)buff = 1;
            return RES_OK;
				
        default:
            return RES_PARERR;
    }
}
