#include <stdio.h>
#include "i2c_manager.h"
#include "ssd1306.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"

// void app_main(void)
// {
   

//     I2C_SSD1306_update_display(ssd1306);
//     vTaskDelay(4000/portTICK_PERIOD_MS);

//     I2C_SSD1306_buffer_clear(ssd1306);
//     I2C_SSD1306_buffer_text_on(ssd1306, 12, 0, "Hello, world!", true);
//     I2C_SSD1306_buffer_fill_space(ssd1306, 0, 9, 127, 10, true);
//     I2C_SSD1306_buffer_text_on(ssd1306, 0, 11, "ABCDEFGHIJKLMNOP", false);
//     I2C_SSD1306_update_display(ssd1306);
//     vTaskDelay(4000/portTICK_PERIOD_MS);
//     I2C_SSD1306_buffer_clear(ssd1306);
// }


// // Cấu hình UART
// #define UART_NUM UART_NUM_1
// #define BUF_SIZE (1024)
// #define TXD_PIN (17)
// #define RXD_PIN (16)

// // Cấu hình GPIO cho nút nhấn
// #define BUTTON_PIN (0)  // Thay đổi theo chân GPIO thực tế

// // Biến toàn cục để điều khiển trạng thái tạm dừng
// volatile bool paused = false;

// // Hàm xử lý sự kiện nút nhấn
// void IRAM_ATTR button_isr_handler(void* arg) {
//     paused = !paused;
// }

// // Hàm gửi bản tin qua UART
// void uart_send_task(void* arg) {
//     uint8_t data[BUF_SIZE];
//     while (1) {
//         if (!paused) {
//             len = uart_read_bytes(UART_NUM, data, BUF_SIZE - 1, pdMS_TO_TICKS(1000));
//         }
//         vTaskDelay(1000 / portTICK_PERIOD_MS);  // Gửi bản tin mỗi giây
//     }
// }

// // Hàm hiển thị thông tin trên OLED
// void oled_display_task(void* arg) {
//     ssd1306_handle_t oled = (ssd1306_handle_t)arg;
//     while (1) {
//         if (paused) {
//             I2C_SSD1306_buffer_clear(ssd1306);
//             I2C_SSD1306_buffer_text_on(ssd1306, 0, 10, "PAUSE ", false);
//         } else {
//             I2C_SSD1306_buffer_clear(ssd1306);
//             I2C_SSD1306_buffer_text_on(ssd1306, 0, 10, "PAUSE ", false);
//         }
//         //ssd1306_refresh_gram(oled);
//         vTaskDelay(500 / portTICK_PERIOD_MS);  // Cập nhật mỗi nửa giây
//     }
// }

// void app_main() {
//        //  Cấu hình I2C và Oled
//     // Cấu hình UART
//     uart_config_t uart_config = {
//         .baud_rate = 115200,
//         .data_bits = UART_DATA_8_BITS,
//         .parity = UART_PARITY_DISABLE,
//         .stop_bits = UART_STOP_BITS_1,
//         .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
//     };
//     uart_param_config(UART_NUM, &uart_config);
//     uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
//     uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);

//     // Cấu hình GPIO cho nút nhấn
//     gpio_config_t io_conf = {
//         .intr_type = GPIO_INTR_POSEDGE,
//         .mode = GPIO_MODE_INPUT,
//         .pin_bit_mask = (1ULL << BUTTON_PIN),
//         .pull_down_en = 0,
//         .pull_up_en = 1,
//     };
//     gpio_config(&io_conf);
//     gpio_install_isr_service(0);
//     gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, NULL);

//     //ssd1306_handle_t oled = ssd1306_create(&oled_config);
//     //ssd1306_clear_screen(oled, 0x00);

//     // Tạo các nhiệm vụ FreeRTOS
//     xTaskCreate(uart_send_task, "uart_send_task", 2048, NULL, 10, NULL);
//     xTaskCreate(oled_display_task, "oled_display_task", 2048, oled, 10, NULL);
// }


// Cấu hình UART
#define UART_NUM            UART_NUM_1
#define BUF_SIZE            1024
#define TXD_PIN             (GPIO_NUM_35)
#define RXD_PIN             (GPIO_NUM_34)
// Cấu hình nút
#define BUTTON_PIN          GPIO_NUM_0  // Nút nhấn được gán vào GPIO 0
#define BUTTON_ACTIVE_LEVEL 0           // Mức logic khi nút được nhấn

// Trạng thái tạm dừng
static bool paused = false;

// Hàm xử lý nút nhấn
static void IRAM_ATTR button_isr_handler(void* arg) {
    paused = !paused;  // Đổi trạng thái giữa tạm dừng và tiếp tục
}

void init_uart() {
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
}

void init_button() {
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_NEGEDGE;   // Kích hoạt ngắt khi nút được nhấn (mức thấp)
    io_conf.mode = GPIO_MODE_INPUT;          // Đặt chế độ GPIO là input
    io_conf.pin_bit_mask = (1ULL << BUTTON_PIN);
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE; // Kích hoạt pull-up
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);

    // Đăng ký xử lý ngắt cho nút
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, NULL);
}

void app_main() {
    // Cấu hình UART
    init_uart();

    // Cấu hình nút
    init_button();

    I2C_MANAGER_t* i2c_master = I2C_MANAGER_master_init(I2C_NUM_0, GPIO_NUM_21, GPIO_NUM_22);
    I2C_SSD1306_t* ssd1306 = I2C_SSD1306_init(i2c_master, 0x3C, 128, 64, SSD1306_BOTTOMTOTOP);

    uint8_t data[BUF_SIZE];


    while (1) {
        if (paused) {
            // Nếu đang tạm dừng, hiển thị chữ "PAUSE"
            I2C_SSD1306_buffer_clear(ssd1306);// Xóa màn hình
            I2C_SSD1306_buffer_text_on(ssd1306, 0, 10, "PAUSE ", false);
            vTaskDelay(pdMS_TO_TICKS(500));  // Đợi để giảm tải CPU
        } else {
            // Khi không tạm dừng, nhận dữ liệu từ UART
            int len = uart_read_bytes(UART_NUM, data, BUF_SIZE - 1, pdMS_TO_TICKS(1000));
            if (len > 0) {
                data[len] = '\0';  // Kết thúc chuỗi nhận được

                // In dữ liệu ra màn hình OLED
                I2C_SSD1306_buffer_clear(ssd1306);
                I2C_SSD1306_buffer_int_on(ssd1306, 0, 10, data, false);
            }
        }
    }
}
