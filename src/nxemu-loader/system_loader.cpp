#include "system_loader.h"
#include "core/core.h"
#include "core/file_sys/card_image.h"
#include "core/file_sys/content_archive.h"
#include "core/file_sys/filesystem.h"
#include "core/file_sys/patch_manager.h"
#include "core/file_sys/registered_cache.h"
#include "core/file_sys/romfs_factory.h"
#include "core/file_sys/submission_package.h"
#include "core/file_sys/system_archive/system_archive.h"
#include "core/file_sys/vfs/vfs.h"
#include "core/file_sys/vfs/vfs_real.h"
#include "core/file_sys/vfs/vfs_types.h"
#include "core/file_sys/vfs/vfs_vector.h"
#include "core/loader/loader.h"
#include "firmware_zip.h"
#include "loader_settings.h"
#include "rom_info.h"
#include <algorithm>
#include <cctype>
#include <common/path.h>
#include <cstdint>
#include <filesystem>
#include <fmt/core.h>
#include <nxemu-core/settings/identifiers.h>
#include <random>
#include <yuzu_common/fs/fs.h>

extern IModuleSettings * g_settings;
extern IModuleNotification * g_notify;

namespace
{

StorageId GetStorageIdForFrontendSlot(std::optional<FileSys::ContentProviderUnionSlot> slot)
{
    if (!slot.has_value())
    {
        return StorageId::None;
    }

    switch (*slot)
    {
    case FileSys::ContentProviderUnionSlot::UserNAND: return StorageId::NandUser;
    case FileSys::ContentProviderUnionSlot::SysNAND: return StorageId::NandSystem;
    case FileSys::ContentProviderUnionSlot::SDMC: return StorageId::SdCard;
    case FileSys::ContentProviderUnionSlot::FrontendManual: return StorageId::Host;
    }
    return StorageId::None;
}

std::string GetRegisteredNcaFilename(const std::string & source_name)
{
    std::string filename = source_name;
    const auto pos = filename.find_last_of("/\\");
    if (pos != std::string::npos)
    {
        filename = filename.substr(pos + 1);
    }

    std::string ext = filename;
    const auto dot = ext.rfind('.');
    if (dot != std::string::npos)
    {
        ext = ext.substr(dot);
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    }
    else
    {
        ext.clear();
    }

    if (ext == ".dnca")
    {
        return filename.substr(0, dot) + ".nca";
    }
    if (ext != ".nca")
    {
        return filename + ".nca";
    }
    return filename;
}

constexpr uint64_t SystemUpdateTitleId = 0x0100000000000816ULL;

struct FirmwareInstallSource
{
    FileSys::VirtualFile file;
    std::string registered_name;
};

std::string ToLowerAscii(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool ContainsSystemUpdateMeta(const std::vector<FirmwareInstallSource> & sources)
{
    for (const FirmwareInstallSource & source : sources)
    {
        const FileSys::NCA nca(source.file);
        if (nca.GetStatus() != LoaderResultStatus::Success)
        {
            continue;
        }
        if (nca.GetType() == FileSys::NCAContentType::Meta && nca.GetTitleId() == SystemUpdateTitleId)
        {
            return true;
        }
    }
    return false;
}

std::filesystem::path CreateFirmwareExtractDirectory()
{
    std::filesystem::path directory =
        std::filesystem::temp_directory_path() / "nxemu_firmware_install";
    std::error_code ec;
    std::filesystem::create_directories(directory, ec);

    std::random_device random_device;
    std::uniform_int_distribution<uint64_t> distribution(0, UINT64_MAX);
    for (int attempt = 0; attempt < 16; ++attempt)
    {
        const std::filesystem::path candidate =
            directory / fmt::format("{:016X}", distribution(random_device));
        if (!std::filesystem::exists(candidate))
        {
            std::filesystem::create_directories(candidate, ec);
            if (!ec)
            {
                return candidate;
            }
        }
    }
    return {};
}

void CollectFirmwareSourcesFromPath(const std::filesystem::path & path, const FileSys::VirtualFilesystem & vfs, std::vector<FirmwareInstallSource> & sources)
{
    std::error_code ec;
    if (!std::filesystem::exists(path, ec))
    {
        return;
    }

    if (std::filesystem::is_regular_file(path, ec))
    {
        const std::string extension = ToLowerAscii(path.extension().string());
        if (extension != ".nca" && extension != ".dnca")
        {
            return;
        }

        const FileSys::VirtualFile file = vfs->OpenFile(path.generic_string(), VirtualFileOpenMode::Read);
        if (!file)
        {
            return;
        }

        const FileSys::NCA nca(file);
        if (nca.GetStatus() != LoaderResultStatus::Success)
        {
            return;
        }

        sources.push_back({file, GetRegisteredNcaFilename(path.filename().string())});
        return;
    }

    if (!std::filesystem::is_directory(path, ec))
    {
        return;
    }

    for (const std::filesystem::directory_entry & entry : std::filesystem::recursive_directory_iterator(path, ec))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }

        const std::filesystem::path entry_path = entry.path();
        const std::string filename = entry_path.filename().string();
        const std::string extension = ToLowerAscii(entry_path.extension().string());

        if (filename == "00" && ToLowerAscii(entry_path.parent_path().extension().string()) == ".nca")
        {
            const FileSys::VirtualFile file = vfs->OpenFile(entry_path.generic_string(), VirtualFileOpenMode::Read);
            if (!file)
            {
                continue;
            }

            const FileSys::NCA nca(file);
            if (nca.GetStatus() != LoaderResultStatus::Success)
            {
                continue;
            }

            sources.push_back({file, GetRegisteredNcaFilename(entry_path.parent_path().filename().string())});
            continue;
        }

        if (extension != ".nca" && extension != ".dnca")
        {
            continue;
        }

        const FileSys::VirtualFile file = vfs->OpenFile(entry_path.generic_string(), VirtualFileOpenMode::Read);
        if (!file)
        {
            continue;
        }

        const FileSys::NCA nca(file);
        if (nca.GetStatus() != LoaderResultStatus::Success)
        {
            continue;
        }

        sources.push_back({file, GetRegisteredNcaFilename(filename)});
    }
}

