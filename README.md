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

This is a personal project of [espressif/esp-claw](https://github.com/espressif/esp-claw), configured for the
**Waveshare ESP32-S3-Touch-AMOLED-1.75** board. I have yet to come up with a name for my project, but it's a fun little hobby.

ESP-Claw itself is Espressif's open-source AI agent framework for ESP32-series
chips — for the full feature overview, demo videos, supported LLM/IM
platforms, and official documentation, see the
[upstream repo](https://github.com/espressif/esp-claw) and
[esp-claw.com](https://esp-claw.com/en/). This fork does not duplicate that
content; everything below is specific to this board and this project.

---

## 🔧 Board status

Board configuration located at [`application/edge_agent/boards/waveshare/waveshare_esp32_s3_touch_amoled_175/`](./application/edge_agent/boards/waveshare/waveshare_esp32_s3_touch_amoled_175/).

Hardware / Resources 
- [x] 1.75" capacitive touch high-definition AMOLED display
	- [x] Display conttroller : CO5300
	- [x] I2C-based capacitive touch : CST9217
- [x] Audio (full duplex)
	- [x] DAC : ES8311
	- [x] ADC : ES7210
- [x] TF Card Slot
- [ ] Power Management : AXP2101

I have achieved a clean boot sequence with the standard **esp-claw/edge agent** application, except having to disable the **emote** display, on multiple Waveshare ESP32-S3-TOUCH-AMOLED-1.75" development boards.
The **emote** display is not the display I wish to have for my project. I intend to solve the boot loop it causes, but it's low priority.

## 📋 Project documentation

- [`ARCHITECTURE_AND_PLAN.md`](./ARCHITECTURE_AND_PLAN.md) — current status, confirmed-working subsystems, design decisions, and next steps.
- [`BACKLOG.md`](./BACKLOG.md) — known issues, in-progress work, and workflow notes.

## 🎙️ Project-specific features

- [x] Local Large Language Model using Ollama + qwen2.5:14b-instruct
- [ ] Voice pipeline: mic capture → STT → agent → TTS → speaker playback
	- [x] Local Whisper STT server integration (faster-whisper, self-hosted)
	- [x] Local Piper TTS server integration (self-hosted)
	- [ ] End-to-end orchestration / workflow
- [ ] User Interface
- [ ] Case / Housing (via 3D printing)
	- [ ] Design
	- [ ] Printed

## Upstream tracking

This fork tracks [espressif/esp-claw](https://github.com/espressif/esp-claw)
as `upstream` (not `origin`) and is not intended as a contribution back to
that project. `README.md` is excluded from upstream merges
(`merge=ours` in `.gitattributes`) since it's been rewritten for this fork.
