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
        this->infoImage = pu::ui::elm::Image::New(780, 332 * pu::ui::render::ScreenFactor, LoadTexture("romfs:/images/icons/usb-connection-waiting.png"));
        this->Add(this->infoImage);
    }

    void MtpInstallPage::onInput(u64 Down, u64 Up, u64 Held, pu::ui::TouchPoint Pos)
    {
        static u64 tick;
        if (IsLongPress(tick, (Held & HidNpadButton_B) != 0, (Up & HidNpadButton_B) != 0, 1.5f))
        {
            nx::mtp::Cleanup();
            SceneJump(Scene::Main);
            nx::usb::usbDeviceInitialize();
        }
    }
}
