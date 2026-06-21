# Backlog — ESP32-S3-Touch-AMOLED-1.75 / esp-claw

Status as of migration to new repo (`esp-claw-new/application/edge_agent`).
Items are not in strict priority order; rough grouping by area.

---

## New-repo migration carry-overs (need real fixes, currently working around)

### 1. Emote/animation disabled
- **Status:** `CONFIG_APP_CLAW_ENABLE_EMOTE=n` — disabled to get a stable boot.
- **Root cause:** Built-in emote mmap animation assets are incompatible with
  this board's 466×466 AMOLED panel (wrong resolution/format for the
  decoder — `eaf_dec` reports invalid bit depth / unknown encoding /
  invalid frame headers, eventually corrupting memory and crashing inside
  `memcpy`).
- **Fix needed:** Source or build emote animation assets correctly sized/
  formatted for 466×466 AMOLED, then re-enable.
- **Risk if ignored:** None currently — app runs fine without emote. This is
  a feature gap, not a stability risk.

### 2. `swap_xy not supported by this panel`
- **Status:** Non-fatal warning at every boot:
  `E (...) co5300_spi: swap_xy is not supported by this panel`
  `W (...) DEV_DISPLAY_LCD: Failed to swap LCD panel XY: ESP_ERR_NOT_SUPPORTED`
- **Confirmed:** Present in OLD repo too — not a migration regression.
- **Likely cause:** `board_devices.yaml`'s touch entry (or display profile)
  requests `swap_xy`, but the `co5300_spi` panel driver doesn't implement
  that capability at the panel level.
- **Fix needed:** Test actual touch behavior on screen (tap and confirm X/Y
  register correctly). If rotated/mirrored, fix via the touch driver's own
  `mirror_x` / `mirror_y` / rotation flags in `board_devices.yaml` instead of
  relying on display-level `swap_xy`.

### 3. Brief I2C write failure on ES8311 init
- **Status:** `E (...) I2C_If: Fail to write to dev 30` appears once during
  boot, immediately before `ES8311: Work in Slave mode` and successful codec
  init.
- **Likely benign** — codec initializes successfully right after. Has not
  caused any observed problem.
- **Fix needed:** Watch for audio playback issues; if none surface, lower
  priority. If they do, investigate I2C timing/bus contention at that boot
  stage (multiple `i2c_master` reuse events happen in quick succession here).

### 4. `dev_lcd_touch_i2c` deprecated
- **Status:** Cosmetic compiler warning every build:
  `#warning "dev_lcd_touch_i2c is deprecated... Use dev_lcd_touch with
  sub_type: i2c instead."`
- **Fix needed:** Migrate `board_devices.yaml`'s touch device entry from
  `dev_lcd_touch_i2c`-style declaration to `dev_lcd_touch` with
  `sub_type: i2c`, mirroring the same pattern already used for the display
  (`type: display_lcd, sub_type: spi`).
- **Not blocking** — old repo carried this warning too without issue.

### 9. "Out of memory serializing request" on new repo — PSRAM allocator strategy mismatch
- **Status:** Hit when sending a message to the agent via Telegram or console
  on the new repo. Old repo never showed this.
- **Root cause found:** New repo's `sdkconfig` defaults to
  `CONFIG_SPIRAM_USE_CAPS_ALLOC=y` (only code that explicitly requests
  `MALLOC_CAP_SPIRAM` gets PSRAM). Old repo used
  `CONFIG_SPIRAM_USE_MALLOC=y` (plain `malloc()` — which is what cJSON uses
  to serialize the LLM request/response — transparently spills into PSRAM
  once internal DRAM is tight). Confirmed via direct `sdkconfig` diff between
  the two repos.
- **Fix applied:** Added to new repo's board-specific
  `sdkconfig.defaults.board` (kept board-specific rather than app-level,
  since PSRAM size/strategy is genuinely a hardware property of this exact
  module — 8MB Octal PSRAM):
  ```
  CONFIG_SPIRAM_USE_MALLOC=y
  # CONFIG_SPIRAM_USE_CAPS_ALLOC is not set
  CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=16384
  CONFIG_SPIRAM_MALLOC_RESERVE_INTERNAL=32768
  ```
  `ALWAYSINTERNAL=16384`: allocations under 16KB always go to internal RAM
  (PSRAM has higher latency; keeps small/frequent allocations fast).
  `RESERVE_INTERNAL=32768`: keeps a 32KB floor of internal RAM free for
  things that genuinely require it (e.g. DMA buffers).
  These two values were carried forward as-is from the old repo's proven
  config, not re-derived.
