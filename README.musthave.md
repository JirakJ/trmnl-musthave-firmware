# trmnl-musthave-firmware

Fork of [usetrmnl/firmware](https://github.com/usetrmnl/firmware) for the TRMNL OG, fed by the
[trmnl-musthave](https://github.com/JirakJ/trmnl-musthave) BYOS server. GPLv3, like upstream. Branch `musthave`.

## What is different

- **Protocol v1** (`lib/trmnl/…/api_types.h`, `parse_response_api_display.cpp`, `src/bl.cpp`): the device sends
  `X-Frame-Id` (frame currently on the panel, persisted in NVS after every drawn image); the server answers
  `action` `none` | `partial` | `full`, `frame_id`, `full_url`, `regions_url`, `full_mode`, `sleep_mode`, `ota_wait`.
  `none` = nothing downloaded or drawn. `full` + `full_mode=full` = real full refresh (ghost cleaning); otherwise the
  stock non-flashing partial refresh against the cached previous image. Stock servers keep working (fallback to
  `image_url`/`filename`).
- **Update waiting mode**: `ota_wait: true` → poll every `refresh_rate` seconds without drawing, show
  "Waiting for firmware update from jakubjirak.com", update as soon as the server offers a build, then back to sleep.
  No daily OTA throttle (the server only offers a build when the version differs).
- **Build-time server URL**: `BYOS_SERVER_URL` is written to NVS at boot (forces a fresh `/api/setup`), so the
  device is repointed without the captive portal.
- **Branding**: loading screen (`src/loading.h`), small logo (`src/logo_small.h`), medium logo, OTA messages —
  "TRMNL enhanced by jakubjirak.com". Images are Group5-compressed `BB_BITMAP`s generated with a tiny host tool built
  from bb_epaper's `Group5.cpp` (see `scripts/` history / the project notes).
- `[platformio] workspace_dir` outside the repo (faster git and security scans).

## Build and flash

```bash
uv tool install platformio                      # or pipx
export BYOS_SERVER_URL=http://192.168.0.84:8080   # your BYOS server, no trailing slash
pio run -e trmnl_byos                        # → ~/.platformio/workspaces/trmnl-musthave/build/trmnl_byos/firmware.bin
pio run -e trmnl_byos -t upload --upload-port /dev/cu.usbmodemXXXX
pio test -e native                               # unit tests incl. test_v1_parse, test_v1_headers
```

First flash over USB-C: power off, hold the button, power on, release (the ESP32-C3 shows up as `/dev/cu.usbmodem*`).
Later updates go over the air: copy `firmware.bin` to the server's `deploy/firmware/latest.bin` and write the version
into `firmware_version.txt`; `python3 -m musthave ota-mode on` makes the device wait for it with 20 s polls.

Back to stock: flash the upstream release, or the full 4 MB backup taken with `esptool read_flash 0 0x400000`.

## Upstream

The protocol/flow part without images or branding is proposed upstream as
[usetrmnl/trmnl-firmware#647](https://github.com/usetrmnl/trmnl-firmware/pull/647).
