#include "install/HttpWorker.hpp"
#include "util/i18n.hpp"
#include "nx/error.hpp"

namespace app::install
{
    HttpWorker::HttpWorker(std::unique_ptr<nx::Content> content, const std::string &url)
        : Worker(std::move(content)), m_download(url)
    {
        RetrieveHeader();
    }

    HttpWorker::~HttpWorker() = default;

    void HttpWorker::ReadThread(void* in)
    {
        ThreadData* args = static_cast<ThreadData*>(in);

        auto streamFunc = [&](u8* streamBuf, size_t streamBufSize) -> size_t
        {
            while (true)
            {
                if (args->bufferedPlaceholderWriter->CanAppendData(streamBufSize))
                {
                    break;
                }
            }

            args->bufferedPlaceholderWriter->AppendData(streamBuf, streamBufSize);
            return streamBufSize;
        };

        if (static_cast<nx::network::HTTPDownload*>(args->in)->StreamDataRange(args->dataOffset, args->dataSize, streamFunc) == false)
        {
            stopThreads = true;
            if (args->errorMessage != nullptr)
            {
                *args->errorMessage = "inst.net.transfer_interput"_lang;
            }
        }
    }

    void HttpWorker::StreamToPlaceholder(std::shared_ptr<nx::ncm::ContentStorage>& contentStorage, NcmContentId ncaId, nx::nca::NcaHeader* header)
    {
        this->WriteToPlaceholderBuffered(contentStorage, ncaId, (void *)&m_download, header, appletGetAppletType() == AppletType_LibraryApplet ? 8 : 128);
    }

    void HttpWorker::BufferData(void* buf, off_t offset, size_t size)
    {
        m_download.BufferDataRange(buf, offset, size, nullptr);
    }
}
