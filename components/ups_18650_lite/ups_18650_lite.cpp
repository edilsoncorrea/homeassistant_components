#include "ups_18650_lite.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace ups_18650_lite {

// MAX174043 is a 1-Cell Fuel Gauge with ModelGauge and Low-Battery Alert
// Consult the datasheet at https://www.analog.com/en/products/max17043.html

static const char *const TAG = "ups_18650_lite";

static const uint8_t UPS_18650_LITE_VCELL = 0x02;
static const uint8_t UPS_18650_LITE_SOC = 0x04;
static const uint8_t UPS_18650_LITE_CONFIG = 0x0c;

static const uint16_t UPS_18650_LITE_CONFIG_POWER_UP_DEFAULT = 0x971C;
static const uint16_t UPS_18650_LITE_CONFIG_SAFE_MASK = 0xFF1F;  // mask out sleep bit (7), unused bit (6) and alert bit (4)
static const uint16_t UPS_18650_LITE_CONFIG_SLEEP_MASK = 0x0080;

void UPS_18650_LITEComponent::update() {
  uint16_t raw_voltage, raw_percent;

  if (this->voltage_sensor_ != nullptr) {
    if (!this->read_byte_16(UPS_18650_LITE_VCELL, &raw_voltage)) {
      this->status_set_warning("Unable to read UPS_18650_LITE_VCELL");
    } else {
      float voltage = (1.25 * (float) (raw_voltage >> 4)) / 1000.0;
      this->voltage_sensor_->publish_state(voltage);
      this->status_clear_warning();
    }
  }
  if (this->battery_remaining_sensor_ != nullptr) {
    if (!this->read_byte_16(UPS_18650_LITE_SOC, &raw_percent)) {
      this->status_set_warning("Unable to read UPS_18650_LITE_SOC");
    } else {
      float percent = (float) ((raw_percent >> 8) + 0.003906f * (raw_percent & 0x00ff));
      this->battery_remaining_sensor_->publish_state(percent);
      this->status_clear_warning();
    }
  }
  if (this->ups_status_sensor_ != nullptr) {
    bool using_battery = digitalRead(1);  // Lê o estado do GPIO1
    this->ups_status_sensor_->publish_state(using_battery);
  }  
}

void UPS_18650_LITEComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up UPS_18650_LITE...");

  uint16_t config_reg;
  if (this->write(&UPS_18650_LITE_CONFIG, 1) != i2c::ERROR_OK) {
    this->status_set_warning();
    return;
  }

  if (this->read(reinterpret_cast<uint8_t *>(&config_reg), 2) != i2c::ERROR_OK) {
    this->status_set_warning();
    return;
  }

  config_reg = i2c::i2ctohs(config_reg) & UPS_18650_LITE_CONFIG_SAFE_MASK;
  ESP_LOGV(TAG, "UPS_18650_LITE CONFIG register reads 0x%X", config_reg);

  if (config_reg != UPS_18650_LITE_CONFIG_POWER_UP_DEFAULT) {
    ESP_LOGE(TAG, "Device does not appear to be a UPS_18650_LITE");
    this->status_set_error("unrecognised");
    this->mark_failed();
    return;
  }

  // need to write back to config register to reset the sleep bit
  if (!this->write_byte_16(UPS_18650_LITE_CONFIG, UPS_18650_LITE_CONFIG_POWER_UP_DEFAULT)) {
    this->status_set_error("sleep reset failed");
    this->mark_failed();
    return;
  }

  pinMode(1, INPUT);  // Configura GPIO1 como entrada
}

void UPS_18650_LITEComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "UPS_18650_LITE:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with UPS_18650_LITE failed");
  }
  LOG_UPDATE_INTERVAL(this);
  LOG_SENSOR("  ", "Battery Voltage", this->voltage_sensor_);
  LOG_SENSOR("  ", "Battery Level", this->battery_remaining_sensor_);
  LOG_BINARY_SENSOR("  ", "UPS Status", this->ups_status_sensor_);
}

float UPS_18650_LITEComponent::get_setup_priority() const { return setup_priority::DATA; }

void UPS_18650_LITEComponent::sleep_mode() {
  if (!this->is_failed()) {
    if (!this->write_byte_16(UPS_18650_LITE_CONFIG, UPS_18650_LITE_CONFIG_POWER_UP_DEFAULT | UPS_18650_LITE_CONFIG_SLEEP_MASK)) {
      ESP_LOGW(TAG, "Unable to write the sleep bit to config register");
      this->status_set_warning();
    }
  }
}

}  // namespace ups_18650_lite
}  // namespace esphome
