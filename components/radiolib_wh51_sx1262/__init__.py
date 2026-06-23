import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, sensor, text_sensor
from esphome.const import (
    CONF_DEVICE_CLASS,
    CONF_ENTITY_CATEGORY,
    CONF_FREQUENCY,
    CONF_ID,
    CONF_MOISTURE,
    CONF_STATE_CLASS,
    CONF_UNIT_OF_MEASUREMENT,
    DEVICE_CLASS_BATTERY,
    DEVICE_CLASS_HUMIDITY,
    DEVICE_CLASS_SIGNAL_STRENGTH,
    DEVICE_CLASS_VOLTAGE,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
)

DEPENDENCIES = ["esp32"]

CONF_SENSORS = "sensors"
CONF_SENSOR_ID = "id"
CONF_BIT_RATE = "bit_rate"
CONF_FREQUENCY_DEVIATION = "frequency_deviation"
CONF_BANDWIDTH = "bandwidth"
CONF_PREAMBLE_LENGTH = "preamble_length"
CONF_SYNC_WORD = "sync_word"
CONF_TCXO_VOLTAGE = "tcxo_voltage"
CONF_SCK_PIN = "sck_pin"
CONF_MISO_PIN = "miso_pin"
CONF_MOSI_PIN = "mosi_pin"
CONF_CS_PIN = "cs_pin"
CONF_RST_PIN = "rst_pin"
CONF_BUSY_PIN = "busy_pin"
CONF_DIO1_PIN = "dio1_pin"
CONF_ANTENNA_SWITCH_PIN = "antenna_switch_pin"
CONF_LED_PIN = "led_pin"
CONF_LED_ACTIVE_LOW = "led_active_low"
CONF_LED_FLASH_DURATION = "led_flash_duration"
CONF_FRAME_INTERVAL = "frame_interval"
CONF_FRAME_TOLERANCE = "frame_tolerance"
CONF_RAW_AD = "raw_ad"
CONF_BATTERY_VOLTAGE = "battery_voltage"
CONF_BATTERY_LEVEL = "battery_level"
CONF_BOOST = "boost"
CONF_RSSI = "rssi"
CONF_LOST_FRAMES = "lost_frames"
CONF_BATTERY_LOW = "battery_low"
CONF_LAST_FRAME = "last_frame"

ns = cg.esphome_ns.namespace("radiolib_wh51_sx1262")
RadioLibWH51SX1262 = ns.class_("RadioLibWH51SX1262", cg.Component)

