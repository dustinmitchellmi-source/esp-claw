#pragma once
#include "esp_err.h"

/**
 * @brief Configure I2S MCLK output on GPIO42 for the ES8311 codec.
 *
 * Board Manager does not expose an MCLK pin field for audio_codec devices.
 * Call this function from main.c or app_claw.c before starting the
 * capability stack (claw_cap_start_all).
 */
esp_err_t ws175_configure_i2s_mclk(void);
