#include <stdio.h>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "ssd1306.h"  // Thư viện OLED SSD1306
#include "font8x8_basic.h"  // Font chữ

#define UART_PORT_NUM UART_NUM_1
#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)
#define BUTTON_PIN GPIO_NUM_0  // Pin nút nhấn
#define I2C_MASTER_SCL_IO 22   // SCL pin
#define I2C_MASTER_SDA_IO 21   // SDA pin
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 100000

// Semaphore để tạm dừng chương trình
SemaphoreHandle_t xSemaphorePause;

// Khởi tạo UART
void uart_init() {
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_driver_install(UART_PORT_NUM, 1024 * 2, 0, 0, NULL, 0);
    uart_param_config(UART_PORT_NUM, &uart_config);
    uart_set_pin(UART_PORT_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

// Khởi tạo I2C cho OLED
void i2c_master_init() {
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &i2c_config);
    i2c_driver_install(I2C_MASTER_NUM, i2c_config.mode, 0, 0, 0);
}

// Hàm hiển thị chuỗi ký tự lên OLED
void oled_display_message(SSD1306_t *oled, char *message) {
    ssd1306_clear_screen(oled, false);
    ssd1306_display_text(oled, 0, message, strlen(message), false);
}

// Task kiểm tra nút nhấn
void button_task(void *arg) {
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_pulldown_en(BUTTON_PIN);
    
    while (1) {
        if (gpio_get_level(BUTTON_PIN) == 1) {
            xSemaphoreGive(xSemaphorePause);  // Khi nhấn nút, gửi tín hiệu tạm dừng
            vTaskDelay(200 / portTICK_PERIOD_MS);  // Tránh nhấn lặp lại
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);  // Giảm tải CPU
    }
}

// Task gửi bản tin qua UART và hiển thị lên OLED
void uart_send_task(void *arg) {
    SSD1306_t oled;
    i2c_master_init();  // Khởi tạo giao tiếp I2C
    ssd1306_init(&oled, I2C_MASTER_NUM, 0x3C);  // Khởi tạo OLED
    ssd1306_clear_screen(&oled, false);

    char *message = "ESP32 UART sending data...\n";
    while (1) {
        if (xSemaphoreTake(xSemaphorePause, 0) == pdTRUE) {
            // Hiển thị PAUSE trên OLED
            oled_display_message(&oled, "PAUSE");
            // Dừng gửi bản tin qua UART
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        uart_write_bytes(UART_PORT_NUM, message, strlen(message));  // Gửi bản tin qua UART
        oled_display_message(&oled, message);  // Hiển thị bản tin lên OLED
        vTaskDelay(1000 / portTICK_PERIOD_MS);  // Đợi 1 giây trước khi gửi tiếp
    }
}

// Hàm main khởi động chương trình
void app_main() {
    uart_init();
    xSemaphorePause = xSemaphoreCreateBinary();  // Khởi tạo Semaphore tạm dừng

    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);
    xTaskCreate(uart_send_task, "uart_send_task", 4096, NULL, 10, NULL);
}
