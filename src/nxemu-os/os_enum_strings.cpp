#include "os_enum_strings.h"

const char* ControllerTypeToString(ControllerType value)
{
    switch (value) {
    case ControllerType::ProController: return "ProController";
    case ControllerType::DualJoyconDetached: return "DualJoyconDetached";
    case ControllerType::LeftJoycon: return "LeftJoycon";
    case ControllerType::RightJoycon: return "RightJoycon";
    case ControllerType::Handheld: return "Handheld";
    case ControllerType::GameCube: return "GameCube";
    case ControllerType::Pokeball: return "Pokeball";
    case ControllerType::NES: return "NES";
    case ControllerType::SNES: return "SNES";
    case ControllerType::N64: return "N64";
    case ControllerType::SegaGenesis: return "SegaGenesis";
    }
    return "ProController";
}

ControllerType ControllerTypeFromString(std::string_view str)
{
    if (str == "ProController") return ControllerType::ProController;
    if (str == "DualJoyconDetached") return ControllerType::DualJoyconDetached;
    if (str == "LeftJoycon") return ControllerType::LeftJoycon;
    if (str == "RightJoycon") return ControllerType::RightJoycon;
    if (str == "Handheld") return ControllerType::Handheld;
    if (str == "GameCube") return ControllerType::GameCube;
    if (str == "Pokeball") return ControllerType::Pokeball;
    if (str == "NES") return ControllerType::NES;
    if (str == "SNES") return ControllerType::SNES;
    if (str == "N64") return ControllerType::N64;
    if (str == "SegaGenesis") return ControllerType::SegaGenesis;
    return ControllerType::ProController;
}

const char* AudioEngineToString(Settings::AudioEngine value)
{
    switch (value) {
    case Settings::AudioEngine::Auto: return "auto";
    case Settings::AudioEngine::Cubeb: return "cubeb";
    case Settings::AudioEngine::Sdl2: return "sdl2";
    case Settings::AudioEngine::Null: return "null";
    case Settings::AudioEngine::Oboe: return "oboe";
    }
    return "auto";
}

Settings::AudioEngine AudioEngineFromString(std::string_view str)
{
    if (str == "auto") return Settings::AudioEngine::Auto;
    if (str == "cubeb") return Settings::AudioEngine::Cubeb;
    if (str == "sdl2") return Settings::AudioEngine::Sdl2;
    if (str == "null") return Settings::AudioEngine::Null;
    if (str == "oboe") return Settings::AudioEngine::Oboe;
    return Settings::AudioEngine::Auto;
}

const char* AudioModeToString(Settings::AudioMode value)
{
    switch (value) {
    case Settings::AudioMode::Mono: return "Mono";
    case Settings::AudioMode::Stereo: return "Stereo";
    case Settings::AudioMode::Surround: return "Surround";
    }
    return "Stereo";
}

Settings::AudioMode AudioModeFromString(std::string_view str)
{
    if (str == "Mono") return Settings::AudioMode::Mono;
    if (str == "Stereo") return Settings::AudioMode::Stereo;
    if (str == "Surround") return Settings::AudioMode::Surround;
    return Settings::AudioMode::Stereo;
}

const char* LanguageToString(Settings::Language value)
{
    switch (value) {
    case Settings::Language::Japanese: return "Japanese";
    case Settings::Language::EnglishAmerican: return "EnglishAmerican";
    case Settings::Language::French: return "French";
    case Settings::Language::German: return "German";
    case Settings::Language::Italian: return "Italian";
    case Settings::Language::Spanish: return "Spanish";
    case Settings::Language::Chinese: return "Chinese";
    case Settings::Language::Korean: return "Korean";
    case Settings::Language::Dutch: return "Dutch";
    case Settings::Language::Portuguese: return "Portuguese";
    case Settings::Language::Russian: return "Russian";
    case Settings::Language::Taiwanese: return "Taiwanese";
    case Settings::Language::EnglishBritish: return "EnglishBritish";
    case Settings::Language::FrenchCanadian: return "FrenchCanadian";
    case Settings::Language::SpanishLatin: return "SpanishLatin";
    case Settings::Language::ChineseSimplified: return "ChineseSimplified";
    case Settings::Language::ChineseTraditional: return "ChineseTraditional";
    case Settings::Language::PortugueseBrazilian: return "PortugueseBrazilian";
    }
    return "EnglishAmerican";
}

Settings::Language LanguageFromString(std::string_view str)
{
    if (str == "Japanese") return Settings::Language::Japanese;
    if (str == "EnglishAmerican") return Settings::Language::EnglishAmerican;
    if (str == "French") return Settings::Language::French;
    if (str == "German") return Settings::Language::German;
    if (str == "Italian") return Settings::Language::Italian;
    if (str == "Spanish") return Settings::Language::Spanish;
    if (str == "Chinese") return Settings::Language::Chinese;
    if (str == "Korean") return Settings::Language::Korean;
    if (str == "Dutch") return Settings::Language::Dutch;
    if (str == "Portuguese") return Settings::Language::Portuguese;
    if (str == "Russian") return Settings::Language::Russian;
    if (str == "Taiwanese") return Settings::Language::Taiwanese;
    if (str == "EnglishBritish") return Settings::Language::EnglishBritish;
    if (str == "FrenchCanadian") return Settings::Language::FrenchCanadian;
    if (str == "SpanishLatin") return Settings::Language::SpanishLatin;
    if (str == "ChineseSimplified") return Settings::Language::ChineseSimplified;
    if (str == "ChineseTraditional") return Settings::Language::ChineseTraditional;
    if (str == "PortugueseBrazilian") return Settings::Language::PortugueseBrazilian;
    return Settings::Language::EnglishAmerican;
}

const char* DockedModeToString(Settings::DockedMode value)
{
    switch (value) {
    case Settings::DockedMode::Handheld: return "Handheld";
    case Settings::DockedMode::Docked: return "Docked";
    }
    return "Docked";
}

Settings::DockedMode DockedModeFromString(std::string_view str)
{
    if (str == "Handheld") return Settings::DockedMode::Handheld;
    if (str == "Docked") return Settings::DockedMode::Docked;
    return Settings::DockedMode::Docked;
}