WH51_SENSOR_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_SENSOR_ID): cv.hex_uint32_t,
        cv.Required(CONF_MOISTURE): sensor.sensor_schema(
            unit_of_measurement="%",
            device_class=DEVICE_CLASS_HUMIDITY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Required(CONF_RAW_AD): sensor.sensor_schema(
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Required(CONF_BATTERY_VOLTAGE): sensor.sensor_schema(
            unit_of_measurement="V",
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Required(CONF_BATTERY_LEVEL): sensor.sensor_schema(
            unit_of_measurement="%",
            device_class=DEVICE_CLASS_BATTERY,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Required(CONF_BOOST): sensor.sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Required(CONF_RSSI): sensor.sensor_schema(
            unit_of_measurement="dBm",
            device_class=DEVICE_CLASS_SIGNAL_STRENGTH,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Required(CONF_LOST_FRAMES): sensor.sensor_schema(
            unit_of_measurement="frames",
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Required(CONF_BATTERY_LOW): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_BATTERY,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Required(CONF_LAST_FRAME): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
    }
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(RadioLibWH51SX1262),
        cv.Required(CONF_SENSORS): cv.All(
            cv.ensure_list(WH51_SENSOR_SCHEMA), cv.Length(min=1)
        ),
        cv.Required(CONF_FREQUENCY): cv.float_range(min=150.0, max=960.0),
        cv.Required(CONF_BIT_RATE): cv.float_range(min=0.6, max=300.0),
        cv.Required(CONF_FREQUENCY_DEVIATION): cv.float_range(min=0.6, max=200.0),
        cv.Required(CONF_BANDWIDTH): cv.float_range(min=4.8, max=467.0),
        cv.Required(CONF_PREAMBLE_LENGTH): cv.int_range(min=1, max=65535),
        cv.Required(CONF_SYNC_WORD): cv.All(
            cv.ensure_list(cv.hex_uint8_t), cv.Length(min=1, max=8)
        ),
        cv.Optional(CONF_TCXO_VOLTAGE, default=1.8): cv.float_range(min=0.0, max=3.3),
        cv.Required(CONF_SCK_PIN): cv.int_range(min=0, max=48),
        cv.Required(CONF_MISO_PIN): cv.int_range(min=0, max=48),
        cv.Required(CONF_MOSI_PIN): cv.int_range(min=0, max=48),
        cv.Required(CONF_CS_PIN): cv.int_range(min=0, max=48),
        cv.Required(CONF_RST_PIN): cv.int_range(min=0, max=48),
        cv.Required(CONF_BUSY_PIN): cv.int_range(min=0, max=48),
        cv.Required(CONF_DIO1_PIN): cv.int_range(min=0, max=48),
        cv.Required(CONF_ANTENNA_SWITCH_PIN): cv.int_range(min=0, max=48),
        cv.Optional(CONF_LED_PIN, default=21): cv.int_range(min=0, max=48),
        cv.Optional(CONF_LED_ACTIVE_LOW, default=True): cv.boolean,
        cv.Optional(CONF_LED_FLASH_DURATION, default="100ms"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_FRAME_INTERVAL, default="70s"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_FRAME_TOLERANCE, default="10s"): cv.positive_time_period_milliseconds,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add_library("jgromes/RadioLib", "7.7.1")
    cg.add(var.set_radio_config(
        config[CONF_FREQUENCY],
        config[CONF_BIT_RATE],
        config[CONF_FREQUENCY_DEVIATION],
        config[CONF_BANDWIDTH],
        config[CONF_PREAMBLE_LENGTH],
        config[CONF_TCXO_VOLTAGE],
    ))
    cg.add(var.set_sync_word(config[CONF_SYNC_WORD]))
    cg.add(var.set_pins(
        config[CONF_SCK_PIN], config[CONF_MISO_PIN], config[CONF_MOSI_PIN],
        config[CONF_CS_PIN], config[CONF_RST_PIN], config[CONF_BUSY_PIN],
        config[CONF_DIO1_PIN], config[CONF_ANTENNA_SWITCH_PIN],
    ))
    cg.add(var.set_led_config(
        config[CONF_LED_PIN],
        config[CONF_LED_ACTIVE_LOW],
        config[CONF_LED_FLASH_DURATION].total_milliseconds,
    ))
    cg.add(var.set_frame_monitoring(
        config[CONF_FRAME_INTERVAL].total_milliseconds,
        config[CONF_FRAME_TOLERANCE].total_milliseconds,
    ))

    for wh51_sensor in config[CONF_SENSORS]:
        moisture = await sensor.new_sensor(wh51_sensor[CONF_MOISTURE])
        raw_ad = await sensor.new_sensor(wh51_sensor[CONF_RAW_AD])
        battery_voltage = await sensor.new_sensor(wh51_sensor[CONF_BATTERY_VOLTAGE])
        battery_level = await sensor.new_sensor(wh51_sensor[CONF_BATTERY_LEVEL])
        boost = await sensor.new_sensor(wh51_sensor[CONF_BOOST])
        rssi = await sensor.new_sensor(wh51_sensor[CONF_RSSI])
        lost_frames = await sensor.new_sensor(wh51_sensor[CONF_LOST_FRAMES])
        battery_low = await binary_sensor.new_binary_sensor(wh51_sensor[CONF_BATTERY_LOW])
        last_frame = await text_sensor.new_text_sensor(wh51_sensor[CONF_LAST_FRAME])
        cg.add(var.add_sensor(
            wh51_sensor[CONF_SENSOR_ID],
            moisture,
            raw_ad,
            battery_voltage,
            battery_level,
            boost,
            rssi,
            lost_frames,
            battery_low,
            last_frame,
        ))
