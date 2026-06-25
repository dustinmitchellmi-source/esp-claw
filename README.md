<div align="center">

  <h1>ESP-Claw 🦞</h1>
  <h2>Waveshare ESP32-S3-Touch-AMOLED-1.75</h2>

  <p>
    <a href="https://www.espressif.com">
      <img src="https://img.shields.io/badge/runs_on-ESP32_Series-red?style=flat-square" alt="Runs on ESP32 Series" />
    </a>
    <a href="./LICENSE">
      <img src="https://img.shields.io/github/license/espressif/esp-claw?style=flat-square" alt="License" />
    </a>
  </p>

</div>

This is a personal project of [espressif/esp-claw](https://github.com/espressif/esp-claw), configured and extended for the
**Waveshare ESP32-S3-Touch-AMOLED-1.75** board (466×466 round AMOLED touch
display, ES8311 DAC + ES7210 dual-mic ADC, SD card via SPI).

ESP-Claw itself is Espressif's open-source AI agent framework for ESP32-series
chips — for the full feature overview, demo videos, supported LLM/IM
platforms, and official documentation, see the
[upstream repo](https://github.com/espressif/esp-claw) and
[esp-claw.com](https://esp-claw.com/en/). This fork does not duplicate that
content; everything below is specific to this board and this project.

---

## 🔧 Board port

Full board port at [`application/edge_agent/boards/waveshare/waveshare_esp32_s3_touch_amoled_175/`](./application/edge_agent/boards/waveshare/waveshare_esp32_s3_touch_amoled_175/).

Confirmed working: display (CO5300 QSPI AMOLED), touch (CST9217), audio
(ES8311 DAC + ES7210 dual-mic ADC, full duplex), boot button, and SD card
storage (~2GB usable, see known limitation in `ARCHITECTURE_AND_PLAN.md`).

## 📋 Project documentation

- [`ARCHITECTURE_AND_PLAN.md`](./ARCHITECTURE_AND_PLAN.md) — current status,
  confirmed-working subsystems, design decisions, and next steps.
- [`BACKLOG.md`](./BACKLOG.md) — known issues, in-progress work, and
  workflow notes (including some real gotchas hit during board bring-up).

## 🎙️ Project-specific additions (in progress)

[x] Local LLM using Ollama + qwen2.5:14b-instruct
[ ] Voice pipeline: mic capture → STT → agent → TTS → speaker playback
	[ ] Local Whisper STT server integration (faster-whisper, self-hosted)
	[ ] Local Piper TTS server integration (self-hosted)
[ ] AXP2101 power management
[ ] User Interface to engage with the agent

## Upstream tracking

This fork tracks [espressif/esp-claw](https://github.com/espressif/esp-claw)
as `upstream` (not `origin`) and is not intended as a contribution back to
that project. `README.md` is excluded from upstream merges
(`merge=ours` in `.gitattributes`) since it's been rewritten for this fork.
