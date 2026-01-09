#pragma once
#include <filesystem>
#include <pu/Plutonium>

namespace app::util {
    void initApp();
    void deinitApp();
    void initInstallServices();
    void deinitInstallServices();
    bool ignoreCaseCompare(const std::string &a, const std::string &b);
    std::vector<std::filesystem::path> getDirectoryFiles(const std::string & dir, const std::vector<std::string> & extensions);
    std::vector<std::filesystem::path> getDirsAtPath(const std::string & dir);
    std::string formatUrlString(std::string ourString);
    std::string formatUrlLink(std::string ourString);
    std::string shortenString(std::string ourString, int ourLength, bool isFile);
    std::string softwareKeyboard(std::string guideText, std::string initialText, int LenMax);
    std::vector<uint32_t> setClockSpeed(int deviceToClock, uint32_t clockSpeed);
    std::string getIPAddress();
    void playAudio(std::string audioPath);
    void lightningStart();
    void lightningStop();
    std::string* getBatteryCharge();

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
}
