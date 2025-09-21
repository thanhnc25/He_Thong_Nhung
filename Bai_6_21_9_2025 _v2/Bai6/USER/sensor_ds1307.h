#ifndef _SENSOR_DS1307_H_
#define _SENSOR_DS1307_H_

#include "stm32f10x_i2c.h"

#define DS1307_ADDR 0xD0
#define DS1307_I2C I2C1
// Registers location
#define DS1307_SECONDS 0x00
#define DS1307_MINUTES 0x01
#define DS1307_HOURS 0x02
#define DS1307_DAY 0x03
#define DS1307_DATE 0x04
#define DS1307_MONTH 0x05
#define DS1307_YEAR 0x06
#define DS1307_CONTROL 0x07

// Bits in control register
#define TM_DS1307_CONTROL_OUT 7
#define TM_DS1307_CONTROL_SQWE 4
#define TM_DS1307_CONTROL_RS1 1
#define TM_DS1307_CONTROL_RS0 0

typedef struct
{
   uint8_t seconds; // Seconds,          00-59
   uint8_t minutes; // Minutes,          00-59
   uint8_t hours;   // Hours,         		00-23
   uint8_t day;     // Day in a week,    1-7
   uint8_t date;    // Day in a month   	1-31
   uint8_t month;   // Month,         		1-12
   uint8_t year;    // Year            	00-99
} DS1307_Time_t;

unsigned char DS1307_GetSeconds(void);
unsigned char DS1307_GetMinutes(void);
unsigned char DS1307_GetHours(void);
unsigned char DS1307_GetDay(void);
unsigned char DS1307_GetDate(void);
unsigned char DS1307_GetMonth(void);
unsigned char DS1307_GetYear(void);

void DS1307_Get_Time_7reg(DS1307_Time_t *time);

u8 DS1307_Read(I2C_TypeDef *I2Cx, u8 RegData);
void DS1307_SetTime(DS1307_Time_t *time);

/* ONLY READ TIME  */
unsigned char BIN2BCD(unsigned char data);
unsigned char BCD2BIN(unsigned char data);

#endif
