#include <string>
#include <thread>
#include <memory>
#include <malloc.h>
#include "install/InstallTask.hpp"
#include "install/LocalWorker.hpp"
#include "install/UsbWorker.hpp"
#include "nx/nsp.hpp"
#include "nx/xci.hpp"
#include "nx/error.hpp"
#include "nx/misc.hpp"
#include "nx/usb.hpp"
#include "util/config.hpp"
#include "util/util.hpp"
#include "util/i18n.hpp"
#include "installer.hpp"
#include "manager.hpp"
#include "facade.hpp"

#ifdef ENABLE_NET
#include <fcntl.h>
#include <jtjson.h>
#include <curl/curl.h>
#include "nx/network.hpp"
#include "install/HttpWorker.hpp"
#endif

namespace app::installer
{
    NX_INLINE void OnFailed(const std::string& msg, const std::exception& e)
    {
        LOG_DEBUG("Failed to install");
        LOG_DEBUG("%s", e.what());
        app::facade::SendInstallInfoText("inst.info_page.failed"_lang + msg);
        app::facade::SendInstallProgress(0);
        if (app::config::enableLightning)
        {
            app::manager::lightningStart();
        }
        std::thread audioThread(app::manager::playAudio, "/fail.wav");
        app::facade::ShowDialog("inst.info_page.failed"_lang + msg + "!", "inst.info_page.failed_desc"_lang + "\n\n" + (std::string)e.what(), {"common.ok"_lang}, true);
        audioThread.join();
        if (app::config::enableLightning)
        {
            app::manager::lightningStop();
        }
    }

    NX_INLINE void OnSuccess(const size_t count, const std::string& msg)
    {
        app::facade::SendInstallBarText("inst.info_page.complete"_lang);
        app::facade::SendInstallProgress(100);
        if (app::config::enableLightning)
        {
            app::manager::lightningStart();
        }
        std::thread audioThread(app::manager::playAudio, "/success.wav");
        if (count > 1)
        {
            app::facade::ShowDialog(std::to_string(count) + "inst.info_page.desc0"_lang, app::i18n::GetRandomMsg(), {"common.ok"_lang}, true);
        }
        else
        {
            app::facade::ShowDialog(msg + "inst.info_page.desc1"_lang, app::i18n::GetRandomMsg(), {"common.ok"_lang}, true);
        }
        audioThread.join();
        if (app::config::enableLightning)
        {
            app::manager::lightningStop();
        }
    }

    NX_INLINE std::vector<uint32_t> OnStart()
    {
        app::manager::initInstallServices();
        app::facade::ShowInstaller();

        std::vector<uint32_t> previousClockValues;
        if (app::config::overClock)
        {
            previousClockValues.push_back(nx::misc::SetClockSpeed(0, 1785000000));
            previousClockValues.push_back(nx::misc::SetClockSpeed(1, 76800000));
            previousClockValues.push_back(nx::misc::SetClockSpeed(2, 1600000000));
        }

        app::facade::SendInstallInfoText("inst.info_page.preparing"_lang);
        return previousClockValues;
    }

    NX_INLINE void OnExit(const std::vector<uint32_t>& previousClockValues)
    {
        if (previousClockValues.size() > 0)
        {
            nx::misc::SetClockSpeed(0, previousClockValues[0]);
            nx::misc::SetClockSpeed(1, previousClockValues[1]);
            nx::misc::SetClockSpeed(2, previousClockValues[2]);
        }

        LOG_DEBUG("Done");
        app::facade::SendInstallFinished();
        app::manager::deinitInstallServices();
    }

    namespace Local
    {
        void InstallFromFile(std::vector<std::filesystem::path> ourTitleList, NcmStorageId destStorageId, StorageSource storageSrc)
        {
            std::vector<uint32_t> previousClockValues = OnStart();

            bool fileInstalled = true;
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

                    std::string ext = ourTitleList[titleItr].extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    std::unique_ptr<nx::Content> content;
                    if (ext == ".xci" || ext == ".xcz")
                    {
                        content = std::make_unique<nx::XCI>();
                    }
                    else
                    {
                        content = std::make_unique<nx::NSP>();
                    }
                    std::unique_ptr<app::install::Worker> worker = std::make_unique<app::install::LocalWorker>(std::move(content), ourTitleList[titleItr]);
                    std::unique_ptr<app::InstallTask> installTask = std::make_unique<app::InstallTask>(destStorageId, app::config::ignoreReqVers, app::config::fixTicket, std::move(worker));

                    app::facade::SendInstallProgress(0);
                    installTask->Prepare();
                    installTask->InstallTicketCert();
                    installTask->Begin();
                }
            }
            catch (std::exception& e)
            {
                OnFailed(app::util::shortenString(ourTitleList[titleItr].filename().string(), 42, true), e);
                fileInstalled = false;
            }

