#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// Định nghĩa cấu trúc để chứa các tham số cho LED
typedef struct {
    uint32_t frequency_hz;
    uint32_t duty_cycle_percent; // Độ rộng xung (0-100%)
} LedParameters_t;

// Khai báo các hàm của task
void TaskLedControl(void *pvParameters);
void TaskUpdateParameters(void *pvParameters);

// Khai báo handle cho queue
QueueHandle_t xParameterQueue;

// Hàm cấu hình GPIO
void GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    // Bật clock cho Port C
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    // Cấu hình Pin C13 là output push-pull
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

int main(void)
{
    // Cấu hình GPIO cho LED
    GPIO_Config();

    // Tạo queue để chứa 1 phần tử kiểu LedParameters_t
    xParameterQueue = xQueueCreate(1, sizeof(LedParameters_t));

    if (xParameterQueue != NULL)
    {
        // Tạo task điều khiển LED, ưu tiên 1
        xTaskCreate(TaskLedControl, "LedControl", 128, NULL, 1, NULL);
        // Tạo task cập nhật tham số, ưu tiên 2 (cao hơn)
        xTaskCreate(TaskUpdateParameters, "UpdateParams", 128, NULL, 2, NULL);

        // Bắt đầu bộ lập lịch
        vTaskStartScheduler();
    }

    // Vòng lặp vô tận (chương trình không bao giờ đến đây)
    while (1);
}

// Task điều khiển nhấp nháy LED với tần số và độ rộng xung thay đổi
void TaskLedControl(void *pvParameters)
{
    LedParameters_t current_params = {2, 50}; // Tham số mặc định: 2Hz, 50% duty cycle
    TickType_t period_ticks, on_time_ticks, off_time_ticks;
    BaseType_t xStatus;

    while (1)
    {
        // Tính toán thời gian cho một chu kỳ (tính bằng tick)
        if (current_params.frequency_hz == 0) current_params.frequency_hz = 1; // Tránh chia cho 0
        if (current_params.duty_cycle_percent > 100) current_params.duty_cycle_percent = 100;
        period_ticks = pdMS_TO_TICKS(1000 / current_params.frequency_hz);
        
        // Tính toán thời gian sáng (ON) và tắt (OFF)
        on_time_ticks = (period_ticks * current_params.duty_cycle_percent) / 100;
        off_time_ticks = period_ticks - on_time_ticks;

        // Bật LED (mức 0) và chờ trong khi lắng nghe queue
        if (on_time_ticks > 0)
        {
            GPIO_ResetBits(GPIOC, GPIO_Pin_13);
            xStatus = xQueueReceive(xParameterQueue, &current_params, on_time_ticks);
            if (xStatus == pdPASS) {
                // Nhận được tham số mới, bắt đầu lại chu kỳ ngay lập tức
                continue;
            }
        }

        // Tắt LED (mức 1) và chờ trong khi lắng nghe queue
        if (off_time_ticks > 0)
        {
            GPIO_SetBits(GPIOC, GPIO_Pin_13);
            xStatus = xQueueReceive(xParameterQueue, &current_params, off_time_ticks);
            if (xStatus == pdPASS) {
                // Nhận được tham số mới, bắt đầu lại chu kỳ ngay lập tức
                continue;
            }
        }
    }
}

// Task gửi các tham số (tần số và độ rộng xung)
void TaskUpdateParameters(void *pvParameters)
{
    // Mảng 2 chiều: {tần số (Hz), độ rộng xung (%)}
    const uint32_t parameters[][2] = {
        {2, 10},  // 2Hz, 10%
        {4, 30},  // 4Hz, 30%
        {6, 50},  // 6Hz, 50%
        {8, 70},  // 8Hz, 70%
        {10, 90}  // 10Hz, 90%
    };
    const uint8_t num_param_sets = sizeof(parameters) / sizeof(parameters[0]);
    uint8_t index = 0;
    LedParameters_t params_to_send;

    while (1)
    {
        // Chuẩn bị dữ liệu để gửi
        params_to_send.frequency_hz = parameters[index][0];
        params_to_send.duty_cycle_percent = parameters[index][1];

        // Gửi cấu trúc tham số vào queue, ghi đè nếu đã đầy
        xQueueOverwrite(xParameterQueue, &params_to_send);

        // Chuyển đến bộ tham số tiếp theo
        index++;
        if (index >= num_param_sets)
        {
            index = 0; // Quay lại từ đầu
        }

        // Chờ 5 giây
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

