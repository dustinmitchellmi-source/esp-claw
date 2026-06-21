# Architecture Status & Plan — ESP32-S3-Touch-AMOLED-1.75 Voice Assistant

## Two repos, one hardware target

| | Old repo | New repo |
|---|---|---|
| Path | `C:\esp\projects\esp-claw\application\esp32-s3-touch-assist` | `C:\esp\projects\esp-claw-new\application\edge_agent` |
| Board convention | `components/<board_name>/` (pre-dates `boards/` convention) | `boards/<vendor>/<board_name>/` (standard) |
| Status | Working, but being retired. Reference/fallback only. | Active development target going forward. |
| Board manager flags | Required manual `sdkconfig`/CMakeLists patching due to a real generator regression (extensively diagnosed and fixed). | Generator handles peripheral/device support flags automatically. No patching needed. |
| Mic/audio pipeline | Fully proven end-to-end through real Whisper transcription. | Board YAML ported as-is; not yet re-verified post-migration. |
| Whisper → agent wiring | Not built (was the next planned step when migration started). | Not built. |
| UI / emote / display-arbiter | Existing, working, not yet ported. | Not yet ported — emote currently disabled (asset incompatibility). |

**Decision:** New repo is the path forward. Old repo stays untouched as a
known-good reference for re-deriving any board-specific values if something
gets lost in translation (pin assignments, gain values, init sequences).

## Current confirmed-working state (new repo)

- Board fully ported to `boards/waveshare/waveshare_esp32_s3_touch_amoled_175/`.
- Clean `idf.py build`, clean boot, confirmed via real hardware flash + serial
  monitor.
- Subsystems verified at boot: SPI/I2C/I2S peripherals, display (CO5300 AMOLED
  466×466), touch (CST9217), audio codecs (ES8311 DAC + ES7210 ADC, original
  I2S din/dout pin fix preserved), boot button, FATFS (`/system` read-only +
  `/fatfs` writable), WiFi provisioning AP, event router (8 rules loaded),
  scheduler (3 entries), memory subsystem, skill registry (18 skills), 73
  capabilities across 19 groups, Lua runtime (23 modules), console REPL.
- Emote/animation disabled (`CONFIG_APP_CLAW_ENABLE_EMOTE=n`) pending
  display-correct assets — see BACKLOG.md item 1.
- PSRAM allocator strategy fixed (`CONFIG_SPIRAM_USE_MALLOC=y`) — resolved
  "Out of memory serializing request" on agent chat requests (Telegram and
  local IM both confirmed working end-to-end with Ollama/qwen2.5:14b-instruct
  backend). See BACKLOG.md item 9.
- **Speaker output confirmed working** via `audio_device_tone.lua` builtin
  test script (plays a 3-note tone sequence through `audio_dac`/ES8311) — no
  additional fix needed. The din/dout I2S pin correction from the original
  mic debugging marathon fixed the *bus wiring* shared by both directions,
  so speaker output was never actually broken — this was a verification
  step, not a new fix. Confirms full-duplex I2S audio (mic input AND speaker
  output) both work cleanly on the new repo's board port.

## FATFS layout note (new repo)

New repo's `fatfs_image/` has no flat `scripts/` folder to drop one-off test
scripts into (unlike the old repo). Layout is now:
- `fatfs_image/storage/` → writable `/fatfs` partition (skills, fonts, static
  assets at build time; runtime-writable at `/fatfs` on device)
- `fatfs_image/system/` → read-only `/system` partition seed
- Builtin Lua test/demo scripts are NOT in either source tree directly —
  they're synced at build time from each `lua_modules/*/lua_scripts/*.lua`
  component folder into
  `build/system_fs_image/skills/builtin_lua_modules/scripts/builtin/test/`,
  then flashed onto the device's read-only `/system` partition. Confirmed
  on-device runnable path pattern:
  `/system/skills/builtin_lua_modules/scripts/builtin/test/<script>.lua`
  (run via `lua --run --path <path> --timeout-ms <ms>` from console).
- This new repo ships a notably larger built-in test script library than
  the old repo (LVGL demos, MCPWM, rotary encoder, servo, IR learn/replay,
  image conversion, threading examples, etc.) — worth browsing this folder
  when looking for an existing test before writing a new one-off script.

