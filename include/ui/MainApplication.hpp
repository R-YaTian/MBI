#pragma once

#include <pu/Plutonium>
#include <string>

#include "ui/MainPage.hpp"
#include "ui/sdInstPage.hpp"
#include "ui/usbHDDInstPage.hpp"
#include "ui/InstallerPage.hpp"

namespace app::ui
{
    #define COLOR(hex) pu::ui::Color::FromHex(hex)

    enum class Scene
    {
        Main,
        Options,
        NetworkInstll,
        UsbInstll,
        SdInstll,
        UdiskInstll,
        MtpInstll,
        Installer,
    };

    enum class Resources
    {
        UncheckedImage,
        CheckedImage,
        DirectoryImage,
        BackToParentImage,
    };

    class MainApplication : public pu::ui::Application
    {
        public:
            using Application::Application;
            PU_SMART_CTOR(MainApplication)
            void OnLoad() override;
            MainPage::Ref mainPage;
            sdInstPage::Ref sdinstPage;
            usbHDDInstPage::Ref usbhddinstPage;
            InstallerPage::Ref installerPage;
            pu::sdl2::TextureHandle::Ref checkboxBlank;
            pu::sdl2::TextureHandle::Ref checkboxTick;
            pu::sdl2::TextureHandle::Ref dirImg;
            pu::sdl2::TextureHandle::Ref dirbackImg;

            inline void SetBottomText(std::string text)
            {
                this->botText->SetText(text);
            }

            inline void SetPageInfoText(std::string text)
            {
                this->pageInfoText->SetText(text);
            }

            inline void HidePageInfo()
            {
                this->infoRect->SetVisible(false);
                this->pageInfoText->SetVisible(false);
            }

            inline void ShowPageInfo()
            {
                this->infoRect->SetVisible(true);
                this->pageInfoText->SetVisible(true);
            }
        private:
            std::string freeSpaceCurrentText;
            u32 batteryCurrentValue;
            pu::sdl2::TextureHandle::Ref bgImg;
            pu::sdl2::TextureHandle::Ref logoImg;
            pu::ui::elm::Image::Ref titleImage;
            pu::ui::elm::TextBlock::Ref appVersionText;
            pu::ui::elm::TextBlock::Ref freeSpaceText;
            pu::ui::elm::TextBlock::Ref batteryValueText;
            pu::ui::elm::Rectangle::Ref topRect;
            pu::ui::elm::Rectangle::Ref botRect;
            pu::ui::elm::Rectangle::Ref infoRect;
            pu::ui::elm::TextBlock::Ref botText;
            pu::ui::elm::TextBlock::Ref pageInfoText;
            pu::sdl2::TextureHandle::Ref LoadBackground(std::string bgDir);
            void UpdateStats();
    };

    inline pu::sdl2::TextureHandle::Ref LoadTexture(const std::string &path)
    {
        return pu::sdl2::TextureHandle::New(pu::ui::render::LoadImageFromFile(path));
    }

    void SceneJump(Scene idx);
    pu::sdl2::TextureHandle::Ref GetResource(Resources idx);
    void CloseWithFadeOut();
    bool IsShown();
    bool IsTouched();
}
