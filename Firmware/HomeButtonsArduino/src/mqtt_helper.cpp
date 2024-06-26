#include "mqtt_helper.h"

#include <Arduino.h>
#include <ArduinoJson.h>

#include "config.h"
#include "network.h"
#include "state.h"
#include "hardware.h"
#include "static_string.h"

template <size_t SIZE>
bool convertToJson(const StaticString<SIZE>& src, JsonVariant dst) {
  return dst.set(
      const_cast<char*>(src.c_str()));  // Warning: use char*, not const char*
                                        // to force a copy in ArduinoJson.
}

using FormatterType = StaticString<64>;

MQTTHelper::MQTTHelper(DeviceState& state, Network& network)
    : _device_state(state), _network(network) {}

void MQTTHelper::send_discovery_config() {
  // Construct topics

  TopicType trigger_topic_common(
      "%s/device_automation/%s",
      _device_state.user_preferences().mqtt.discovery_prefix.c_str(),
      _device_state.factory().unique_id.c_str());

  // sensor config topics
  TopicType sensor_topic_common =
      TopicType{} + _device_state.user_preferences().mqtt.discovery_prefix +
      "/sensor/" + _device_state.factory().unique_id;

  // device objects
  StaticJsonDocument<256> device_full;
  device_full["ids"][0] = _device_state.factory().unique_id;
  device_full["mdl"] = _device_state.factory().model_name;
  device_full["name"] = _device_state.device_name();
  device_full["sw"] = SW_VERSION;
  device_full["hw"] = _device_state.factory().hw_version;
  device_full["mf"] = MANUFACTURER;

  StaticJsonDocument<128> device_short;
  device_short["ids"][0] = _device_state.factory().unique_id;

  // send mqtt msg
  char buffer[MQTT_PYLD_SIZE];

  // button single press
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    StaticJsonDocument<MQTT_PYLD_SIZE> conf;
    conf["atype"] = "trigger";
    conf["t"] = t_btn_press(i);
    conf["pl"] = BTN_PRESS_PAYLOAD;
    conf["type"] = "button_short_press";
    conf["stype"] = FormatterType("button_%d", i + 1);
    if (i == 0) {
      conf["dev"] = device_full;
    } else {
      conf["dev"] = device_short;
    }
    serializeJson(conf, buffer, sizeof(buffer));
    TopicType topic_name("%s/button_%d/config", trigger_topic_common.c_str(),
                         i + 1);
    _network.publish(topic_name, buffer, true);
  }

  // button double press
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    StaticJsonDocument<MQTT_PYLD_SIZE> conf;
    conf["atype"] = "trigger";
    conf["t"] = t_btn_press(i) + "_double";
    conf["pl"] = BTN_PRESS_PAYLOAD;
    conf["type"] = "button_double_press";
    conf["stype"] = FormatterType("button_%d", i + 1);
    conf["dev"] = device_short;
    serializeJson(conf, buffer, sizeof(buffer));
    TopicType topic_name("%s/button_%d_double/config",
                         trigger_topic_common.c_str(), i + 1);
    _network.publish(topic_name, buffer, true);
  }

  // button triple press
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    StaticJsonDocument<MQTT_PYLD_SIZE> conf;
    conf["atype"] = "trigger";
    conf["t"] = t_btn_press(i) + "_triple";
    conf["pl"] = BTN_PRESS_PAYLOAD;
    conf["type"] = "button_triple_press";
    conf["stype"] = FormatterType("button_%d", i + 1);
    conf["dev"] = device_short;
    serializeJson(conf, buffer, sizeof(buffer));
    TopicType topic_name("%s/button_%d_triple/config",
                         trigger_topic_common.c_str(), i + 1);
    _network.publish(topic_name, buffer, true);
  }

  // button quad press
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    StaticJsonDocument<MQTT_PYLD_SIZE> conf;
    conf["atype"] = "trigger";
    conf["t"] = t_btn_press(i) + "_quad";
    conf["pl"] = BTN_PRESS_PAYLOAD;
    conf["type"] = "button_quadruple_press";
    conf["stype"] = FormatterType("button_%d", i + 1);
    conf["dev"] = device_short;
    serializeJson(conf, buffer, sizeof(buffer));
    TopicType topic_name("%s/button_%d_quad/config",
                         trigger_topic_common.c_str(), i + 1);
    _network.publish(topic_name, buffer, true);
  }

  uint16_t expire_after = _device_state.sensor_interval() * 60 + 60;  // seconds

  {
    // temperature
    TopicType temperature_config_topic =
        sensor_topic_common + "/temperature/config";
    StaticJsonDocument<MQTT_PYLD_SIZE> temp_conf;
    temp_conf["name"] = "Temperature";
    temp_conf["uniq_id"] =
        FormatterType{} + _device_state.factory().unique_id + "_temperature";
    temp_conf["stat_t"] = t_temperature();
    temp_conf["dev_cla"] = "temperature";
    temp_conf["unit_of_meas"] =
        _device_state.get_use_fahrenheit() ? "°F" : "°C";
    temp_conf["exp_aft"] = expire_after;
    temp_conf["dev"] = device_short;
    serializeJson(temp_conf, buffer, sizeof(buffer));
    _network.publish(temperature_config_topic, buffer, true);
  }

  {
    // humidity
    TopicType humidity_config_topic = sensor_topic_common + "/humidity/config";
    StaticJsonDocument<MQTT_PYLD_SIZE> humidity_conf;
    humidity_conf["name"] = "Humidity";
    humidity_conf["uniq_id"] =
        FormatterType{} + _device_state.factory().unique_id + "_humidity";
    humidity_conf["stat_t"] = t_humidity();
    humidity_conf["dev_cla"] = "humidity";
    humidity_conf["unit_of_meas"] = "%";
    humidity_conf["exp_aft"] = expire_after;
    humidity_conf["dev"] = device_short;
    serializeJson(humidity_conf, buffer, sizeof(buffer));
    _network.publish(humidity_config_topic, buffer, true);
  }

  {
    // battery
    TopicType battery_config_topic = sensor_topic_common + "/battery/config";
    StaticJsonDocument<MQTT_PYLD_SIZE> battery_conf;
    battery_conf["name"] = "Battery";
    battery_conf["uniq_id"] =
        FormatterType{} + _device_state.factory().unique_id + "_battery";
    battery_conf["stat_t"] = t_battery();
    battery_conf["dev_cla"] = "battery";
    battery_conf["unit_of_meas"] = "%";
    battery_conf["exp_aft"] = expire_after;
    battery_conf["dev"] = device_short;
    serializeJson(battery_conf, buffer, sizeof(buffer));
    _network.publish(battery_config_topic, buffer, true);
  }

  {
    // sensor interval slider
    TopicType sensor_interval_config_topic =
        TopicType{} + _device_state.user_preferences().mqtt.discovery_prefix +
        "/number/" + _device_state.factory().unique_id +
        "/sensor_interval/config";
    StaticJsonDocument<MQTT_PYLD_SIZE> sensor_interval_conf;
    sensor_interval_conf["name"] = "Sensor interval";
    sensor_interval_conf["uniq_id"] = FormatterType{} +
                                      _device_state.factory().unique_id +
                                      "_sensor_interval";
    sensor_interval_conf["cmd_t"] = t_sensor_interval_cmd();
    sensor_interval_conf["stat_t"] = t_sensor_interval_state();
    sensor_interval_conf["unit_of_meas"] = "min";
    sensor_interval_conf["min"] = SEN_INTERVAL_MIN;
    sensor_interval_conf["max"] = SEN_INTERVAL_MAX;
    sensor_interval_conf["mode"] = "slider";
    sensor_interval_conf["ic"] = "mdi:timer-sand";
    sensor_interval_conf["ret"] = "true";
    sensor_interval_conf["dev"] = device_short;
    serializeJson(sensor_interval_conf, buffer, sizeof(buffer));
    _network.publish(sensor_interval_config_topic, buffer, true);
  }

  // button labels
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    TopicType button_label_config_topics =
        TopicType{} + _device_state.user_preferences().mqtt.discovery_prefix +
        "/text/" + _device_state.factory().unique_id + "/button_" + (i + 1) +
        "_label/config";
    StaticJsonDocument<MQTT_PYLD_SIZE> conf;
    conf["name"] = FormatterType{} + "Button " + (i + 1) + " label";
    conf["uniq_id"] = FormatterType{} + _device_state.factory().unique_id +
                      "_button_" + (i + 1) + "_label";
    conf["cmd_t"] = t_btn_label_cmd(i);
    conf["stat_t"] = t_btn_label_state(i);
    conf["max"] = BTN_LABEL_MAXLEN;
    conf["ic"] = FormatterType("mdi:numeric-%d-box", i + 1);
    conf["ret"] = "true";
    conf["dev"] = device_short;
    serializeJson(conf, buffer, sizeof(buffer));
    _network.publish(button_label_config_topics, buffer, true);
  }

  {
    // user message
    TopicType user_message_config_topic =
        TopicType{} + _device_state.user_preferences().mqtt.discovery_prefix +
        "/text/" + _device_state.factory().unique_id + "/user_message/config";
    StaticJsonDocument<MQTT_PYLD_SIZE> user_message_conf;
    user_message_conf["name"] = "Show message";
    user_message_conf["uniq_id"] =
        FormatterType{} + _device_state.factory().unique_id + "_user_message";
    user_message_conf["cmd_t"] = t_disp_msg_cmd();
    user_message_conf["stat_t"] = t_disp_msg_state();
    user_message_conf["max"] = USER_MSG_MAXLEN;
    user_message_conf["ic"] = "mdi:message-text";
    user_message_conf["ret"] = "true";
    user_message_conf["dev"] = device_short;
    serializeJson(user_message_conf, buffer, sizeof(buffer));
    _network.publish(user_message_config_topic, buffer, true);
  }

  {
    // schedule wakeup
    TopicType schedule_wakeup_config_topic =
        TopicType{} + _device_state.user_preferences().mqtt.discovery_prefix +
        "/number/" + _device_state.factory().unique_id +
        "/schedule_wakeup/config";
    StaticJsonDocument<MQTT_PYLD_SIZE> schedule_wakeup_conf;
    schedule_wakeup_conf["name"] = "Schedule wakeup";
    schedule_wakeup_conf["uniq_id"] = FormatterType{} +
                                      _device_state.factory().unique_id +
                                      "_schedule_wakeup";
    schedule_wakeup_conf["cmd_t"] = t_schedule_wakeup_cmd();
    schedule_wakeup_conf["stat_t"] = t_schedule_wakeup_state();
    schedule_wakeup_conf["unit_of_meas"] = "s";
    schedule_wakeup_conf["min"] = SCHEDULE_WAKEUP_MIN;
    schedule_wakeup_conf["max"] = SCHEDULE_WAKEUP_MAX;
    schedule_wakeup_conf["mode"] = "box";
    schedule_wakeup_conf["ic"] = "mdi:alarm";
    schedule_wakeup_conf["ret"] = "true";
    schedule_wakeup_conf["dev"] = device_short;
    serializeJson(schedule_wakeup_conf, buffer, sizeof(buffer));
    _network.publish(schedule_wakeup_config_topic, buffer, true);
  }