## What "UI and everything back" means — scope for next session

Known pieces from the old project not yet present/ported in the new repo:

1. **Display arbiter wiring** — old project had `display_arbiter` as a
   top-level component (`components/display_arbiter/`); new repo has the
   equivalent under `components/common/display_arbiter/` per its
   `app_claw`-adjacent shared-component layout. Need to confirm the new
   repo's stock `main.c`/`app_claw` already wires this generically (likely
   yes, since boot log showed `display_session: start session` /
   `active changed` events happening automatically) and identify what, if
   anything, is board-specific UI code versus already-generic framework
   behavior.
   - **Known friction point (carried over from prior work, not yet
     re-verified on new repo):** Emote has historically taken exclusive
     ownership of the display via the arbiter (`display_session: active
     changed: session=... active=0->1` in boot logs corresponds to emote's
     session becoming active). This previously interfered with the
     project's own UI/Lua scripts trying to draw to the same display.
     Before porting UI scripts back, need to understand exactly how the
     arbiter hands ownership between emote and custom display code — e.g.
     does emote need to explicitly release/yield ownership, does the UI
     need to request it, or does emote staying disabled (current state,
     see BACKLOG.md item 1) sidestep this entirely for now. Worth deciding
     whether to keep emote disabled until UI work is further along, even
     after display-correct animation assets are eventually sourced.
2. **Emote setup** — old project had its own emote init/wiring
   (`espressif2022__esp_emote_expression`, `espressif2022__esp_emote_gfx`
   both appeared in old build logs). New repo has the same dependency but
   disabled. Real work: source/build 466×466-appropriate animation assets,
   re-enable, confirm no repeat of the `eaf_dec` crash.
3. **Any custom Lua UI scripts** — old project's `fatfs_image/scripts/` likely
   has board-specific UI Lua (settings screens, status displays, etc.) not
   yet copied to new repo's equivalent `fatfs_image/storage/` or
   `fatfs_image/system/` trees, or to a board-specific
   `boards/waveshare/waveshare_esp32_s3_touch_amoled_175/fatfs_image/`
   overlay (new repo explicitly supports board-specific FATFS overlays onto
   the system partition — confirmed via README).
4. **App-level customizations** — anything added to `main.c`/`app_capabilities.c`
   equivalents in the old project beyond stock framework behavior (need to
   diff old project's `main/` against new repo's `main/` to find these).

**Not yet itemized in detail** — this is the planning task for the next
session: diff old project's UI-related files against new repo's stock
equivalents to produce a concrete port checklist, rather than guessing at
scope here.

## Server startup automation (Whisper + Piper)

**Current phase: active development on personal Windows 11 machine.**
Both servers need to be manually restarted after every reboot (Windows
Update reboots, etc.) until this moves to a dedicated always-on server.
Decision: use simple double-click `.bat` files for now, specifically
because development phase benefits from seeing the terminal windows and
their live output (easier to spot crashes, errors, or silent-failure
patterns like the `python server.py` vs `uvicorn server:app` gotcha
documented above) — NOT hiding them as a background service yet.

**`start_whisper.bat`:**
```batfile
@echo off
cd C:\whisper-server
call venv\Scripts\activate.bat
uvicorn server:app --host 0.0.0.0 --port 8001
pause
```

**`start_piper.bat`:**
```batfile
@echo off
cd C:\piper-server
call venv_new\Scripts\activate.bat
python -m piper.http_server -m en_US-lessac-medium
pause
```