FirmwareInstallResult InstallFirmwareFilesToRegistered(::FileSystemController & fs_controller, const std::vector<FirmwareInstallSource> & sources)
{
    if (sources.empty())
    {
        return FirmwareInstallResult::NoNCAsFound;
    }

    const FileSys::VirtualDir sys_content = fs_controller.GetSystemNANDContentDirectory();
    if (!sys_content)
    {
        return FirmwareInstallResult::SystemNandUnavailable;
    }
    if (!sys_content->IsWritable())
    {
        return FirmwareInstallResult::NotWritable;
    }
    if (!sys_content->CleanSubdirectoryRecursive("registered"))
    {
        return FirmwareInstallResult::FailedClearRegistered;
    }

    const FileSys::VirtualDir registered = sys_content->GetDirectoryRelative("registered");
    if (!registered)
    {
        return FirmwareInstallResult::FailedClearRegistered;
    }

    for (const FirmwareInstallSource & source : sources)
    {
        if (!source.file)
        {
            return FirmwareInstallResult::FailedCopy;
        }

        const std::string filename = source.registered_name.empty() ? GetRegisteredNcaFilename(source.file->GetName()) : source.registered_name;
        const FileSys::VirtualFile dst_file = registered->CreateFileRelative(filename);
        if (!dst_file || !FileSys::VfsRawCopy(source.file, dst_file))
        {
            LOG_ERROR(Core, "Firmware install: copy failed for {}", filename);
            return FirmwareInstallResult::FailedCopy;
        }
    }

    return FirmwareInstallResult::Success;
}

bool HasSupportedFileExtension(const char * fileName)
{
    static const char * supported_extensions[] = {
        "nro",
        "dxci",
        "dnsp"
    };

    std::string ext = Path(fileName).GetExtension();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    for (const char * supported : supported_extensions)
    {
        if (ext == supported)
        {
            return true;
        }
    }
    return false;
}

} // Anonymous namespace