- **Status as of this note:** Fix applied, rebuild/reflash in progress —
  not yet confirmed whether this fully resolves the OOM. Update this entry
  once confirmed.

### 10. `claw_task.c` — internal-only stack policy override table (investigated, NOT the OOM fix)
- **Found while investigating item 9.** `components/claw_modules/claw_core/src/claw_task.c`
  has a `s_task_configs[]` override table that force-pins specific named
  tasks (`event_router`, `claw_mem_extract`, `cap_scheduler`, `cap_time_sync`,
  `claw_core`) to `CLAW_TASK_STACK_INTERNAL_ONLY` — i.e. it forces these
  tasks' *stacks* to stay in internal RAM regardless of what their own
  calling code requests. This is the opposite of a PSRAM fix — it's PSRAM
  *avoidance* for specific tasks, likely because PSRAM's higher access
  latency caused problems for these particular latency-sensitive/long-running
  tasks in the old repo.
- **Important caveat:** The lookup function that would actually apply this
  table, `claw_task_find_override()`, is commented out in the old repo's
  copy of this file. Unclear whether the table is currently doing anything
  at all in the old repo, or if it's dead code from an earlier experiment.
- **Mechanism also depends on:** `CONFIG_FREERTOS_TASK_CREATE_ALLOW_EXT_MEM`
  — if unset, `claw_task_external_memory_available()` always returns false
  and every PSRAM-stack request silently falls back to internal RAM anyway.
  Worth checking whether this flag is set in either repo if task stack
  placement becomes a problem later (stack overflow, task creation failure).
- **Action needed:** Not believed to be the fix for item 9's OOM (that's a
  heap/malloc-strategy issue, not a task-stack issue). Revisit only if a
  *different* symptom shows up later — specifically stack overflow or
  "could not allocate required memory" task-creation errors, not generic
  serialization OOM. If revisiting, first re-enable `claw_task_find_override()`
  (currently commented out) and confirm the override table is actually wired
  up before assuming its presence has any effect.

---

## Old-repo work not yet re-applied / re-verified on new repo

### 5. Whisper STT integration — not wired up
- **Status:** Fully proven on OLD repo: real microphone capture (din:10/
  dout:8 I2S pins, ES7210 3-mic TDM, gain 50.0, ~200ms settle delay to kill
  startup click) → WAV file → `curl --data-binary` style upload → Whisper
  server (`http://192.168.1.17:8001/transcribe`) → correct transcription
  confirmed (`"This is a test to see if you can hear me."`).
- **Not done:** Wiring the transcribed text into `claw_event_router` so the
  agent can respond. The clean entry point is
  `event_publisher.publish_message({source_cap, channel, chat_id, text,
  sender_id, message_id})`, already exposed to Lua via
  `lua_module_event_publisher` — confirmed working, mirrors
  `cap_im_tg`'s `claw_event_router_publish_message()` pattern exactly.
- **Blocker:** Getting the WAV file from the board to Whisper. Whisper's
  endpoint requires multipart/form-data with a `file` field (confirmed via
  curl: raw-body POST returns
  `{"detail":[{"type":"missing","loc":["body","file"],"msg":"Field
  required"}]}`). The existing `cap_http_request` capability only supports
  simple string-body POST — no multipart support.
- **Two architecture options discussed, neither built yet:**
  1. **Lua-orchestrated:** A Lua script transcribes first (calls Whisper
     directly, however that gets solved), then publishes the resulting text
     via `event_publisher.publish_message()`.
  2. **Agent-orchestrated:** Build a new `transcribe_audio` capability
     (mirrors `cap_llm_inspect`'s `inspect_image` pattern — takes a `path`,
     returns text) that the LLM agent can call as a tool when it sees a
     voice note exists, letting the agent itself decide when to transcribe
     rather than deciding upfront in Lua.
  - Leaning toward investigating Option 2 first since it reuses the existing
    capability-calling architecture (same shape as `tg_send_message`,
    `inspect_image`), but both are valid.
