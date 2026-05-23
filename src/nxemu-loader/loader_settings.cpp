#include "loader_settings.h"
#include "loader_settings_identifiers.h"
#include <common/json_util.h>
#include <nxemu-module-spec/base.h>
#include <yuzu_common/yuzu_assert.h>

extern IModuleSettings * g_settings;

LoaderSettings loaderSettings = {};

namespace
{
    enum class SettingType
    {
        BooleanValue,
    };

    class LoaderSetting
    {
    public:
        LoaderSetting(const char * id, const char * path, bool * val, bool defaultValue);

        const char * identifier;
        const char * json_path;
        SettingType settingType;
        union
        {
            bool * boolValue;
        } setting;
        union
        {
            bool boolValue;
        } defaultValue;
    };

    static LoaderSetting settings[] = {
        {NXLoaderSetting::CheckForUpdatedFirmware, "CheckForUpdatedFirmware", &loaderSettings.checkForUpdatedFirmware, true},
    };
};

void LoaderSettingChanged(const char * setting, void * /*userData*/)
{
    for (const LoaderSetting & loaderSetting : settings)
    {
        if (loaderSetting.identifier == nullptr)
        {
            continue;
        }

        if (strcmp(loaderSetting.identifier, setting) != 0)
        {
            continue;
        }
        switch (loaderSetting.settingType)
        {
        case SettingType::BooleanValue:
            *loaderSetting.setting.boolValue = g_settings->GetBool(setting);
            break;
        default:
            UNIMPLEMENTED();
        }
    }
}

void SetupLoaderSetting(void)
{
    for (const LoaderSetting & loaderSetting : settings)
    {
        switch (loaderSetting.settingType)
        {
        case SettingType::BooleanValue:
            *loaderSetting.setting.boolValue = loaderSetting.defaultValue.boolValue;
            break;
        default:
            UNIMPLEMENTED();
        }
    }

    JsonValue root;
    JsonReader reader;
    const std::string json = g_settings->GetSectionSettings("nxemu-loader");

    if (!json.empty() && reader.Parse(json.data(), json.data() + json.size(), root) && root.isObject())
    {
        for (const LoaderSetting & loaderSetting : settings)
        {
            const JsonValue value = JsonGetNestedValue(root, loaderSetting.json_path);
            switch (loaderSetting.settingType)
            {
            case SettingType::BooleanValue:
                if (value.isBool())
                {
                    *loaderSetting.setting.boolValue = value.asBool();
                }
                break;
            default:
                UNIMPLEMENTED();
            }
        }
    }

    for (const LoaderSetting & loaderSetting : settings)
    {
        if (loaderSetting.identifier == nullptr)
        {
            continue;
        }

        switch (loaderSetting.settingType)
        {
        case SettingType::BooleanValue:
            g_settings->SetDefaultBool(loaderSetting.identifier, loaderSetting.defaultValue.boolValue);
            g_settings->SetBool(loaderSetting.identifier, *loaderSetting.setting.boolValue);
            break;
        default:
            UNIMPLEMENTED();
        }
        g_settings->RegisterCallback(loaderSetting.identifier, LoaderSettingChanged, nullptr);
    }
}

void SaveLoaderSettings(void)
{
    JsonValue root;

    for (const LoaderSetting & loaderSetting : settings)
    {
        switch (loaderSetting.settingType)
        {
        case SettingType::BooleanValue:
            if (*loaderSetting.setting.boolValue != loaderSetting.defaultValue.boolValue)
            {
                JsonSetNestedValue(root, loaderSetting.json_path, *loaderSetting.setting.boolValue != 0);
            }
            break;
        default:
            UNIMPLEMENTED();
        }
    }
    g_settings->SetSectionSettings("nxemu-loader", root.isNull() ? "" : JsonStyledWriter().write(root));
}

LoaderSetting::LoaderSetting(const char * id, const char * path, bool * val, bool defaultValue_) :
    identifier(id),
    json_path(path),
    settingType(SettingType::BooleanValue)
{
    setting.boolValue = val;
    defaultValue.boolValue = defaultValue_;
}