class HostFilesystemImpl final :
    public IFilesystem
{
public:
    explicit HostFilesystemImpl(FileSys::VirtualFilesystem & vfs) :
        m_vfs(vfs)
    {
    }

    IVirtualDirectory * CreateDirectory(const char * path, VirtualFileOpenMode perms) override
    {
        if (!m_vfs || path == nullptr)
        {
            return nullptr;
        }
        FileSys::VirtualDir dir = m_vfs->CreateDirectory(path, perms);
        if (!dir)
        {
            return nullptr;
        }
        return std::make_unique<VirtualDirectoryImpl>(dir).release();
    }

private:
    FileSys::VirtualFilesystem & m_vfs;
};

struct Systemloader::Impl
{
    explicit Impl(Systemloader & loader, ISystemModules & modules) :
        m_loader(loader),
        m_modules(modules),
        m_fsController(loader),
        m_manualContentProvider(std::make_unique<ManualContentProviderImpl>()),
        m_hostFilesystem(m_virtualFilesystem),
        m_processID(0),
        m_titleID(0)
    {
    }
    ~Impl()
    {
        m_appLoader.reset();
    }

    FileSys::VirtualFile m_file;
    std::shared_ptr<Loader::AppLoader> m_appLoader;
    Systemloader & m_loader;
    ISystemModules & m_modules;
    /// RealVfsFilesystem instance
    FileSys::VirtualFilesystem m_virtualFilesystem;
    std::unique_ptr<FileSys::ContentProviderUnion> m_contentProvider;
    ::FileSystemController m_fsController;
    std::unique_ptr<ManualContentProviderImpl> m_manualContentProvider;
    HostFilesystemImpl m_hostFilesystem;
    uint64_t m_processID;
    uint64_t m_titleID;
};

Systemloader::Systemloader(ISystemModules & modules) :
    impl(std::make_unique<Impl>(*this, modules))
{
}

Systemloader::~Systemloader()
{
}

bool Systemloader::Initialize()
{
    SetupLoaderSetting();
    if (impl->m_virtualFilesystem == nullptr)
    {
        impl->m_virtualFilesystem = std::make_shared<FileSys::RealVfsFilesystem>();
    }
    if (impl->m_contentProvider == nullptr)
    {
        impl->m_contentProvider = std::make_unique<FileSys::ContentProviderUnion>();
    }
    impl->m_contentProvider->SetSlot(FileSys::ContentProviderUnionSlot::FrontendManual, &impl->m_manualContentProvider->Provider());
    impl->m_fsController.CreateFactories(*impl->m_virtualFilesystem, false);
    return true;
}

bool Systemloader::SelectAndLoad(void * parentWindow)
{
#ifndef __ANDROID__
    Path fileToOpen;
    std::string fileName;
    const char * filter = "Switch Files (*.dxci, *.dnsp, *.nro)\0*.dxci;*.dnsp;*.nro;\0All files (*.*)\0*.*\0";
    if (fileToOpen.FileSelect(parentWindow, Path(Path::MODULE_DIRECTORY), filter, true))
    {
        fileName = (const std::string &)fileToOpen;
    }
    if (fileName.length() == 0)
    {
        return false;
    }
    return LoadRom(fileName.c_str());
#else
    return false;
#endif
}

