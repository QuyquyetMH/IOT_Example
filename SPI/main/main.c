#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"

#define CLK_PIN     33
#define MOSI_PIN    32
#define CS_PIN      25

#define DECODE_MODE_REG     0x09
#define INTENSITY_REG       0x0A
#define SCAN_LIMIT_REG      0x0B
#define SHUTDOWN_REG        0x0C
#define DISPLAY_TEST_REG    0x0F

spi_device_handle_t spi2;
#define NUM_MATRICES 4  // Số ma trận LED

static void spi_init() {
    esp_err_t ret;

    spi_bus_config_t buscfg={
        .miso_io_num = -1,
        .mosi_io_num = MOSI_PIN,
        .sclk_io_num = CLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };

    ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);

    spi_device_interface_config_t devcfg={
        .clock_speed_hz = 1000000,  // 1 MHz
        .mode = 0,                  // SPI mode 0
        .spics_io_num = CS_PIN,     
        .queue_size = 1,
        .flags = SPI_DEVICE_HALFDUPLEX,
        .pre_cb = NULL,
        .post_cb = NULL,
    };

    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi2));
};

static void write_reg_all(uint8_t reg, uint8_t value) {
    uint8_t tx_data[NUM_MATRICES * 2];

    for (int i = 0; i < NUM_MATRICES; i++) {
        tx_data[i * 2] = reg;
        tx_data[i * 2 + 1] = value;
    }

    spi_transaction_t t = {
        .tx_buffer = tx_data,
        .length = NUM_MATRICES * 2 * 8
    };

    ESP_ERROR_CHECK(spi_device_polling_transmit(spi2, &t));
}

static void write_reg_matrix(int matrix_index, uint8_t reg, uint8_t value) {
    uint8_t tx_data[NUM_MATRICES * 2];

    for (int i = 0; i < NUM_MATRICES; i++) {
        if (i == matrix_index) {
            tx_data[i * 2] = reg;
            tx_data[i * 2 + 1] = value;
        } else {
            tx_data[i * 2] = 0x00;  // No operation (NOP) for other matrices
            tx_data[i * 2 + 1] = 0x00;
        }
    }

    spi_transaction_t t = {
        .tx_buffer = tx_data,
        .length = NUM_MATRICES * 2 * 8
    };

    ESP_ERROR_CHECK(spi_device_polling_transmit(spi2, &t));
}

static void clear(void) {
    for (int i = 0; i < 8; i++) {
        write_reg_all(i + 1, 0x00);
    }
}

static void max7219_init() {
    write_reg_all(DISPLAY_TEST_REG, 0);
    write_reg_all(SCAN_LIMIT_REG, 7);
    write_reg_all(DECODE_MODE_REG, 0);
    write_reg_all(SHUTDOWN_REG, 1);
    clear();
}

// Ma trận bitmap của các ký tự "L", "U", "M", "I"
const uint8_t L[8] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF, 0x00};
const uint8_t U[8] = {0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF, 0x00};
const uint8_t M[8] = {0x81, 0xC3, 0xA5, 0x99, 0x81, 0x81, 0x81, 0x00};
const uint8_t I[8] = {0xFF, 0x18, 0x18, 0x18, 0x18, 0x18, 0xFF, 0x00};

// Hàm hiển thị một ký tự trên một ma trận cụ thể
static void display_char_on_matrix(int matrix_index, const uint8_t *character) {
    for (int i = 0; i < 8; i++) {
        write_reg_matrix(matrix_index, i + 1, character[i]);
    }
}

// Hàm hiển thị chuỗi "LUMI" trên 4 ma trận
static void display_lumi() {
    display_char_on_matrix(0, L);
    display_char_on_matrix(1, U);
    display_char_on_matrix(2, M);
    display_char_on_matrix(3, I);
}

void app_main(void) {
    spi_init();
    max7219_init();

    while (1) {
        display_lumi();
        vTaskDelay(5000 / portTICK_PERIOD_MS); // Giữ chuỗi "LUMI" trong 5 giây
        clear();  // Xóa LED sau khi hiển thị
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Dừng 1 giây trước khi hiển thị lại
    }
}
