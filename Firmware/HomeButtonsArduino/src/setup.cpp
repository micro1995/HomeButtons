#include "setup.h"

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiManager.h>

#include "config.h"
#include "display.h"
#include "hardware.h"
#include "state.h"
#include "logger.h"

static WiFiManager wifi_manager;

static WiFiManagerParameter device_name_param("device_name", "Device Name", "",
                                              20);
static WiFiManagerParameter mqtt_server_param("mqtt_server", "MQTT Server", "",
                                              50);
static WiFiManagerParameter mqtt_port_param("mqtt_port", "MQTT Port", "", 6);
static WiFiManagerParameter mqtt_user_param("mqtt_user", "MQTT User", "", 50);
static WiFiManagerParameter mqtt_password_param("mqtt_password",
                                                "MQTT Password", "", 50);
static WiFiManagerParameter base_topic_param("base_topic", "Base Topic", "",
                                             50);
static WiFiManagerParameter discovery_prefix_param("disc_prefix",
                                                   "Discovery Prefix", "", 50);
static WiFiManagerParameter static_ip_param("static_ip", "Static IP", "", 16);
static WiFiManagerParameter gateway_param("gateway", "Gateway", "", 16);
static WiFiManagerParameter subnet_param("subnet", "Subnet Mask", "", 16);
static WiFiManagerParameter btn1_label_param("btn1_lbl", "Button 1 Label", "",
                                             BTN_LABEL_MAXLEN);
static WiFiManagerParameter btn2_label_param("btn2_lbl", "Button 2 Label", "",
                                             BTN_LABEL_MAXLEN);
static WiFiManagerParameter btn3_label_param("btn3_lbl", "Button 3 Label", "",
                                             BTN_LABEL_MAXLEN);
static WiFiManagerParameter btn4_label_param("btn4_lbl", "Button 4 Label", "",
                                             BTN_LABEL_MAXLEN);
static WiFiManagerParameter btn5_label_param("btn5_lbl", "Button 5 Label", "",
                                             BTN_LABEL_MAXLEN);
static WiFiManagerParameter btn6_label_param("btn6_lbl", "Button 6 Label", "",
                                             BTN_LABEL_MAXLEN);

static bool web_portal_saved = false;

void start_wifi_setup(DeviceState& device_state, Display& display) {
  static Logger setupLogger("W_SETUP");
  display.disp_ap_config();
  display.update();

  WiFi.mode(WIFI_STA);
  wifi_manager.setConfigPortalTimeout(SETUP_TIMEOUT);
  wifi_manager.resetSettings();
  wifi_manager.setTitle(WIFI_MANAGER_TITLE);
  wifi_manager.setBreakAfterConfig(true);
  wifi_manager.setDarkMode(true);
  wifi_manager.setShowInfoUpdate(false);

  wifi_manager.startConfigPortal(device_state.get_ap_ssid().c_str(),
                                 device_state.get_ap_password());

  bool wifi_connected = false;
  WiFi.mode(WIFI_STA);
  uint32_t wifi_start_time = millis();
  WiFi.begin();
  while (true) {
    delay(100);
    if (WiFi.status() == WL_CONNECTED) {
      wifi_connected = true;
      break;
    } else if (millis() - wifi_start_time >= WIFI_TIMEOUT) {
      wifi_connected = false;
      break;
    }
  }

  if (wifi_connected) {
    device_state.persisted().wifi_done = true;
    device_state.persisted().restart_to_setup = true;
    device_state.save_all();
    setupLogger.info("Wi-Fi connected.");
    display.disp_message_large("Wi-Fi\nconnected\n:)");
    display.update();
    delay(3000);
    ESP.restart();
  } else {
    device_state.persisted().wifi_done = false;
    device_state.save_all();
    setupLogger.warning("Wi-Fi error.");
    display.disp_error("Wi-Fi\nconnection\nerror");
    display.update();
    delay(5000);
    ESP.restart();
  }
}

void save_params_callback(DeviceState* device_state) {
  device_state->set_device_name(DeviceName{device_name_param.getValue()});
  device_state->set_mqtt_parameters(
      mqtt_server_param.getValue(), String(mqtt_port_param.getValue()).toInt(),
      mqtt_user_param.getValue(), mqtt_password_param.getValue(),
      base_topic_param.getValue(), discovery_prefix_param.getValue());
  device_state->set_btn_label(0, btn1_label_param.getValue());
  device_state->set_btn_label(1, btn2_label_param.getValue());
  device_state->set_btn_label(2, btn3_label_param.getValue());
  device_state->set_btn_label(3, btn4_label_param.getValue());
  device_state->set_btn_label(4, btn5_label_param.getValue());
  device_state->set_btn_label(5, btn6_label_param.getValue());

  IPAddress static_ip, gateway, subnet;
  static_ip.fromString(static_ip_param.getValue());
  gateway.fromString(gateway_param.getValue());
  subnet.fromString(subnet_param.getValue());
  device_state->set_static_ip_config(static_ip, gateway, subnet);
  web_portal_saved = true;
}

