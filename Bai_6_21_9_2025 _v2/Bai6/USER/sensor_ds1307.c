#include "sensor_ds1307.h"

u8 DS1307_Read(I2C_TypeDef *I2Cx, u8 RegData)
{
  u8 data = 0;
  /* Send START condition */
  I2C_GenerateSTART(I2Cx, ENABLE);
  /* Test on EV5 and clear it */
  while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT))
    ;
  /* Send PCF8574A address for write */
  I2C_Send7bitAddress(I2Cx, DS1307_ADDR, I2C_Direction_Transmitter);
  /* Test on EV6 and clear it */
  while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    ;
  /* Send the data byte to be written */
  I2C_SendData(I2Cx, RegData);
  /* Test on EV8 and clear it */
  while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    ;
  /* Send START condition */
  I2C_GenerateSTART(I2Cx, ENABLE);
  /* Test on EV5 and clear it */
  while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT))
    ;
  I2C_Send7bitAddress(I2Cx, DS1307_ADDR, I2C_Direction_Receiver);
  while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED))
    ;
  data = I2C_ReceiveData(I2Cx);
  I2C_GenerateSTOP(I2C1, ENABLE);
  I2C_AcknowledgeConfig(I2C1, DISABLE);
  return data;
}

void DS1307_GetDateTime(DS1307_Time_t *time)
{
  unsigned char i, data[7];
  // TM_I2C_ReadMulti(DS1307_I2C, TM_DS1307_I2C_ADDR, TM_DS1307_SECONDS, data, 7);
  for (i = 0; i < 8; i++)
  {
    data[i] = DS1307_Read(DS1307_I2C, i);
  }

  time->seconds = BCD2BIN(data[0] &= 0x7F);

  time->minutes = BCD2BIN(data[1] &= 0x7F);

  if ((data[1] & 0x40) != 0)
    time->hours = BCD2BIN(data[2] &= 0x1F);
  else
    time->hours = BCD2BIN(data[2] &= 0x3F);

  time->day = BCD2BIN(data[3] &= 0x07);
  time->date = BCD2BIN(data[4] &= 0x3F);
  time->month = BCD2BIN(data[5] &= 0x1F);
  time->year = BCD2BIN(data[6] &= 0xFF);
}