Save both somewhere convenient (Desktop or a dedicated `C:\start-scripts\`
folder). Double-click after any reboot — two terminal windows open and
stay running, with `pause` at the end so a window doesn't vanish instantly
if a server crashes on startup (lets you actually read the error).

**Future phase: dedicated always-on server (planned, not yet scheduled).**
Once moved off the personal Windows 11 machine to hardware meant to run
24/7, switch to running both as actual Windows Services (e.g. via NSSM —
"Non-Sucking Service Manager") so they:
- Start automatically before login, with no visible window
- Auto-restart on crash
- Don't depend on a logged-in desktop session at all

This is intentionally deferred — not worth the setup overhead while still
actively iterating on configuration (voice model choice, gain/volume
tuning, endpoint testing, etc.) where visibility into live server output
is more valuable than hands-off reliability. Revisit when ready for "full
scale deployment."

## Piper TTS server — setup and operational notes

**Location:** `C:\piper-server\` (same home server as Whisper, RTX 3090,
reachable at `http://192.168.1.17:5000`)

**Confirmed working setup (as of this session):**
- Required **Python 3.12+** — `piper-tts` 1.4.2 (current as of this session)
  requires Python >=3.9, but the machine only had Python 3.8 installed,
  which silently caused pip to resolve old `piper-tts` 1.1.0/1.2.0 releases
  instead (both depend on `piper-phonemize`, a compiled C++ extension with
  no installable wheels in this environment — surfaced as a confusing
  `ResolutionImpossible` dependency conflict, but the real cause was just
  the Python version being too old).
- Installed Python 3.14 fresh from python.org (note: as of this session,
  python.org's installer defaulted to the latest release, 3.14, rather than
  3.12 — Python 3.11 has reached end-of-life for new binary installers, so
  if targeting an older/more conservative version, 3.12 is the right choice
  to select explicitly rather than relying on "latest"). 3.14 worked fine —
  `piper-tts` 1.4.2 has prebuilt wheels for it and no longer depends on
  `piper-phonemize` at all (uses `onnxruntime` directly instead — a real
  architecture change in the package, not just a packaging fix).
- Install sequence:
  ```powershell
  py -3.14 -m venv venv_new
  .\venv_new\Scripts\Activate.ps1
  python -m pip install --upgrade pip
  pip install "piper-tts[http]"
  python -m piper.download_voices en_US-lessac-medium
  python -m piper.http_server -m en_US-lessac-medium
  ```
- Correct startup looks like:
  ```
  * Serving Flask app 'http_server'
  * Running on all addresses (0.0.0.0)
  * Running on http://127.0.0.1:5000
  * Running on http://192.168.1.17:5000
  ```
  Server binds to port 5000 by default and runs in the foreground (prompt
  does not return) — same "stays running" pattern as the Whisper uvicorn
  server.

**How to start/restart the server (once already set up):**
```powershell
cd C:\piper-server
.\venv_new\Scripts\Activate.ps1
python -m piper.http_server -m en_US-lessac-medium
```
Correct startup looks like real, persistent output (`Running on
http://192.168.1.17:5000`, Flask dev server banner) and the terminal prompt
does NOT return — it stays in the foreground actively serving, same
"prompt does not come back" pattern as the Whisper uvicorn server. If the
prompt returns immediately, the server did not start.

**Common failure mode:** Running `python -m piper.http_server` from the
wrong venv (e.g. the old Python 3.8 `venv` folder instead of `venv_new`)
will fail to import `piper` at all, or may silently use an incompatible
old install. Always confirm `(venv_new)` shows in the prompt before
starting the server.

**API usage — CONFIRMED working request format:**
- Endpoint: `POST http://192.168.1.17:5000/` (root path, not `/v1/audio/speech`
  — that path is specific to third-party Piper wrapper projects, NOT Piper's
  own built-in `piper.http_server`)
- Body: plain JSON, e.g. `{"text":"Hello from Piper"}`
- Response: raw WAV audio bytes
- **PowerShell quoting gotcha:** passing JSON directly via `curl.exe -d
  "{...}"` is unreliable in PowerShell — embedded quotes/spaces get
  mis-parsed by PowerShell's own argument splitting (manifests as bizarre
  errors like `curl: (3) URL rejected` or `Could not resolve host`, or a
  malformed-JSON 500 error server-side). Reliable approach: write the JSON
  to a file first, then reference it with `--data-binary "@file"`:
  ```powershell
  '{"text":"Hello from Piper"}' | Out-File -Encoding utf8 -NoNewline payload.json
  curl.exe -X POST -o output.wav -H "Content-Type: application/json" --data-binary "@payload.json" http://192.168.1.17:5000/
  ```
  This avoids all shell-quoting ambiguity. The same file-based pattern is
  worth reusing for any future PowerShell + curl + JSON body testing.

