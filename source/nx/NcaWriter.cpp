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

#include "nx/NcaWriter.hpp"
#include "nx/Crypto.hpp"
#include "nx/error.hpp"

#include <zstd.h>

constexpr auto NCZ_HEADER_OFFSET = 0x4000;
constexpr auto DEFLATE_BUFFER_MAX_SIZE = 0x400000;

static void append(std::vector<u8>& buffer, const u8* ptr, u64 sz)
{
    u64 offset = buffer.size();
    buffer.resize(offset + sz);
    std::memcpy(buffer.data() + offset, ptr, sz);
}

class NcaBodyWriter
{
public:
    NcaBodyWriter(const NcmContentId& ncaId, std::shared_ptr<nx::ncm::ContentStorage>& contentStorage, Sha256Context* sha256ctx = nullptr) :
        m_contentStorage(contentStorage),
        m_ncaId(ncaId),
        m_sha256ctx(sha256ctx)
    {}

    virtual ~NcaBodyWriter() = default;

    virtual void close() {}

    virtual u64 write(const u8* ptr, u64 sz)
    {
        m_contentStorage->WritePlaceholder(*(NcmPlaceHolderId*)&m_ncaId, m_offset, (void*)ptr, sz);
        sha256ContextUpdate(m_sha256ctx, ptr, sz);
        m_offset += sz;
        return sz;
    }

protected:
    std::shared_ptr<nx::ncm::ContentStorage> m_contentStorage;
    NcmContentId m_ncaId;
    u64 m_offset = NCZ_HEADER_OFFSET;
    Sha256Context* m_sha256ctx;
};

// code - https://github.com/nicoboss/nsz
// https://github.com/nicoboss/nsz/blob/master/nsz/NszDecompressor.py
// https://github.com/nicoboss/nsz/blob/master/nsz/BlockDecompressorReader.py

class NczHeader
{
public:
    static const u64 MAGIC = 0x4E544345535A434E; // NTCESZCN
    static const u64 BLOCK = 0x4B434F4C425A434E; // NCZBLOCK at 0x40D0

    class Section
    {
    public:
        u64 offset;
        u64 size;
        u8 cryptoType;
        u8 padding1[7];
        u64 padding2;
        u8 cryptoKey[0x10];
        u8 cryptoCounter[0x10];
    } NX_PACKED;

    class SectionContext : public Section
    {
    public:
        SectionContext(const Section& s) : Section(s), crypto(s.cryptoKey, nx::Crypto::AesCtr(nx::Crypto::swapEndian(((u64*)&s.cryptoCounter)[0]))) {}

        ~SectionContext() {}

        void encrypt(void* p, u64 sz, u64 offset)
        {
            if (this->cryptoType == 3 || this->cryptoType == 4)
            {
                crypto.seek(offset);
                crypto.encrypt(p, p, sz);
            }
        }

        nx::Crypto::Aes128Ctr crypto;
    };

    const u64 size() const
    {
        return sizeof(m_magic) + sizeof(m_sectionCount) + sizeof(Section) * m_sectionCount;
    }

    const Section& section(u64 i) const
    {
        return m_sections[i];
    }

    const u64 sectionCount() const
    {
        return m_sectionCount;
    }

protected:
    u64 m_magic;
    u64 m_sectionCount;
    Section m_sections[1];
} NX_PACKED;

struct BlockHeader {
    u64 magic;
    u8 version;
    u8 type;
    u8 unused;
    u8 blockSizeExponent;
    u32 numberOfBlocks;
    u64 decompressedSize;
};

struct BlockInfo {
    void init(const u8 *buffer)
    {
        header = *(BlockHeader*)buffer;
        assert(header.blockSizeExponent >= 14 && header.blockSizeExponent < 32);
        blockSize = size_t(1) << header.blockSizeExponent;
        for (u32 i = 0; i < header.numberOfBlocks; i++)
        {
            compressedBlockSizeList.push_back(*(u32 *)(buffer + sizeof(BlockHeader) + i * sizeof(u32)));
        }
    }

    u32 getCurBlockSize() { return compressedBlockSizeList[curBlockId]; }

    size_t decompressBlock(const std::vector<u8> &buffer, std::vector<u8> &dest)
    {
        auto curBlockSize = getCurBlockSize();
        assert(buffer.size() >= curBlockSize);
        size_t outSize;
        if (curBlockSize < blockSize)
        {
            outSize = ZSTD_getFrameContentSize(buffer.data(), curBlockSize);
            if (outSize == ZSTD_CONTENTSIZE_UNKNOWN || outSize == ZSTD_CONTENTSIZE_ERROR)
            {
                THROW_FORMAT("ZSTD_getFrameContentSize error");
            }
            dest.resize(outSize);
            const auto ret = ZSTD_decompress(dest.data(), outSize, buffer.data(), curBlockSize);
            if (ZSTD_isError(ret))
            {
                throw std::runtime_error(std::string("ZSTD_decompress error: ") + ZSTD_getErrorName(ret));
            }
        }
        else
        {
            outSize = curBlockSize;
            dest.resize(outSize);
            memcpy(dest.data(), buffer.data(), curBlockSize);
        }
        curBlockId += 1;
        return outSize;
    }