void DS1307_SetTime(DS1307_Time_t *time)
{
  uint8_t data[7];
  u8 i;
  data[0] = BIN2BCD(time->seconds);
  data[1] = BIN2BCD(time->minutes);
  data[2] = BIN2BCD(time->hours);
  data[3] = BIN2BCD(time->day);
  data[4] = BIN2BCD(time->date);
  data[5] = BIN2BCD(time->month);
  data[6] = BIN2BCD(time->year);

  I2C_GenerateSTART(DS1307_I2C, ENABLE);
  /* Test on EV5 and clear it */
  while (!I2C_CheckEvent(DS1307_I2C, I2C_EVENT_MASTER_MODE_SELECT))
    ;
  /* Send PCF8574A address for write */
  I2C_Send7bitAddress(DS1307_I2C, DS1307_ADDR, I2C_Direction_Transmitter);
  /* Test on EV6 and clear it */
  while (!I2C_CheckEvent(DS1307_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    ;
  /* Send the data byte to be written */
  I2C_SendData(DS1307_I2C, DS1307_SECONDS);
  /* Test on EV8 and clear it */
  while (!I2C_CheckEvent(DS1307_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    ;
  //	/* Send START condition */
  //  I2C_GenerateSTART(DS1307_I2C,ENABLE);
  //	/* Test on EV5 and clear it */
  //  while (!I2C_CheckEvent(DS1307_I2C, I2C_EVENT_MASTER_MODE_SELECT));
  for (i = 0; i < 7; i++)
  {
    I2C_SendData(DS1307_I2C, data[i]);
    /* Test on EV8 and clear it */
    while (!I2C_CheckEvent(DS1307_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
      ;
  }

  /* Test on EV8 and clear it */
  while (!I2C_CheckEvent(DS1307_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    ;
  I2C_GenerateSTOP(DS1307_I2C, ENABLE);
}

unsigned char DS1307_GetSeconds(void)
{
  unsigned char sec;
  sec = DS1307_Read(DS1307_I2C, 0x00); // dia chi de doc gia tri thanh ghi giay
  sec = BCD2BIN(sec &= 0x7F);          // chuyen doi nguoc tu so BCd sang nhi phan
  return sec;
}
unsigned char DS1307_GetMinutes(void)
{
  unsigned char min;
  min = DS1307_Read(DS1307_I2C, 0x01);
  min = BCD2BIN(min &= 0x7F);
  return min;
}
unsigned char DS1307_GetHours(void)
{
  unsigned char hour;
  hour = DS1307_Read(DS1307_I2C, 0x02);
  if ((hour & 0x40) != 0)
    hour = BCD2BIN(hour &= 0x1F);
  else
    hour = BCD2BIN(hour &= 0x3F);
  return hour;
}
unsigned char DS1307_GetDay(void)
{
  unsigned char day;
  day = DS1307_Read(DS1307_I2C, 0x03);
  day = BCD2BIN(day &= 0x07);
  return day;
}
unsigned char DS1307_GetDate(void)
{
  unsigned char date;
  date = DS1307_Read(DS1307_I2C, 0x04);
  date = BCD2BIN(date &= 0x3F);
  return date;
}
unsigned char DS1307_GetMonth(void)
{
  unsigned char month;
  month = DS1307_Read(DS1307_I2C, 0x05);
  month = BCD2BIN(month &= 0x1F);
  return month;
}
unsigned char DS1307_GetYear(void)
{
  unsigned char year;
  year = DS1307_Read(DS1307_I2C, 0x06);
  year = BCD2BIN(year &= 0xFF);
  return year;
}

int DS1307_HexToDec(int Hex)
{
  return (Hex % 10 + ((Hex / 10) << 4));
}

unsigned char BCD2BIN(unsigned char data)
{
  unsigned char high, low;
  high = (data >> 4) & 0x0F;
  low = data & 0x0F;
  return ((high * 10) + low);
}
unsigned char BIN2BCD(unsigned char data)
{
  unsigned char high, low;
  high = data / 10;
  high = (high << 4) & 0xF0;
  low = data % 10;
  low &= 0x0F;
  return ((high) | low);
}

// Đọc liền 7 thanh ghi thời gian DS1307, trả về qua struct time
void DS1307_Get_Time_7reg(DS1307_Time_t *time)
{
  uint8_t data[7];
  uint8_t i;

  // Bắt đầu giao tiếp I2C
  I2C_AcknowledgeConfig(DS1307_I2C, ENABLE); // Bật ACK trước khi đọc nhiều byte
  I2C_GenerateSTART(DS1307_I2C, ENABLE);
  while (!I2C_CheckEvent(DS1307_I2C, I2C_EVENT_MASTER_MODE_SELECT))
    ;
  // Gửi địa chỉ DS1307 (ghi)
  I2C_Send7bitAddress(DS1307_I2C, DS1307_ADDR, I2C_Direction_Transmitter);
  while (!I2C_CheckEvent(DS1307_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    ;
  // Gửi địa chỉ thanh ghi bắt đầu (giây)
  I2C_SendData(DS1307_I2C, DS1307_SECONDS);
  while (!I2C_CheckEvent(DS1307_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    ;
  // Gửi lại START để chuyển sang chế độ đọc
  I2C_GenerateSTART(DS1307_I2C, ENABLE);
  while (!I2C_CheckEvent(DS1307_I2C, I2C_EVENT_MASTER_MODE_SELECT))
    ;
  // Gửi địa chỉ DS1307 (đọc)
  I2C_Send7bitAddress(DS1307_I2C, DS1307_ADDR, I2C_Direction_Receiver);
  while (!I2C_CheckEvent(DS1307_I2C, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
    ;

  // Đọc 6 byte đầu (giây, phút, giờ, thứ, ngày, tháng) với ACK
  for (i = 0; i < 6; i++)
  {
    while (!I2C_CheckEvent(DS1307_I2C, I2C_EVENT_MASTER_BYTE_RECEIVED))
      ;
    data[i] = I2C_ReceiveData(DS1307_I2C);
  }
  // Đọc byte cuối (năm) với NACK
  I2C_AcknowledgeConfig(DS1307_I2C, DISABLE); // Tắt ACK trước khi nhận byte cuối
  while (!I2C_CheckEvent(DS1307_I2C, I2C_EVENT_MASTER_BYTE_RECEIVED))
    ;
  data[6] = I2C_ReceiveData(DS1307_I2C);

  // Kết thúc giao tiếp
  I2C_GenerateSTOP(DS1307_I2C, ENABLE);

  // Bật lại ACK cho I2C nếu cần dùng tiếp
  I2C_AcknowledgeConfig(DS1307_I2C, ENABLE);

  // Chuyển đổi dữ liệu BCD sang BIN
  time->seconds = BCD2BIN(data[0] & 0x7F);
  time->minutes = BCD2BIN(data[1] & 0x7F);
  if ((data[2] & 0x40) != 0)
    time->hours = BCD2BIN(data[2] & 0x1F);
  else
    time->hours = BCD2BIN(data[2] & 0x3F);
  time->day = BCD2BIN(data[3] & 0x07);
  time->date = BCD2BIN(data[4] & 0x3F);
  time->month = BCD2BIN(data[5] & 0x1F);
  time->year = BCD2BIN(data[6]);
}

uint8_t DS1307_IsValidTime(const DS1307_Time_t *t)
{
  if (t->seconds > 59)
    return 0;
  if (t->minutes > 59)
    return 0;
  if (t->hours > 23)
    return 0;
  if (t->day < 1 || t->day > 7)
    return 0;
  if (t->date < 1 || t->date > 31)
    return 0;
  if (t->month < 1 || t->month > 12)
    return 0;
  if (t->year > 99)
    return 0;
  return 1;
}

void DS1307_Set_Time_7reg(const DS1307_Time_t *time)
{
  uint8_t data[7];
  if (!DS1307_IsValidTime(time))
    return; // Không hợp lệ thì bỏ qua

  data[0] = BIN2BCD(time->seconds & 0x7F);
  data[1] = BIN2BCD(time->minutes & 0x7F);
  data[2] = BIN2BCD(time->hours & 0x3F); // 24h mode
  data[3] = BIN2BCD(time->day & 0x07);
  data[4] = BIN2BCD(time->date & 0x3F);
  data[5] = BIN2BCD(time->month & 0x1F);
  data[6] = BIN2BCD(time->year);

  I2C_GenerateSTART(DS1307_I2C, ENABLE);
  while (!I2C_CheckEvent(DS1307_I2C, I2C_EVENT_MASTER_MODE_SELECT))
    ;
  I2C_Send7bitAddress(DS1307_I2C, DS1307_ADDR, I2C_Direction_Transmitter);
  while (!I2C_CheckEvent(DS1307_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    ;
  I2C_SendData(DS1307_I2C, DS1307_SECONDS);
  while (!I2C_CheckEvent(DS1307_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    ;
  for (uint8_t i = 0; i < 7; i++)
  {
    I2C_SendData(DS1307_I2C, data[i]);
    while (!I2C_CheckEvent(DS1307_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
      ;
  }
  I2C_GenerateSTOP(DS1307_I2C, ENABLE);
}