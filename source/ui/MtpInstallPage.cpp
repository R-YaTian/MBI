#include "ui/MtpInstallPage.hpp"
#include "ui/MainApplication.hpp"
#include "util/config.hpp"
#include "util/i18n.hpp"
#include "nx/mtp.hpp"
#include "nx/usb.hpp"
#include "installer.hpp"
#include "facade.hpp"

namespace app::ui
{
    MtpInstallPage::MtpInstallPage() : Layout::Layout()
    {
        this->SetOnInput(std::bind(&MtpInstallPage::onInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        this->infoImage = pu::ui::elm::Image::New(780, 332 * pu::ui::render::ScreenFactor, LoadTexture("romfs:/images/icons/usb-connection-waiting.png"));
        this->Add(this->infoImage);
    }

    void MtpInstallPage::onInput(const u64 Down, const u64 Up, const u64 Held, const pu::ui::TouchPoint Pos)
    {
        static u64 tick;
        if (IsLongPress(tick, (Held & HidNpadButton_B) != 0, (Up & HidNpadButton_B) != 0, 1.0f))
        {
            nx::mtp::Cleanup();
            SceneJump(Scene::Main);
            nx::usb::usbDeviceInitialize();
        }
    }
}
