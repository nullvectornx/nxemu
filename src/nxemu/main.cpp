#include "settings/ui_settings.h"
#include "startup_checks.h"
#include "user_interface/notification.h"
#include "user_interface/sciter_main_window.h"
#include <common/path.h>
#include <common/std_string.h>
#include <memory>
#include <nxemu-core/app_init.h>
#include <nxemu-core/version.h>
#include <yuzu_common/fs/path_util.h>
#include <sciter_ui.h>
#include <widgets/list_box.h>
#include <widgets/combo_box.h>
#include <widgets/menubar.h>
#include <widgets/page_nav.h>
#include <widgets/tooltip_host.h>
#include "user_interface/widgets/rom_browser.h"
#include <windows.h>

void RegisterWidgets(ISciterUI & sciterUI)
{
    Register_WidgetListBox(sciterUI);
    Register_WidgetComboBox(sciterUI);
    Register_WidgetMenuBar(sciterUI);
    Register_WidgetPageNav(sciterUI);
    Register_WidgetToolTipHost(sciterUI);
    Register_WidgetRomBrowser(sciterUI);
}

extern "C" {
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;         // NVIDIA
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;   // AMD
}

int WINAPI WinMain(_In_ HINSTANCE /*hInstance*/, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPSTR /*lpszArgs*/, _In_ int /*nWinMode*/)
{
    bool res = AppInit(&Notification::GetInstance(), Path(Path::MODULE_DIRECTORY), Common::FS::PathToUTF8String(Common::FS::GetDefaultAppStorageDirectory()).c_str());

    if (uiSettings.performVulkanCheck)
    {
        VulkanCheckResult result = StartupVulkanChecks();
        if (result != VULKAN_CHECK_DONE)
        {
            return result == EXIT_VULKAN_AVAILABLE ? 0 : 1;
        }
    }

    ISciterUI * sciterUI = nullptr;
    if (res && !SciterUIInit(uiSettings.languageDir, uiSettings.languageBase.c_str(), uiSettings.languageCurrent.c_str(), uiSettings.sciterConsole, sciterUI))
    {
        res = false;
    }
    if (res)
    {
        RegisterWidgets(*sciterUI);
        SciterMainWindow window(*sciterUI, stdstr_f("NXEmu %s", VER_FILE_VERSION_STR).c_str());
        window.Show();
        sciterUI->Run();
    }
    if (sciterUI != nullptr)
    {
        sciterUI->Shutdown();
    }
    AppCleanup();
    Notification::CleanUp();
    return res ? 0 : 1;
}