bool Systemloader::LoadRom(const char * fileName)
{
    g_settings->SetInt(NXCoreSetting::EmulationState, (int32_t)EmulationState::LoadingRom);
    impl->m_file = Core::GetGameFileFromPath(impl->m_virtualFilesystem, fileName);
    impl->m_appLoader = Loader::GetLoader(*this, impl->m_file, 0, 0);
    if (!impl->m_appLoader)
    {
        g_notify->DisplayError("The file format is not supported.", "Error loading file!");
        g_settings->SetBool(NXCoreSetting::EmulationState, (int32_t)EmulationState::Stopped);
        return false;
    }

    g_settings->SetInt(NXCoreSetting::EmulationState, (int32_t)EmulationState::RomLoaded);
    Loader::AppLoader * app_loader = impl->m_appLoader.get();
    const LoaderFileType file_type = impl->m_appLoader->GetFileType();
    if (file_type == LoaderFileType::Unknown || file_type == LoaderFileType::Error)
    {
        g_notify->DisplayError("The file format is not supported.", "Error loading file!");
        g_settings->SetBool(NXCoreSetting::EmulationState, (int32_t)EmulationState::Stopped);
        return false;
    }
    uint64_t program_id = 0;
    const LoaderResultStatus res = app_loader->ReadProgramId(program_id);
    if (res == LoaderResultStatus::Success && file_type == LoaderFileType::NCA)
    {
        UNIMPLEMENTED();
    }
    else if (res == LoaderResultStatus::Success && (file_type == LoaderFileType::XCI || file_type == LoaderFileType::NSP))
    {
        const auto nsp = file_type == LoaderFileType::NSP ? std::make_shared<FileSys::NSP>(impl->m_file) : FileSys::XCI{impl->m_file}.GetSecurePartitionNSP();
        for (const auto & title : nsp->GetNCAs())
        {
            for (const auto & entry : title.second)
            {
                FileSys::VirtualFile file = entry.second->GetBaseFile();
                impl->m_manualContentProvider->AddEntry(entry.first.first, entry.first.second, title.first, std::make_unique<VirtualFileImpl>(file).release());
            }
        }
    }
    else if (res != LoaderResultStatus::Success)
    {
        LOG_ERROR(Core, "Failed to find title id for ROM!");
    }

    uint32_t title_size = 0;
    std::string title = "Unknown program";
    LoaderResultStatus result = app_loader->ReadTitle(nullptr, &title_size);
    if (result == LoaderResultStatus::Success)
    {
        title.resize(title_size);
        result = app_loader->ReadTitle(title.data(), &title_size);
    }
    if (result != LoaderResultStatus::Success)
    {
        LOG_ERROR(Core, "Failed to read title for ROM!");
    }

    const auto [load_result, load_parameters] = app_loader->Load(*this, impl->m_modules);
    if (load_result != LoaderResultStatus::Success)
    {
        LOG_CRITICAL(Core, "Failed to load ROM (Error {})!", load_result);
        g_settings->SetBool(NXCoreSetting::EmulationState, (int32_t)EmulationState::Stopped);
        return false;
    }
    std::vector<u8> nacp_data;
    FileSys::NACP nacp;
    if (app_loader->ReadControlData(nacp) == LoaderResultStatus::Success)
    {
        nacp_data = nacp.GetRawBytes();
    }
    else
    {
        nacp_data.resize(sizeof(FileSys::RawNACP));
    }

    FileSys::PatchManager pm(impl->m_titleID, impl->m_fsController, *impl->m_contentProvider);
    uint32_t version = pm.GetGameVersion().value_or(0);

    g_settings->SetString(NXCoreSetting::GameName, title.c_str());
    g_settings->SetString(NXCoreSetting::GameFile, fileName);
    g_settings->SetBool(NXCoreSetting::EmulationRunning, true);

    // TODO(DarkLordZach): When FSController/Game Card Support is added, if
    // current_process_game_card use correct StorageId
    StorageId baseGameStorageId = GetStorageIdForFrontendSlot(impl->m_contentProvider->GetSlotForEntry(impl->m_titleID, LoaderContentRecordType::Program));
    StorageId updateStorageId = GetStorageIdForFrontendSlot(impl->m_contentProvider->GetSlotForEntry(FileSys::GetUpdateTitleID(impl->m_titleID), LoaderContentRecordType::Program));

    IOperatingSystem & operatingSystem = impl->m_modules.OperatingSystem();
    operatingSystem.StartApplicationProcess(load_parameters->main_thread_priority, load_parameters->main_thread_stack_size, version, baseGameStorageId, updateStorageId, nacp_data.data(), (uint32_t)nacp_data.size());
    return true;
}