**Status:** Confirmed end-to-end — text in, playable WAV out, correct
speech content ("Hello from Piper" both requested and heard). Architecture
now has both halves of the voice pipeline independently proven (Whisper STT
+ Piper TTS), both running locally on the same home server with zero API
cost. Not yet wired into the ESP32 board — next step is connecting
`cap_http_request` (or a new capability) to call this endpoint with the
agent's response text and play the returned WAV through the speaker
(`audio.player()`/`audio.new_output()`, same API already proven working via
`audio_device_tone.lua` and `audio_record_play_aac.lua`).

## SD card storage — confirmed working (new repo, native mechanism)

**No custom C code needed.** The new repo's `app_fs.c` already has full SD
card support built in (`storage_sdcard_mount_point()` + `app_fs_init_storage()`)
— if a `type: fs_fat` device exists in `board_devices.yaml` and mounts
successfully, it's automatically preferred over internal flash as the
active `/fatfs` storage path, with zero changes to app/main code. This
made the old repo's hand-rolled `app_sdcard.c`/`app_sdcard.h` (manual SD
mount, manual first-boot factory-file copy from a separate
`/fatfs_internal` partition, manual factory-version tracking) entirely
unnecessary — the new repo's `/system` (read-only seed) +
`/fatfs` (writable, SD-or-flash) + `recover_missing_files()` design already
covers the same need generically, for any board, not just this one.

**Pins (confirmed from schematic, `ESP32-S3-Touch-AMOLED-1.75` netlist):**
- MOSI: GPIO1
- MISO: GPIO3
- SCK: GPIO2
- CS: GPIO41
- No card-detect line wired to the ESP32 (only 4 of the SD connector's
  pins are actually connected — confirmed by checking the netlist for any
  other connection to the `CD`/`CD-D3` pins; there are none).

**YAML added — `board_peripherals.yaml`** (pre-existing `spi_sd` entry was
already present from the original board YAML port, just never wired to a
device — only `max_transfer_sz` was changed):
```yaml
  # ── SD card (SPI mode) ─────────────────────────────────────────────────────
  - name: spi_sd
    type: spi
    config:
      spi_bus_config:
        spi_port: SPI3_HOST
        mosi_io_num: 1          # GPIO1  — SD MOSI (confirmed)
        miso_io_num: 3          # GPIO3  — SD MISO (confirmed)
        sclk_io_num: 2          # GPIO2  — SD SCK  (confirmed)
        quadwp_io_num: -1
        quadhd_io_num: -1
        max_transfer_sz: 32768  # was 4096, increased for larger audio file transfers
```

**YAML added — `board_devices.yaml`** (new entry, did not exist before):
```yaml
  - name: fs_sdcard
    type: fs_fat
    sub_type: spi
    version: default
    config:
      mount_point: "/sdcard"
      vfs_config:
        format_if_mount_failed: false
        max_files: 10
        allocation_unit_size: 16384
      sub_config:
        cs_gpio_num: 41
    peripherals:
      - name: spi_sd
```

**Confirmed via boot log:** Card identified correctly (`SD64G`, SDHC,
20MHz, ~59GB raw / ~2GB FAT32-visible per the mounted volume size shown),
mounted at `/sdcard`, and `app_fs` automatically selected it as the active
writable storage (`Using SD card at '/sdcard' as fatfs storage`,
`FATFS at /sdcard total=2080342016 used=3670016`) — writable storage went
from ~3MB (internal flash partition) to ~2GB. Recovery-seed files copied
into the SD card correctly on first mount, same as they would for internal
flash. The `cmd=5, R1 response: command not supported` log line is benign
— a standard SDIO-mode detection command that a plain SD/SDHC card
correctly rejects, not an error.

