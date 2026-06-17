#pragma once

#include <nxemu-module-spec/input.h>
#include <string_view>
#include <yuzu_common/settings_enums.h>

const char* ControllerTypeToString(ControllerType value);
ControllerType ControllerTypeFromString(std::string_view str);

const char* AudioEngineToString(Settings::AudioEngine value);
Settings::AudioEngine AudioEngineFromString(std::string_view str);

const char* AudioModeToString(Settings::AudioMode value);
Settings::AudioMode AudioModeFromString(std::string_view str);

const char* LanguageToString(Settings::Language value);
Settings::Language LanguageFromString(std::string_view str);

const char* DockedModeToString(Settings::DockedMode value);
Settings::DockedMode DockedModeFromString(std::string_view str);
