#pragma once

#include <RadioLib.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace esphome::radiolib_wh51_sx1262 {

class EspIdfRadioLibHal : public RadioLibHal {
 public:
  EspIdfRadioLibHal(int sck, int miso, int mosi)
      : RadioLibHal(GPIO_MODE_INPUT, GPIO_MODE_OUTPUT, 0, 1, GPIO_INTR_POSEDGE, GPIO_INTR_NEGEDGE),
        sck_(sck), miso_(miso), mosi_(mosi) {}

  void init() override { this->spiBegin(); }
  void term() override { this->spiEnd(); }

  void pinMode(uint32_t pin, uint32_t mode) override {
    if (pin == RADIOLIB_NC)
      return;
    gpio_config_t config{};
    config.pin_bit_mask = 1ULL << pin;
    config.mode = static_cast<gpio_mode_t>(mode);
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&config);
  }

  void digitalWrite(uint32_t pin, uint32_t value) override {
    if (pin != RADIOLIB_NC)
      gpio_set_level(static_cast<gpio_num_t>(pin), value);
  }
  uint32_t digitalRead(uint32_t pin) override {
    return pin == RADIOLIB_NC ? 0 : gpio_get_level(static_cast<gpio_num_t>(pin));
  }

  void attachInterrupt(uint32_t pin, void (*callback)(), uint32_t mode) override {
    if (pin == RADIOLIB_NC)
      return;
    gpio_install_isr_service(0);
    callbacks_[pin] = callback;
    gpio_set_intr_type(static_cast<gpio_num_t>(pin), static_cast<gpio_int_type_t>(mode));
    gpio_isr_handler_add(static_cast<gpio_num_t>(pin), &EspIdfRadioLibHal::gpio_isr_,
                         reinterpret_cast<void *>(pin));
  }
  void detachInterrupt(uint32_t pin) override {
    if (pin == RADIOLIB_NC)
      return;
    gpio_isr_handler_remove(static_cast<gpio_num_t>(pin));
    gpio_set_intr_type(static_cast<gpio_num_t>(pin), GPIO_INTR_DISABLE);
    callbacks_[pin] = nullptr;
  }

  void delay(RadioLibTime_t ms) override { vTaskDelay(pdMS_TO_TICKS(ms) ?: 1); }
  void delayMicroseconds(RadioLibTime_t us) override {
    const int64_t end = esp_timer_get_time() + us;
    while (esp_timer_get_time() < end) {}
  }
  RadioLibTime_t millis() override { return esp_timer_get_time() / 1000ULL; }
  RadioLibTime_t micros() override { return esp_timer_get_time(); }
  long pulseIn(uint32_t pin, uint32_t state, RadioLibTime_t timeout) override {
    const RadioLibTime_t start = this->micros();
    while (this->digitalRead(pin) == state)
      if (this->micros() - start >= timeout)
        return 0;
    while (this->digitalRead(pin) != state)
      if (this->micros() - start >= timeout)
        return 0;
    const RadioLibTime_t pulse = this->micros();
    while (this->digitalRead(pin) == state)
      if (this->micros() - pulse >= timeout)
        return 0;
    return this->micros() - pulse;
  }

  void spiBegin() override {
    spi_bus_config_t bus{};
    bus.mosi_io_num = this->mosi_;
    bus.miso_io_num = this->miso_;
    bus.sclk_io_num = this->sck_;
    bus.quadwp_io_num = -1;
    bus.quadhd_io_num = -1;
    bus.max_transfer_sz = 256;
    if (spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO) != ESP_OK)
      return;
    spi_device_interface_config_t device{};
    device.clock_speed_hz = 2'000'000;
    device.mode = 0;
    device.spics_io_num = -1;
    device.queue_size = 1;
    spi_bus_add_device(SPI2_HOST, &device, &this->spi_);
  }
  void spiBeginTransaction() override {}
  void spiTransfer(uint8_t *out, size_t len, uint8_t *in) override {
    spi_transaction_t transaction{};
    transaction.length = len * 8;
    transaction.tx_buffer = out;
    transaction.rx_buffer = in;
    spi_device_polling_transmit(this->spi_, &transaction);
  }
  void spiEndTransaction() override {}
  void spiEnd() override {
    if (this->spi_ != nullptr) {
      spi_bus_remove_device(this->spi_);
      this->spi_ = nullptr;
      spi_bus_free(SPI2_HOST);
    }
  }

 private:
  static void gpio_isr_(void *arg) {
    const uint32_t pin = reinterpret_cast<uintptr_t>(arg);
    if (pin < GPIO_NUM_MAX && callbacks_[pin] != nullptr)
      callbacks_[pin]();
  }
  inline static void (*callbacks_[GPIO_NUM_MAX])() = {};
  int sck_;
  int miso_;
  int mosi_;
  spi_device_handle_t spi_{nullptr};
};

}  // namespace esphome::radiolib_wh51_sx1262
