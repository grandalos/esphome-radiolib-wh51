#include "radiolib_wh51_sx1262.h"
#include "driver/gpio.h"
#include "esphome/core/log.h"
#include <algorithm>
#include <ctime>

namespace esphome::radiolib_wh51_sx1262 {

static const char *const TAG = "radiolib_sx1262";
static constexpr uint32_t LOSS_WINDOW_MS = 24UL * 60UL * 60UL * 1000UL;
static constexpr uint32_t LOSS_PUBLISH_INTERVAL_MS = 60UL * 1000UL;

void RadioLibWH51SX1262::packet_received_() {
  if (instance_ != nullptr)
    instance_->packet_ready_ = true;
}

void RadioLibWH51SX1262::add_sensor(uint32_t sensor_id, sensor::Sensor *moisture, sensor::Sensor *raw_ad,
                                    sensor::Sensor *battery_voltage, sensor::Sensor *battery_level,
                                    sensor::Sensor *boost, sensor::Sensor *rssi, sensor::Sensor *lost_frames,
                                    binary_sensor::BinarySensor *battery_low, text_sensor::TextSensor *last_frame) {
  this->sensors_.push_back({sensor_id, moisture, raw_ad, battery_voltage, battery_level, boost, rssi, lost_frames,
                            battery_low, last_frame});
}

void RadioLibWH51SX1262::setup() {
  instance_ = this;
  gpio_set_direction(static_cast<gpio_num_t>(this->antenna_switch_), GPIO_MODE_OUTPUT);
  gpio_set_level(static_cast<gpio_num_t>(this->antenna_switch_), 1);
  gpio_set_direction(static_cast<gpio_num_t>(this->led_pin_), GPIO_MODE_OUTPUT);
  this->set_led_(false);

  this->hal_ = std::make_unique<EspIdfRadioLibHal>(this->sck_, this->miso_, this->mosi_);
  this->module_ =
      std::make_unique<Module>(this->hal_.get(), this->cs_, this->dio1_, this->rst_, this->busy_);
  this->radio_ = std::make_unique<SX1262>(this->module_.get());

  int16_t state = this->radio_->beginFSK(this->frequency_, this->bit_rate_, this->deviation_,
                                         this->bandwidth_, 10, this->preamble_length_,
                                         this->tcxo_voltage_, false);
  if (state == RADIOLIB_ERR_NONE)
    state = this->radio_->setSyncWord(this->sync_word_.data(), this->sync_word_.size());
  if (state == RADIOLIB_ERR_NONE)
    state = this->radio_->setCRC(0);
  if (state == RADIOLIB_ERR_NONE)
    state = this->radio_->fixedPacketLengthMode(14);
  if (state == RADIOLIB_ERR_NONE) {
    this->radio_->setPacketReceivedAction(&RadioLibWH51SX1262::packet_received_);
    state = this->radio_->startReceive();
  }
  if (state != RADIOLIB_ERR_NONE) {
    ESP_LOGE(TAG, "RadioLib SX1262 initialization failed: %d", state);
    this->mark_failed();
    return;
  }
  ESP_LOGI(TAG, "RadioLib SX1262 receiver initialized");
  for (auto &sensor : this->sensors_)
    sensor.lost_frames->publish_state(0);
}

void RadioLibWH51SX1262::loop() {
  this->update_frame_monitoring_();
  if (!this->packet_ready_ || this->radio_ == nullptr)
    return;
  this->packet_ready_ = false;
  const float rssi = this->radio_->getRSSI(true);
  std::array<uint8_t, 14> frame{};
  const int16_t state = this->radio_->readData(frame.data(), frame.size());
  this->radio_->startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    ESP_LOGW(TAG, "RadioLib readData failed: %d", state);
    return;
  }
  ESP_LOGD(TAG, "Raw packet (14 bytes), RSSI %.1f dBm: %s", rssi,
           format_hex(frame.data(), frame.size()).c_str());
  this->process_packet_(frame, rssi);
}

uint8_t RadioLibWH51SX1262::crc8_(const uint8_t *data, size_t length) {
  uint8_t crc = 0;
  for (size_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; bit++)
      crc = (crc & 0x80) ? static_cast<uint8_t>((crc << 1) ^ 0x31) : static_cast<uint8_t>(crc << 1);
  }
  return crc;
}

