#include <string>
#include <memory>
#include "install/InstallTask.hpp"
#include "nx/nsp.hpp"
#include "nx/xci.hpp"
#include "nx/error.hpp"
#include "nx/misc.hpp"
#include "util/config.hpp"
#include "util/util.hpp"
#include "util/i18n.hpp"
#include "installer.hpp"
#include "facade.hpp"

#ifdef ENABLE_NET
#include <jtjson.h>
#include "nx/network.hpp"
#include "install/HttpWorker.hpp"
#endif

namespace app::installer
{
#ifdef ENABLE_NET
    namespace Network
    {
        std::vector<std::string> WaitingForNetworkData()
        {
            u64 freq = armGetSystemTickFreq();
            u64 startTime = armGetSystemTick();

            padConfigureInput(8, HidNpadStyleSet_NpadStandard);
            PadState pad;
            padInitializeAny(&pad);

            try
            {
                nx::network::InitializeServerSocket();

                std::string ourIPAddress = nx::network::GetIPAddress();
                app::facade::SendPageInfoText("inst.net.top_info"_lang + ourIPAddress);
                app::facade::SendRenderRequest();
                LOG_DEBUG("%s %s\n", "Switch IP is ", ourIPAddress.c_str());
                LOG_DEBUG("%s\n", "Waiting for network");
                LOG_DEBUG("%s\n", "B to cancel");

                std::vector<std::string> urls;
                while (true)
                {
                    // If we don't update the UI occasionally the Switch basically crashes on this screen if you press the home button
                    u64 newTime = armGetSystemTick();
                    if (newTime - startTime >= freq * 0.01)
                    {
                        startTime = newTime;
                        app::facade::SendRenderRequest();
                    }

                    // Break on input pressed
                    padUpdate(&pad);
                    u64 kDown = padGetButtonsDown(&pad);

                    if (kDown & HidNpadButton_B)
                    {
                        break;
                    }
                    if (kDown & HidNpadButton_Y)
                    {
                        return {"supplyUrl"};
                    }
                    if (kDown & HidNpadButton_Minus)
                    {
                        app::facade::ShowDialog("common.help"_lang, "inst.net.help_desc"_lang, {"common.ok"_lang}, true, "information");
                    }
                    if (kDown & HidNpadButton_X)
                    {
                        std::string url = nx::misc::OpenSoftwareKeyboard("inst.net.url.hint"_lang, app::config::httpIndexUrl, 500);
                        if (url == "")
                        {
                            url = "https://";
                        }

                        std::string response;
                        if (nx::network::FormatUrlString(url) == "" || url == "https://" || url == "http://")
                        {
                            app::facade::ShowDialog("common.warning"_lang,
                                                    "inst.net.url.invalid"_lang, {"common.ok"_lang}, false, "warning");
                            goto back_to_loop;
                        }
                        else
                        {
                            app::config::httpIndexUrl = url;
                            app::config::SaveSettings();
                            if (url[url.size() - 1] != '/')
                            {
                                url += '/';
                            }
                            response = nx::network::DownloadToBuffer(url);
                        }

                        if (!response.empty())
                        {
                            if (response[0] == '{')
                            {
                                jt::Json parse = jt::Json::parse(response);

                                if (parse.contains("files") && parse["files"].is_array())
                                {
                                    const auto& fileArray = parse["files"];
                                    for (const auto& curFile : fileArray)
                                    {
                                        if (curFile.contains("url"))
                                        {
                                            urls.push_back(curFile["url"].get<std::string>());
                                        }
                                        else
                                        {
                                            continue;
                                        }
                                    }
                                    return urls;
                                }
                                else
                                {
                                    LOG_DEBUG("Failed to parse JSON\n");
                                }
                            }
                            else if (response[0] == '<')
                            {
                                std::size_t index = 0;
                                while (index < response.size())
                                {
                                    std::string link;
                                    auto found = response.find("href=\"", index);
                                    if (found == std::string::npos)
                                    {
                                        break;
                                    }
                                    index = found + 6;
                                    while (index < response.size())
                                    {
                                        if (response[index] == '"')
                                        {
                                            if (link.find("../") == std::string::npos)
                                            {
                                                if (link.find(".nsp") != std::string::npos ||
                                                    link.find(".nsz") != std::string::npos ||
                                                    link.find(".xci") != std::string::npos ||
                                                    link.find(".xcz") != std::string::npos)
                                                {
                                                    urls.push_back(url + link);
                                                }
                                            }
                                            break;
                                        }
                                        link += response[index++];
                                    }
                                }
                                if (urls.size() > 0)
                                {
                                    return urls;
                                }
                                LOG_DEBUG("Failed to parse data from HTML\n");
                            }
                        }
                        else
                        {
                            LOG_DEBUG("Failed to fetch file list\n");
                        }
                        app::facade::ShowDialog("inst.net.index_error"_lang, "inst.net.index_error_info"_lang, {"common.ok"_lang}, true, "warning");
                    }
back_to_loop:
                    std::string remoteData = nx::network::ReceiveRemoteString();
                    if (remoteData != "")
                    {
                        // Split the string up into individual URLs
                        std::stringstream urlStream(remoteData);
                        std::string segment;
                        while (std::getline(urlStream, segment, '\n'))
                        {
                            urls.push_back(segment);
                        }
                        std::sort(urls.begin(), urls.end(), app::util::IgnoreCaseCompare);

                        break;
                    }
                }

                return urls;
            }
            catch (std::runtime_error& e)
            {
                LOG_DEBUG("%s", e.what());
                app::facade::ShowDialog("inst.net.failed"_lang, (std::string)e.what(), {"common.ok"_lang}, true, "error");
                return {};
            }
        }