#ifndef HOME_BUTTONS_MINI
  {
    // awake mode toggle
    TopicType awake_mode_config_topic =
        TopicType{} + _device_state.user_preferences().mqtt.discovery_prefix +
        "/switch/" + _device_state.factory().unique_id + "/awake_mode/config";
    StaticJsonDocument<MQTT_PYLD_SIZE> awake_mode_conf;
    awake_mode_conf["name"] = "Awake mode";
    awake_mode_conf["uniq_id"] =
        FormatterType{} + _device_state.factory().unique_id + "_awake_mode";
    awake_mode_conf["cmd_t"] = t_awake_mode_cmd();
    awake_mode_conf["stat_t"] = t_awake_mode_state();
    awake_mode_conf["ic"] = "mdi:coffee";
    awake_mode_conf["ret"] = "true";
    awake_mode_conf["avty_t"] = t_awake_mode_avlb();
    awake_mode_conf["dev"] = device_short;
    serializeJson(awake_mode_conf, buffer, sizeof(buffer));
    _network.publish(awake_mode_config_topic, buffer, true);
  }
#endif
}

void MQTTHelper::update_discovery_config() {
  // sensor config topics
  TopicType sensor_topic_common(
      "%s/sensor/%s",
      _device_state.user_preferences().mqtt.discovery_prefix.c_str(),
      _device_state.factory().unique_id.c_str());

  StaticJsonDocument<128> device_short;
  device_short["ids"][0] = _device_state.factory().unique_id;

  char buffer[MQTT_PYLD_SIZE];

  uint16_t expire_after = _device_state.sensor_interval() * 60 + 60;  // seconds

  {
    TopicType temperature_config_topic =
        sensor_topic_common + "/temperature/config";
    StaticJsonDocument<MQTT_PYLD_SIZE> temp_conf;
    temp_conf["name"] = "Temperature";
    temp_conf["uniq_id"] =
        FormatterType{} + _device_state.factory().unique_id + "_temperature";
    temp_conf["stat_t"] = t_temperature();
    temp_conf["dev_cla"] = "temperature";
    temp_conf["unit_of_meas"] =
        _device_state.get_use_fahrenheit() ? "°F" : "°C";
    temp_conf["exp_aft"] = expire_after;
    temp_conf["dev"] = device_short;
    serializeJson(temp_conf, buffer, sizeof(buffer));
    _network.publish(temperature_config_topic, buffer, true);
  }

  {
    TopicType humidity_config_topic = sensor_topic_common + "/humidity/config";
    StaticJsonDocument<MQTT_PYLD_SIZE> humidity_conf;
    humidity_conf["name"] = "Humidity";
    humidity_conf["uniq_id"] =
        FormatterType{} + _device_state.factory().unique_id + "_humidity";
    humidity_conf["stat_t"] = t_humidity();
    humidity_conf["dev_cla"] = "humidity";
    humidity_conf["unit_of_meas"] = "%";
    humidity_conf["exp_aft"] = expire_after;
    humidity_conf["dev"] = device_short;
    serializeJson(humidity_conf, buffer, sizeof(buffer));
    _network.publish(humidity_config_topic, buffer, true);
  }

  {
    TopicType battery_config_topic = sensor_topic_common + "/battery/config";
    StaticJsonDocument<MQTT_PYLD_SIZE> battery_conf;
    battery_conf["name"] = "Battery";
    battery_conf["uniq_id"] =
        FormatterType{} + _device_state.factory().unique_id + "_battery";
    battery_conf["stat_t"] = t_battery();
    battery_conf["dev_cla"] = "battery";
    battery_conf["unit_of_meas"] = "%";
    battery_conf["exp_aft"] = expire_after;
    battery_conf["dev"] = device_short;
    serializeJson(battery_conf, buffer, sizeof(buffer));
    _network.publish(battery_config_topic, buffer, true);
  }
}

