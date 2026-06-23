# ESPHome RadioLib WH51 Receiver

ESPHome external component for receiving Ecowitt WH51 soil moisture sensor
frames with an SX1262 radio and RadioLib.

The component was tested with:

- Seeed Studio XIAO ESP32-S3
- Seeed Studio Wio-SX1262
- ESP-IDF
- RadioLib 7.7.1
- Ecowitt WH51 at 868 MHz

It exposes soil moisture, raw AD value, battery voltage and level,
transmission boost, RSSI, battery-low state, last frame time, and the number
of frames missed during the last 24 hours. The onboard XIAO LED can flash
after a valid frame from the configured sensor ID.

## ESPHome configuration

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/grandalos/esphome-radiolib-wh51
      ref: main
    components: [radiolib_wh51_sx1262]

radiolib_wh51_sx1262:
  id: wh51_radio
  frequency: 868.35
  bit_rate: 17.24
  frequency_deviation: 33.5
  bandwidth: 234.3
  preamble_length: 64
  sync_word: [0x2D, 0xD4]
  tcxo_voltage: 1.8

  sck_pin: 7
  miso_pin: 8
  mosi_pin: 9
  cs_pin: 41
  rst_pin: 42
  busy_pin: 40
  dio1_pin: 39
  antenna_switch_pin: 38

  led_pin: 21
  led_active_low: true
  led_flash_duration: 100ms
  frame_interval: 70s
  frame_tolerance: 10s

  sensors:
    - id: 0x0FCD59
      moisture: { name: "0FCD59 - Soil Moisture" }
      raw_ad: { name: "0FCD59 - Raw AD" }
      battery_voltage: { name: "0FCD59 - Battery Voltage" }
      battery_level: { name: "0FCD59 - Battery Level" }
      boost: { name: "0FCD59 - Transmission Boost" }
      rssi: { name: "0FCD59 - RSSI" }
      lost_frames: { name: "0FCD59 - Lost Frames - 24h" }
      battery_low: { name: "0FCD59 - Battery Low" }
      last_frame: { name: "0FCD59 - Last Frame" }

    - id: 0x00D890
      moisture: { name: "00D890 - Soil Moisture" }
      raw_ad: { name: "00D890 - Raw AD" }
      battery_voltage: { name: "00D890 - Battery Voltage" }
      battery_level: { name: "00D890 - Battery Level" }
      boost: { name: "00D890 - Transmission Boost" }
      rssi: { name: "00D890 - RSSI" }
      lost_frames: { name: "00D890 - Lost Frames - 24h" }
      battery_low: { name: "00D890 - Battery Low" }
      last_frame: { name: "00D890 - Last Frame" }
```

Frame-loss monitoring is tracked independently for each configured sensor and
starts after its first valid frame. A frame is counted as lost when it is not
received within `frame_interval + frame_tolerance`. The rolling history is kept
in RAM and resets when the ESP restarts.

## RadioLib version

RadioLib is pinned in the component to version 7.7.1. Update the version in
`components/radiolib_wh51_sx1262/__init__.py` and rebuild to test a newer
release.
