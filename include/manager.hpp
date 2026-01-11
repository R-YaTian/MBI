#pragma once

#include <pu/Plutonium>

namespace app::manager
{
    void initApp();
    void deinitApp();
    void initInstallServices();
    void deinitInstallServices();

    inline pu::sdl2::TextureHandle::Ref LoadTexture(const std::string &path) {
        return pu::sdl2::TextureHandle::New(pu::ui::render::LoadImageFromFile(path));
    }

    inline pu::sdl2::TextureHandle::Ref LoadBackground(std::string appDir) {
        static const std::vector<std::string> exts = {".png", ".jpg", ".bmp"};
        for (auto const& ext : exts) {
            auto path = appDir + "/background" + ext;
            if (std::filesystem::exists(path)) {
                return LoadTexture(path);
            }
        }
        return LoadTexture("romfs:/images/background.jpg");
    }

    void playAudio(std::string audioPath);
    void lightningStart();
    void lightningStop();
}