**Known limitation — FatFs 32-bit LBA cap (accepted, not a bug to fix):**
Mounted SD card capacity is reported/usable as only ~2GB
(`total=2080342016` bytes) despite the card being a genuine 64GB SDXC card,
correctly formatted as FAT32 (confirmed via Windows: `FAT32`, `62,209,327,104
bytes free` ≈ 57.9GB visible from the PC). Root cause confirmed directly in
this ESP-IDF version's bundled FatFs config
(`C:\esp\v5.5.2\esp-idf\components\fatfs\src\ffconf.h`):
```c
#define FF_LBA64    0   // 64-bit LBA addressing disabled
#define FF_FS_EXFAT 0   // exFAT support disabled
```
Both are hardcoded off, NOT exposed via Kconfig/sdkconfig at all (confirmed
— no matching option found anywhere in `sdkconfig`). This caps FatFs to
32-bit sector addressing, which combined with FAT32's own internal limits
produces the observed ~2GB ceiling regardless of actual card capacity. This
is a genuine ESP-IDF/FatFs build limitation, NOT a board config mistake, NOT
a card problem, NOT something fixable via YAML.
- **Decision: accepted, not fixing.** ~2GB is more than sufficient for this
  project's actual storage needs (audio clips, recordings, TTS responses —
  expected usage is tens to low-hundreds of MB, not tens of GB). Patching
  ESP-IDF's bundled `ffconf.h` to enable `FF_LBA64`/`FF_FS_EXFAT` was
  considered but rejected: it edits ESP-IDF's own component source (not
  this project's code), risks being silently reverted on any ESP-IDF
  reinstall/update, and the capacity gain isn't needed for this use case.
- **If more capacity is ever needed later:** either (a) revisit patching
  `ffconf.h` properly at that time, accepting the maintenance tradeoff, or
  (b) simpler — just buy/use a ≤32GB card instead of a 64GB+ one, since
  smaller cards are natively within whatever range this FatFs build
  actually handles correctly without any tricks.

**Important workflow note from this work:** Adding/changing `fs_fat`
devices (or any board device affecting chip target resolution) surfaced a
real bug in the previously-documented sdkconfig regen sequence — see
item 11 in BACKLOG.md. The corrected order is delete `sdkconfig` + fullclean
BEFORE running `idf.py bmgr`, not after.

## Whisper server — operational notes

**Location:** `C:\whisper-server\` (separate machine/host from the ESP32 board,
reachable at `http://192.168.1.17:8001`)

**Server implementation:** `server.py` — FastAPI + `faster-whisper`
(`WhisperModel`, model size `medium.en`, `device="cuda"`,
`compute_type="float16"`). Defines `/transcribe` (POST, multipart file
upload, field name `file`) and `/health` (GET) routes. The script only
*defines* the FastAPI `app` object — it does NOT call `uvicorn.run()` or
have an `if __name__ == "__main__":` block, so running it directly via
`python server.py` will NOT start a server. It loads the model (brief pause)
then exits cleanly (exit code 0) with no error and no obvious symptom —
easy to mistake for "did nothing" rather than "didn't actually start."

**Correct way to start/restart the server:**
```powershell
cd C:\whisper-server
.\venv\Scripts\Activate.ps1
uvicorn server:app --host 0.0.0.0 --port 8001
```
Correct startup looks like real, persistent output (`Uvicorn running on
http://0.0.0.0:8001`) and the terminal prompt does NOT return — it stays
in the foreground actively serving. If the prompt returns immediately after
running a start command, the server did NOT start, regardless of exit code.

**`/transcribe` endpoint requirements:**
- Requires multipart/form-data upload with field name `file` (confirmed via
  testing — a raw-body POST returns
  `{"detail":[{"type":"missing","loc":["body","file"],"msg":"Field required"}]}`).
- Test from a board/PC with curl:
  ```powershell
  curl.exe -X POST http://192.168.1.17:8001/transcribe -F "file=@C:\path\to\recording.wav"
  ```
- **CONFIRMED: `.aac` files work directly against this endpoint** — no WAV
  conversion needed. Tested with a 3-second AAC recording from the new
  repo's `audio_record_play_aac.lua`-style recorder (`volume=100`, i.e. the
  30dB gain ceiling — see note below) saying "Hello"; Whisper correctly
  returned `"Hello"`. `faster-whisper` decodes via ffmpeg under the hood,
  which explains why non-WAV input "just works" without any special
  handling needed on the server side.