void start_setup(DeviceState& device_state, Display& display,
                 HardwareDefinition& HW) {
  // config
  static Logger setupLogger("SETUP");
  wifi_manager.setTitle(WIFI_MANAGER_TITLE);
  wifi_manager.setSaveParamsCallback(
      std::bind(&save_params_callback, &device_state));
  wifi_manager.setBreakAfterConfig(true);
  wifi_manager.setShowPassword(true);
  wifi_manager.setParamsPage(true);
  wifi_manager.setDarkMode(true);
  wifi_manager.setShowInfoUpdate(true);

  // parameters
  device_name_param.setValue(device_state.device_name().c_str(), 20);
  mqtt_server_param.setValue(
      device_state.user_preferences().mqtt.server.c_str(), 50);
  mqtt_port_param.setValue(
      String(device_state.user_preferences().mqtt.port).c_str(), 6);
  mqtt_user_param.setValue(device_state.user_preferences().mqtt.user.c_str(),
                           50);
  mqtt_password_param.setValue(
      device_state.user_preferences().mqtt.password.c_str(), 50);
  base_topic_param.setValue(
      device_state.user_preferences().mqtt.base_topic.c_str(), 50);
  discovery_prefix_param.setValue(
      device_state.user_preferences().mqtt.discovery_prefix.c_str(), 50);
  static_ip_param.setValue(
      device_state.user_preferences().network.static_ip.toString().c_str(), 16);
  gateway_param.setValue(
      device_state.user_preferences().network.gateway.toString().c_str(), 16);
  subnet_param.setValue(
      device_state.user_preferences().network.subnet.toString().c_str(), 16);
  btn1_label_param.setValue(device_state.get_btn_label(0).c_str(), 20);
  btn2_label_param.setValue(device_state.get_btn_label(1).c_str(), 20);
  btn3_label_param.setValue(device_state.get_btn_label(2).c_str(), 20);
  btn4_label_param.setValue(device_state.get_btn_label(3).c_str(), 20);
  btn5_label_param.setValue(device_state.get_btn_label(4).c_str(), 20);
  btn6_label_param.setValue(device_state.get_btn_label(5).c_str(), 20);
  wifi_manager.addParameter(&device_name_param);
  wifi_manager.addParameter(&mqtt_server_param);
  wifi_manager.addParameter(&mqtt_port_param);
  wifi_manager.addParameter(&mqtt_user_param);
  wifi_manager.addParameter(&mqtt_password_param);
  wifi_manager.addParameter(&base_topic_param);
  wifi_manager.addParameter(&discovery_prefix_param);
  wifi_manager.addParameter(&static_ip_param);
  wifi_manager.addParameter(&gateway_param);
  wifi_manager.addParameter(&subnet_param);
  wifi_manager.addParameter(&btn1_label_param);
  wifi_manager.addParameter(&btn2_label_param);
  wifi_manager.addParameter(&btn3_label_param);
  wifi_manager.addParameter(&btn4_label_param);
  wifi_manager.addParameter(&btn5_label_param);
  wifi_manager.addParameter(&btn6_label_param);

  // connect Wi-Fi
  WiFi.mode(WIFI_STA);
  int remaining_tries = MAX_WIFI_RETRIES_DURING_MQTT_SETUP;

  while (true) {
    uint32_t wifi_start_time = millis();
    WiFi.begin();
    while (WiFi.status() != WL_CONNECTED) {
      delay(100);
      if (millis() - wifi_start_time >= WIFI_TIMEOUT) {
        break;
      }
    }

    if (WiFi.status() == WL_CONNECTED) {
      break;
    } else if (remaining_tries > 0) {
      remaining_tries--;
      setupLogger.warning("Wi-Fi error, retrying (remainig tries: %d)",
                          remaining_tries);
      WiFi.disconnect();
      delay(1000);
    } else {
      device_state.persisted().wifi_done = false;
      device_state.save_all();
      setupLogger.warning("Wi-Fi error.");
      display.disp_error("Wi-Fi\nerror");
      display.update();
      delay(3000);
      ESP.restart();
    }
  }
  device_state.set_ip(WiFi.localIP());
  display.disp_web_config();
  display.update();
  uint32_t setup_start_time = millis();

  web_portal_saved = false;
  wifi_manager.startWebPortal();
  while (millis() - setup_start_time < SETUP_TIMEOUT * 1000L) {
    wifi_manager.process();
    if (HW.any_button_pressed() || web_portal_saved) {
      break;
    }
    delay(10);
  }
  wifi_manager.stopWebPortal();

  // test MQTT connection
  uint32_t mqtt_start_time = millis();
  WiFiClient wifi_client;
  PubSubClient mqtt_client(wifi_client);
  setupLogger.debug("Trying to connect to mqtt://%s:%d",
                    device_state.user_preferences().mqtt.server.c_str(),
                    device_state.user_preferences().mqtt.port);
  mqtt_client.setServer(device_state.user_preferences().mqtt.server.c_str(),
                        device_state.user_preferences().mqtt.port);
  if (device_state.user_preferences().mqtt.user.length() > 0 &&
      device_state.user_preferences().mqtt.password.length() > 0) {
    mqtt_client.connect(device_state.factory().unique_id.c_str(),
                        device_state.user_preferences().mqtt.user.c_str(),
                        device_state.user_preferences().mqtt.password.c_str());
  } else {
    mqtt_client.connect(device_state.factory().unique_id.c_str());
  }

  display.disp_message("Confirming\nsetup...");
  display.update();

  while (!mqtt_client.connected()) {
    delay(10);
    if (millis() - mqtt_start_time >= MQTT_TIMEOUT) {
      device_state.persisted().setup_done = false;
      device_state.save_all();
      setupLogger.warning("MQTT error.");
      display.disp_error("MQTT\nerror");
      display.update();
      delay(3000);
      ESP.restart();
    }
  }

  mqtt_client.disconnect();
  WiFi.disconnect(true);
  device_state.persisted().setup_done = true;
  device_state.persisted().send_discovery_config = true;
  device_state.save_all();

  setupLogger.info("setup successful");
  display.disp_message_large("Setup\ncomplete\n:)");
  display.update();
  delay(3000);
  ESP.restart();
}