TopicType MQTTHelper::get_button_topic(ButtonEvent event) const {
  if (event.id < 1 || event.id > NUM_BUTTONS) return {};

  if (event.action == Button::SINGLE)
    return t_common() + "button_" + event.id;
  else if (event.action == Button::DOUBLE)
    return t_common() + "button_" + event.id + "_double";
  else if (event.action == Button::TRIPLE)
    return t_common() + "button_" + event.id + "_triple";
  else if (event.action == Button::QUAD)
    return t_common() + "button_" + event.id + "_quad";
  else
    return {};
}

TopicType MQTTHelper::t_common() const {
  return TopicType(_device_state.user_preferences().mqtt.base_topic.c_str()) +
         "/" + _device_state.user_preferences().device_name.c_str() + "/";
}

TopicType MQTTHelper::t_cmd() const { return t_common() + "cmd/"; }
TopicType MQTTHelper::t_temperature() const {
  return t_common() + "temperature";
}
TopicType MQTTHelper::t_humidity() const { return t_common() + "humidity"; }
TopicType MQTTHelper::t_battery() const { return t_common() + "battery"; }
TopicType MQTTHelper::t_btn_press(uint8_t i) const {
  if (i < NUM_BUTTONS)
    return t_common() + "button_" + (i + 1);
  else
    return {};
}