            if (fileInstalled)
            {
                OnSuccess(ourTitleList.size(), app::util::shortenString(ourTitleList[0].filename().string(), 42, true));
                if (app::config::deletePrompt)
                {
                    if (ourTitleList.size() > 1)
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
                }
            }

            OnExit(previousClockValues);
        }
    }

    namespace Usb
    {
        int transferData(void* buf, size_t size, u64 timeout = 5000000000)
        {
            u8* tempBuffer = (u8*)memalign(0x1000, size);
            if (nx::usb::USBReadData(tempBuffer, size, timeout) == 0)
            {
                return 0;
            }
            memcpy(buf, tempBuffer, size);
            free(tempBuffer);
            return size;
        }

        std::vector<std::string> WaitingForFileList()
        {
            nx::usb::FileListHeader header;

            padConfigureInput(8, HidNpadStyleSet_NpadStandard);
            PadState pad;
            padInitializeAny(&pad);

            while (true)
            {
                app::facade::SendRenderRequest();
                if (transferData(&header, sizeof(nx::usb::FileListHeader), 500000000) != 0)
                {
                    break;
                }
                padUpdate(&pad);
                u64 kDown = padGetButtonsDown(&pad);

                if (kDown & HidNpadButton_B)
                {
                    return {};
                }
                if (kDown & HidNpadButton_X)
                {
                    app::facade::ShowDialog("inst.usb.help.title"_lang, "inst.usb.help.desc"_lang, {"common.ok"_lang}, true);
                }

                if (!nx::usb::usbDeviceIsConnected())
                {
                    return {};
                }
            }

            if (header.magic != 0x304C5554)
            {
                return {};
            }

            std::vector<std::string> titleNames;
            char* titleNameBuffer = (char*)memalign(0x1000, header.titleListSize + 1);
            memset(titleNameBuffer, 0, header.titleListSize + 1);

            nx::usb::USBReadData(titleNameBuffer, header.titleListSize, 10000000000);

            // Split the string up into individual title names
            std::stringstream titleNameStream(titleNameBuffer);
            std::string segment;
            while (std::getline(titleNameStream, segment, '\n'))
            {
                titleNames.push_back(segment);
            }
            free(titleNameBuffer);

            std::sort(titleNames.begin(), titleNames.end(), app::util::ignoreCaseCompare);

            return titleNames;
        }

        void InstallTitles(std::vector<std::string> ourTitleList, NcmStorageId destStorageId)
        {
            std::vector<uint32_t> previousClockValues = OnStart();

            std::vector<std::string> fileNames;
            for (long unsigned int i = 0; i < ourTitleList.size(); i++)
            {
                fileNames.push_back(app::util::shortenString(ourTitleList[i], 30, true));
            }

            bool fileInstalled = true;
            unsigned int fileItr;
            try
            {
                unsigned int titleCount = ourTitleList.size();
                for (fileItr = 0; fileItr < titleCount; fileItr++)
                {
                    if (titleCount > 1)
                    {
                        app::facade::SendPageInfoTextAndRender("inst.info_page.top_info0"_lang +
                                                               "(" + std::to_string(fileItr+1) + "/"  + std::to_string(titleCount) +
                                                               ") " + fileNames[fileItr] + "inst.usb.source_string"_lang);
                    }
                    else
                    {
                        app::facade::SendPageInfoTextAndRender("inst.info_page.top_info0"_lang + fileNames[fileItr] +
                                                               "inst.usb.source_string"_lang);
                    }

                    std::string extPart = ourTitleList[fileItr].substr(ourTitleList[fileItr].size() - 3, 2);
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
                    std::unique_ptr<app::install::Worker> worker = std::make_unique<app::install::UsbWorker>(std::move(content), ourTitleList[fileItr]);
                    std::unique_ptr<app::InstallTask> installTask = std::make_unique<app::InstallTask>(destStorageId, app::config::ignoreReqVers, app::config::fixTicket, std::move(worker));

                    app::facade::SendInstallProgress(0);
                    installTask->Prepare();
                    installTask->InstallTicketCert();
                    installTask->Begin();
                }
            }
            catch (std::exception& e)
            {
                OnFailed(fileNames[fileItr], e);
                fileInstalled = false;
            }

            if (fileInstalled)
            {
                nx::usb::USBCommandManager::SendFinishedCommand();
                OnSuccess(ourTitleList.size(), fileNames[0]);
            }

            nx::usb::usbDeviceReset();
            OnExit(previousClockValues);
        }
    }

