#include "system_config_system.h"
#include "config_setting.h"
#include "system_config.h"
#include <nxemu-os/os_settings_identifiers.h>
#include <nxemu-loader/loader_settings_identifiers.h>
#include <yuzu_common/settings_enums.h>

namespace
{
static ConfigSetting systemSettings[] = {
    ConfigSetting(ConfigSetting::ComboBox, "DockedMode", true, Settings::EnumMetadata<Settings::DockedMode>::Index(), NXOsSetting::DockedMode),
    ConfigSetting(ConfigSetting::CheckBox, "CheckForUpdatedFirmware", true, NXLoaderSetting::CheckForUpdatedFirmware),
};
}

SystemConfigSystem::SystemConfigSystem(ISciterUI & sciterUI, SystemConfig & config, SciterElement page) :
    m_sciterUI(sciterUI),
    m_config(config),
    m_page(page)
{
    m_config.SetupPage(page, systemSettings, sizeof(systemSettings) / sizeof(systemSettings[0]));
}

void SystemConfigSystem::SaveSetting(void)
{
    m_config.SavePage(m_page, systemSettings, sizeof(systemSettings) / sizeof(systemSettings[0]));
}