IRomInfo * Systemloader::RomInfo(const char * fileName, uint64_t programId, uint64_t programIndex)
{
    if (!HasSupportedFileExtension(fileName))
    {
        return nullptr;
    }
    const FileSys::VirtualFile file = impl->m_virtualFilesystem->OpenFile(fileName, VirtualFileOpenMode::Read);
    if (!file)
    {
        return nullptr;
    }

    std::shared_ptr<Loader::AppLoader> loader = Loader::GetLoader(*this, file, programId, programIndex);
    if (!loader)
    {
        return nullptr;
    }
    std::unique_ptr<::RomInfo> info = std::make_unique<::RomInfo>(file, std::move(loader));
    return info.release();
}

IRomInfo * Systemloader::LoadedRomInfo()
{
    std::unique_ptr<::RomInfo> info = std::make_unique<::RomInfo>(impl->m_file, impl->m_appLoader);
    return info.release();
}

FileSys::VirtualFilesystem Systemloader::GetFilesystem()
{
    return impl->m_virtualFilesystem;
}

FileSystemController & Systemloader::GetFileSystemController()
{
    return impl->m_fsController;
}

FileSys::ContentProvider & Systemloader::GetContentProvider()
{
    return *impl->m_contentProvider;
}

void Systemloader::RegisterContentProvider(FileSys::ContentProviderUnionSlot slot, FileSys::ContentProvider * provider)
{
    impl->m_contentProvider->SetSlot(slot, provider);
}

void Systemloader::SetProcessID(uint64_t processID)
{
    impl->m_processID = processID;
}

void Systemloader::SetTitleID(uint64_t titleID)
{
    impl->m_titleID = titleID;
}

IFileSystemController & Systemloader::FileSystemController()
{
    return impl->m_fsController;
}

IContentProvider & Systemloader::ContentProvider()
{
    return *impl->m_contentProvider;
}

IFilesystem & Systemloader::Filesystem()
{
    return impl->m_hostFilesystem;
}

IVirtualFile * Systemloader::SynthesizeSystemArchive(const uint64_t title_id)
{
    FileSys::VirtualFile file = FileSys::SystemArchive::SynthesizeSystemArchive(title_id);
    if (file)
    {
        return std::make_unique<VirtualFileImpl>(file).release();
    }
    return nullptr;
}

IVirtualFile * Systemloader::CreateMemoryFile(const uint8_t * data, uint64_t size, const char * name)
{
    if (name == nullptr)
    {
        return nullptr;
    }
    if (size > 0 && data == nullptr)
    {
        return nullptr;
    }

    std::vector<u8> copy;
    if (size > 0)
    {
        copy.assign(data, data + size);
    }

    FileSys::VirtualFile file = std::make_shared<FileSys::VectorVfsFile>(std::move(copy), name);
    return std::make_unique<VirtualFileImpl>(file).release();
}

uint32_t Systemloader::GetContentProviderEntriesCount(bool useTitleType, LoaderTitleType titleType, bool useContentRecordType, LoaderContentRecordType contentRecordType, bool useTitleId, unsigned long long titleId)
{
    const FileSys::ContentProviderUnion & rcu = *impl->m_contentProvider;
    std::optional<LoaderTitleType> title = useTitleType ? std::optional<LoaderTitleType>((LoaderTitleType)titleType) : std::nullopt;
    std::optional<LoaderContentRecordType> record = useContentRecordType ? std::optional<LoaderContentRecordType>((LoaderContentRecordType)contentRecordType) : std::nullopt;
    std::optional<uint64_t> id = useTitleId ? std::optional<uint64_t>(titleId) : std::nullopt;
    const std::vector<ContentProviderEntry> list = rcu.ListEntriesFilter(title, record, id);
    return (uint32_t)list.size();
}

uint32_t Systemloader::GetContentProviderEntries(bool useTitleType, LoaderTitleType titleType, bool useContentRecordType, LoaderContentRecordType contentRecordType, bool useTitleId, unsigned long long titleId, ContentProviderEntry * entries, uint32_t entryCount)
{
    if (entries == nullptr || entryCount == 0)
    {
        return 0;
    }
    const FileSys::ContentProviderUnion & rcu = *impl->m_contentProvider;
    std::optional<LoaderTitleType> title = useTitleType ? std::optional<LoaderTitleType>((LoaderTitleType)titleType) : std::nullopt;
    std::optional<LoaderContentRecordType> record = useContentRecordType ? std::optional<LoaderContentRecordType>((LoaderContentRecordType)contentRecordType) : std::nullopt;
    std::optional<uint64_t> id = useTitleId ? std::optional<uint64_t>(titleId) : std::nullopt;
    const std::vector<ContentProviderEntry> list = rcu.ListEntriesFilter(title, record, id);
    entryCount = std::min((uint32_t)list.size(), entryCount);
    memcpy(entries, list.data(), entryCount * sizeof(ContentProviderEntry));
    return entryCount;
}

IFileSysNCA * Systemloader::GetContentProviderEntry(uint64_t title_id, LoaderContentRecordType type)
{
    return GetContentProvider().GetEntryNCA(title_id, type).release();
}

IFileSysNACP * Systemloader::GetPMControlMetadata(uint64_t programID)
{
    const FileSys::PatchManager pm(programID, GetFileSystemController(), GetContentProvider());
    FileSys::PatchManager::Metadata metadata = pm.GetControlMetadata();
    if (metadata.first == nullptr)
    {
        return nullptr;
    }
    return metadata.first.release();
}

IManualContentProvider & Systemloader::ManualContentProvider()
{
    return *(impl->m_manualContentProvider.get());
}

FirmwareInstallResult Systemloader::InstallFirmwareFromFolder(const char * utf8_folder_path)
{
    if (utf8_folder_path == nullptr || utf8_folder_path[0] == '\0')
    {
        return FirmwareInstallResult::InvalidArgument;
    }

    const std::filesystem::path source_folder(utf8_folder_path);
    if (!Common::FS::IsDir(source_folder))
    {
        return FirmwareInstallResult::SourceNotDirectory;
    }

    std::vector<std::filesystem::path> nca_paths;
    for (const std::filesystem::directory_entry & entry : std::filesystem::directory_iterator(source_folder))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (ext == ".dnca")
        {
            nca_paths.push_back(entry.path());
        }
    }

    if (nca_paths.empty())
    {
        return FirmwareInstallResult::NoNCAsFound;
    }

    std::vector<FirmwareInstallSource> sources;
    sources.reserve(nca_paths.size());
    for (const std::filesystem::path & src_path : nca_paths)
    {
        const std::string path_utf8 = src_path.generic_string();
        const FileSys::VirtualFile src_file = impl->m_virtualFilesystem->OpenFile(path_utf8, VirtualFileOpenMode::Read);
        if (!src_file)
        {
            LOG_ERROR(Core, "Firmware install: could not open source file {}", path_utf8);
            return FirmwareInstallResult::FailedCopy;
        }
        sources.push_back({src_file, GetRegisteredNcaFilename(src_path.filename().string())});
    }

    ::FileSystemController & fs_controller = impl->m_fsController;
    const FirmwareInstallResult result = InstallFirmwareFilesToRegistered(fs_controller, sources);
    if (result != FirmwareInstallResult::Success)
    {
        return result;
    }

    fs_controller.CreateFactories(*impl->m_virtualFilesystem, true);
    LOG_INFO(Core, "Firmware install: copied {} .dnca file(s) as .nca into system NAND registered storage", nca_paths.size());
    return FirmwareInstallResult::Success;
}

