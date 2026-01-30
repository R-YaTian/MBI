#include "nx/nca.hpp"
#include "nx/xfs0.hpp"
#include "nx/ByteBuffer.hpp"
#include <cstring>

#include "facade.hpp"

namespace nx::nca
{
    auto write_padding(nx::data::ByteIOBuffer& buf, u64 off, u64 block) -> u64
    {
        const u64 size = block - (off % block);
        if (size)
        {
            std::vector<u8> padding(size);
            buf.write(padding.data(), padding.size());
        }
        return size;
    }

    void write_nca_padding(nx::data::ByteIOBuffer& buf)
    {
        write_padding(buf, buf.tell(), 0x200);
    }

    void write_nca_section(NcaHeader& nca_header, u8 index, u64 start, u64 end)
    {
        auto& section = nca_header.fs_table[index];
        section.media_start_offset = start / 0x200; // 0xC00 / 0x200
        section.media_end_offset = end / 0x200; // Section end offset / 200
        section._0x8[0] = 0x1; // Always 1
    }

    void write_nca_fs_header_pfs0(NcaHeader& nca_header, u8 index, const std::vector<u8>& master_hash, u64 hash_table_size, u32 block_size)
    {
        auto& fs_header = nca_header.fs_header[index];
        fs_header.hash_type = 0x2;
        fs_header.fs_type = 0x1;
        fs_header.version = 0x2; // Always 2
        fs_header.hash_data.hierarchical_sha256_data.layer_count = 0x2;
        fs_header.hash_data.hierarchical_sha256_data.block_size = block_size;
        fs_header.encryption_type = 0x1;
        fs_header.hash_data.hierarchical_sha256_data.hash_layer.size = hash_table_size;
        std::memcpy(fs_header.hash_data.hierarchical_sha256_data.master_hash, master_hash.data(), master_hash.size());
        sha256CalculateHash(&nca_header.fs_header_hash[index], &fs_header, sizeof(fs_header));
    }

    std::string GetNcaIdString(const NcmContentId& ncaId)
    {
        char ncaIdStr[FS_MAX_PATH] = {0};
        u64 ncaIdLower = __bswap64(*(u64 *)ncaId.c);
        u64 ncaIdUpper = __bswap64(*(u64 *)(ncaId.c + 0x8));
        std::snprintf(ncaIdStr, FS_MAX_PATH, "%016lx%016lx", ncaIdLower, ncaIdUpper);
        return std::string(ncaIdStr);
    }

    NcmContentId GetNcaIdFromString(std::string ncaIdStr)
    {
        NcmContentId ncaId = {0};
        char lowerU64[17] = {0};
        char upperU64[17] = {0};
        memcpy(lowerU64, ncaIdStr.c_str(), 16);
        memcpy(upperU64, ncaIdStr.c_str() + 16, 16);

        *(u64 *)ncaId.c = __bswap64(strtoul(lowerU64, NULL, 16));
        *(u64 *)(ncaId.c + 8) = __bswap64(strtoul(upperU64, NULL, 16));

        return ncaId;
    }

    auto build_pfs0(const std::vector<FileEntry>& entries) -> std::vector<u8>
    {
        nx::data::ByteIOBuffer buf;

        XFS0BaseHeader header{};
        std::vector<PFS0FileEntry> file_table(entries.size());
        std::vector<char> string_table;

        u64 string_offset{};
        u64 data_offset{};

        for (u32 i = 0; i < entries.size(); i++)
        {
            file_table[i].dataOffset = data_offset;
            file_table[i].fileSize = entries[i].data.size();
            file_table[i].stringTableOffset = string_offset;
            file_table[i].padding = 0;

            string_table.resize(string_offset + entries[i].name.length() + 1);
            std::memcpy(string_table.data() + string_offset, entries[i].name.c_str(), entries[i].name.length() + 1);

            data_offset += entries[i].data.size();
            string_offset += entries[i].name.length() + 1;
        }

        header.magic = 0x30534650;
        header.numFiles = entries.size();
        header.stringTableSize = string_table.size();
        header.reserved = 0;

        buf.write(&header, sizeof(header));
        buf.write(file_table.data(), sizeof(PFS0FileEntry) * file_table.size());

        constexpr size_t ALIGN = 0x20;
        size_t cur = buf.tell() + string_table.size();
        size_t pad = (ALIGN - (cur % ALIGN)) % ALIGN;
        string_table.resize(string_table.size() + pad);

        buf.write(string_table.data(), string_table.size());

        for (const auto&e : entries)
        {
            buf.write(e.data.data(), e.data.size());
        }

        return buf.buf;
    }

    auto build_pfs0_hash_table(const std::vector<u8>& pfs0, u32 block_size) -> std::vector<u8>
    {
        nx::data::ByteIOBuffer buf;
        u8 hash[SHA256_HASH_SIZE];
        u32 read_size = block_size;

        for (u32 i = 0; i < pfs0.size(); i += read_size)
        {
            if (i + read_size >= pfs0.size()) {
                read_size = pfs0.size() - i;
            }
            sha256CalculateHash(hash, pfs0.data() + i, read_size);
            buf.write(hash, sizeof(hash));
        }

        return buf.buf;
    }

    auto build_pfs0_master_hash(const std::vector<u8>& pfs0_hash_table) -> std::vector<u8>
    {
        std::vector<u8> hash(SHA256_HASH_SIZE);
        sha256CalculateHash(hash.data(), pfs0_hash_table.data(), pfs0_hash_table.size());
        return hash;
    }

    void WriteNcaPfs0(NcaHeader& nca_header, u8 index, const std::vector<FileEntry>& entries, u32 block_size, nx::data::ByteIOBuffer& buf)
    {
        const auto pfs0 = build_pfs0(entries);
        app::facade::ShowDialog("Installing...", "PFS0构造完毕", { "OK" }, false);
        const auto pfs0_hash_table = build_pfs0_hash_table(pfs0, block_size);
        app::facade::ShowDialog("Installing...", "PFS0HASH完毕", { "OK" }, false);
        const auto pfs0_master_hash = build_pfs0_master_hash(pfs0_hash_table);
        app::facade::ShowDialog("Installing...", "PFS0 MASTER HASH完毕", { "OK" }, false);

        buf.write(pfs0_hash_table.data(), pfs0_hash_table.size());

        nca_header.fs_header[index].hash_data.hierarchical_sha256_data.pfs0_layer.offset = pfs0_hash_table.size();
        nca_header.fs_header[index].hash_data.hierarchical_sha256_data.pfs0_layer.size = pfs0.size();

        buf.write(pfs0.data(), pfs0.size());
        write_nca_padding(buf);

        const auto section_start = index == 0 ? sizeof(nca_header) : nca_header.fs_table[index-1].media_end_offset * 0x200;
        write_nca_section(nca_header, index, section_start, buf.tell());
        write_nca_fs_header_pfs0(nca_header, index, pfs0_master_hash, pfs0_hash_table.size(), block_size);
    }
}
