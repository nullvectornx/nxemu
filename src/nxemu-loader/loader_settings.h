#pragma once

struct LoaderSettings
{
    bool checkForUpdatedFirmware;
};

extern LoaderSettings loaderSettings;

void SetupLoaderSetting(void);
void SaveLoaderSettings(void);
