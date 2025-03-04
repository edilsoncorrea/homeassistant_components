
#pragma once
#include "esphome/core/automation.h"
#include "ups_18650_lite.h"

namespace esphome {
namespace ups_18650_lite {

template<typename... Ts> class SleepAction : public Action<Ts...> {
 public:
  explicit SleepAction(UPS_18650_LITEComponent *ups_18650_lite) : ups_18650_lite_(ups_18650_lite) {}

  void play(Ts... x) override { this->ups_18650_lite_->sleep_mode(); }

 protected:
  UPS_18650_LITEComponent *ups_18650_lite_;
};

}  // namespace ups_18650_lite
}  // namespace esphome
