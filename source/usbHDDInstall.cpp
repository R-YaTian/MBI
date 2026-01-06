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

#include <cstring>
#include <sstream>
#include <filesystem>
#include <ctime>
#include <thread>
#include <memory>
#include "usbHDDInstall.hpp"
#include "install/install_nsp.hpp"
#include "install/install_xci.hpp"
#include "install/sdmc_xci.hpp"
#include "install/sdmc_nsp.hpp"
#include "nx/fs.hpp"
#include "util/file_util.hpp"
#include "util/title_util.hpp"
#include "util/error.hpp"
#include "util/config.hpp"
#include "util/util.hpp"
#include "util/lang.hpp"
#include "ui/MainApplication.hpp"
#include "ui/instPage.hpp"

namespace app::ui {
    extern MainApplication *mainApp;
}

namespace hddInstStuff {

    void installNspFromFile(std::vector<std::filesystem::path> ourTitleList, int whereToInstall)
    {
        app::util::initInstallServices();
        app::ui::instPage::loadInstallScreen();
        bool nspInstalled = true;
        NcmStorageId m_destStorageId = NcmStorageId_SdCard;

        if (whereToInstall) m_destStorageId = NcmStorageId_BuiltInUser;
        unsigned int titleItr;

        std::vector<int> previousClockValues;
        if (app::config::overClock) {
            previousClockValues.push_back(app::util::setClockSpeed(0, 1785000000)[0]);
            previousClockValues.push_back(app::util::setClockSpeed(1, 76800000)[0]);
            previousClockValues.push_back(app::util::setClockSpeed(2, 1600000000)[0]);
        }

        try
        {
            unsigned int titleCount = ourTitleList.size();
            for (titleItr = 0; titleItr < titleCount; titleItr++) {
                if (titleCount > 1) {
                    app::ui::instPage::setTopInstInfoText("inst.info_page.top_info0"_lang + "(" + std::to_string(titleItr+1) + "/"  + std::to_string(titleCount) + ") " + app::util::shortenString(ourTitleList[titleItr].filename().string(), 30, true) + "inst.hdd.source_string"_lang);
                } else {
                    app::ui::instPage::setTopInstInfoText("inst.info_page.top_info0"_lang + app::util::shortenString(ourTitleList[titleItr].filename().string(), 30, true) + "inst.hdd.source_string"_lang);
                }
                std::unique_ptr<app::install::Install> installTask;

                if (ourTitleList[titleItr].extension() == ".xci" || ourTitleList[titleItr].extension() == ".xcz") {
                    auto sdmcXCI = std::make_shared<app::install::xci::SDMCXCI>(ourTitleList[titleItr]);
                    installTask = std::make_unique<app::install::xci::XCIInstallTask>(m_destStorageId, app::config::ignoreReqVers, sdmcXCI);
                } else {
                    auto sdmcNSP = std::make_shared<app::install::nsp::SDMCNSP>(ourTitleList[titleItr]);
                    installTask = std::make_unique<app::install::nsp::NSPInstall>(m_destStorageId, app::config::ignoreReqVers, sdmcNSP);
                }

                LOG_DEBUG("%s\n", "Preparing installation");
                app::ui::instPage::setInstInfoText("inst.info_page.preparing"_lang);
                app::ui::instPage::setInstBarPerc(0);
                installTask->Prepare();
                installTask->InstallTicketCert();
                installTask->Begin();
            }
        }
        catch (std::exception& e)
        {
            LOG_DEBUG("Failed to install");
            LOG_DEBUG("%s", e.what());
            fprintf(stdout, "%s", e.what());
            app::ui::instPage::setInstInfoText("inst.info_page.failed"_lang + app::util::shortenString(ourTitleList[titleItr].filename().string(), 42, true));
            app::ui::instPage::setInstBarPerc(0);
            if (app::config::enableLightning) {
                app::util::lightningStart();
            }
            std::string audioPath = "romfs:/audio/fail.wav";
            if (std::filesystem::exists(app::config::appDir + "/fail.wav")) audioPath = app::config::appDir + "/fail.wav";
            std::thread audioThread(app::util::playAudio, audioPath);
            app::ui::mainApp->CreateShowDialog("inst.info_page.failed"_lang + app::util::shortenString(ourTitleList[titleItr].filename().string(), 42, true) + "!", "inst.info_page.failed_desc"_lang + "\n\n" + (std::string)e.what(), {"common.ok"_lang}, true);
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
            app::ui::instPage::setInstInfoText("inst.info_page.complete"_lang);
            app::ui::instPage::setInstBarPerc(100);
            if (app::config::enableLightning) {
                app::util::lightningStart();
            }
            std::string audioPath = "romfs:/audio/success.wav";
            if (std::filesystem::exists(app::config::appDir + "/success.wav")) audioPath = app::config::appDir + "/success.wav";
            std::thread audioThread(app::util::playAudio, audioPath);
            if (ourTitleList.size() > 1) {
                if (app::config::deletePrompt) {
                    if(app::ui::mainApp->CreateShowDialog(std::to_string(ourTitleList.size()) + "inst.hdd.delete_info_multi"_lang, "inst.hdd.delete_desc"_lang, {"common.no"_lang,"common.yes"_lang}, false) == 1) {
                        for (long unsigned int i = 0; i < ourTitleList.size(); i++) {
                            if (std::filesystem::exists(ourTitleList[i])) {
                                try {
                                    std::filesystem::remove(ourTitleList[i]);
                                } catch (...){ };
                            }
                        }
                    }
                } else app::ui::mainApp->CreateShowDialog(std::to_string(ourTitleList.size()) + "inst.info_page.desc0"_lang, Language::GetRandomMsg(), {"common.ok"_lang}, true);
            } else {
                if (app::config::deletePrompt) {
                    if(app::ui::mainApp->CreateShowDialog(app::util::shortenString(ourTitleList[0].filename().string(), 32, true) + "inst.hdd.delete_info"_lang, "inst.hdd.delete_desc"_lang, {"common.no"_lang,"common.yes"_lang}, false) == 1) {
                        if (std::filesystem::exists(ourTitleList[0])) {
                            try {
                                std::filesystem::remove(ourTitleList[0]);
                            } catch (...){ };
                        }
                    }
                } else app::ui::mainApp->CreateShowDialog(app::util::shortenString(ourTitleList[0].filename().string(), 42, true) + "inst.info_page.desc1"_lang, Language::GetRandomMsg(), {"common.ok"_lang}, true);
            }
            audioThread.join();
            if (app::config::enableLightning) {
                app::util::lightningStop();
            }
        }

        LOG_DEBUG("Done");
        app::ui::instPage::loadMainMenu();
        app::util::deinitInstallServices();
        return;
    }
}