    BlockHeader header;
    size_t blockSize = 0;
    size_t curBlockId = 0;
    std::vector<u32> compressedBlockSizeList;
};

class NczBodyWriter : public NcaBodyWriter
{
public:
    NczBodyWriter(const NcmContentId& ncaId, std::shared_ptr<nx::ncm::ContentStorage>& contentStorage, Sha256Context* sha256ctx = nullptr)
        : NcaBodyWriter(ncaId, contentStorage, sha256ctx)
    {
        buffOut.resize(buffOutSize);
        dctx = ZSTD_createDCtx();
    }

    ~NczBodyWriter() override
    {
        ZSTD_freeDCtx(dctx);
    }

    void close() override
    {
        if (!m_isBlockCompression)
        {
            if (m_buffer.size())
            {
                processChunk(m_buffer.data(), m_buffer.size());
                m_buffer.resize(0);
            }

            encrypt(m_deflateBuffer.data(), m_deflateBuffer.size(), m_offset);
            flush();
        }
        else
        {
            if (m_buffer.size())
            {
                throw std::runtime_error("block decompress error");
            }
        }
    }

    void flush()
    {
        if (m_deflateBuffer.size())
        {
            m_contentStorage->WritePlaceholder(*(NcmPlaceHolderId*)&m_ncaId, m_offset, m_deflateBuffer.data(), m_deflateBuffer.size());
            sha256ContextUpdate(m_sha256ctx, m_deflateBuffer.data(), m_deflateBuffer.size());
            m_offset += m_deflateBuffer.size();
            m_deflateBuffer.resize(0);
        }
    }

    NczHeader::SectionContext& section(u64 offset)
    {
        for (u64 i = 0; i < sections.size(); i++)
        {
            if (offset >= sections[i].offset && offset < sections[i].offset + sections[i].size)
            {
                return sections[i];
            }
        }
        return sections[0];
    }

    void encrypt(const void* ptr, u64 sz, u64 offset)
    {
        const u8* start = (u8*)ptr;
        const u8* end = start + sz;

        while (start < end)
        {
            auto& s = section(offset);
            u64 sectionEnd = s.offset + s.size;
            u64 chunk = offset + sz > sectionEnd ? sectionEnd - offset : sz;
            s.encrypt((void*)start, chunk, offset);
            offset += chunk;
            start += chunk;
            sz -= chunk;
        }
    }

    void processChunk(const u8* ptr, u64 sz)
    {
        while(sz)
        {
            const size_t readChunkSz = std::min(sz, (u64)buffInSize);
            ZSTD_inBuffer input = { ptr, readChunkSz, 0 };

            while(input.pos < input.size)
            {
                ZSTD_outBuffer output = { buffOut.data(), buffOutSize, 0 };
                const auto ret = ZSTD_decompressStream(dctx, std::addressof(output), std::addressof(input));

                if (ZSTD_isError(ret))
                {
                    throw std::runtime_error(std::string("ZSTD_decompressStream error: ") + ZSTD_getErrorName(ret));
                }

                size_t len = output.pos;
                size_t written = 0;

                while(len)
                {
                    const size_t writeChunkSz = std::min(DEFLATE_BUFFER_MAX_SIZE - m_deflateBuffer.size(), len);

                    append(m_deflateBuffer, buffOut.data() + written, writeChunkSz);

                    if(m_deflateBuffer.size() >= DEFLATE_BUFFER_MAX_SIZE)
                    {
                        encrypt(m_deflateBuffer.data(), m_deflateBuffer.size(), m_offset);
                        flush();
                    }

                    written += writeChunkSz;
                    len -= writeChunkSz;
                }
            }

            sz -= readChunkSz;
            ptr += readChunkSz;
        }
    }