- **Multipart implementation options if building a new capability:**
  - Custom C capability mirroring `cap_im_tg.c`'s
    `cap_im_tg_send_multipart_file()` (proven, robust, more code).
  - Hand-rolled multipart body construction in Lua using `string` library
    (binary-safe in Lua 5.x, less code, less proven for this use case).
- **Also unresolved:** Whether `claw_core_llm_infer_media`'s
  `CLAW_MEDIA_ASSET_KIND_LOCAL_PATH` media-asset abstraction (used by
  `inspect_image`) could be extended with a new audio media kind, letting
  the LLM/agent core handle transcription natively rather than via a
  separate bolt-on HTTP capability. Not investigated — would be a bigger,
  more architecturally-elegant change than either option above.

### 6. Channel count decision for ES7210 — settled, documented, not re-tested on new repo
- **Decision made (old repo):** Keep the 4-channel/3-mic TDM config
  (`adc_max_channel: 4`, `adc_channel_mask: "0111"`) matching Espressif's
  official Brookesia reference for this exact board, even though raw
  capture (no AEC processing yet) sounds the same to the ear as simpler
  1-channel STD mode. Rationale: keeps the door open for future AEC/AFE
  processing which needs multi-channel raw input.
- **Action needed:** Re-verify this config still produces clean capture on
  the new repo (new repo's board YAML was ported as-is, so this should
  carry over, but has not been explicitly re-tested with `tmp_miclevel.lua`/
  `test_record.lua` since the migration).

---

### 14. AXP2101 full power management — not yet implemented (real C work, not YAML)
- **Status:** Mic power rail (BLDO2) was diagnosed and manually poked via
  raw I2C from Lua in an earlier session (`tmp_axp_bldo2.lua`,
  `tmp_bldo2_volt.lua`) — confirmed register `0x90` bit 5 = enable,
  register `0x97` = BLDO2 voltage. That was a diagnostic exercise, NOT a
  durable integration: nothing in firmware currently manages AXP2101 on
  boot. Whatever rail state exists right now is whatever was last poked by
  hand and happens to persist (PMIC is its own power domain, survives ESP32
  resets but not a cold power-cycle/battery disconnect).
- **No built-in Board Manager device type for AXP2101 exists.** Confirmed
  by searching `managed_components/espressif__esp_board_manager/devices/`
  for `*axp*` — no results. Must use Board Manager's generic `type: custom`
  device mechanism instead (same mechanism the CO5300 display uses for its
  real init sequence).
- **Reference found:** `m5stack_cores3` board declares
  `axp2101_power_manager` as `type: custom` with a `power_manager.c`/`.h`
  pair implementing `CUSTOM_DEVICE_IMPLEMENT(axp2101_power_manager,
  cores3_power_manager_init, cores3_power_manager_deinit)`. Useful as a
  STRUCTURAL template (custom device registration pattern, I2C device
  handle setup via `esp_board_periph_get_handle`/`i2c_master_bus_add_device`,
  auto-generated config struct via `gen_board_device_custom.h`) but NOT a
  feature template — M5Stack's implementation is purely
  write-only register pokes for enabling power rails (LCD, touch, SD,
  speaker, 5V, camera voltage rails). It has NO battery percentage, NO
  charge status, NO power button handling at all.