void RadioLibWH51SX1262::process_packet_(std::array<uint8_t, 14> &frame, float rssi) {
  if (frame[0] != 0x51)
    return;
  if (frame[5] != 0x7F)
    frame[5] = 0x7F;
  frame[7] = (frame[7] & 0x01) | 0xF8;
  for (size_t i = 9; i <= 11; i++)
    frame[i] = 0xFF;

  uint8_t checksum = 0;
  for (size_t i = 0; i < 13; i++)
    checksum += frame[i];
  const uint8_t expected_crc = crc8_(frame.data(), 12);
  if (expected_crc != frame[12] || checksum != frame[13]) {
    ESP_LOGW(TAG, "Invalid WH51 integrity: CRC %02X/%02X, sum %02X/%02X", expected_crc, frame[12], checksum,
             frame[13]);
    return;
  }

  const uint32_t sensor_id =
      (static_cast<uint32_t>(frame[1]) << 16) | (static_cast<uint32_t>(frame[2]) << 8) | frame[3];
  auto *matched_sensor = static_cast<Wh51Sensor *>(nullptr);
  for (auto &sensor : this->sensors_) {
    if (sensor.id == sensor_id) {
      matched_sensor = &sensor;
      break;
    }
  }
  if (matched_sensor == nullptr) {
    ESP_LOGD(TAG, "Ignoring unconfigured WH51 ID=%06X", sensor_id);
    return;
  }
  const uint8_t boost = (frame[4] & 0xE0) >> 5;
  const uint16_t battery_mv = (frame[4] & 0x1F) * 100;
  const float battery_level = std::clamp((battery_mv - 700.0f) / 900.0f * 100.0f, 0.0f, 100.0f);
  const uint8_t moisture = frame[6];
  const uint16_t raw_ad = (static_cast<uint16_t>(frame[7] & 0x01) << 8) | frame[8];
  if (moisture > 100)
    return;

  this->handle_valid_frame_(matched_sensor);
  ESP_LOGI(TAG, "WH51 ID=%06X moisture=%u%% AD=%u battery=%.1fV boost=%u RSSI=%.1f", sensor_id, moisture,
           raw_ad, battery_mv / 1000.0f, boost, rssi);
  matched_sensor->moisture->publish_state(moisture);
  matched_sensor->raw_ad->publish_state(raw_ad);
  matched_sensor->battery_voltage->publish_state(battery_mv / 1000.0f);
  matched_sensor->battery_level->publish_state(battery_level);
  matched_sensor->boost->publish_state(boost);
  matched_sensor->rssi->publish_state(rssi);
  matched_sensor->battery_low->publish_state(battery_mv <= 800);

  const time_t now = time(nullptr);
  if (now > 100000) {
    char timestamp[24];
    struct tm local_time;
    localtime_r(&now, &local_time);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &local_time);
    matched_sensor->last_frame->publish_state(timestamp);
  }
}

void RadioLibWH51SX1262::handle_valid_frame_(Wh51Sensor *sensor) {
  const uint32_t now = millis();
  this->set_led_(true);
  this->led_off_at_ = now + this->led_flash_duration_ms_;
  sensor->frame_monitoring_started = true;
  sensor->next_expected_frame_ms = now + this->frame_interval_ms_;
  this->publish_lost_frames_(sensor, now);
}

void RadioLibWH51SX1262::update_frame_monitoring_() {
  const uint32_t now = millis();
  if (this->led_on_ && static_cast<int32_t>(now - this->led_off_at_) >= 0)
    this->set_led_(false);

  bool loss_changed = false;
  for (auto &sensor : this->sensors_) {
    loss_changed = false;
    if (sensor.frame_monitoring_started) {
      while (static_cast<int32_t>(now - (sensor.next_expected_frame_ms + this->frame_tolerance_ms_)) >= 0) {
        sensor.lost_frame_times.push_back(sensor.next_expected_frame_ms);
        sensor.next_expected_frame_ms += this->frame_interval_ms_;
        loss_changed = true;
      }
    }

    while (!sensor.lost_frame_times.empty() &&
           static_cast<uint32_t>(now - sensor.lost_frame_times.front()) >= LOSS_WINDOW_MS) {
      sensor.lost_frame_times.pop_front();
      loss_changed = true;
    }

    if (loss_changed || static_cast<uint32_t>(now - sensor.last_loss_publish_ms) >= LOSS_PUBLISH_INTERVAL_MS)
      this->publish_lost_frames_(&sensor, now);
  }
}

void RadioLibWH51SX1262::publish_lost_frames_(Wh51Sensor *sensor, uint32_t now) {
  sensor->last_loss_publish_ms = now;
  sensor->lost_frames->publish_state(sensor->lost_frame_times.size());
}

void RadioLibWH51SX1262::set_led_(bool on) {
  this->led_on_ = on;
  const int level = this->led_active_low_ ? !on : on;
  gpio_set_level(static_cast<gpio_num_t>(this->led_pin_), level);
}

void RadioLibWH51SX1262::dump_config() {
  ESP_LOGCONFIG(TAG, "RadioLib WH51 SX1262 receiver:");
  ESP_LOGCONFIG(TAG, "  Configured sensors: %u", this->sensors_.size());
  for (const auto &sensor : this->sensors_)
    ESP_LOGCONFIG(TAG, "    WH51 ID: %06X", sensor.id);
  ESP_LOGCONFIG(TAG, "  Frequency: %.3f MHz", this->frequency_);
  ESP_LOGCONFIG(TAG, "  Bit rate: %.3f kbps", this->bit_rate_);
  ESP_LOGCONFIG(TAG, "  Deviation: %.1f kHz", this->deviation_);
  ESP_LOGCONFIG(TAG, "  Bandwidth: %.1f kHz", this->bandwidth_);
  ESP_LOGCONFIG(TAG, "  TCXO: %.1f V", this->tcxo_voltage_);
  ESP_LOGCONFIG(TAG, "  Pins: SCK=%d MISO=%d MOSI=%d CS=%d RST=%d BUSY=%d DIO1=%d ANT=%d",
                this->sck_, this->miso_, this->mosi_, this->cs_, this->rst_, this->busy_, this->dio1_,
                this->antenna_switch_);
  ESP_LOGCONFIG(TAG, "  RX LED: GPIO%d, active %s, flash %u ms", this->led_pin_,
                this->led_active_low_ ? "low" : "high", this->led_flash_duration_ms_);
  ESP_LOGCONFIG(TAG, "  Expected frame interval: %u ms (+%u ms tolerance)", this->frame_interval_ms_,
                this->frame_tolerance_ms_);
}

}  // namespace esphome::radiolib_wh51_sx1262
