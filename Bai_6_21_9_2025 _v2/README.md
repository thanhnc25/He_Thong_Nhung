# HƯỚNG DẪN GIAO TIẾP DS1307 VỚI STM32 QUA I2C

## 1. Giới thiệu giao tiếp I2C

I2C (Inter-Integrated Circuit) là giao thức truyền thông nối tiếp đồng bộ, sử dụng hai dây:
- **SCL** (Serial Clock Line): Dây xung nhịp do master phát ra.
- **SDA** (Serial Data Line): Dây truyền dữ liệu hai chiều.

I2C cho phép nhiều thiết bị (master/slave) kết nối trên cùng bus, mỗi thiết bị có địa chỉ riêng (7 bit hoặc 10 bit).

## 2. Cấu hình I2C trên STM32

### 2.1. Cấu hình chân I2C

Chọn chân SCL/SDA phù hợp với I2C1 hoặc I2C2. Ví dụ, I2C1 dùng PB6 (SCL) và PB7 (SDA):

```c
void I2Cx_PinOut(uint32_t Pinout)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    if (Pinout == I2C1_B67)
    {
        GPIO_I2C_InitStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    }
    // ... các cấu hình khác ...
    GPIO_I2C_InitStruct.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_I2C_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_I2C_InitStruct);
}
```
- Chế độ **AF_OD** (Alternate Function Open-Drain) là bắt buộc cho I2C.

### 2.2. Khởi tạo I2C

Sau khi cấu hình chân, tiến hành khởi tạo I2C với tốc độ mong muốn (ví dụ 100kHz):

```c
void I2Cx_Init(I2C_TypeDef *I2Cx, uint32_t Pinout, uint32_t ClockSpeed)
{
    // Bật clock, reset I2C, cấu hình các thông số
    // ...
    I2C_InitStruct.I2C_ClockSpeed = ClockSpeed;
    // ...
    I2C_Init(I2Cx, &I2C_InitStruct);
    I2C_Cmd(I2Cx, ENABLE);
}
```
- Hàm này sẽ được gọi trong `main.c`:
```c
I2Cx_Init(I2C1, I2C1_B67, 100000); // 100kHz
```

## 3. Giao tiếp DS1307 qua I2C

### 3.1. Địa chỉ và thanh ghi DS1307

- Địa chỉ DS1307: `0x68` (7 bit).
- Các thanh ghi thời gian: từ 0x00 (giây) đến 0x06 (năm).

### 3.2. Đọc 1 byte từ DS1307

Quy trình đọc 1 byte:
1. Gửi START.
2. Gửi địa chỉ DS1307 (ghi), gửi địa chỉ thanh ghi cần đọc.
3. Gửi lại START, gửi địa chỉ DS1307 (đọc).
4. Đọc dữ liệu, gửi STOP.

```c
u8 DS1307_Read(I2C_TypeDef *I2Cx, u8 RegData)
{
    u8 data = 0;
    I2C_GenerateSTART(I2Cx, ENABLE);
    while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT));
    I2C_Send7bitAddress(I2Cx, DS1307_ADDR, I2C_Direction_Transmitter);
    while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
    I2C_SendData(I2Cx, RegData);
    while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    I2C_GenerateSTART(I2Cx, ENABLE);
    while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT));
    I2C_Send7bitAddress(I2Cx, DS1307_ADDR, I2C_Direction_Receiver);
    while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED));
    data = I2C_ReceiveData(I2Cx);
    I2C_GenerateSTOP(I2C1, ENABLE);
    I2C_AcknowledgeConfig(I2C1, DISABLE);
    return data;
}
```

### 3.3. Đọc toàn bộ thời gian (7 thanh ghi)

Để đọc đủ thông tin thời gian (giây, phút, giờ, thứ, ngày, tháng, năm), cần đọc liên tiếp 7 thanh ghi:

