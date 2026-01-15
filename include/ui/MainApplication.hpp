#pragma once

#include <pu/Plutonium>
#include <string>

#include "ui/MainPage.hpp"
#include "ui/netInstPage.hpp"
#include "ui/sdInstPage.hpp"
#include "ui/usbInstPage.hpp"
#include "ui/usbHDDInstPage.hpp"
#include "ui/InstallerPage.hpp"
#include "ui/OptionsPage.hpp"

namespace app::ui
{
    #define COLOR(hex) pu::ui::Color::FromHex(hex)

    class MainApplication : public pu::ui::Application {
        public:
            using Application::Application;
            PU_SMART_CTOR(MainApplication)
            void OnLoad() override;
            MainPage::Ref mainPage;
            netInstPage::Ref netinstPage;
            sdInstPage::Ref sdinstPage;
            usbInstPage::Ref usbinstPage;
            usbHDDInstPage::Ref usbhddinstPage;
            InstallerPage::Ref installerPage;
            OptionsPage::Ref optionspage;
            pu::sdl2::TextureHandle::Ref checkboxBlank;
            pu::sdl2::TextureHandle::Ref checkboxTick;
            pu::sdl2::TextureHandle::Ref dirImg;
            pu::sdl2::TextureHandle::Ref dirbackImg;
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

            pu::sdl2::TextureHandle::Ref LoadBackground(std::string bgDir);
            void UpdateStats();
    };

    inline pu::sdl2::TextureHandle::Ref LoadTexture(const std::string &path)
    {
        return pu::sdl2::TextureHandle::New(pu::ui::render::LoadImageFromFile(path));
    }
}
