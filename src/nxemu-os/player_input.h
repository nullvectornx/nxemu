#pragma once

#include <nxemu-module-spec/input.h>
#include <nxemu-module-spec/operating_system.h>
#include <array>
#include <cstdint>
#include <string>

using ButtonsRaw = std::array<std::string, static_cast<size_t>(NativeButtonValues::NumButtons)>;
using AnalogsRaw = std::array<std::string, static_cast<size_t>(NativeAnalogValues::NumAnalogs)>;
using MotionsRaw = std::array<std::string, static_cast<size_t>(NativeMotionValues::NumMotions)>;
using RingconRaw = std::string;

struct PlayerInput {
    bool connected{};
    ControllerType controller_type{ControllerType::ProController};
    ButtonsRaw buttons{};
    AnalogsRaw analogs{};
    MotionsRaw motions{};

    bool vibration_enabled{};
    int vibration_strength{};

    uint32_t body_color_left{};
    uint32_t body_color_right{};
    uint32_t button_color_left{};
    uint32_t button_color_right{};
    std::string profile_name;

    // Tells platform frontends (e.g. Android) whether to use a device vibrator or the
    // controller's rumble.
    bool use_system_vibrator{};
};

struct TouchscreenInput {
    bool enabled{};
    std::string device;

    uint32_t finger{};
    uint32_t diameter_x{};
    uint32_t diameter_y{};
    uint32_t rotation_angle{};
};
