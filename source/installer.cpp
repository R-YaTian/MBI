#include <string>
#include <thread>
#include <memory>
#include "install/install_nsp.hpp"
#include "install/install_xci.hpp"
#include "install/sdmc_xci.hpp"
#include "install/sdmc_nsp.hpp"
#include "nx/error.hpp"
#include "nx/misc.hpp"
#include "util/config.hpp"
#include "util/util.hpp"
#include "util/i18n.hpp"
#include "installer.hpp"
#include "manager.hpp"
#include "facade.hpp"

namespace app::installer
{
    namespace Local
    {
        void installFromFile(std::vector<std::filesystem::path> ourTitleList, int whereToInstall, StorageSource storageSrc)
        {
            app::manager::initInstallServices();
            app::facade::ShowInstaller();
            bool nspInstalled = true;
            NcmStorageId m_destStorageId = NcmStorageId_SdCard;
            if (whereToInstall)
            {
                m_destStorageId = NcmStorageId_BuiltInUser;
            }

            std::vector<uint32_t> previousClockValues;
            if (app::config::overClock)
            {
                previousClockValues.push_back(nx::misc::SetClockSpeed(0, 1785000000));
                previousClockValues.push_back(nx::misc::SetClockSpeed(1, 76800000));
                previousClockValues.push_back(nx::misc::SetClockSpeed(2, 1600000000));
            }

            unsigned int titleItr;
            try
            {
                unsigned int titleCount = ourTitleList.size();
                for (titleItr = 0; titleItr < titleCount; titleItr++)
                {
                    if (titleCount > 1)
                    {
                        app::facade::SendPageInfoTextAndRender("inst.info_page.top_info0"_lang +
                                                               "(" + std::to_string(titleItr+1) + "/"  + std::to_string(titleCount) +
                                                               ") " + app::util::shortenString(ourTitleList[titleItr].filename().string(), 30, true) +
                                                               (storageSrc == StorageSource::SD ? "inst.sd.source_string"_lang : "inst.hdd.source_string"_lang));
                    }
                    else
                    {
                        app::facade::SendPageInfoTextAndRender("inst.info_page.top_info0"_lang + app::util::shortenString(ourTitleList[titleItr].filename().string(), 30, true) +
                                                               (storageSrc == StorageSource::SD ? "inst.sd.source_string"_lang : "inst.hdd.source_string"_lang));
                    }

                    std::unique_ptr<app::install::Install> installTask;
                    if (ourTitleList[titleItr].extension() == ".xci" || ourTitleList[titleItr].extension() == ".xcz")
                    {
                        auto sdmcXCI = std::make_shared<app::install::xci::SDMCXCI>(ourTitleList[titleItr]);
                        installTask = std::make_unique<app::install::xci::XCIInstallTask>(m_destStorageId, app::config::ignoreReqVers, sdmcXCI);
                    }
                    else
                    {
                        auto sdmcNSP = std::make_shared<app::install::nsp::SDMCNSP>(ourTitleList[titleItr]);
                        installTask = std::make_unique<app::install::nsp::NSPInstall>(m_destStorageId, app::config::ignoreReqVers, sdmcNSP);
                    }

                    LOG_DEBUG("%s\n", "Preparing installation");
                    app::facade::SendInstallInfoText("inst.info_page.preparing"_lang);
                    app::facade::SendInstallProgress(0);
                    installTask->Prepare();
                    installTask->InstallTicketCert();
                    installTask->Begin();
                }
            }
            catch (std::exception& e)
            {
                LOG_DEBUG("Failed to install");
                LOG_DEBUG("%s", e.what());
                app::facade::SendInstallInfoText("inst.info_page.failed"_lang + app::util::shortenString(ourTitleList[titleItr].filename().string(), 42, true));
                app::facade::SendInstallProgress(0);
                if (app::config::enableLightning)
                {
                    app::manager::lightningStart();
                }
                std::thread audioThread(app::manager::playAudio, "/fail.wav");
                app::facade::ShowDialog("inst.info_page.failed"_lang + app::util::shortenString(ourTitleList[titleItr].filename().string(), 42, true) + "!", "inst.info_page.failed_desc"_lang + "\n\n" + (std::string)e.what(), {"common.ok"_lang}, true);
                audioThread.join();
                if (app::config::enableLightning)
                {
                    app::manager::lightningStop();
                }
                nspInstalled = false;
            }

            if (previousClockValues.size() > 0)
            {
                nx::misc::SetClockSpeed(0, previousClockValues[0]);
                nx::misc::SetClockSpeed(1, previousClockValues[1]);
                nx::misc::SetClockSpeed(2, previousClockValues[2]);
            }

            if(nspInstalled)
            {
                app::facade::SendInstallInfoText("inst.info_page.complete"_lang);
                app::facade::SendInstallProgress(100);
                if (app::config::enableLightning)
                {
                    app::manager::lightningStart();
                }
                std::thread audioThread(app::manager::playAudio, "/success.wav");
                if (ourTitleList.size() > 1)
                {
                    if (app::config::deletePrompt)
                    {
                        if(app::facade::ShowDialog(std::to_string(ourTitleList.size()) +
                                                   (storageSrc == StorageSource::SD ? "inst.sd.delete_info_multi"_lang : "inst.hdd.delete_info_multi"_lang),
                                                   "inst.sd.delete_desc"_lang, {"common.no"_lang, "common.yes"_lang}, false) == 1)
                        {
                            for (size_t i = 0; i < ourTitleList.size(); i++)
                            {
                                if (std::filesystem::exists(ourTitleList[i]))
                                {
                                    try { std::filesystem::remove(ourTitleList[i]); } catch (...) {};
                                }
                            }
                        }
                    }
                    else 
                    {
                        app::facade::ShowDialog(std::to_string(ourTitleList.size()) +
                                                "inst.info_page.desc0"_lang, app::i18n::GetRandomMsg(), {"common.ok"_lang}, true);
                    }
                }
                else
                {
                    if (app::config::deletePrompt)
                    {
                        if (app::facade::ShowDialog(app::util::shortenString(ourTitleList[0].filename().string(), 32, true) +
                                                    (storageSrc == StorageSource::SD ? "inst.sd.delete_info"_lang : "inst.hdd.delete_info"_lang),
                                                    "inst.sd.delete_desc"_lang, {"common.no"_lang, "common.yes"_lang}, false) == 1)
                        {
                            if (std::filesystem::exists(ourTitleList[0]))
                            {
                                try { std::filesystem::remove(ourTitleList[0]); } catch (...) {};
                            }
                        }
                    }
                    else
                    {
                        app::facade::ShowDialog(app::util::shortenString(ourTitleList[0].filename().string(), 42, true) +
                                                "inst.info_page.desc1"_lang, app::i18n::GetRandomMsg(), {"common.ok"_lang}, true);
                    }
                }
                audioThread.join();
                if (app::config::enableLightning)
                {
                    app::manager::lightningStop();
                }
            }

            LOG_DEBUG("Done");
            app::facade::SendInstallFinished();
            app::manager::deinitInstallServices();
        }
    }
}