TopicType MQTTHelper::t_btn_label_state(uint8_t i) const {
  if (i < NUM_BUTTONS)
    return t_common() + "btn_" + (i + 1) + "_label";
  else
    return {};
}

TopicType MQTTHelper::t_btn_label_cmd(uint8_t i) const {
  if (i < NUM_BUTTONS)
    return t_cmd() + "btn_" + (i + 1) + "_label";
  else
    return {};
}

TopicType MQTTHelper::t_sensor_interval_state() const {
  return t_common() + "sensor_interval";
}

TopicType MQTTHelper::t_sensor_interval_cmd() const {
  return t_cmd() + "sensor_interval";
}

TopicType MQTTHelper::t_awake_mode_state() const {
  return t_common() + "awake_mode";
}

TopicType MQTTHelper::t_awake_mode_cmd() const {
  return t_cmd() + "awake_mode";
}

TopicType MQTTHelper::t_awake_mode_avlb() const {
  return t_awake_mode_state() + "/available";
}

TopicType MQTTHelper::t_disp_msg_cmd() const { return t_cmd() + "disp_msg"; }

TopicType MQTTHelper::t_disp_msg_state() const {
  return t_common() + "disp_msg";
}

TopicType MQTTHelper::t_schedule_wakeup_cmd() const {
  return t_cmd() + "schedule_wakeup";
}

TopicType MQTTHelper::t_schedule_wakeup_state() const {
  return t_common() + "schedule_wakeup";
}