- **Mic gain ceiling is NOT a blocker for Whisper transcription.** New
  repo's `audio.new_input()` `volume` parameter is capped at 30dB
  (`AUDIO_INPUT_GAIN_DB_MAX = 30.0f` in `audio_private.h`), noticeably
  lower/quieter to the ear than the old repo's effective gain (which
  combined `adc_init_gain: 30` in YAML with an explicit `50.0` runtime
  boost). Despite sounding faint on playback, Whisper still transcribed
  correctly at this lower level — confirmed "good enough to hear" and
  "good enough for STT" are different bars, and the lower one already
  clears. No need to chase a hardware-level `adc_init_gain` increase
  unless a future real-world test (further from the mic, noisier room,
  etc.) shows transcription accuracy problems.

**Common failure mode:** Forgetting `uvicorn server:app --host 0.0.0.0
--port 8001` and instead running `python server.py` directly. Looks
successful (no error, clean exit) but the server never actually starts
listening — `curl`/board requests to the `/transcribe` endpoint will fail
to connect since nothing is bound to port 8001.

## Design principle: audio file housekeeping (applies to STT/TTS wiring)

With ~2GB of SD storage confirmed working (see SD card section above) and a
real, recurring record→transcribe / generate→play pipeline about to be
built, audio files need a deliberate lifecycle from the start rather than
being retrofitted later. Principles to follow when implementing the
Whisper/Piper wiring:

1. **Recordings and TTS output are ephemeral by default.** Once a recorded
   clip has been transcribed and the text handed off, or a TTS response
   has been played back, the audio file has done its job. Delete it as
   part of the same flow that consumes it — don't leave cleanup as a
   separate, easy-to-forget step.
2. **Prefer fixed, reused filenames for steady-state pipeline traffic**
   (e.g. always `/sdcard/tmp/recording.wav`, always
   `/sdcard/tmp/response.wav`), overwritten each cycle. This gives zero
   accumulation by construction — no separate cleanup logic needed for the
   common case. Reserve unique/timestamped filenames only for cases where
   history is deliberately wanted (debugging, training data capture).
3. **If deliberate retention is wanted**, use a clearly-named subdirectory
   with an explicit policy (keep last N files, or auto-delete anything
   older than X days) rather than letting files accumulate unbounded.
4. **Check whether `cap_files` (already registered — confirmed in boot
   log: `Register files cap ok`) already exposes a delete/cleanup
   capability the agent itself can call.** If so, the agent can clean up
   after itself as a natural part of its own response flow, rather than
   requiring a separate housekeeping script or scheduled task. Worth
   checking `cap_files.c`'s actual capability list before building a
   custom cleanup mechanism.

This should be designed into the STT/TTS capability wiring from the start
(item 3 in "Suggested order of work" below), not added as an afterthought
once storage pressure becomes a visible problem.

## Whisper/voice-agent integration — architecture decision pending

See BACKLOG.md item 5 for full detail. Core open question: should the *Lua
script* orchestrate (transcribe-then-publish) or should the *agent* orchestrate
(publish "voice note exists at path X", let the LLM's tool-calling invoke a new
`transcribe_audio` capability)? Leaning toward agent-orchestrated for
architectural consistency with existing capabilities (`tg_send_message`,
`inspect_image`), but not decided. Either path needs a multipart-capable HTTP
mechanism that doesn't currently exist (`cap_http_request` only supports
simple string bodies).

## Suggested order of work for next session

1. Re-verify mic/audio capture still works correctly on new repo (quick
   regression check using existing `tmp_miclevel.lua`/`test_record.lua`
   patterns, before building anything new on top).
2. Diff old vs. new `main/` and any UI-related Lua scripts to produce a
   concrete "what's actually missing" checklist (replaces the vague "UI and
   everything" framing with a real list).
3. Decide and implement the Whisper multipart path (custom C capability vs.
   Lua-rolled multipart vs. extending `claw_core_llm_infer_media`'s media-kind
   abstraction).
4. Wire transcription result into `claw_event_router` via
   `event_publisher.publish_message()` (mechanism already confirmed working).
5. Revisit emote assets once core functionality is solid — lowest urgency
   item, purely cosmetic.
