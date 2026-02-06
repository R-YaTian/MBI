#pragma once

#include <pu/Plutonium>
#include <string>

namespace app::ui
{
    #define COLOR(hex) pu::ui::Color::FromHex(hex)

    enum class Scene
    {
        Main,
        Options,
        NetworkInstall,
        UsbInstall,
        SdInstall,
        UdiskInstall,
        MtpInstall,
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

            void HideClickableButton();
            void ShowClickableButton();
            void SetBackwardButtonCallback(std::function<void()> cb);
            void SetConfirmButtonCallback(std::function<void()> cb);
        private:
            std::string freeSpaceCurrentText;
            u32 batteryCurrentValue;
            pu::sdl2::TextureHandle::Ref bgImg;
            pu::sdl2::TextureHandle::Ref logoImg;
            pu::sdl2::TextureHandle::Ref backImg;
            pu::sdl2::TextureHandle::Ref confirmImg;
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

    inline bool IsLongPress(u64& start, bool held, bool up, double seconds)
    {
        if (up || !held)
        {
            start = 0;
            return false;
        }
        if (start == 0)
        {
            start = armGetSystemTick();
            return false;
        }
        u64 freq = armGetSystemTickFreq();
        if (armGetSystemTick() - start >= (u64)(seconds * freq))
        {
            start = 0;
            return true;
        }
        return false;
    }

    void SceneJump(Scene idx);
    pu::sdl2::TextureHandle::Ref GetResource(Resources idx);
    void CloseWithFadeOut();
    bool IsShown();
    bool IsTouchUp();
    void UpdateTouchState(const pu::ui::TouchPoint pos, const s32 region_x, const s32 region_y, const s32 region_w, const s32 region_h);
}
