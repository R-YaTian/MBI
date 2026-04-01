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

#include "nx/network.hpp"
#include "nx/error.hpp"

#include <curl/curl.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <memory>
#include <sstream>
#include <cstring>

namespace nx::network
{
    constexpr auto MAX_URL_SIZE = 1024;
    constexpr auto MAX_URL_COUNT = 256;
    constexpr auto REMOTE_PORT = 2000;
    static int g_serverSocket = 0;
    static int g_clientSocket = 0;

    // HTTPHeader
    HTTPHeader::HTTPHeader(std::string url) :
        m_url(url)
    {
    }

    size_t HTTPHeader::ParseHTMLHeader(char* bytes, size_t size, size_t numItems, void* userData)
    {
        HTTPHeader* header = reinterpret_cast<HTTPHeader*>(userData);
        size_t numBytes = size * numItems;
        std::string line(bytes, numBytes);

        // Remove any newlines or carriage returns
        line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());

        // Split into key and value
        if (!line.empty())
        {
            auto keyEnd = line.find(": ");

            if (keyEnd != 0)
            {
                std::string key = line.substr(0, keyEnd);
                std::string value = line.substr(keyEnd + 2);

                // Make key lowercase
                std::transform(key.begin(), key.end(), key.begin(), ::tolower);
                header->m_values[key] = value;
            }
        }

