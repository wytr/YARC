# YARC — Yet Another Reflow Controller

An ESP32-based reflow-oven controller with an on-device TFT UI, a WiFi access-point web interface, and per-phase PID tuning.

![YARC logo](data/logo.png)

## Features

- Live K-type thermocouple reading via MAX6675 (250 ms update interval).
- Five-phase reflow state machine: `IDLE → PREHEAT → SOAK → REFLOWRAMP → REFLOW → COOLDOWN`.
- Phase-specific PID tuning (separate Kp/Ki/Kd for preheat, soak, and reflow).
- On-device TFT GUI powered by [LVGL](https://lvgl.io/) 7.
- Built-in WiFi access point with web UI for:
  - Live monitoring (WebSocket-streamed chart, CSV download).
  - Profile create / list / remove (persisted to SPIFFS).
- Passive-buzzer feedback (startup tone, phase changes, alarm).
- DS3231 RTC for a wall clock on the display.
- All web assets are served from the on-device SPIFFS partition.
- A prebuilt firmware binary is shipped in [prebuilt/](prebuilt/).

## Hardware / BOM

| Part | Notes |
| --- | --- |
| ESP32 dev board | Tested on `az-delivery-devkit-v4`; any ESP32 with enough GPIOs should work. |
| MAX6675 + K-type thermocouple | Reads oven-chamber temperature. |
| DS3231 RTC module | I²C wall clock. |
| TFT display | ILI-class panel driven by [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI); see [configexamples/](configexamples/) for a known-good config. |
| Solid-state relay (SSR) | Sized for your oven's mains current. Switched by GPIO 33. |
| Passive buzzer | GPIO 32. |
| Toaster oven | Anything with a resistive heating element. **Mains wiring is dangerous — proceed only if you know what you're doing.** |

## Pin map

Defined in [include/periphery.h](include/periphery.h):

| Function | GPIO |
| --- | --- |
| MAX6675 DO | 26 |
| MAX6675 CS | 27 |
| MAX6675 CLK | 14 |
| MAX6675 VCC | 12 |
| SSR output | 33 |
| Buzzer | 32 |

The TFT pins are not set in this repo — configure them in `TFT_eSPI/User_Setup.h` (see [configexamples/TFT_eSPI_USER_SETUP.h](configexamples/TFT_eSPI_USER_SETUP.h)). The DS3231 uses the ESP32's default I²C pins.

## Reflow profiles

Profiles are stored in [data/profiles.json](data/profiles.json) and served from SPIFFS. Schema:

```json
{
  "name": "leaded",
  "soakRampRate": "2",
  "soakTemperature": "150",
  "soakDuration": "80",
  "reflowRampRate": "2",
  "reflowTemperature": "230",
  "reflowDuration": "60"
}
```

| Field | Unit | Meaning |
| --- | --- | --- |
| `name` | — | Display name; must be unique. |
| `soakRampRate` | °C/s | Slope from ambient up to `soakTemperature`. |
| `soakTemperature` | °C | Plateau before reflow ramp. |
| `soakDuration` | s | Time held at `soakTemperature`. |
| `reflowRampRate` | °C/s | Slope from soak up to `reflowTemperature`. |
| `reflowTemperature` | °C | Peak temperature. |
| `reflowDuration` | s | Time held above reflow temperature. |

`soakRampDuration` and `reflowRampDuration` are **derived at runtime** from the ambient temperature measured at the start of the cycle ([src/main.cpp:110-112](src/main.cpp#L110-L112)) and from the rate/setpoint pair ([src/profile.cpp:21](src/profile.cpp#L21)) — you don't supply them.

## PID tuning

Defaults are compiled in at [include/pid_setup.h](include/pid_setup.h):

| Phase | Kp | Ki | Kd |
| --- | --- | --- | --- |
| Preheat | 100 | 0.025 | 20 |
| Soak | 300 | 0.05 | 250 |
| Reflow | 300 | 0.05 | 350 |

Sample time is 1000 ms. These are reasonable starting values — re-tune for your oven's thermal mass.

## Build & flash (from source)

1. Install [VS Code](https://code.visualstudio.com/) and the [PlatformIO](https://platformio.org/) extension.
2. Clone this repo and open the folder in VS Code. PlatformIO will fetch the libraries listed in [platformio.ini](platformio.ini) into `.pio/libdeps/az-delivery-devkit-v4/`.
3. Replace `TFT_eSPI/User_Setup.h` with [configexamples/TFT_eSPI_USER_SETUP.h](configexamples/TFT_eSPI_USER_SETUP.h) (path: `.pio/libdeps/az-delivery-devkit-v4/TFT_eSPI/User_Setup.h`).
4. Replace `lvgl/lv_conf.h` with [configexamples/lv_conf_ESP32_ILI_setup.h](configexamples/lv_conf_ESP32_ILI_setup.h) (path: `.pio/libdeps/az-delivery-devkit-v4/lvgl/lv_conf.h`). Make sure `LV_USE_CANVAS 1` is set.
5. Patch `lv_lib_qrcode/src/lv_qrcode.c` lines 49–52 per [todo.txt](todo.txt) (the LVGL 6/7 `lv_mem_assert` / `LV_ASSERT_MEM` guard).
6. PlatformIO → **Build** → **Upload** to flash the firmware.
7. PlatformIO → Project Tasks → **Build Filesystem Image** → **Upload Filesystem Image** to flash the SPIFFS partition. **This step is mandatory** — without the web assets in [data/](data/) the firmware will crash on boot.
8. If your wiring differs, edit the GPIO defines in [include/periphery.h](include/periphery.h) and rebuild.

## Flash the prebuilt binary

A prebuilt firmware image is available in [prebuilt/firmware.bin](prebuilt/firmware.bin):

```bash
esptool.py --chip esp32 --port /dev/ttyUSB0 write_flash 0x10000 prebuilt/firmware.bin
```

The SPIFFS image is **not** prebuilt — you still need to run the "Build Filesystem Image / Upload Filesystem Image" steps above from PlatformIO, or the controller will crash on first boot.

## Web interface

On power-up the controller starts a WiFi access point:

- **SSID:** `YARC`
- **Password:** `123456789`

(Defined in [src/yarcweb.cpp:3-4](src/yarcweb.cpp#L3-L4) — change them there before flashing if you want different credentials.)

The device IP is shown on the TFT (typically `192.168.4.1` in SoftAP mode). Connect to the AP and open the IP in a browser. Available routes:

| Route | Purpose |
| --- | --- |
| `/` | Home — links to monitoring and profiles. |
| `/monitoring` | Live chart of target vs. measured temperature with CSV download. |
| `/profiles` | List existing profiles. |
| `/create-profile` | Form to add a new profile. |

A WebSocket at `/ws` streams `time;target;current` rows once per second while monitoring is active ([src/main.cpp:60-61](src/main.cpp#L60-L61), [src/yarcweb.cpp:27-29](src/yarcweb.cpp#L27-L29)).

## Troubleshooting

- **`THERMOCOUPLE NOT CONNECTED OR DAMAGED!` on the serial monitor / warning icon on the TFT** — the firmware forces `COOLDOWN` whenever the reading is `NaN` or exactly `0.0` ([src/main.cpp:63-69](src/main.cpp#L63-L69)). Check the MAX6675 wiring and that the thermocouple tip is intact.
- **Boot loop or panic on startup** — the SPIFFS image isn't flashed. Run "Upload Filesystem Image".
- **Black / garbled TFT** — `TFT_eSPI/User_Setup.h` wasn't replaced with the example, or driver/pins don't match your panel.
- **LVGL compile errors about `canvas`** — `LV_USE_CANVAS` isn't enabled in `lv_conf.h`.
- **lv_lib_qrcode fails to compile** — apply the patch from [todo.txt](todo.txt).

## Roadmap / TODO

- Integrate [IotWebConf](https://github.com/prampec/IotWebConf) so the controller can join a local WiFi instead of running its own AP, reachable as `http://yarc.local/`.
- Track `maxProcessTemperature` across a cycle (marked TODO at [include/phase.h:23](include/phase.h#L23)).
- Move the GPIO defines out of [include/periphery.h](include/periphery.h) into [include/pindefines.h](include/pindefines.h) (currently a stub).

## License

Released under the **MIT License**. (TODO: add a top-level `LICENSE` file.)

## Acknowledgements

This project stands on the shoulders of:

- [LVGL](https://github.com/lvgl/lvgl) — embedded graphics library.
- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) — TFT driver.
- [AsyncTCP](https://github.com/me-no-dev/AsyncTCP) and [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) — non-blocking HTTP + WebSocket stack.
- [RTClib](https://github.com/adafruit/RTClib) — DS3231 driver.
- [MAX6675 library](https://github.com/adafruit/MAX6675-library) — thermocouple driver.
- [Tone32](https://github.com/lbernstone/Tone32) — buzzer tones on ESP32.
- [PID](https://github.com/br3ttb/Arduino-PID-Library) — Brett Beauregard's PID library.
- [Plotter](https://github.com/devinaconley/arduino-plotter) — serial plotter helper.
- [lv_lib_qrcode](https://github.com/lvgl/lv_lib_qrcode) — QR-code rendering for LVGL.
- [ArduinoJson](https://arduinojson.org/) — JSON parser/serializer.
- [Materialize CSS](https://materializecss.com/) — web UI styling.
- [Chart.js](https://www.chartjs.org/) — live monitoring chart.