```c
void DS1307_Get_Time_7reg(DS1307_Time_t *time)
{
    uint8_t data[7];
    uint8_t i;

    I2C_AcknowledgeConfig(DS1307_I2C, ENABLE);
    I2C_GenerateSTART(DS1307_I2C, ENABLE);
    while (!I2C_CheckEvent(DS1307_I2C, I2C_EVENT_MASTER_MODE_SELECT));
    I2C_Send7bitAddress(DS1307_I2C, DS1307_ADDR, I2C_Direction_Transmitter);
    while (!I2C_CheckEvent(DS1307_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
    I2C_SendData(DS1307_I2C, DS1307_SECONDS);
    while (!I2C_CheckEvent(DS1307_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    I2C_GenerateSTART(DS1307_I2C, ENABLE);
    while (!I2C_CheckEvent(DS1307_I2C, I2C_EVENT_MASTER_MODE_SELECT));
    I2C_Send7bitAddress(DS1307_I2C, DS1307_ADDR, I2C_Direction_Receiver);
    while (!I2C_CheckEvent(DS1307_I2C, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

    // Đọc 6 byte đầu với ACK
    for (i = 0; i < 6; i++)
    {
        while (!I2C_CheckEvent(DS1307_I2C, I2C_EVENT_MASTER_BYTE_RECEIVED));
        data[i] = I2C_ReceiveData(DS1307_I2C);
    }
    // Đọc byte cuối với NACK
    I2C_AcknowledgeConfig(DS1307_I2C, DISABLE);
    while (!I2C_CheckEvent(DS1307_I2C, I2C_EVENT_MASTER_BYTE_RECEIVED));
    data[6] = I2C_ReceiveData(DS1307_I2C);

    I2C_GenerateSTOP(DS1307_I2C, ENABLE);
    I2C_AcknowledgeConfig(DS1307_I2C, ENABLE);

    // Chuyển đổi BCD sang nhị phân
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
```

### 3.4. Ghi thời gian vào DS1307

Để cập nhật thời gian, cần ghi 7 byte liên tiếp vào các thanh ghi thời gian:

```c
void DS1307_Set_Time_7reg(const DS1307_Time_t *time)
{
    uint8_t data[7];
    if (!DS1307_IsValidTime(time))
        return;

    data[0] = BIN2BCD(time->seconds & 0x7F);
    data[1] = BIN2BCD(time->minutes & 0x7F);
    data[2] = BIN2BCD(time->hours & 0x3F);
    data[3] = BIN2BCD(time->day & 0x07);
    data[4] = BIN2BCD(time->date & 0x3F);
    data[5] = BIN2BCD(time->month & 0x1F);
    data[6] = BIN2BCD(time->year);

    I2C_GenerateSTART(DS1307_I2C, ENABLE);
    while (!I2C_CheckEvent(DS1307_I2C, I2C_EVENT_MASTER_MODE_SELECT));
    I2C_Send7bitAddress(DS1307_I2C, DS1307_ADDR, I2C_Direction_Transmitter);
    while (!I2C_CheckEvent(DS1307_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
    I2C_SendData(DS1307_I2C, DS1307_SECONDS);
    while (!I2C_CheckEvent(DS1307_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    for (uint8_t i = 0; i < 7; i++)
    {
        I2C_SendData(DS1307_I2C, data[i]);
        while (!I2C_CheckEvent(DS1307_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    }
    I2C_GenerateSTOP(DS1307_I2C, ENABLE);
}
```

### 3.5. Ứng dụng thực tế trong main.c

- Đọc thời gian DS1307 mỗi 100ms.
- Nếu phát hiện giá trị giây thay đổi, gửi UART.
- Nhận lệnh UART để cập nhật thời gian.

```c
int main(void)
{
    Timer2_Init();
    uart_Init();
    uart1_interrupt_init();
    I2Cx_Init(I2C1, I2C1_B67, 100000);

    DS1307_Time_t now;
    uint32_t last_read = 0;
    uint8_t last_second = 0xFF;

    while (1)
    {
        if (millis() - last_read >= 100)
        {
            last_read = millis();
            DS1307_Get_Time_7reg(&now);
            if (now.seconds != last_second)
            {
                char buf[64];
                snprintf(buf, sizeof(buf), "Time: %02d:%02d:%02d %02d/%02d/20%02d\r\n",
                         now.hours, now.minutes, now.seconds, now.date, now.month, now.year);
                uart_SendStr(buf);
                last_second = now.seconds;
            }
        }
        if (uart_rx_ready)
        {
            uart_rx_ready = 0;
            // SET 22:09:56 20/09/25
            parse_and_set_time((char*)uart_rx_buf);
        }
    }
}
```

## 4. Tổng kết

- Đã cấu hình I2C trên STM32, chọn chân, tốc độ.
- Đã xây dựng hàm đọc/ghi DS1307 qua I2C đúng chuẩn.
- Ứng dụng thực tế: Đọc thời gian, gửi UART, cập nhật thời gian qua lệnh UART.

---

**Tài liệu tham khảo:**  
- [DS1307 Datasheet](https://datasheets.maximintegrated.com/en/ds/DS1307.pdf)  
- [STM32 Reference Manual](https://www.st.com/resource/en/reference_manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf)