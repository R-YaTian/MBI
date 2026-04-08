#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <haze.h>
#include "nx/error.hpp"
#include "nx/mtp.hpp"
#include "nx/fs.hpp"

// Set to 1 if switch freezes here when allocating a huge (1+GB) file.
// afaik this only happens when using emuMMC + windows.
#define NX_MTP_DISABLE_SET_FILE_SIZE 0

namespace nx::mtp
{
    #define R_SUCCEED() return (Result)0
    #define R_THROW(_rc) return _rc
    #define R_UNLESS(expr, res) { \
        if (!(expr)) { \
            R_THROW(res); \
        } \
    }
    #define R_TRY(r) { \
        if (const auto _rc = (r); R_FAILED(_rc)) { \
            R_THROW(_rc); \
        } \
    }
    #define CONCATENATE_IMPL(s1, s2) s1##s2
    #define CONCATENATE(s1, s2) CONCATENATE_IMPL(s1, s2)
    #define ANONYMOUS_VARIABLE(pref) CONCATENATE(pref, __COUNTER__)

    template<typename Function>
    struct ScopeGuard {
        ScopeGuard(Function&& function) : m_function(std::forward<Function>(function)) {}

        ~ScopeGuard()
        {
            m_function();
        }

        ScopeGuard(const ScopeGuard&) = delete;
        void operator=(const ScopeGuard&) = delete;

    private:
        const Function m_function;
    };

    struct ScopedMutex {
        ScopedMutex(Mutex* mutex) : m_mutex{mutex}
        {
            mutexLock(m_mutex);
        }

        ~ScopedMutex()
        {
            mutexUnlock(m_mutex);
        }

        ScopedMutex(const ScopedMutex&) = delete;
        void operator=(const ScopedMutex&) = delete;

    private:
        Mutex* const m_mutex;
    };

    #define ON_SCOPE_EXIT(_f) ScopeGuard ANONYMOUS_VARIABLE(SCOPE_EXIT_STATE_){[&] { _f; }};
    #define SCOPED_MUTEX(_m) ScopedMutex ANONYMOUS_VARIABLE(SCOPE_EXIT_STATE_){_m}

    enum FsError {
        FsError_PathNotFound = 0x202,
        FsError_PathAlreadyExists = 0x402,
        FsError_NotImplemented = 0x177202,
    };

    struct InstallSharedData {
        Mutex mutex;
        std::string current_file;
        FsContentStorageId target_storage = FsContentStorageId_SdCard;

        OnInstallStart on_start;
        OnInstallWrite on_write;
        OnInstallClose on_close;

        bool in_progress;
        bool enabled;
    };
    InstallSharedData g_shared_data{};

    void OnInstallTask()
    {
        SCOPED_MUTEX(&g_shared_data.mutex);

        if (!g_shared_data.in_progress)
        {
            if (!g_shared_data.current_file.empty())
            {
                LOG_DEBUG("[MTP] pushing new file data\n");
                if (!g_shared_data.on_start || !g_shared_data.on_start(g_shared_data.current_file.c_str()))
                {
                    g_shared_data.current_file.clear();
                }
                else
                {
                    LOG_DEBUG("[MTP] success on new file push\n");
                    g_shared_data.in_progress = true;
                }
            }
        }
    }

    struct FsNative : haze::FileSystemProxyImpl {
        using File = FsFile;
        using Dir = FsDir;
        using DirEntry = FsDirectoryEntry;

        FsNative() = default;

        FsNative(FsFileSystem* fs, bool own)
        {
            m_fs = *fs;
            m_own = own;
        }

        ~FsNative()
        {
            fsFsCommit(&m_fs);
            if (m_own)
            {
                fsFsClose(&m_fs);
            }
        }

        auto FixPath(const char* path, char* out = nullptr) const -> const char*
        {
            static char buf[FS_MAX_PATH];
            const auto len = std::strlen(GetName());

            if (!out)
            {
                out = buf;
            }

            if (len && !strncasecmp(path, GetName(), len))
            {
                std::snprintf(out, sizeof(buf), "/%s", path + len);
            }
            else
            {
                std::strcpy(out, path);
            }

            return out;
        }

        Result GetTotalSpace(const char *path, s64 *out) override
        {
            return fsFsGetTotalSpace(&m_fs, FixPath(path), out);
        }

        Result GetFreeSpace(const char *path, s64 *out) override
        {
            return fsFsGetFreeSpace(&m_fs, FixPath(path), out);
        }

        Result GetEntryType(const char *path, haze::FileAttrType *out_entry_type) override
        {
            FsDirEntryType type;
            R_TRY(fsFsGetEntryType(&m_fs, FixPath(path), &type));
            *out_entry_type = (type == FsDirEntryType_Dir) ? haze::FileAttrType_DIR : haze::FileAttrType_FILE;
            R_SUCCEED();
        }

        Result GetEntryAttributes(const char *path, haze::FileAttr *out) override
        {
            FsDirEntryType type;
            R_TRY(fsFsGetEntryType(&m_fs, FixPath(path), &type));

            if (type == FsDirEntryType_File)
            {
                out->type = haze::FileAttrType_FILE;

                // it doesn't matter if this fails.
                FsTimeStampRaw timestamp{};
                if (R_SUCCEEDED(fsFsGetFileTimeStampRaw(&m_fs, FixPath(path), &timestamp)) && timestamp.is_valid)
                {
                    out->ctime = timestamp.created;
                    out->mtime = timestamp.modified;
                }

                FsFile file;
                R_TRY(fsFsOpenFile(&m_fs, FixPath(path), FsOpenMode_Read, &file));
                ON_SCOPE_EXIT(fsFileClose(&file));

                s64 size;
                R_TRY(fsFileGetSize(&file, &size));
                out->size = size;
            }
            else
            {
                out->type = haze::FileAttrType_DIR;
            }

            if (IsReadOnly())
            {
                out->flag |= haze::FileAttrFlag_READ_ONLY;
            }

            R_SUCCEED();
        }

        Result CreateFile(const char* path, s64 size) override
        {
            u32 flags = 0;
            const s64 _4_GB = 0x100000000;
            if (size >= _4_GB)
            {
                flags = FsCreateOption_BigFile;
            }

            // do not set the size here because it can block for too long which may cause timeouts.
            // SEE: https://github.com/ITotalJustice/libhaze/issues/1#issuecomment-3305067733
            return fsFsCreateFile(&m_fs, FixPath(path), 0, flags);
        }

        Result DeleteFile(const char* path) override
        {
            return fsFsDeleteFile(&m_fs, FixPath(path));
        }

        Result RenameFile(const char *old_path, const char *new_path) override
        {
            char temp[FS_MAX_PATH];
            return fsFsRenameFile(&m_fs, FixPath(old_path, temp), FixPath(new_path));
        }

        Result OpenFile(const char *path, haze::FileOpenMode mode, haze::File *out_file) override
        {
            u32 flags = FsOpenMode_Read;
            if (mode == haze::FileOpenMode_WRITE)
            {
                flags = FsOpenMode_Write | FsOpenMode_Append;
            }

            auto f = new File();
            const auto rc = fsFsOpenFile(&m_fs, FixPath(path), flags, f);
            if (R_FAILED(rc))
            {
                delete f;
                return rc;
            }

            out_file->impl = f;
            R_SUCCEED();
        }

        Result GetFileSize(haze::File *file, s64 *out_size) override
        {
            auto f = static_cast<File*>(file->impl);
            return fsFileGetSize(f, out_size);
        }

        Result SetFileSize(haze::File *file, s64 size) override
        {
#if NX_MTP_DISABLE_SET_FILE_SIZE
            R_SUCCEED();
#else
            auto f = static_cast<File*>(file->impl);
            return fsFileSetSize(f, size);
#endif
        }

        Result ReadFile(haze::File *file, s64 off, void *buf, u64 read_size, u64 *out_bytes_read) override
        {
            auto f = static_cast<File*>(file->impl);
            return fsFileRead(f, off, buf, read_size, FsReadOption_None, out_bytes_read);
        }

        Result WriteFile(haze::File *file, s64 off, const void *buf, u64 write_size) override
        {
            auto f = static_cast<File*>(file->impl);
            return fsFileWrite(f, off, buf, write_size, FsWriteOption_None);
        }

        void CloseFile(haze::File *file) override
        {
            auto f = static_cast<File*>(file->impl);
            if (f) {
                fsFileClose(f);
                delete f;
                file->impl = nullptr;
            }
        }

        Result CreateDirectory(const char* path) override
        {
            return fsFsCreateDirectory(&m_fs, FixPath(path));
        }

        Result DeleteDirectoryRecursively(const char* path) override
        {
            return fsFsDeleteDirectoryRecursively(&m_fs, FixPath(path));
        }

        Result RenameDirectory(const char *old_path, const char *new_path) override
        {
            char temp[FS_MAX_PATH];
            return fsFsRenameDirectory(&m_fs, FixPath(old_path, temp), FixPath(new_path));
        }

        Result OpenDirectory(const char *path, haze::Dir *out_dir) override
        {
            auto dir = new Dir();
            const auto rc = fsFsOpenDirectory(&m_fs, FixPath(path), FsDirOpenMode_ReadDirs | FsDirOpenMode_ReadFiles | FsDirOpenMode_NoFileSize, dir);
            if (R_FAILED(rc))
            {
                delete dir;
                return rc;
            }

            out_dir->impl = dir;
            R_SUCCEED();
        }

        Result ReadDirectory(haze::Dir *d, s64 *out_total_entries, size_t max_entries, haze::DirEntry *buf) override
        {
            auto dir = static_cast<Dir*>(d->impl);

            std::vector<FsDirectoryEntry> entries(max_entries);
            R_TRY(fsDirRead(dir, out_total_entries, entries.size(), entries.data()));

            for (s64 i = 0; i < *out_total_entries; i++)
            {
                std::strcpy(buf[i].name, entries[i].name);
            }

            R_SUCCEED();
        }

        Result GetDirectoryEntryCount(haze::Dir *d, s64 *out_count) override
        {
            auto dir = static_cast<Dir*>(d->impl);
            return fsDirGetEntryCount(dir, out_count);
        }

        void CloseDirectory(haze::Dir *d) override
        {
            auto dir = static_cast<Dir*>(d->impl);
            if (dir)
            {
                fsDirClose(dir);
                delete dir;
                d->impl = nullptr;
            }
        }

        FsFileSystem m_fs{};
        bool m_own{true};
    };

    struct FsSdmc final : FsNative {
        FsSdmc() : FsNative(fsdevGetDeviceFileSystem("sdmc"), false) {}

        const char* GetName() const
        {
            return "";
        }

        const char* GetDisplayName() const
        {
            return "micro SD Card";
        }
    };

    struct FsAlbum final : FsNative {
        FsAlbum(FsImageDirectoryId id)
        {
            fsOpenImageDirectoryFileSystem(&m_fs, id);
        }

        const char* GetName() const
        {
            return "album:/";
        }

        const char* GetDisplayName() const
        {
            return "Album";
        }

        bool IsReadOnly() override { return true; }
    };

    struct FsProxyBase : haze::FileSystemProxyImpl {
        FsProxyBase(const char* name, const char* display_name) : m_name{name}, m_display_name{display_name} {}

        auto FixPath(const char* base, const char* path) const
        {
            static char buf[FS_MAX_PATH];
            const auto len = std::strlen(GetName());

            if (len && !strncasecmp(path, GetName(), len)) {
                std::snprintf(buf, sizeof(buf), "%s/%s", base, path + len);
            } else {
                std::snprintf(buf, sizeof(buf), "%s/%s", base, path);
            }

            return (std::string)buf;
        }

        const char* GetName() const override
        {
            return m_name.c_str();
        }

        const char* GetDisplayName() const override
        {
            return m_display_name.c_str();
        }

    protected:
        const std::string m_name;
        const std::string m_display_name;
    };

    struct FsProxyVfs : FsProxyBase {
        struct File {
            u64 index{};
            haze::FileOpenMode mode{};
        };

        struct Dir {
            u64 pos{};
        };

        using FsProxyBase::FsProxyBase;

        virtual ~FsProxyVfs() = default;

        auto FixPath(const char* path) const
        {
            return FsProxyBase::FixPath("", path);
        }

        auto GetFileName(const char* s) -> const char*
        {
            const auto file_name = std::strrchr(s, '/');
            if (!file_name || file_name[1] == '\0')
            {
                return nullptr;
            }
            return file_name + 1;
        }

        virtual Result GetEntryType(const char *path, haze::FileAttrType *out_entry_type)
        {
            if (FixPath(path) == "/")
            {
                *out_entry_type = haze::FileAttrType_DIR;
                R_SUCCEED();
            }
            else
            {
                const auto file_name = GetFileName(path);
                R_UNLESS(file_name, FsError_PathNotFound);

                const auto it = std::ranges::find_if(m_entries, [file_name](auto& e){
                    return !strcasecmp(file_name, e.name);
                });
                R_UNLESS(it != m_entries.end(), FsError_PathNotFound);

                *out_entry_type = haze::FileAttrType_FILE;
                R_SUCCEED();
            }
        }

        virtual Result CreateFile(const char* path, s64 size)
        {
            const auto file_name = GetFileName(path);
            R_UNLESS(file_name, FsError_PathNotFound);

            const auto it = std::ranges::find_if(m_entries, [file_name](auto& e){
                return !strcasecmp(file_name, e.name);
            });
            R_UNLESS(it == m_entries.end(), FsError_PathAlreadyExists);

            FsDirectoryEntry entry{};
            std::strcpy(entry.name, file_name);
            entry.type = FsDirEntryType_File;
            entry.file_size = size;

            m_entries.emplace_back(entry);
            R_SUCCEED();
        }

        virtual Result DeleteFile(const char* path)
        {
            const auto file_name = GetFileName(path);
            R_UNLESS(file_name, FsError_PathNotFound);

            const auto it = std::ranges::find_if(m_entries, [file_name](auto& e){
                return !strcasecmp(file_name, e.name);
            });
            R_UNLESS(it != m_entries.end(), FsError_PathNotFound);

            m_entries.erase(it);
            R_SUCCEED();
        }

        virtual Result RenameFile(const char *old_path, const char *new_path)
        {
            const auto file_name = GetFileName(old_path);
            R_UNLESS(file_name, FsError_PathNotFound);

            const auto it = std::ranges::find_if(m_entries, [file_name](auto& e){
                return !strcasecmp(file_name, e.name);
            });
            R_UNLESS(it != m_entries.end(), FsError_PathNotFound);

            const auto file_name_new = GetFileName(new_path);
            R_UNLESS(file_name_new, FsError_PathNotFound);

            const auto new_it = std::ranges::find_if(m_entries, [file_name_new](auto& e){
                return !strcasecmp(file_name_new, e.name);
            });
            R_UNLESS(new_it == m_entries.end(), FsError_PathAlreadyExists);

            std::strcpy(it->name, file_name_new);
            R_SUCCEED();
        }

        virtual Result OpenFile(const char *path, haze::FileOpenMode mode, haze::File *out_file)
        {
            const auto file_name = GetFileName(path);
            R_UNLESS(file_name, FsError_PathNotFound);

            const auto it = std::ranges::find_if(m_entries, [file_name](auto& e){
                return !strcasecmp(file_name, e.name);
            });
            R_UNLESS(it != m_entries.end(), FsError_PathNotFound);

            auto f = new File();
            f->index = std::distance(m_entries.begin(), it);
            f->mode = mode;
            out_file->impl = f;
            R_SUCCEED();
        }

        virtual Result GetFileSize(haze::File *file, s64 *out_size)
        {
            auto f = static_cast<File*>(file->impl);
            *out_size = m_entries[f->index].file_size;
            R_SUCCEED();
        }

        virtual Result SetFileSize(haze::File *file, s64 size)
        {
            auto f = static_cast<File*>(file->impl);
            m_entries[f->index].file_size = size;
            R_SUCCEED();
        }

        virtual Result ReadFile(haze::File *file, s64 off, void *buf, u64 read_size, u64 *out_bytes_read)
        {
            // stub for now as it may confuse users who think that the returned file is valid.
            // the code below can be used to benchmark mtp reads.
            R_THROW(FsError_NotImplemented);
        }

        virtual Result WriteFile(haze::File *file, s64 off, const void *buf, u64 write_size)
        {
            auto f = static_cast<File*>(file->impl);
            auto& e = m_entries[f->index];
            e.file_size = std::max<s64>(e.file_size, off + write_size);
            R_SUCCEED();
        }

        virtual void CloseFile(haze::File *file)
        {
            auto f = static_cast<File*>(file->impl);
            if (f)
            {
                delete f;
                file->impl = nullptr;
            }
        }

        Result CreateDirectory(const char* path) override
        {
            R_THROW(FsError_NotImplemented);
        }

        Result DeleteDirectoryRecursively(const char* path) override
        {
            R_THROW(FsError_NotImplemented);
        }

        Result RenameDirectory(const char *old_path, const char *new_path) override
        {
            R_THROW(FsError_NotImplemented);
        }

        Result OpenDirectory(const char *path, haze::Dir *out_dir) override
        {
            auto dir = new Dir();
            out_dir->impl = dir;
            R_SUCCEED();
        }

        Result ReadDirectory(haze::Dir *d, s64 *out_total_entries, size_t max_entries, haze::DirEntry *buf) override
        {
            auto dir = static_cast<Dir*>(d->impl);

            max_entries = std::min<s64>(m_entries.size() - dir->pos, max_entries);

            for (size_t i = 0; i < max_entries; i++)
            {
                std::strcpy(buf[i].name, m_entries[dir->pos + i].name);
            }

            dir->pos += max_entries;
            *out_total_entries = max_entries;
            R_SUCCEED();
        }

        Result GetDirectoryEntryCount(haze::Dir *d, s64 *out_count) override
        {
            *out_count = m_entries.size();
            R_SUCCEED();
        }

        void CloseDirectory(haze::Dir *d) override
        {
            auto dir = static_cast<Dir*>(d->impl);
            if (dir)
            {
                delete dir;
                d->impl = nullptr;
            }
        }

    protected:
        std::vector<FsDirectoryEntry> m_entries;
    };

    struct FsInstallProxy final : FsProxyVfs {
        const char* SUPPORTED_EXT[4] = {
            ".nsp", ".xci", ".nsz", ".xcz",
        };
        using FsProxyVfs::FsProxyVfs;

        Result FailedIfNotEnabled()
        {
            SCOPED_MUTEX(&g_shared_data.mutex);
            if (!g_shared_data.enabled)
            {
                R_THROW(FsError_NotImplemented);
            }
            R_SUCCEED();
        }

        Result IsValidFileType(const char* name)
        {
            const char* ext = std::strrchr(name, '.');
            if (!ext)
            {
                R_THROW(FsError_NotImplemented);
            }

            bool found = false;
            for (size_t i = 0; i < std::size(SUPPORTED_EXT); i++)
            {
                if (!strcasecmp(ext, SUPPORTED_EXT[i]))
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                R_THROW(FsError_NotImplemented);
            }

            R_SUCCEED();
        }

        Result GetTotalSpace(const char *path, s64 *out) override
        {
            *out = fs::GetTotalSpaceSize(g_shared_data.target_storage);
            R_SUCCEED();
        }

        Result GetFreeSpace(const char *path, s64 *out) override
        {
            *out = fs::GetFreeSpaceSize(g_shared_data.target_storage);
            R_SUCCEED();
        }

        Result GetEntryType(const char *path, haze::FileAttrType *out_entry_type) override
        {
            R_TRY(FsProxyVfs::GetEntryType(path, out_entry_type));
            if (*out_entry_type == haze::FileAttrType_FILE)
            {
                R_TRY(FailedIfNotEnabled());
            }
            R_SUCCEED();
        }

        Result CreateFile(const char* path, s64 size) override
        {
            R_TRY(FailedIfNotEnabled());
            R_TRY(IsValidFileType(path));
            R_TRY(FsProxyVfs::CreateFile(path, size));
            R_SUCCEED();
        }

        Result OpenFile(const char *path, haze::FileOpenMode mode, haze::File *out_file) override
        {
            R_TRY(FailedIfNotEnabled());
            R_TRY(IsValidFileType(path));
            R_TRY(FsProxyVfs::OpenFile(path, mode, out_file));
            LOG_DEBUG("[MTP] done file open: %s mode: 0x%X\n", path, mode);

            if (mode == haze::FileOpenMode_WRITE)
            {
                auto f = static_cast<File*>(out_file->impl);
                const auto& e = m_entries[f->index];

                // check if we already have this file queued.
                LOG_DEBUG("[MTP] checking if empty\n");
                R_UNLESS(g_shared_data.current_file.empty(), FsError_NotImplemented);
                LOG_DEBUG("[MTP] is empty\n");
                g_shared_data.current_file = e.name;
                OnInstallTask();
            }

            LOG_DEBUG("[MTP] got file: %s\n", path);
            R_SUCCEED();
        }

        Result WriteFile(haze::File *file, s64 off, const void *buf, u64 write_size) override
        {
            SCOPED_MUTEX(&g_shared_data.mutex);
            if (!g_shared_data.enabled)
            {
                LOG_DEBUG("[MTP] failing as not enabled\n");
                R_THROW(FsError_NotImplemented);
            }

            if (!g_shared_data.on_write || !g_shared_data.on_write(buf, write_size))
            {
                LOG_DEBUG("[MTP] failing as not written\n");
                R_THROW(FsError_NotImplemented);
            }

            R_TRY(FsProxyVfs::WriteFile(file, off, buf, write_size));
            R_SUCCEED();
        }

        void CloseFile(haze::File *file) override
        {
            auto f = static_cast<File*>(file->impl);
            if (!f)
            {
                return;
            }

            bool update{};
            {
                SCOPED_MUTEX(&g_shared_data.mutex);
                if (f->mode == haze::FileOpenMode_WRITE)
                {
                    LOG_DEBUG("[MTP] closing current file\n");
                    if (g_shared_data.on_close)
                    {
                        g_shared_data.on_close();
                    }

                    g_shared_data.in_progress = false;
                    g_shared_data.current_file.clear();
                    update = true;
                }
            }

            if (update)
            {
                OnInstallTask();
            }

            FsProxyVfs::CloseFile(file);
        }
    };

    void InitInstallMode(const OnInstallStart& on_start, const OnInstallWrite& on_write, const OnInstallClose& on_close)
    {
        SCOPED_MUTEX(&g_shared_data.mutex);
        g_shared_data.on_start = on_start;
        g_shared_data.on_write = on_write;
        g_shared_data.on_close = on_close;
        g_shared_data.enabled = true;
    }

    void DisableInstallMode()
    {
        SCOPED_MUTEX(&g_shared_data.mutex);
        g_shared_data.enabled = false;
    }

    void Setup()
    {
        haze::FsEntries fs_entries;
        fs_entries.emplace_back(std::make_shared<FsSdmc>());
        fs_entries.emplace_back(std::make_shared<FsAlbum>(FsImageDirectoryId_Nand));
        fs_entries.emplace_back(std::make_shared<FsInstallProxy>("install", "Install (NSP, XCI, NSZ, XCZ)"));

        haze::Initialize(nullptr, fs_entries);
    }

    void Cleanup()
    {
        haze::Exit();
    }
}
