/*
Copyright (c) 2017-2018 Adubbz

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <string>
#include <thread>
#include <malloc.h>
#include "usbInstall.hpp"
#include "install/usb_nsp.hpp"
#include "install/install_nsp.hpp"
#include "install/usb_xci.hpp"
#include "install/install_xci.hpp"
#include "nx/error.hpp"
#include "util/usb_util.hpp"
#include "util/util.hpp"
#include "util/config.hpp"
#include "util/lang.hpp"
#include "ui/MainApplication.hpp"
#include "ui/usbInstPage.hpp"
#include "ui/instPage.hpp"

namespace app::ui {
    extern MainApplication *mainApp;
}

namespace usbInstStuff {
    struct TUSHeader
    {
        u32 magic; // TUL0 (Tinfoil Usb List 0)
        u32 titleListSize;
        u64 padding;
    } NX_PACKED;

    int bufferData(void* buf, size_t size, u64 timeout = 5000000000)
    {
        u8* tempBuffer = (u8*)memalign(0x1000, size);
        if (app::util::USBRead(tempBuffer, size, timeout) == 0) return 0;
        memcpy(buf, tempBuffer, size);
        free(tempBuffer);
        return size;
    }

    std::vector<std::string> OnSelected() {
        TUSHeader header;

        padConfigureInput(8, HidNpadStyleSet_NpadStandard);
        PadState pad;
        padInitializeAny(&pad);

        while(true) {
            if (bufferData(&header, sizeof(TUSHeader), 500000000) != 0) break;
            padUpdate(&pad);
            u64 kDown = padGetButtonsDown(&pad);
            
            if (kDown & HidNpadButton_B) return {};
            if (kDown & HidNpadButton_X) app::ui::mainApp->CreateShowDialog("inst.usb.help.title"_lang, "inst.usb.help.desc"_lang, {"common.ok"_lang}, true);
            if (!nx::usb::usbDeviceIsConnected()) return {};
        }

        if (header.magic != 0x304C5554) return {};

        std::vector<std::string> titleNames;
        char* titleNameBuffer = (char*)memalign(0x1000, header.titleListSize + 1);
        memset(titleNameBuffer, 0, header.titleListSize + 1);

        app::util::USBRead(titleNameBuffer, header.titleListSize, 10000000000);

        // Split the string up into individual title names
        std::stringstream titleNamesStream(titleNameBuffer);
        std::string segment;
        while (std::getline(titleNamesStream, segment, '\n')) titleNames.push_back(segment);
        free(titleNameBuffer);
        std::sort(titleNames.begin(), titleNames.end(), app::util::ignoreCaseCompare);

        return titleNames;
    }

    void installTitleUsb(std::vector<std::string> ourTitleList, int ourStorage)
    {
        app::util::initInstallServices();
        app::ui::instPage::loadInstallScreen();
        bool nspInstalled = true;
        NcmStorageId m_destStorageId = NcmStorageId_SdCard;

        if (ourStorage) m_destStorageId = NcmStorageId_BuiltInUser;
        unsigned int fileItr;

        std::vector<std::string> fileNames;
        for (long unsigned int i = 0; i < ourTitleList.size(); i++) {
            fileNames.push_back(app::util::shortenString(app::util::formatUrlString(ourTitleList[i]), 30, true));
        }

        std::vector<int> previousClockValues;
        if (app::config::overClock) {
            previousClockValues.push_back(app::util::setClockSpeed(0, 1785000000)[0]);
            previousClockValues.push_back(app::util::setClockSpeed(1, 76800000)[0]);
            previousClockValues.push_back(app::util::setClockSpeed(2, 1600000000)[0]);
        }

        try {
            unsigned int titleCount = ourTitleList.size();
            for (fileItr = 0; fileItr < titleCount; fileItr++) {
                if (titleCount > 1) {
                    app::ui::instPage::setTopInstInfoText("inst.info_page.top_info0"_lang + "(" + std::to_string(fileItr+1) + "/"  + std::to_string(titleCount) + ") " + fileNames[fileItr] + "inst.usb.source_string"_lang);
                } else {
                    app::ui::instPage::setTopInstInfoText("inst.info_page.top_info0"_lang + fileNames[fileItr] + "inst.usb.source_string"_lang);
                }
                std::unique_ptr<app::install::Install> installTask;

                if (ourTitleList[fileItr].compare(ourTitleList[fileItr].size() - 3, 2, "xc") == 0) {
                    auto usbXCI = std::make_shared<app::install::xci::USBXCI>(ourTitleList[fileItr]);
                    installTask = std::make_unique<app::install::xci::XCIInstallTask>(m_destStorageId, app::config::ignoreReqVers, usbXCI);
                } else {
                    auto usbNSP = std::make_shared<app::install::nsp::USBNSP>(ourTitleList[fileItr]);
                    installTask = std::make_unique<app::install::nsp::NSPInstall>(m_destStorageId, app::config::ignoreReqVers, usbNSP);
                }

                LOG_DEBUG("%s\n", "Preparing installation");
                app::ui::instPage::setInstInfoText("inst.info_page.preparing"_lang);
                app::ui::instPage::setInstBarPerc(0);
                installTask->Prepare();
                installTask->InstallTicketCert();
                installTask->Begin();
            }
        }
        catch (std::exception& e) {
            LOG_DEBUG("Failed to install");
            LOG_DEBUG("%s", e.what());
            fprintf(stdout, "%s", e.what());
            app::ui::instPage::setInstInfoText("inst.info_page.failed"_lang + fileNames[fileItr]);
            app::ui::instPage::setInstBarPerc(0);
            if (app::config::enableLightning) {
                app::util::lightningStart();
            }
            std::string audioPath = "romfs:/audio/fail.wav";
            if (std::filesystem::exists(app::config::appDir + "/fail.wav")) audioPath = app::config::appDir + "/fail.wav";
            std::thread audioThread(app::util::playAudio, audioPath);
            app::ui::mainApp->CreateShowDialog("inst.info_page.failed"_lang + fileNames[fileItr] + "!", "inst.info_page.failed_desc"_lang + "\n\n" + (std::string)e.what(), {"common.ok"_lang}, true);
            audioThread.join();
            if (app::config::enableLightning) {
                app::util::lightningStop();
            }
            nspInstalled = false;
        }

        if (previousClockValues.size() > 0) {
            app::util::setClockSpeed(0, previousClockValues[0]);
            app::util::setClockSpeed(1, previousClockValues[1]);
            app::util::setClockSpeed(2, previousClockValues[2]);
        }

        if(nspInstalled) {
            app::util::USBCmdManager::SendExitCmd();
            app::ui::instPage::setInstInfoText("inst.info_page.complete"_lang);
            app::ui::instPage::setInstBarPerc(100);
            if (app::config::enableLightning) {
                app::util::lightningStart();
            }
            std::string audioPath = "romfs:/audio/success.wav";
            if (std::filesystem::exists(app::config::appDir + "/success.wav")) audioPath = app::config::appDir + "/success.wav";
            std::thread audioThread(app::util::playAudio, audioPath);
            if (ourTitleList.size() > 1) app::ui::mainApp->CreateShowDialog(std::to_string(ourTitleList.size()) + "inst.info_page.desc0"_lang, Language::GetRandomMsg(), {"common.ok"_lang}, true);
            else app::ui::mainApp->CreateShowDialog(fileNames[0] + "inst.info_page.desc1"_lang, Language::GetRandomMsg(), {"common.ok"_lang}, true);
            audioThread.join();
            if (app::config::enableLightning) {
                app::util::lightningStop();
            }
        }

        LOG_DEBUG("Done");
        nx::usb::usbDeviceReset();
        app::ui::instPage::loadMainMenu();
        app::util::deinitInstallServices();
        return;
    }
}