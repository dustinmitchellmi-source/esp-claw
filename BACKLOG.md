# Backlog

## Backlog
- [ ] Fix emote boot-loop crash (incompatible 466x466 AMOLED animation assets) — needed only for standard esp-claw/emote compatibility; not used in this project, low priority
- [ ] Migrate `dev_lcd_touch_i2c` → `dev_lcd_touch` with `sub_type: i2c`
- [ ] Wire Whisper STT as a capability + companion SKILL.md
- [ ] Wire Piper TTS as a capability + companion SKILL.md
- [ ] Implement AXP2101 power management (battery %, charge status, power button)
- [ ] Look up `category`/`peripherals` allowlist before writing the `transcribe_audio` SKILL.md

## Watchlist
- [ ] ES8311 I2C write failure on init — benign so far, watch for recurrence/audio playback issues

## Non-issues (confirmed, no action needed)
- swap_xy unsupported by CO5300 panel driver — confirmed in `esp_lcd_co5300_spi.c` (`panel_co5300_swap_xy` explicitly returns `ESP_ERR_NOT_SUPPORTED`), not a board misconfiguration
- Boot button — initializes correctly, not currently in project roadmap
