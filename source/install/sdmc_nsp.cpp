#include "install/sdmc_nsp.hpp"
#include "nx/error.hpp"
#include "nx/NcaWriter.hpp"
#include "ui/instPage.hpp"
#include "util/lang.hpp"

namespace app::install::nsp
{
    SDMCNSP::SDMCNSP(std::string path)
    {
        m_nspFile = fopen((path).c_str(), "rb");
        if (!m_nspFile)
            THROW_FORMAT("can't open file at %s\n", path.c_str());
    }

    SDMCNSP::~SDMCNSP()
    {
        fclose(m_nspFile);
    }

    void SDMCNSP::StreamToPlaceholder(std::shared_ptr<nx::ncm::ContentStorage>& contentStorage, NcmContentId ncaId)
    {
        const PFS0FileEntry* fileEntry = this->GetFileEntryByNcaId(ncaId);
        std::string ncaFileName = this->GetFileEntryName(fileEntry);

        LOG_DEBUG("Retrieving %s\n", ncaFileName.c_str());
        size_t ncaSize = fileEntry->fileSize;

        NcaWriter writer(ncaId, contentStorage);

        float progress;

        u64 fileStart = GetDataOffset() + fileEntry->dataOffset;
        u64 fileOff = 0;
        size_t readSize = 0x400000; // 4MB buff
        auto readBuffer = std::make_unique<u8[]>(readSize);

        u64 freq = armGetSystemTickFreq();
        u64 startTime = armGetSystemTick();
        size_t startSizeBuffered = 0;
        double speed = 0.0;

        try
        {
            app::ui::InstallerPage::setInstInfoText("inst.info_page.top_info0"_lang + ncaFileName + "...");
            app::ui::InstallerPage::setInstBarPerc(0);
            while (fileOff < ncaSize)
            {
                progress = (float) fileOff / (float) ncaSize;
                u64 newTime = armGetSystemTick();

                if (fileOff % (0x400000 * 3) == 0) {
                    size_t newSizeBuffered = fileOff;
                    double mbBuffered = (newSizeBuffered / 1000000.0) - (startSizeBuffered / 1000000.0);
                    double duration = ((double)(newTime - startTime) / (double)freq);
                    speed =  mbBuffered / duration;
                    startTime = newTime;
                    startSizeBuffered = newSizeBuffered;

                    LOG_DEBUG("> Progress: %lu/%lu MB (%d%s)\r", (fileOff / 1000000), (ncaSize / 1000000), (int)(progress * 100.0), "%");
                    app::ui::InstallerPage::setInstBarPerc((double)(progress * 100.0));
                    std::stringstream x;
                    x << (int)(progress * 100.0);
                    app::ui::InstallerPage::setInstInfoText("inst.info_page.top_info0"_lang + ncaFileName + " " + x.str() + "% " + "inst.info_page.at"_lang + std::to_string(speed).substr(0, std::to_string(speed).size()-4) + "MB/s");
                }

                if (fileOff + readSize >= ncaSize) readSize = ncaSize - fileOff;

                this->BufferData(readBuffer.get(), fileOff + fileStart, readSize);
                writer.write(readBuffer.get(), readSize);

                fileOff += readSize;
            }
            app::ui::InstallerPage::setInstBarPerc(100);
        }
        catch (std::exception& e)
        {
            LOG_DEBUG("something went wrong: %s\n", e.what());
        }

        writer.close();
    }

    void SDMCNSP::BufferData(void* buf, off_t offset, size_t size)
    {
        fseeko(m_nspFile, offset, SEEK_SET);
        fread(buf, 1, size, m_nspFile);
    }
}