FirmwareInstallResult InstallFirmwareFromDxciFile(const FileSys::VirtualFilesystem & vfs, ::FileSystemController & fs_controller, const std::filesystem::path & source_file)
{
    const std::string path_utf8 = source_file.generic_string();
    const FileSys::VirtualFile file = vfs->OpenFile(path_utf8, VirtualFileOpenMode::Read);
    if (!file)
    {
        return FirmwareInstallResult::SourceNotFound;
    }

    FileSys::XCI xci(file);
    if (xci.GetStatus() != LoaderResultStatus::Success)
    {
        return FirmwareInstallResult::InvalidDxci;
    }

    const FileSys::VirtualDir update = xci.GetUpdatePartition();
    if (update == nullptr)
    {
        return FirmwareInstallResult::UpdatePartitionNotFound;
    }

    if (xci.GetSystemUpdateVersion() == 0)
    {
        return FirmwareInstallResult::NoNCAsFound;
    }

    std::vector<FirmwareInstallSource> sources;
    for (const FileSys::VirtualFile & update_file : update->GetFiles())
    {
        const FileSys::NCA nca(update_file);
        if (nca.GetStatus() != LoaderResultStatus::Success)
        {
            continue;
        }
        sources.push_back({update_file, GetRegisteredNcaFilename(update_file->GetName())});
    }

    if (sources.empty())
    {
        return FirmwareInstallResult::NoNCAsFound;
    }

    const FirmwareInstallResult result = InstallFirmwareFilesToRegistered(fs_controller, sources);
    if (result != FirmwareInstallResult::Success)
    {
        return result;
    }

    LOG_INFO(Core, "Firmware install: copied {} NCA(s) from DXCI update partition into system NAND registered storage", sources.size());
    return FirmwareInstallResult::Success;
}

FirmwareInstallResult InstallFirmwareFromZipFile(const FileSys::VirtualFilesystem & vfs, ::FileSystemController & fs_controller, const std::filesystem::path & source_file)
{
    const std::filesystem::path extract_directory = CreateFirmwareExtractDirectory();
    if (extract_directory.empty())
    {
        return FirmwareInstallResult::InvalidZip;
    }

    struct ExtractDirectoryCleanup
    {
        std::filesystem::path path;
        ~ExtractDirectoryCleanup()
        {
            if (!path.empty())
            {
                std::error_code ec;
                std::filesystem::remove_all(path, ec);
            }
        }
    } cleanup{extract_directory};

    if (!ExtractFirmwareZipToDirectory(source_file, extract_directory))
    {
        return FirmwareInstallResult::InvalidZip;
    }

    std::vector<FirmwareInstallSource> sources;
    CollectFirmwareSourcesFromPath(extract_directory, vfs, sources);
    if (sources.empty() || !ContainsSystemUpdateMeta(sources))
    {
        return FirmwareInstallResult::NoNCAsFound;
    }

    const FirmwareInstallResult result = InstallFirmwareFilesToRegistered(fs_controller, sources);
    if (result != FirmwareInstallResult::Success)
    {
        return result;
    }

    LOG_INFO(Core, "Firmware install: copied {} NCA(s) from ZIP into system NAND registered storage", sources.size());
    return FirmwareInstallResult::Success;
}

FirmwareInstallResult Systemloader::InstallFirmwareFromFile(const char * utf8_file_path)
{
    if (utf8_file_path == nullptr || utf8_file_path[0] == '\0')
    {
        return FirmwareInstallResult::InvalidArgument;
    }

    const std::filesystem::path source_file(utf8_file_path);
    if (!Common::FS::IsFile(source_file))
    {
        return FirmwareInstallResult::SourceNotFound;
    }

    const std::string extension = ToLowerAscii(source_file.extension().string());
    ::FileSystemController & fs_controller = impl->m_fsController;
    FirmwareInstallResult result = FirmwareInstallResult::InvalidArgument;

    if (extension == ".dxci")
    {
        result = InstallFirmwareFromDxciFile(impl->m_virtualFilesystem, fs_controller, source_file);
    }
    else if (extension == ".zip")
    {
        result = InstallFirmwareFromZipFile(impl->m_virtualFilesystem, fs_controller, source_file);
    }
    else
    {
        return FirmwareInstallResult::InvalidArgument;
    }

    if (result != FirmwareInstallResult::Success)
    {
        return result;
    }

    fs_controller.CreateFactories(*impl->m_virtualFilesystem, true);
    return FirmwareInstallResult::Success;
}
