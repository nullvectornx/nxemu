#pragma once

#include "player_input.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <yuzu_common/common_types.h>
#include <yuzu_common/settings_enums.h>

struct TouchFromButtonMap
{
    std::string name;
    std::vector<std::string> buttons;
};

struct OSSettings
{
    // Applet
    Settings::AppletMode cabinet_applet_mode;
    Settings::AppletMode controller_applet_mode;
    Settings::AppletMode data_erase_applet_mode;
    Settings::AppletMode error_applet_mode;
    Settings::AppletMode net_connect_applet_mode;
    Settings::AppletMode player_select_applet_mode;
    Settings::AppletMode swkbd_applet_mode;
    Settings::AppletMode mii_edit_applet_mode;
    Settings::AppletMode web_applet_mode;
    Settings::AppletMode shop_applet_mode;
    Settings::AppletMode photo_viewer_applet_mode;
    Settings::AppletMode offline_web_applet_mode;
    Settings::AppletMode login_share_applet_mode;
    Settings::AppletMode wifi_web_auth_applet_mode;
    Settings::AppletMode my_page_applet_mode;

    // Audio
    Settings::AudioEngine sink_id;
    std::string audio_output_device_id;
    std::string audio_input_device_id;
    Settings::AudioMode sound_index;
    u8 volume;
    bool audio_muted;
    bool dump_audio_commands;

    // System
    Settings::Language language_index;
    Settings::Region region_index;
    Settings::TimeZone time_zone_index;
    bool custom_rtc_enabled;
    s64 custom_rtc;
    s64 custom_rtc_offset;
    bool rng_seed_enabled;
    u32 rng_seed;
    std::string device_name;
    s32 current_user;
    Settings::DockedMode use_docked_mode;
    bool use_multi_core;
    bool use_speed_limit;
    u16 speed_limit;

    // Linux
    bool enable_gamemode;

    // Controls
    std::array<PlayerInput, 10> players;
    bool enable_raw_input;
    bool controller_navigation;
    bool enable_joycon_driver;
    bool enable_procon_driver;
    bool vibration_enabled;
    bool enable_accurate_vibrations;
    bool motion_enabled;
    std::string udp_input_servers;
    bool enable_udp_controller;
    bool pause_tas_on_load;
    bool tas_enable;
    bool tas_loop;
    bool mouse_panning;
    u8 mouse_panning_sensitivity;
    bool mouse_enabled;
    u8 mouse_panning_x_sensitivity;
    u8 mouse_panning_y_sensitivity;
    u8 mouse_panning_deadzone_counterweight;
    u8 mouse_panning_decay_strength;
    u8 mouse_panning_min_decay;
    bool emulate_analog_keyboard;
    bool keyboard_enabled;
    bool debug_pad_enabled;
    ButtonsRaw debug_pad_buttons;
    AnalogsRaw debug_pad_analogs;
    TouchscreenInput touchscreen;
    std::string touch_device;
    int touch_from_button_map_index;
    std::vector<TouchFromButtonMap> touch_from_button_maps;
    bool enable_ring_controller;
    RingconRaw ringcon_analogs;
    bool enable_ir_sensor;
    std::string ir_sensor_device;
    bool random_amiibo_id;
};

extern OSSettings osSettings;

void SetupOsSetting(void);
void SaveOsSettings(void);
