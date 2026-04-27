#pragma once

#include <functional>

namespace nx::mtp
{
    using OnInstallStart = std::function<bool(const char* path)>;
    using OnInstallWrite = std::function<bool(const void* buf, size_t size)>;
    using OnInstallClose = std::function<void()>;

    void InitInstallMode(const OnInstallStart& on_start, const OnInstallWrite& on_write, const OnInstallClose& on_close);
    void DisableInstallMode();
    void FinishInstallProgress();
    void Setup();
    void Cleanup();
}
