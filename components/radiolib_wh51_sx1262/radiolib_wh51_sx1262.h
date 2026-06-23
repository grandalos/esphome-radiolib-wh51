#pragma once

#include <RadioLib.h>
#include "esp_idf_hal.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/component.h"
#include <array>
#include <deque>
#include <memory>
#include <vector>

namespace esphome::radiolib_wh51_sx1262 {

class RadioLibWH51SX1262 : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  void set_radio_config(float frequency, float bit_rate, float deviation, float bandwidth,
                        uint16_t preamble_length, float tcxo_voltage) {
    this->frequency_ = frequency;
    this->bit_rate_ = bit_rate;
    this->deviation_ = deviation;
    this->bandwidth_ = bandwidth;
    this->preamble_length_ = preamble_length;
    this->tcxo_voltage_ = tcxo_voltage;
  }
  void set_sync_word(const std::vector<uint8_t> &value) { this->sync_word_ = value; }
  void set_pins(int sck, int miso, int mosi, int cs, int rst, int busy, int dio1, int antenna_switch) {
    this->sck_ = sck;
    this->miso_ = miso;
    this->mosi_ = mosi;
    this->cs_ = cs;
    this->rst_ = rst;
    this->busy_ = busy;
    this->dio1_ = dio1;
    this->antenna_switch_ = antenna_switch;
  }
  void set_led_config(int pin, bool active_low, uint32_t flash_duration_ms) {
    this->led_pin_ = pin;
    this->led_active_low_ = active_low;
    this->led_flash_duration_ms_ = flash_duration_ms;
  }
  void set_frame_monitoring(uint32_t interval_ms, uint32_t tolerance_ms) {
    this->frame_interval_ms_ = interval_ms;
    this->frame_tolerance_ms_ = tolerance_ms;
  }

  void add_sensor(uint32_t sensor_id, sensor::Sensor *moisture, sensor::Sensor *raw_ad,
                  sensor::Sensor *battery_voltage, sensor::Sensor *battery_level, sensor::Sensor *boost,
                  sensor::Sensor *rssi, sensor::Sensor *lost_frames, binary_sensor::BinarySensor *battery_low,
                  text_sensor::TextSensor *last_frame);

 protected:
  struct Wh51Sensor {
    uint32_t id;
    sensor::Sensor *moisture;
    sensor::Sensor *raw_ad;
    sensor::Sensor *battery_voltage;
    sensor::Sensor *battery_level;
    sensor::Sensor *boost;
    sensor::Sensor *rssi;
    sensor::Sensor *lost_frames;
    binary_sensor::BinarySensor *battery_low;
    text_sensor::TextSensor *last_frame;
    uint32_t next_expected_frame_ms{0};
    uint32_t last_loss_publish_ms{0};
    bool frame_monitoring_started{false};
    std::deque<uint32_t> lost_frame_times;
  };

  static void packet_received_();
  void process_packet_(std::array<uint8_t, 14> &frame, float rssi);
  void handle_valid_frame_(Wh51Sensor *sensor);
  void update_frame_monitoring_();
  void publish_lost_frames_(Wh51Sensor *sensor, uint32_t now);
  void set_led_(bool on);
  static uint8_t crc8_(const uint8_t *data, size_t length);

  inline static RadioLibWH51SX1262 *instance_{nullptr};
  volatile bool packet_ready_{false};
  std::unique_ptr<EspIdfRadioLibHal> hal_;
  std::unique_ptr<Module> module_;
  std::unique_ptr<SX1262> radio_;

  std::vector<Wh51Sensor> sensors_;
  float frequency_{868.35f};
  float bit_rate_{17.24f};
  float deviation_{33.5f};
  float bandwidth_{234.3f};
  float tcxo_voltage_{1.8f};
  uint16_t preamble_length_{64};
  std::vector<uint8_t> sync_word_;
  int sck_{7}, miso_{8}, mosi_{9}, cs_{41}, rst_{42}, busy_{40}, dio1_{39}, antenna_switch_{38};
  int led_pin_{21};
  bool led_active_low_{true};
  bool led_on_{false};
  uint32_t led_flash_duration_ms_{100};
  uint32_t led_off_at_{0};

  uint32_t frame_interval_ms_{70000};
  uint32_t frame_tolerance_ms_{10000};
};

}  // namespace esphome::radiolib_wh51_sx1262
