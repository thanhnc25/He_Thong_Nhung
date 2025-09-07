# Tạo project Keil C

## 1. Chuẩn bị thư mục dự án

Trước khi tạo project, nên tạo sẵn một **folder chính** để lưu trữ.  
Bên trong folder dự án chia thành 2 thư mục con:

- **MDK**: Lưu file cấu hình của KeilC.  
- **USER**: Lưu code nguồn do người dùng viết.  

## 2. Tạo Project trong KeilC

1. Mở **KeilC**.  
2. Chọn `Project -> New uVision Project`.  
3. Chọn đường dẫn đến folder **MDK**, đặt tên file `.uvprojx`.  
4. Chọn **Device** (ví dụ: `STM32F103C8`).  
5. Tick chọn:
   - **Startup**  
   - Các thư viện driver cần thiết (GPIO, RCC, …).  
6. Nhấn **Resolve** và chọn **OK**.

---

## 3. Cấu trúc source code

Trong project, tạo 2 group để quản lý code:  

- **incl/**: Chứa file header `.h` (ví dụ: `dht11.h`, `delay.h`, …).  
- **src/**: Chứa file nguồn `.c` (ví dụ: `dht11.c`, `delay.c`, `main.c`, …).  

Ví dụ trong `main.c`:
```c
#include "test.h"

int main(void) {
    test();   // Gọi hàm test
    while(1);
}