    u64 write(const  u8* ptr, u64 sz) override
    {
        if (!m_sectionsInitialized)
        {
            if (!m_buffer.size())
            {
                append(m_buffer, ptr, sizeof(u64)*2);
                ptr += sizeof(u64) * 2;
                sz -= sizeof(u64) * 2;
            }

            auto header = (NczHeader*)m_buffer.data();

            u64 chunk = std::min(sz, header->size() - m_buffer.size());
            append(m_buffer, ptr, chunk);
            ptr += chunk;
            sz -= chunk;

            header = (NczHeader*)m_buffer.data();

            if (m_buffer.size() == header->size())
            {
                for (u64 i = 0; i < header->sectionCount(); i++)
                {
                    sections.emplace_back(header->section(i));
                }

                m_sectionsInitialized = true;
                m_buffer.resize(0);
            }
        }

        if (!m_blockInitialized)
        {
            u64 chunk = std::min((size_t)sz, sizeof(BlockHeader));
            append(m_buffer, ptr, chunk);
            ptr += chunk;
            sz -= chunk;

            if (m_buffer.size() == sizeof(BlockHeader))
            {
                m_blockInitialized = true;
                auto block = (BlockHeader*)m_buffer.data();
                if (block->magic == NczHeader::BLOCK)
                {
                    m_isBlockCompression = true;
                    auto listSize = block->numberOfBlocks * sizeof(u32);
                    u64 chunk = std::min((size_t)sz, sizeof(BlockHeader) + listSize - m_buffer.size());
                    append(m_buffer, ptr, chunk);
                    ptr += chunk;
                    sz -= chunk;
                    if (m_buffer.size() == sizeof(BlockHeader) + listSize)
                    {
                        blockInfo.init(m_buffer.data());
                        m_buffer.resize(0);
                    }
                }
            }
        }

        if (m_isBlockCompression)
        {
            while (sz)
            {
                size_t curBlockSize = blockInfo.getCurBlockSize();
                auto chunk = m_buffer.size() < curBlockSize ? std::min((size_t)sz, curBlockSize - m_buffer.size()) : 0;
                append(m_buffer, ptr, chunk);
                sz -= chunk;
                ptr += chunk;
                if (m_buffer.size() >= curBlockSize)
                {
                    blockInfo.decompressBlock(m_buffer, m_deflateBuffer);
                    encrypt(m_deflateBuffer.data(), m_deflateBuffer.size(), m_offset);
                    flush();
                    m_buffer.erase(m_buffer.begin(), m_buffer.begin() + curBlockSize);
                }
            }
        }
        else
        {
            append(m_buffer, ptr, sz);
            processChunk(m_buffer.data(), m_buffer.size());
            m_buffer.resize(0);
        }

        return sz;
    }

private:
    size_t const buffInSize = ZSTD_DStreamInSize();
    size_t const buffOutSize = ZSTD_DStreamOutSize();

    std::vector<u8> buffOut{};
    ZSTD_DCtx* dctx = nullptr;

    std::vector<u8> m_buffer;
    std::vector<u8> m_deflateBuffer;

    bool m_sectionsInitialized = false;
    bool m_blockInitialized = false;
    bool m_isBlockCompression = false;

    std::vector<NczHeader::SectionContext> sections;
    BlockInfo blockInfo;
};

NcaWriter::NcaWriter(const NcmContentId& ncaId, std::shared_ptr<nx::ncm::ContentStorage>& contentStorage) : m_ncaId(ncaId), m_contentStorage(contentStorage)
{
    sha256ContextCreate(&m_sha256ctx);
}

NcaWriter::~NcaWriter() = default;

void NcaWriter::close(void* hash_dest)
{
    if (m_writer)
    {
        m_writer->close();
        m_writer = nullptr;
    }

    m_buffer.resize(0);

    if (hash_dest)
    {
        sha256ContextGetHash(&m_sha256ctx, hash_dest);
    }
    else
    {
        m_sha256ctx.finalized = true;
    }
}

u64 NcaWriter::write(const u8* ptr, u64 sz)
{
    if (m_buffer.size() < NCZ_HEADER_OFFSET)
    {
        u64 remainder = std::min(sz, (u64)NCZ_HEADER_OFFSET);
        append(m_buffer, ptr, remainder);
        ptr += remainder;
        sz -= remainder;

        if (m_buffer.size() >= sizeof(nx::nca::NcaHeader))
        {
            flushHeader();
            sha256ContextUpdate(&m_sha256ctx, m_buffer.data(), m_buffer.size());
        }
    }

    if (sz)
    {
        if (!m_writer)
        {
            if (sz >= sizeof(NczHeader::MAGIC))
            {
                if (*(u64*)ptr == NczHeader::MAGIC)
                {
                    m_writer = std::make_unique<NczBodyWriter>(m_ncaId, m_contentStorage, &m_sha256ctx);
                }
                else
                {
                    m_writer = std::make_unique<NcaBodyWriter>(m_ncaId, m_contentStorage, &m_sha256ctx);
                }
            }
            else
            {
                THROW_FORMAT("Not enough data to determine the header type");
            }
        }

        m_writer->write(ptr, sz);
    }

    return sz;
}

void NcaWriter::flushHeader()
{
    nx::nca::NcaHeader header;
    memcpy(&header, m_buffer.data(), sizeof(header));
    nx::Crypto::AesXtr decryptor(nx::Crypto::Keys().headerKey, false);
    nx::Crypto::AesXtr encryptor(nx::Crypto::Keys().headerKey, true);
    decryptor.decrypt(&header, &header, sizeof(header), 0, 0x200);

    if (header.magic == MAGIC_NCA3)
    {
        m_contentStorage->CreatePlaceholder(m_ncaId, *(NcmPlaceHolderId*)&m_ncaId, header.nca_size);
    }
    else
    {
        THROW_FORMAT("Invalid NCA magic");
    }

    if (header.distribution == 1)
    {
        header.distribution = 0;
    }
    encryptor.encrypt(m_buffer.data(), &header, sizeof(header), 0, 0x200);

    m_contentStorage->WritePlaceholder(*(NcmPlaceHolderId*)&m_ncaId, 0, m_buffer.data(), m_buffer.size());
}
