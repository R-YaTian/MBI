#include "ui/MainApplication.hpp"
#include "ui/MtpInstallPage.hpp"
#include "install/InstallTask.hpp"
#include "install/MtpWorker.hpp"
#include "util/ScopedMutex.hpp"
#include "util/config.hpp"
#include "util/i18n.hpp"
#include "nx/mtp.hpp"
#include "nx/usb.hpp"
#include "nx/nsp.hpp"
#include "nx/xci.hpp"
#include "nx/misc.hpp"
#include "facade.hpp"
#include "installer.hpp"

namespace app::ui
{
    enum class State
    {
        // not connected.
        None,
        // just connected, starts the transfer.
        Connected,
        // set whilst transfer is in progress.
        Progress,
        // set when the transfer is finished.
        Done,
        // failed to transfer.
        Failed,
    };

    struct MtpInstallPage::InternalData
    {
        std::unique_ptr<install::MtpWorker> m_source{};
        std::string m_currentFile{};
        nx::mtp::InstallProxyTargetStorage m_targetStorage{nx::mtp::InstallProxyTargetStorage::SdCard};
        Mutex m_mutex{};
        State m_state{State::None};
        std::stop_source m_stop_source{};
        bool m_anyButtonTriggered{};
    };

    MtpInstallPage::MtpInstallPage() : InstallerPage()
    {
        this->SetOnInput(std::bind(&MtpInstallPage::onInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        this->infoImage = pu::ui::elm::Image::New(780, 332 * pu::ui::render::ScreenFactor, LoadTexture("romfs:/images/icons/usb-connection-waiting.webp"));
        this->Add(this->infoImage);
        this->AddRenderCallback(std::bind(&MtpInstallPage::updateState, this));
        this->AddRenderCallback(std::bind(&MtpInstallPage::onInstallTask, this));
        pageData = std::make_unique<InternalData>();
        mutexInit(&pageData->m_mutex);
    }

    MtpInstallPage::~MtpInstallPage() = default;

    void MtpInstallPage::onInstallTask()
    {
        if (pageData->m_state == State::Connected)
        {
            pageData->m_state = State::Progress;
            std::string shortFileName;
            try
            {
                shortFileName = nx::misc::ShortenString(pageData->m_currentFile, 42, 4);
                app::facade::SendInstallInfoText("inst.info_page.top_info0"_lang + shortFileName);
                pageData->m_source->SetInstallState(install::MTPInstallState::Progress);
                std::unique_ptr<InstallTask> installTask =
                    std::make_unique<InstallTask>(static_cast<NcmStorageId>(pageData->m_targetStorage + 4), app::config::overClock, app::config::ignoreReqVers, app::config::fixTicket, app::config::skipBase, pageData->m_source.get());
                app::facade::SendInstallProgress(0);
                app::facade::SendInstallBarText("inst.info_page.preparing"_lang);
                pageData->m_source->RetrieveHeader();
                installTask->InstallFromCollections();
                pageData->m_source->SetInstallState(install::MTPInstallState::Finished);
                nx::mtp::FinishInstallProgress();
                pageData->m_source->Disable();
            }
            catch (std::exception& e)
            {
                app::installer::OnFailed(shortFileName, e);
                pageData->m_state = State::Failed;
                pageData->m_source->Disable();
                nx::mtp::FinishInstallProgress();
                nx::mtp::DisableInstallMode();
                return;
            }

            app::installer::OnSuccess(1, shortFileName);
            pageData->m_state = State::Done;
        }
    }

    void MtpInstallPage::updateState()
    {
        char msg[256] = {};
        UsbState usbState = nx::usb::usbDeviceGetState();
        nx::usb::DeviceSpeed usbSpeed = nx::usb::usbDeviceGetSpeed();
        std::snprintf(msg, sizeof(msg), "usbds.message"_lang.c_str(),
                                        app::i18n::GetRelativeMsgAt("usbds.states", usbState).c_str(),
                                        app::i18n::GetRelativeMsgAt("usbds.speed", usbSpeed).c_str());
        std::string pageInfo = msg;
        pageInfo += " | " + "inst.target.info"_lang;
        pageInfo += pageData->m_targetStorage == nx::mtp::InstallProxyTargetStorage::SdCard ? "inst.target.opt0"_lang : "inst.target.opt1"_lang;
        app::facade::SendPageInfoText(pageInfo);
    }

    void MtpInstallPage::onCancel()
    {
        pageData->m_stop_source.request_stop();
        if (pageData->m_source)
        {
            pageData->m_source->Disable();
        }

        if (pageData->m_state == State::Connected || pageData->m_state == State::Progress)
        {
            return;
        }
        else
        {
            if (pageData->m_source)
            {
                pageData->m_source.reset();
            }
            pageData->m_currentFile.clear();
            pageData->m_state = State::None;
            pageData->m_anyButtonTriggered = false;
            nx::mtp::DisableInstallMode();
            nx::mtp::Cleanup();
            SceneJump(Scene::Main);
        }
    }

    void MtpInstallPage::onInitInstallMode()
    {
        nx::mtp::InitInstallMode(
            [this](const char* path){ return onInstallStart(path); },
            [this](const void *buf, size_t size){ return onInstallWrite(buf, size); },
            [this](){ return onInstallClose(); }
        );
        if (!this->infoImage->IsVisible())
        {
            this->infoImage->SetVisible(true);
        }
        pageData->m_stop_source = std::stop_source();
    }

    void MtpInstallPage::onInput(const u64 Down, const u64 Up, const u64 Held, const pu::ui::TouchPoint Pos)
    {
        static u64 tickB, tickY, tickX;
        constexpr u64 longPressButtons = HidNpadButton_B | HidNpadButton_X | HidNpadButton_Y;
        if (pageData->m_anyButtonTriggered)
        {
            if ((Held & longPressButtons) != 0)
            {
                return;
            }
            pageData->m_anyButtonTriggered = false;
        }

        if (IsLongPress(tickB, (Held & HidNpadButton_B) != 0, (Up & HidNpadButton_B) != 0, 1.0f))
        {
            pageData->m_anyButtonTriggered = true;
            onCancel();
        }

        if (pageData->m_state == State::Connected || pageData->m_state == State::Progress)
        {
            return;
        }

        if (IsLongPress(tickX, (Held & HidNpadButton_X) != 0, (Up & HidNpadButton_X) != 0, 1.0f))
        {
            pageData->m_anyButtonTriggered = true;
            app::facade::ShowDialog("common.help"_lang, "inst.mtp.help_desc"_lang, {"common.ok"_lang}, true, "information");
        }

        if (IsLongPress(tickY, (Held & HidNpadButton_Y) != 0, (Up & HidNpadButton_Y) != 0, 1.0f))
        {
            pageData->m_anyButtonTriggered = true;
            pageData->m_targetStorage = static_cast<nx::mtp::InstallProxyTargetStorage>(!pageData->m_targetStorage);
            nx::mtp::SetInstallProxyTargetStorage(pageData->m_targetStorage);
        }
    }

    bool MtpInstallPage::onInstallStart(const char* path)
    {
        for (;;)
        {
            {
                SCOPED_MUTEX(&pageData->m_mutex);

                if (pageData->m_state != State::Progress)
                {
                    break;
                }

                if (pageData->m_stop_source.get_token().stop_requested())
                {
                    return false;
                }
            }

            svcSleepThread(1e+6);
        }

        if (pageData->m_source)
        {
            for (;;)
            {
                {
                    SCOPED_MUTEX(&pageData->m_source->m_mutex);

                    if (!pageData->m_source->m_active &&
                         pageData->m_source->GetInstallState() != install::MTPInstallState::Progress)
                    {
                        break;
                    }

                    if (pageData->m_stop_source.get_token().stop_requested())
                    {
                        return false;
                    }
                }

                svcSleepThread(1e+6);
            }
        }

        SCOPED_MUTEX(&pageData->m_mutex);

        std::string fileName(path);
        std::string extPart = fileName.substr(fileName.size() - 3, 2);
        std::transform(extPart.begin(), extPart.end(), extPart.begin(), ::tolower);
        std::unique_ptr<nx::Content> content;
        if (extPart == "xc")
        {
            content = std::make_unique<nx::XCI>();
        }
        else
        {
            content = std::make_unique<nx::NSP>();
        }
        pageData->m_source = std::make_unique<install::MtpWorker>(std::move(content), pageData->m_stop_source.get_token());
        pageData->m_state = State::Connected;
        pageData->m_currentFile = fileName;

        this->infoImage->SetVisible(false);

        return true;
    }

    bool MtpInstallPage::onInstallWrite(const void* buf, size_t size)
    {
        return pageData->m_source->Push(buf, size);
    }

    void MtpInstallPage::onInstallClose()
    {
        pageData->m_source->Disable();
    }
}