        void InstallFromUrl(std::vector<std::string> ourUrlList, NcmStorageId destStorageId, std::string ourSource)
        {
            facade::ShowInstaller(ourSource);

            std::vector<std::string> urlNames;
            for (size_t i = 0; i < ourUrlList.size(); i++)
            {
                urlNames.push_back(nx::misc::ShortenString(nx::network::FormatUrlString(ourUrlList[i]), 42, 4));
            }

            bool fileInstalled = true;
            unsigned int urlItr;
            try
            {
                unsigned int urlCount = ourUrlList.size();
                for (urlItr = 0; urlItr < urlCount; urlItr++)
                {
                    LOG_DEBUG("%s %s\n", "Install request from", ourUrlList[urlItr].c_str());
                    if (urlCount > 1)
                    {
                        app::facade::SendPageInfoTextAndRender("inst.info_page.installing"_lang +
                                                               "(" + std::to_string(urlItr + 1) + "/"  + std::to_string(urlCount) +
                                                               ") " + urlNames[urlItr]);
                    }
                    else
                    {
                        app::facade::SendPageInfoTextAndRender("inst.info_page.installing"_lang + urlNames[urlItr]);
                    }

                    std::unique_ptr<nx::Content> content;
                    std::string extPart = ourUrlList[urlItr].substr(ourUrlList[urlItr].size() - 3, 2);
                    std::transform(extPart.begin(), extPart.end(), extPart.begin(), ::tolower);
                    if (extPart == "xc")
                    {
                        content = std::make_unique<nx::XCI>();
                    }
                    else
                    {
                        content = std::make_unique<nx::NSP>();
                    }
                    std::unique_ptr<app::install::Worker> worker = std::make_unique<app::install::HttpWorker>(std::move(content), ourUrlList[urlItr]);
                    std::unique_ptr<app::InstallTask> installTask = std::make_unique<app::InstallTask>(destStorageId, app::config::overClock, app::config::ignoreReqVers, app::config::fixTicket, app::config::skipBase, worker.get());

                    app::facade::SendInstallProgress(0);
                    installTask->Prepare();
                    installTask->InstallTicketCert();
                    installTask->Begin();
                }
            }
            catch (std::exception& e)
            {
                facade::NotifyInstallFailed(e, urlNames[urlItr]);
                fileInstalled = false;
            }

            nx::network::PushExitCommand(ourUrlList[0]);
            nx::network::Finalize();

            if (fileInstalled)
            {
                facade::NotifyInstallSuccess(ourUrlList.size(), urlNames[0]);
            }

            app::facade::SendInstallFinished();
        }
    }
#endif
}
