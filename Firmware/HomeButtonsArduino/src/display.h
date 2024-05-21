#ifndef HOMEBUTTONS_DISPLAY_H
#define HOMEBUTTONS_DISPLAY_H

#include <GxEPD2.h>
#include "static_string.h"
#include "state.h"
#include "logger.h"
#include "mdi_helper.h"
#include "types.h"

// parameters for draw_bmp()
static constexpr uint16_t input_buffer_pixels = 800;
static constexpr uint16_t max_palette_pixels = 256;

struct HardwareDefinition;

class DeviceState;
enum class DisplayPage {
  EMPTY,
  MAIN,
  INFO,
  DEVICE_INFO,
  MESSAGE,
  MESSAGE_LARGE,
  ERROR,
  WELCOME,
  SETTINGS,
  AP_CONFIG,
  WEB_CONFIG,
  TEST
};

struct UIState {
  static constexpr size_t MAX_MESSAGE_SIZE = 64;
  using MessageType = StaticString<MAX_MESSAGE_SIZE>;

  DisplayPage page = DisplayPage::EMPTY;
  MessageType message{};
  MDIName mdi_name{};
  uint16_t mdi_size = 0;
  bool disappearing = false;
  uint32_t appear_time = 0;
  uint32_t disappear_timeout = 0;
};

class Display : public Logger {
 public:
  enum class State { IDLE, ACTIVE, CMD_END, ENDING };
  explicit Display(const DeviceState& device_state, MDIHelper& mdi_helper)
      : Logger("Display"), device_state_(device_state), mdi_(mdi_helper) {}
  void begin(HardwareDefinition& HW);
  void end();
  void update();

  void disp_message(const char* message, uint32_t duration = 0);
  void disp_message_large(const char* message, uint32_t duration = 0);
  void disp_error(const char* message, uint32_t duration = 0);
  void disp_main();
  void disp_info();
  void disp_device_info();
  void disp_welcome();
  void disp_settings();
  void disp_ap_config();
  void disp_web_config();
  void disp_test(const char* text, const char* mdi_name, uint16_t mdi_size);

  uint16_t get_global_rotation() const { return global_rotation; }
  void set_global_rotation(uint16_t rotation) {
    global_rotation = rotation;
  }

  UIState get_ui_state();
  void init_ui_state(UIState ui_state);  // used after wakeup
  State get_state();
  bool busy() { return redraw_in_progress; }

 private:
  enum class LabelType : uint8_t { Text, Icon, Mixed };

  State state = State::IDLE;

  UIState current_ui_state = {};
  UIState cmd_ui_state = {};
  UIState draw_ui_state = {};
  UIState pre_disappear_ui_state = {};

  bool new_ui_cmd = false;
  bool redraw_in_progress = false;

  uint16_t text_color = GxEPD_BLACK;
  uint16_t bg_color = GxEPD_WHITE;
  bool just_formatted = 0;
  uint16_t global_rotation = 0;

  const DeviceState& device_state_;
  MDIHelper& mdi_;

  // ### buffers for draw_bmp()
  // up to depth 24
  uint8_t input_buffer[3 * input_buffer_pixels];
  // palette buffer for depth <= 8 b/w
  uint8_t mono_palette_buffer[max_palette_pixels / 8];
  // palette buffer for depth <= 8 c/w
  uint8_t color_palette_buffer[max_palette_pixels / 8];
  // palette buffer for depth <= 8 for buffered graphics, needed for 7-color
  // display
  uint16_t rgb_palette_buffer[max_palette_pixels];

  void set_cmd_state(UIState cmd);

  void draw_message(const UIState::MessageType& message, bool error = false,
                    bool large = false);
  void draw_main();
  void draw_info();
  void draw_device_info();
  void draw_welcome();
  void draw_settings();
  void draw_ap_config();
  void draw_web_config();
  void draw_test(const char* text, const char* mdi_name, uint16_t mdi_size);
  void draw_white();
  void draw_black();
  bool draw_bmp(File& file, int16_t x, int16_t y, int16_t rotation = 0);
  void draw_mdi(const char* name, uint16_t size, int16_t x, int16_t y, int16_t rotation = 0);
};

#endif  // HOMEBUTTONS_DISPLAY_H
