# Waveshare ESP32-S3-TOUCH-AMOLED-1.75 — Board Config for esp-claw

## Placement

Copy this directory to:

```
application\edge_agent\boards\waveshare\waveshare_esp32_s3_touch_amoled_175
```

Then generate:

```bash
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
| I2S ASDOUT | 8 | ES8311 → ESP32 (DAC monitor / ADC) |
| I2S DSDIN | 10 | ESP32 → ES8311 (DAC data in) |
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


## Important
 - I2S ASDOUT Schematic is wrong from Waveshare says 10, but is actually 8
 - I2S DSDIN  Schematic is wrong from Waveshare says 8, but is actually 10