- **Scope for "full integration" (user's stated goal):**
  1. **Battery percentage** — AXP2101 doesn't report % directly; needs
     either its built-in fuel-gauge feature (if enabled/available) or
     reading `VBAT` voltage register and applying a voltage→percentage
     curve (lookup table or simple linear approximation for the battery
     chemistry in use).
  2. **Charge status** — AXP2101 has status register(s) indicating
     charging/discharging/full state. Register addresses not yet looked
     up — would need a fresh datasheet/XPowersLib reference pass (same
     approach as the original BLDO2 register hunt).
  3. **Power button (PWRON) handling** — schematic shows a `PWRON` net and
     a separate `AXP_IRQ` net from the AXP2101 to the ESP32. AXP2101 can
     detect short-press/long-press on its own and raise an interrupt — the
     ESP32 side would need either GPIO interrupt handling on the IRQ line,
     or periodic polling of a status register, plus a decision on what a
     short vs. long press should actually do (sleep, soft shutdown, etc.).
- **This is genuine from-scratch C development**, not a YAML-only task like
  the SD card integration was. Reuse the register addresses already
  confirmed from session 1 (enable: `0x90` bit 5, BLDO2 voltage: `0x97`)
  as a starting point, but battery/charge/button registers need fresh
  lookup (XPowersLib's `XPowersAXP2101.tpp`/`AXP2101Constants.h` — already
  proven useful reference material from the original BLDO2 hunt — is the
  right place to look first).
- **Action needed:** Scope and implement as its own dedicated session —
  not started yet.

### 11. Workflow note: correct order of operations for refreshing `sdkconfig` from board defaults
- **Discovered:** Editing `sdkconfig.defaults.board` (e.g. the PSRAM allocator
  fix in item 9, the emote-disable flag) and running `idf.py fullclean` +
  `idf.py build` does NOT reliably pick up the new value — confirmed directly
  by diffing `sdkconfig` before/after and seeing the OLD value persist (e.g.
  `CONFIG_SPIRAM_USE_CAPS_ALLOC=y` instead of the intended
  `CONFIG_SPIRAM_USE_MALLOC=y`).
- **CORRECTED sequence (order matters — confirmed by direct testing):**
  ```
  Remove-Item ".\sdkconfig" -Force
  idf.py fullclean
  idf.py bmgr -c ./boards -b <board_name>
  idf.py build
  ```
  i.e. delete `sdkconfig` and fullclean FIRST, THEN regen the board config,
  THEN build. An earlier version of this note had `idf.py bmgr` running
  BEFORE the delete — this is WRONG and causes a real, confirmed failure:
  `idf.py bmgr` must run on a freshly-cleared slate so its target/chip
  information (e.g. `esp32s3` from `board_info.yaml`) actually lands in the
  newly-generated `sdkconfig`. Running `bmgr` first, then deleting
  `sdkconfig` afterward, wipes out exactly the target info `bmgr` just
  wrote, leaving `sdkconfig` to fall back to ESP-IDF's bare default target
  (`esp32`, NOT `esp32s3`) on the next configure step. This surfaces as:
  ```
  [ESP_BMGR_ASSIST] Error: CONFIG_IDF_TARGET mismatch: sdkconfig has "esp32"
  (normalized: esp32), board_manager.defaults expects "esp32s3" (esp32s3).
  ```
  Confirmed fix: reorder to delete-first, regen-second. Verified via build
  log showing `Building ESP-IDF components for target esp32s3` (correct)
  instead of `...for target esp32` (wrong) after reordering.
- **Standing rule going forward:** Use the CORRECTED sequence above —
  delete `sdkconfig` and fullclean BEFORE running `idf.py bmgr`, every
  time a board default needs to be refreshed. Don't rely on `fullclean`
  alone, and don't run `bmgr` before the delete step.
- **Real-world cost of forgetting this:** Caused the emote-disable fix
  (`CONFIG_APP_CLAW_ENABLE_EMOTE=n`) to silently revert to `=y` when
  `sdkconfig` was deleted to fix the PSRAM allocator flag, because the
  emote fix had only ever been applied directly to `sdkconfig` and was
  never written into `sdkconfig.defaults.board` — it wasn't durable, so it
  was lost the moment the file was wiped. Lesson: every fix that needs to
  survive a regen MUST go into a `.defaults` file, never edited directly
  in `sdkconfig` alone, even temporarily — it's too easy to forget which
  fixes are durable and which aren't once several have accumulated.

### 12. Old repo's `components/waveshare_esp32_s3_touch_amoled_175/` — fully superseded
- Old project is not being actively developed further; new repo is now the
  primary target. Old project remains as a reference / fallback only.
- `board_dummy.c` and the old peripheral-support Kconfig flags confirmed
  unnecessary in the new structure (see MIGRATION_NOTES.md).

### 13. `boards/` folder existed unused in old repo
- Noted early in old-repo work that a `boards/` directory existed in that
  project but was "never used." Now understood: this was almost certainly
  leftover scaffolding from whenever the old repo first adopted (but the
  project never migrated to) the same convention the new repo now uses
  throughout. No action needed — old repo is being retired in favor of the
  new one rather than migrated in-place.
