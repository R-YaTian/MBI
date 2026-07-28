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

#pragma once

#include <switch/types.h>

#include <functional>
#include <map>
#include <string>

namespace nx::network
{
    class HTTPHeader
    {
        private:
            std::string m_url;
            std::map<std::string, std::string> m_values;

            static size_t ParseHTMLHeader(char* bytes, size_t size, size_t numItems, void* userData);

        public:
            HTTPHeader(std::string url);

            void PerformRequest();

            bool HasValue(std::string key);
            std::string GetValue(std::string key);
    };

    class HTTPDownload
    {
        private:
            std::string m_url;
            HTTPHeader m_header;
            bool m_rangesSupported = false;

            static size_t ParseHTMLData(char* bytes, size_t size, size_t numItems, void* userData);

        public:
            HTTPDownload(std::string url);

            void BufferDataRange(void* buffer, size_t offset, size_t size, std::function<void (size_t sizeRead)> progressFunc);
            bool StreamDataRange(size_t offset, size_t size, std::function<size_t (u8* bytes, size_t size)> streamFunc);
    };

    std::string DownloadToBuffer(const std::string& url, int firstRange = -1, int secondRange = -1, long timeout = 5000);
    std::string FormatUrlString(const std::string& url);
    std::string GetIPAddress();

    void InitializeServerSocket();
    void Finalize();
    std::string ReceiveRemoteString();
    void PushExitCommand(const std::string& url);
}
