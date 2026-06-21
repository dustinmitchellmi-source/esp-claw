# Waveshare ESP32-S3-TOUCH-AMOLED-1.75 — Board Config for esp-claw

## Placement

Copy this directory to:

```
application/basic_demo/components/waveshare_esp32_s3_touch_amoled_175/
```

Then generate:

```bash
cd application/basic_demo
idf.py gen-bmgr-config -c ./components -b waveshare_esp32_s3_touch_amoled_175
idf.py build
idf.py flash monitor
```

---

## Pin reference (from schematic)

| Signal | GPIO | Notes |
|---|---|---|
| QSPI SIO0 | 4 | LCD data 0 |
| QSPI SI1 | 5 | LCD data 1 |
| QSPI SI2 | 6 | LCD data 2 (WP) |
| QSPI SI3 | 7 | LCD data 3 (HD) |
| QSPI SCL | 38 | LCD clock |
| LCD CS | 12 | |
| LCD TE | 13 | Tearing effect (optional) |
| LCD RESET | 39 | Active LOW |
| TP SDA | 15 | Shared I2C bus |
| TP SCL | 14 | Shared I2C bus |
| TP INT | 11 | Touch interrupt |
| TP RESET | 40 | Active LOW |
| I2S SCLK | 9 | Shared by DAC + ADC |
| I2S LRCK | 45 | Shared by DAC + ADC |
| I2S MCLK | 42 | ES8311 master clock — see note |
| I2S ASDOUT | 10 | ES8311 → ESP32 (DAC monitor / ADC) |
| I2S DSDIN | 8 | ESP32 → ES8311 (DAC data in) |
| PA CTRL | 46 | Speaker amp enable (HIGH = on) |
| SD MOSI | 1 | |
| SD SCK | 2 | |
| SD MISO | 3 | |
| SD CS | 41 | |
| USB D- | 19 | |
| USB D+ | 20 | |
| UART TX | 43 | U0TXD |
| UART RX | 44 | U0RXD |
| QMI INT2 | 21 | IMU interrupt 2 |
| BOOT | 0 | Active LOW |

EXIO0–EXIO7 are pins on the TCA9554 I2C GPIO expander (address on the shared I2C bus):
EXIO3 = RTC INT, EXIO4 = SYS OUT, EXIO5 = AXP IRQ, EXIO6 = QMI INT1, EXIO7 = GPS RST

---

## Key things to verify before your first build

1. **CO5300 driver**: `setup_device.c` sends the init command table and uses
   `esp_lcd_new_panel_st7789` as a structural shim. Once an upstream
   `esp_lcd_co5300` component appears on the IDF registry, swap the call in
   `lcd_panel_factory_entry`. The init command table should still be correct.

2. **I2S MCLK**: Board Manager does not wire MCLK automatically. Call
   `ws175_configure_i2s_mclk()` from `main.c` before `app_claw_start()`.

3. **CST9217 I2C address**: 0x15 is the datasheet default. If touch init
   fails, probe the bus with `i2cdetect` on the console to confirm.

4. **ES7210 I2C address**: 0x40 assumes ADDR pin tied to GND. Verify in
   the schematic.

5. **ES8311 I2C address**: 0x18 assumes ADDR low. Verify in the schematic.

6. **CO5300 init sequence**: The command table in `setup_device.c` is derived
   from Waveshare's published Arduino/ESP-IDF demos. Download the official
   resource package from the wiki and cross-check if display output looks wrong.