#ifdef ENABLE_NET
    namespace Network
    {
        constexpr auto MAX_URL_SIZE = 1024;
        constexpr auto MAX_URLS = 256;
        constexpr auto REMOTE_INSTALL_PORT = 2000;
        static int m_serverSocket = 0;
        static int m_clientSocket = 0;

        void InitializeServerSocket() try
        {
            // Create a socket
            m_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

            if (m_serverSocket < -1)
            {
                THROW_FORMAT("Failed to create a server socket. Error code: %u\n", errno);
            }

            struct sockaddr_in server;
            server.sin_family = AF_INET;
            server.sin_port = htons(REMOTE_INSTALL_PORT);
            server.sin_addr.s_addr = htonl(INADDR_ANY);

            if (bind(m_serverSocket, (struct sockaddr*) &server, sizeof(server)) < 0)
            {
                THROW_FORMAT("Failed to bind server socket. Error code: %u\n", errno);
            }

            // Set as non-blocking
            fcntl(m_serverSocket, F_SETFL, fcntl(m_serverSocket, F_GETFL, 0) | O_NONBLOCK);

            if (listen(m_serverSocket, 5) < 0)
            {
                THROW_FORMAT("Failed to listen on server socket. Error code: %u\n", errno);
            }
        }
        catch (std::exception& e)
        {
            LOG_DEBUG("Failed to initialize server socket!\n");

            if (m_serverSocket != 0)
            {
                close(m_serverSocket);
                m_serverSocket = 0;
            }

            app::facade::ShowDialog("Failed to initialize server socket!", (std::string)e.what(), {"OK"}, true);
        }

        void Cleanup()
        {
            LOG_DEBUG("Network::Cleanup\n");
            if (m_clientSocket != 0)
            {
                close(m_clientSocket);
                m_clientSocket = 0;
            }
            curl_global_cleanup();
        }

        void PushExitCommand(std::string url)
        {
            LOG_DEBUG("Telling the server we're done installing\n");
            // Send 1 byte ack to close the server, OG tinfoil compatibility
            u8 ack = 0;
            nx::network::WaitSendNetworkData(m_clientSocket, &ack, sizeof(u8));
            // Send 'DROP' header so ns-usbloader knows we're done
            nx::network::NSULDrop(url);
        }

        std::vector<std::string> WaitingForNetworkData()
        {
            u64 freq = armGetSystemTickFreq();
            u64 startTime = armGetSystemTick();

            padConfigureInput(8, HidNpadStyleSet_NpadStandard);
            PadState pad;
            padInitializeAny(&pad);

            try
            {
                ASSERT_OK(curl_global_init(CURL_GLOBAL_ALL), "Curl failed to initialized");

                // Initialize the server socket if it hasn't already been
                if (m_serverSocket == 0)
                {
                    InitializeServerSocket();

                    if (m_serverSocket <= 0)
                    {
                        THROW_FORMAT("Server socket failed to initialize.\n");
                        close(m_serverSocket);
                        m_serverSocket = 0;
                    }
                }

                std::string ourIPAddress = nx::network::getIPAddress();
                app::facade::SendPageInfoText("inst.net.top_info1"_lang + ourIPAddress);
                app::facade::SendRenderRequest();
                LOG_DEBUG("%s %s\n", "Switch IP is ", ourIPAddress.c_str());
                LOG_DEBUG("%s\n", "Waiting for network");
                LOG_DEBUG("%s\n", "B to cancel");

                std::vector<std::string> urls;
                while (true)
                {
                    // If we don't update the UI occasionally the Switch basically crashes on this screen if you press the home button
                    u64 newTime = armGetSystemTick();
                    if (newTime - startTime >= freq * 0.25)
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
                        app::facade::ShowDialog("inst.net.help.title"_lang, "inst.net.help.desc"_lang, {"common.ok"_lang}, true);
                    }
                    if (kDown & HidNpadButton_X)
                    {
                        std::string url = nx::misc::OpenSoftwareKeyboard("inst.net.url.hint"_lang, app::config::httpIndexUrl, 500);
                        if (url == "")
                        {
                            url = "https://";
                        }

                        std::string response;
                        if (nx::network::formatUrlString(url) == "" || url == "https://" || url == "http://")
                        {
                            app::facade::ShowDialog("inst.net.url.warn"_lang,
                                                    "inst.net.url.invalid"_lang, {"common.ok"_lang}, false);
                            goto back_to_loop;
                        }
                        else
                        {
                            app::config::httpIndexUrl = url;
                            if (url[url.size() - 1] != '/')
                            {
                                url += '/';
                            }
                            response = nx::network::downloadToBuffer(url);
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
                        app::facade::ShowDialog("inst.net.index_error"_lang, "inst.net.index_error_info"_lang, {"common.ok"_lang}, true);
                    }
back_to_loop:
                    struct sockaddr_in client;
                    socklen_t clientLen = sizeof(client);

                    m_clientSocket = accept(m_serverSocket, (struct sockaddr*)&client, &clientLen);
                    if (m_clientSocket >= 0)
                    {
                        LOG_DEBUG("%s\n", "Server accepted");
                        u32 size = 0;
                        nx::network::WaitReceiveNetworkData(m_clientSocket, &size, sizeof(u32));
                        size = ntohl(size);

                        LOG_DEBUG("Received url buf size: 0x%x\n", size);
                        if (size > MAX_URL_SIZE * MAX_URLS)
                        {
                            THROW_FORMAT("URL size %x is too large!\n", size);
                        }

                        // Make sure the last string is null terminated
                        auto urlBuf = std::make_unique<char[]>(size + 1);
                        memset(urlBuf.get(), 0, size + 1);

                        nx::network::WaitReceiveNetworkData(m_clientSocket, urlBuf.get(), size);

                        // Split the string up into individual URLs
                        std::stringstream urlStream(urlBuf.get());
                        std::string segment;
                        while (std::getline(urlStream, segment, '\n'))
                        {
                            urls.push_back(segment);
                        }
                        std::sort(urls.begin(), urls.end(), app::util::ignoreCaseCompare);

                        break;
                    }
                    else if (errno != EAGAIN)
                    {
                        THROW_FORMAT("Failed to open client socket with code %u\n", errno);
                    }
                }

                return urls;
            }
            catch (std::runtime_error& e)
            {
                close(m_serverSocket);
                m_serverSocket = 0;
                LOG_DEBUG("Failed to perform remote install!\n");
                LOG_DEBUG("%s", e.what());
                app::facade::ShowDialog("inst.net.failed"_lang, (std::string)e.what(), {"common.ok"_lang}, true);
                return {};
            }
        }

        void InstallFromUrl(std::vector<std::string> ourUrlList, NcmStorageId destStorageId, std::string ourSource)
        {
            std::vector<uint32_t> previousClockValues = OnStart();

            std::vector<std::string> urlNames;
            for (long unsigned int i = 0; i < ourUrlList.size(); i++)
            {
                urlNames.push_back(app::util::shortenString(nx::network::formatUrlString(ourUrlList[i]), 28, true));
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
                        app::facade::SendPageInfoTextAndRender("inst.info_page.top_info0"_lang +
                                                               "(" + std::to_string(urlItr+1) + "/"  + std::to_string(urlCount) +
                                                               ") " + urlNames[urlItr] + ourSource);
                    }
                    else
                    {
                        app::facade::SendPageInfoTextAndRender("inst.info_page.top_info0"_lang + urlNames[urlItr] + ourSource);
                    }

                    std::unique_ptr<nx::Content> content;
                    if (nx::network::downloadToBuffer(ourUrlList[urlItr], 0x100, 0x103) == "HEAD")
                    {
                        content = std::make_unique<nx::XCI>();
                    }
                    else
                    {
                        content = std::make_unique<nx::NSP>();
                    }
                    std::unique_ptr<app::install::Worker> worker = std::make_unique<app::install::HttpWorker>(std::move(content), ourUrlList[urlItr]);
                    std::unique_ptr<app::InstallTask> installTask = std::make_unique<app::InstallTask>(destStorageId, app::config::ignoreReqVers, app::config::fixTicket, std::move(worker));

                    app::facade::SendInstallProgress(0);
                    installTask->Prepare();
                    installTask->InstallTicketCert();
                    installTask->Begin();
                }
            }
            catch (std::exception& e)
            {
                OnFailed(urlNames[urlItr], e);
                fileInstalled = false;
            }

            PushExitCommand(app::util::getUrlHost(ourUrlList[0]));
            Cleanup();

            if (fileInstalled)
            {
                OnSuccess(ourUrlList.size(), urlNames[0]);
            }

            OnExit(previousClockValues);
        }
    }
#endif
}
