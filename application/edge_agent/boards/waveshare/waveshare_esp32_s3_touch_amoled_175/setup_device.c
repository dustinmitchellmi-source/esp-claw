/**
 * @file setup_device.c
 * @brief Board-specific device factory functions for the
 *        Waveshare ESP32-S3-TOUCH-AMOLED-1.75.
 *
 * Pin assignments confirmed from board schematic:
 *
 *  LCD (CO5300, QSPI)
 *    QSPI SIO0  GPIO4     QSPI SI1   GPIO5
 *    QSPI SI2   GPIO6     QSPI SI3   GPIO7
 *    QSPI SCL   GPIO38    LCD CS     GPIO12
 *    LCD TE     GPIO13    LCD RESET  GPIO39
 *
 *  Touch (CST9217, I2C)
 *    TP SDA  GPIO15  TP SCL  GPIO14
 *    TP INT  GPIO11  TP RST  GPIO40
 *
 *  Audio (ES8311 + ES7210)
 *    I2S SCLK  GPIO9   I2S LRCK   GPIO45
 *    I2S MCLK  GPIO42  DSDIN      GPIO8
 *    ASDOUT    GPIO10  PA CTRL    GPIO46
 *
 *  SD card (SPI)
 *    MOSI GPIO1  SCK GPIO2  MISO GPIO3  CS GPIO41
 *
 *  I2C bus (shared: touch, IMU, RTC, PMU, GPS)
 *    SDA GPIO15  SCL GPIO14
 */

#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_types.h"
#include "esp_rom_gpio.h"

// Touch driver — CST9217
#include "esp_lcd_touch_cst9217.h"
#include "esp_lcd_co5300.h"
#include "esp_board_periph.h"
#include "driver/i2c_master.h"

static const char *TAG = "ws175_setup";

static const co5300_lcd_init_cmd_t s_co5300_init_cmds[] = {
    {0xFE, (uint8_t[]){0x20}, 1, 0},
    {0x19, (uint8_t[]){0x10}, 1, 0},
    {0x1C, (uint8_t[]){0xA0}, 1, 0},
    {0xFE, (uint8_t[]){0x00}, 1, 0},
    {0xC4, (uint8_t[]){0x80}, 1, 0},
    {0x3A, (uint8_t[]){0x55}, 1, 0},
    {0x35, (uint8_t[]){0x00}, 1, 0},
    {0x53, (uint8_t[]){0x20}, 1, 0},
    {0x51, (uint8_t[]){0xFF}, 1, 0},
    {0x63, (uint8_t[]){0xFF}, 1, 0},
    
    // 1. Set orientation BEFORE display wakes up
    //{0x36, (uint8_t[]){0xA0}, 1, 0},  /* MADCTL: 270° rotation */
    
    // 2. Set windows
    //{0x2A, (uint8_t[]){0x00, 0x06, 0x01, 0xD7}, 4, 0},
    {0x2A, (uint8_t[]){0x00, 0x00, 0x01, 0xD1}, 4, 0},
    {0x2B, (uint8_t[]){0x00, 0x00, 0x01, 0xD1}, 4, 20},
    
    // 3. Sleep Out
    {0x11, NULL, 0, 600},
    
    // 4. Finally, turn it on
    {0x29, NULL, 0, 0},
};

esp_err_t lcd_panel_factory_entry_t(
        esp_lcd_panel_io_handle_t io,
        const esp_lcd_panel_dev_config_t *panel_dev_config,
        esp_lcd_panel_handle_t *ret_panel)
{
    ESP_LOGI(TAG, "Initialising CO5300 AMOLED 466x466 via esp_lcd_co5300");

    co5300_vendor_config_t vendor_config = {
        .init_cmds = s_co5300_init_cmds,
        .init_cmds_size = sizeof(s_co5300_init_cmds) / sizeof(s_co5300_init_cmds[0]),
        .flags = {
            .use_qspi_interface = 1,
        },
    };

    esp_lcd_panel_dev_config_t config = *panel_dev_config;
	
    config.vendor_config = &vendor_config;

    ESP_RETURN_ON_ERROR(
        esp_lcd_new_panel_co5300(io, &config, ret_panel),
        TAG, "esp_lcd_new_panel_co5300 failed");

    esp_lcd_panel_set_gap(*ret_panel, 6, 0);

    ESP_LOGI(TAG, "CO5300 panel ready");
    return ESP_OK;
}

/* ────────────────────────────────────────────────────────────────
 * Touch factory
 * Signature: lcd_touch_factory_entry_t
 *   (esp_lcd_panel_io_handle_t io,
 *    const esp_lcd_touch_config_t *touch_dev_config,
 *    esp_lcd_touch_handle_t *ret_touch)
 * ──────────────────────────────────────────────────────────────── */

//get the I2C bus handle from the Board Manager.
#include "esp_lcd_touch_cst9217.h"
#include "esp_board_periph.h"
#include "driver/i2c_master.h"

esp_err_t lcd_touch_factory_entry_t(
    const esp_lcd_panel_io_handle_t io,
    const esp_lcd_touch_config_t *config,
    esp_lcd_touch_handle_t *tp)
{
    ESP_LOGI(TAG, "Initialising CST9217 touch (I2C addr 0x5A)");

    gpio_set_level(40, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(40, 1);
    vTaskDelay(pdMS_TO_TICKS(200));

    // Get I2C bus handle directly from Board Manager
    i2c_master_bus_handle_t i2c_bus = NULL;
    ESP_RETURN_ON_ERROR(
        esp_board_periph_get_handle("i2c_master", (void **)&i2c_bus),
        TAG, "Failed to get I2C bus handle");

    // Create IO using official CST9217 config macro
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_CST9217_CONFIG();
    tp_io_config.scl_speed_hz = 400000;
    ESP_RETURN_ON_ERROR(
        esp_lcd_new_panel_io_i2c(i2c_bus, &tp_io_config, &tp_io_handle),
        TAG, "Failed to create touch IO");

    esp_err_t ret = esp_lcd_touch_new_i2c_cst9217(tp_io_handle, config, tp);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "lcd_touch_factory_entry_t(%d): CST9217 init failed", __LINE__);
    }
    return ret;
}

/* ────────────────────────────────────────────────────────────────
 * I2S MCLK configuration
 *
 * The ES8311 codec requires a master clock on GPIO42.
 * Board Manager's audio_codec device does not expose an MCLK pin
 * field, so configure it here and call this function from app_claw.c
 * (or main.c) BEFORE cap_lua_register_group() / claw_cap_start_all().
 *
 * Usage:
 *   #include "setup_device.h"
 *   ws175_configure_i2s_mclk();
 * ──────────────────────────────────────────────────────────────── */
esp_err_t ws175_configure_i2s_mclk(void)
{
    ESP_LOGI(TAG, "Routing I2S MCLK → GPIO42");
    gpio_set_direction(42, GPIO_MODE_OUTPUT);

    // esp_rom_gpio_connect_out_signal replaces the old gpio_matrix_out
    // I2S0O_MCK_OUT_IDX = 81 on ESP32-S3 (TRM Table 7-2)
    esp_rom_gpio_connect_out_signal(42, 81 /*I2S0O_MCK_OUT_IDX*/, false, false);

    return ESP_OK;
}
