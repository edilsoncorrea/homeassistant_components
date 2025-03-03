from esphome import automation
from esphome.automation import maybe_simple_id
import esphome.codegen as cg
from esphome.components import i2c, sensor, binary_sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_BATTERY_LEVEL,
    CONF_BATTERY_VOLTAGE,
    CONF_ID,
    DEVICE_CLASS_BATTERY,
    DEVICE_CLASS_VOLTAGE,
    DEVICE_CLASS_POWER,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    UNIT_PERCENT,
    UNIT_VOLT,
)

DEPENDENCIES = ["i2c"]

ups_18650_lite_ns = cg.esphome_ns.namespace("ups_18650_lite")
UPS_18650_LITEComponent = ups_18650_lite_ns.class_(
    "UPS_18650_LITEComponent", cg.PollingComponent, i2c.I2CDevice
)

CONF_UPS_STATUS = "ups_status"

# Actions
SleepAction = ups_18650_lite_ns.class_("SleepAction", automation.Action)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(UPS_18650_LITEComponent),
            cv.Optional(CONF_BATTERY_VOLTAGE): sensor.sensor_schema(
                unit_of_measurement=UNIT_VOLT,
                accuracy_decimals=3,
                device_class=DEVICE_CLASS_VOLTAGE,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_BATTERY_LEVEL): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=3,
                device_class=DEVICE_CLASS_BATTERY,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_UPS_STATUS): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_POWER,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),            
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(i2c.i2c_device_schema(0x36))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    if voltage_config := config.get(CONF_BATTERY_VOLTAGE):
        sens = await sensor.new_sensor(voltage_config)
        cg.add(var.set_voltage_sensor(sens))

    if CONF_BATTERY_LEVEL in config:
        sens = await sensor.new_sensor(config[CONF_BATTERY_LEVEL])
        cg.add(var.set_battery_remaining_sensor(sens))

    if CONF_UPS_STATUS in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_UPS_STATUS])
        cg.add(var.set_ups_status_sensor(sens))

UPS_18650_LITE_ACTION_SCHEMA = maybe_simple_id(
    {
        cv.Required(CONF_ID): cv.use_id(UPS_18650_LITEComponent),
    }
)


@automation.register_action("ups_18650_lite.sleep_mode", SleepAction, UPS_18650_LITE_ACTION_SCHEMA)
async def ups_18650_lite_sleep_mode_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)
