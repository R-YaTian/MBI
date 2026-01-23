#include "install/LocalWorker.hpp"
#include "util/i18n.hpp"
#include "nx/NcaWriter.hpp"
#include "nx/error.hpp"
#include "facade.hpp"

namespace app::install
{
    LocalWorker::LocalWorker(std::unique_ptr<nx::Content> content, const std::string &path)
        : Worker(std::move(content))
    {
        m_file = fopen(path.c_str(), "rb");
        if (!m_file)
        {
            THROW_FORMAT("can't open file at %s\n", path.c_str());
        }
        RetrieveHeader();
    }

    LocalWorker::~LocalWorker()
    {
        if (m_file)
        {
            fclose(m_file);
        }
    }

    void LocalWorker::StreamToPlaceholder(std::shared_ptr<nx::ncm::ContentStorage>& contentStorage, NcmContentId ncaId)
    {
        const void* fileEntry = m_content->GetFileEntryByNcaId(ncaId);
        std::string ncaFileName = m_content->GetFileEntryName(fileEntry);

        LOG_DEBUG("Retrieving %s\n", ncaFileName.c_str());
        size_t ncaSize = m_content->GetFileEntrySize(fileEntry);

        NcaWriter writer(ncaId, contentStorage);

        u64 fileStart = m_content->GetFileEntryOffset(fileEntry);
        u64 fileOff = 0;
        size_t readSize = 0x400000; // 4MB buff
        auto readBuffer = std::make_unique<u8[]>(readSize);

        u64 freq = armGetSystemTickFreq();
        u64 startTime = armGetSystemTick();
        size_t startSizeBuffered = 0;
        double speed = 0.0;

        float progress;
        try
        {
            app::facade::SendInstallInfoText("inst.info_page.top_info0"_lang + ncaFileName + "...");
            app::facade::SendInstallProgress(0);
            while (fileOff < ncaSize)
            {
                progress = (float) fileOff / (float) ncaSize;
                u64 newTime = armGetSystemTick();

                if (fileOff % (0x400000 * 3) == 0)
                {
                    size_t newSizeBuffered = fileOff;
                    double mbBuffered = (newSizeBuffered / 1000000.0) - (startSizeBuffered / 1000000.0);
                    double duration = ((double)(newTime - startTime) / (double)freq);
                    speed =  mbBuffered / duration;
                    startTime = newTime;
                    startSizeBuffered = newSizeBuffered;

                    LOG_DEBUG("> Progress: %lu/%lu MB (%d%s)\r", (fileOff / 1000000), (ncaSize / 1000000), (int)(progress * 100.0), "%");
                    app::facade::SendInstallProgress((double)(progress * 100.0));
                    std::stringstream x;
                    x << (int)(progress * 100.0);
                    app::facade::SendInstallInfoText("inst.info_page.top_info0"_lang + ncaFileName + " " + x.str() + "% " + "inst.info_page.at"_lang + std::to_string(speed).substr(0, std::to_string(speed).size() - 4) + "MB/s");
                }

                if (fileOff + readSize >= ncaSize)
                {
                    readSize = ncaSize - fileOff;
                }

                this->BufferData(readBuffer.get(), fileOff + fileStart, readSize);
                writer.write(readBuffer.get(), readSize);

                fileOff += readSize;
            }
            app::facade::SendInstallProgress(100);
        }
        catch (std::exception& e)
        {
            LOG_DEBUG("something went wrong: %s\n", e.what());
        }

        writer.close();
    }

    void LocalWorker::BufferData(void* buf, off_t offset, size_t size)
    {
        fseeko(m_file, offset, SEEK_SET);
        fread(buf, 1, size, m_file);
    }
}