        return numBytes;
    }

    void HTTPHeader::PerformRequest()
    {
        // We don't want any existing values to get mixed up with this request
        m_values.clear();

        CURL* curl = curl_easy_init();
        CURLcode rc = (CURLcode)0;

        if (!curl)
        {
            THROW_FORMAT("Failed to initialize curl\n");
        }

        curl_easy_setopt(curl, CURLOPT_URL, m_url.c_str());
        curl_easy_setopt(curl, CURLOPT_NOBODY, true);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mohsaua-Buoh-Installer");
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, this);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &nx::network::HTTPHeader::ParseHTMLHeader);

        rc = curl_easy_perform(curl);
        if (rc != CURLE_OK)
        {
            THROW_FORMAT("Failed to retrieve HTTP Header: %s\n", curl_easy_strerror(rc));
        }

        u64 httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        curl_easy_cleanup(curl);

        if (httpCode != 200 && httpCode != 204)
        {
            THROW_FORMAT("Unexpected HTTP response code when retrieving header: %lu\n", httpCode);
        }
    }

    bool HTTPHeader::HasValue(std::string key)
    {
        return m_values.count(key);
    }

    std::string HTTPHeader::GetValue(std::string key)
    {
        return m_values[key];
    }
    // End HTTPHeader

    // HTTPDownload
    HTTPDownload::HTTPDownload(std::string url) :
        m_url(url), m_header(url)
    {
        // The header won't be populated until we do this
        m_header.PerformRequest();

        if (m_header.HasValue("accept-ranges"))
        {
            m_rangesSupported = m_header.GetValue("accept-ranges") == "bytes";
        }
        else
        {
            CURL* curl = curl_easy_init();
            CURLcode rc = (CURLcode)0;

            if (!curl)
            {
                THROW_FORMAT("Failed to initialize curl\n");
            }

            curl_easy_setopt(curl, CURLOPT_URL, m_url.c_str());
            curl_easy_setopt(curl, CURLOPT_NOBODY, true);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mohsaua-Buoh-Installer");
            curl_easy_setopt(curl, CURLOPT_RANGE, "0-0");

            rc = curl_easy_perform(curl);
            if (rc != CURLE_OK)
            {
                THROW_FORMAT("Failed to retrieve HTTP Header: %s\n", curl_easy_strerror(rc));
            }

            u64 httpCode = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
            curl_easy_cleanup(curl);

            m_rangesSupported = httpCode == 206;
        }
    }

    size_t HTTPDownload::ParseHTMLData(char* bytes, size_t size, size_t numItems, void* userData)
    {
        auto streamFunc = *reinterpret_cast<std::function<size_t (u8* bytes, size_t size)>*>(userData);
        size_t numBytes = size * numItems;

        if (streamFunc != nullptr)
            return streamFunc((u8*)bytes, numBytes);

        return numBytes;
    }

    void HTTPDownload::BufferDataRange(void* buffer, size_t offset, size_t size, std::function<void (size_t sizeRead)> progressFunc)
    {
        size_t sizeRead = 0;

        auto streamFunc = [&](u8* streamBuf, size_t streamBufSize) -> size_t
        {
            if (sizeRead + streamBufSize > size)
            {
                LOG_DEBUG("New read size 0x%lx would exceed total expected size 0x%lx\n", sizeRead + streamBufSize, size);
                return 0;
            }

            if (progressFunc != nullptr)
                progressFunc(sizeRead);

            memcpy(reinterpret_cast<u8*>(buffer) + sizeRead, streamBuf, streamBufSize);
            sizeRead += streamBufSize;
            return streamBufSize;
        };

        this->StreamDataRange(offset, size, streamFunc);
    }

    int HTTPDownload::StreamDataRange(size_t offset, size_t size, std::function<size_t (u8* bytes, size_t size)> streamFunc)
    {
        if (!m_rangesSupported)
        {
            THROW_FORMAT("Attempted range request when ranges aren't supported!\n");
        }

        auto writeDataFunc = streamFunc;

        CURL* curl = curl_easy_init();
        CURLcode rc = (CURLcode)0;

        if (!curl)
        {
            THROW_FORMAT("Failed to initialize curl\n");
        }

        std::stringstream ss;
        ss << offset << "-" << (offset + size - 1);
        auto range = ss.str();

        curl_easy_setopt(curl, CURLOPT_URL, m_url.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mohsaua-Buoh-Installer");
        curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &writeDataFunc);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &nx::network::HTTPDownload::ParseHTMLData);

        rc = curl_easy_perform(curl);

        u64 httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        curl_easy_cleanup(curl);

        if (httpCode != 206 || rc != CURLE_OK) return 1;
        return 0;
    }
    // End HTTPDownload

    static size_t WaitReceiveNetworkData(int sockfd, void* buf, size_t len)
    {
        int ret = 0;
        size_t read = 0;

        while ((((ret = recv(sockfd, (u8*)buf + read, len - read, 0)) > 0 && (read += ret) < len) || errno == EAGAIN))
        {
            errno = 0;
        }

        return read;
    }

    static size_t WaitSendNetworkData(int sockfd, void* buf, size_t len)
    {
        int ret = 0;
        size_t written = 0;

        while (written < len)
        {
            errno = 0;
            ret = send(sockfd, (u8*)buf + written, len - written, 0);

            if (ret < 0)
            {
                // If error
                if (errno == EWOULDBLOCK || errno == EAGAIN) // Is it because other side is busy?
                {
                    sleep(5);
                    continue;
                }
                break; // No? Die.
            }

            written += ret;
        }

        return written;
    }

    static size_t WriteDataBuffer(char *ptr, size_t size, size_t nmemb, void *userdata)
    {
        std::ostringstream *stream = (std::ostringstream*)userdata;
        size_t count = size * nmemb;
        stream->write(ptr, count);
        return count;
    }

    static void NSULDrop(const std::string& url)
    {
        CURL* curl = curl_easy_init();

        if (!curl)
        {
            THROW_FORMAT("Failed to initialize curl\n");
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DROP");
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mohsaua-Buoh-Installer");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 50);

        curl_easy_perform(curl); // ignore returning value

        curl_easy_cleanup(curl);
    }

    std::string DownloadToBuffer(const std::string& url, int firstRange, int secondRange, long timeout)
    {
        CURL *curl_handle;
        CURLcode result;
        std::ostringstream stream;

        curl_handle = curl_easy_init();

        curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "Mohsaua-Buoh-Installer");
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT_MS, timeout);
        curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT_MS, timeout);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteDataBuffer);
        if (firstRange && secondRange)
        {
            const char * ourRange = (std::to_string(firstRange) + "-" + std::to_string(secondRange)).c_str();
            curl_easy_setopt(curl_handle, CURLOPT_RANGE, ourRange);
        }

        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &stream);
        result = curl_easy_perform(curl_handle);

        curl_easy_cleanup(curl_handle);

        if (result == CURLE_OK)
        {
            return stream.str();
        }
        else
        {
            LOG_DEBUG(curl_easy_strerror(result));
            return "";
        }
    }

    std::string FormatUrlString(const std::string& url)
    {
        std::stringstream ourStream(url);
        std::string segment;
        std::vector<std::string> seglist;

        while(std::getline(ourStream, segment, '/'))
        {
            seglist.push_back(segment);
        }

        CURL *curl = curl_easy_init();
        int outlength;
        std::string finalString = curl_easy_unescape(curl, seglist[seglist.size() - 1].c_str(), seglist[seglist.size() - 1].length(), &outlength);
        curl_easy_cleanup(curl);

        return finalString;
    }

    std::string GetIPAddress()
    {
        struct in_addr addr = {(in_addr_t) gethostid()};
        return inet_ntoa(addr);
    }

    static void InitializeServerSocket() try
    {
        // Create a socket
        g_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

        if (g_serverSocket < -1)
        {
            THROW_FORMAT("Failed to create a server socket. Error code: %u\n", errno);
        }

        struct sockaddr_in server;
        server.sin_family = AF_INET;
        server.sin_port = htons(REMOTE_PORT);
        server.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(g_serverSocket, (struct sockaddr*) &server, sizeof(server)) < 0)
        {
            THROW_FORMAT("Failed to bind server socket. Error code: %u\n", errno);
        }

        // Set as non-blocking
        fcntl(g_serverSocket, F_SETFL, fcntl(g_serverSocket, F_GETFL, 0) | O_NONBLOCK);

        if (listen(g_serverSocket, 5) < 0)
        {
            THROW_FORMAT("Failed to listen on server socket. Error code: %u\n", errno);
        }
    }
    catch (std::exception& e)
    {
        LOG_DEBUG("Failed to initialize server socket!\n");
        THROW_FORMAT("Failed to initialize server socket:\n%s", e.what());
    }

    void Initialize()
    {
        // Initialize the server socket if it hasn't already been
        if (g_serverSocket == 0)
        {
            InitializeServerSocket();
        }
    }

    void Finalize()
    {
        LOG_DEBUG("nx::network::Finalize\n");
        if (g_clientSocket != 0)
        {
            close(g_clientSocket);
            g_clientSocket = 0;
        }
        if (g_serverSocket != 0)
        {
            close(g_serverSocket);
            g_serverSocket = 0;
        }
    }

    std::string ReceiveRemoteString()
    {
        struct sockaddr_in client;
        socklen_t clientLen = sizeof(client);

        g_clientSocket = accept(g_serverSocket, (struct sockaddr*)&client, &clientLen);
        if (g_clientSocket >= 0)
        {
            LOG_DEBUG("%s\n", "Server accepted");
            u32 size = 0;
            WaitReceiveNetworkData(g_clientSocket, &size, sizeof(u32));
            size = ntohl(size);

            LOG_DEBUG("Received url buf size: 0x%x\n", size);
            if (size > MAX_URL_SIZE * MAX_URL_COUNT)
            {
                THROW_FORMAT("URL size %x is too large!\n", size);
            }

            // Make sure the last string is null terminated
            auto urlBuf = std::make_unique<char[]>(size + 1);
            memset(urlBuf.get(), 0, size + 1);
            WaitReceiveNetworkData(g_clientSocket, urlBuf.get(), size);

            return std::string(urlBuf.get());
        }
        else if (errno != EAGAIN)
        {
            THROW_FORMAT("Failed to open client socket with code %u\n", errno);
        }
    
        return "";
    }

    void PushExitCommand(const std::string& url)
    {
        LOG_DEBUG("Telling the server we're done\n");
        // Send 1 byte ack to close the server, OG tinfoil compatibility
        u8 ack = 0;
        WaitSendNetworkData(g_clientSocket, &ack, sizeof(u8));

        std::string urlHost = url;
        std::string::size_type pos = urlHost.find('/');
        if (pos != std::string::npos)
        {
            urlHost = urlHost.substr(0, pos);
        }
        // Send 'DROP' header so ns-usbloader knows we're done
        NSULDrop(urlHost);
    }
}
