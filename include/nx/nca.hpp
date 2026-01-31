#pragma once

#include <string>
#include <vector>
#include <switch.h>

#include "nx/ByteBuffer.hpp"

#define NCA_HEADER_SIZE 0x4000
#define MAGIC_NCA3 0x3341434E /* "NCA3" */

namespace nx::nca
{
    struct NcaBucketInfo {
        static constexpr size_t HeaderSize = 0x10;
        s64 offset;
        s64 size;
        u8 header[HeaderSize];
    };

    static_assert(sizeof(NcaBucketInfo) == 0x20, "NcaBucketInfo must be 0x20");

    struct NcaSparseInfo {
        NcaBucketInfo bucket;
        s64 physical_offset;
        u16 generation;
        u8  reserved[6];
    };

    static_assert(sizeof(NcaSparseInfo) == 0x30, "NcaSparseInfo must be 0x30");

    struct LayerRegion {
        u64 offset;
        u64 size;
    };

    struct HierarchicalSha256Data {
        u8 master_hash[0x20];
        u32 block_size;
        u32 layer_count;
        LayerRegion hash_layer;
        LayerRegion pfs0_layer;
        LayerRegion unused_layers[3];
        u8 _0x78[0x80];
    };

    #pragma pack(push, 1)
    struct HierarchicalIntegrityVerificationLevelInformation {
        u64 logical_offset;
        u64 hash_data_size;
        u32 block_size; // log2
        u32 _0x14; // reserved
    };
    #pragma pack(pop)

    struct InfoLevelHash {
        u32 max_layers;
        HierarchicalIntegrityVerificationLevelInformation levels[6];
        u8 signature_salt[0x20];
    };

    struct IntegrityMetaInfo {
        u32 magic; // IVFC
        u32 version;
        u32 master_hash_size;
        InfoLevelHash info_level_hash;
        u8 master_hash[0x20];
        u8 _0xE0[0x18];
    };

    static_assert(sizeof(HierarchicalSha256Data) == 0xF8);
    static_assert(sizeof(IntegrityMetaInfo) == 0xF8);
    static_assert(sizeof(HierarchicalSha256Data) == sizeof(IntegrityMetaInfo));

    struct NcaFsHeader
    {
        u16 version;           // always 2.
        u8 fs_type;
        u8 hash_type;
        u8 encryption_type;
        u8 metadata_hash_type;
        u8 _0x6[0x2];          // empty.

        union {
            HierarchicalSha256Data hierarchical_sha256_data;
            IntegrityMetaInfo integrity_meta_info; // used for romfs
        } hash_data;

        u8 patch_Info[0x40];

        union {
            u64 section_ctr;
            struct {
                u32 section_ctr_low;
                u32 section_ctr_high;
            };
        };

        NcaSparseInfo sparse_info; /* only used in sections with sparse storage. */
        u8 _0x178[0x88]; /* Padding. */
    } NX_PACKED;

    static_assert(sizeof(NcaFsHeader) == 0x200, "NcaFsHeader must be 0x200");
    static_assert(sizeof(NcaFsHeader::hash_data) == 0xF8);

    struct NcaSectionEntry
    {
        u32 media_start_offset;
        u32 media_end_offset;
        u8 _0x8[0x8]; /* Padding. */
    } NX_PACKED;

    static_assert(sizeof(NcaSectionEntry) == 0x10, "NcaSectionEntry must be 0x10");

    struct SectionHeaderHash
    {
        u8 sha256[0x20];
    };

    struct NcaHeader
    {
        u8 fixed_key_sig[0x100]; /* RSA-PSS signature over header with fixed key. */
        u8 npdm_key_sig[0x100]; /* RSA-PSS signature over header with key in NPDM. */
        u32 magic;
        u8 distribution; /* System vs gamecard. */
        u8 content_type;
        u8 m_cryptoType; /* Which keyblob (field 1) */
        u8 m_kaekIndex; /* Which kaek index? */
        u64 nca_size; /* Entire archive size. */
        u64 m_titleId;
        u32 m_contentIndex;
        union {
            uint32_t sdk_version; /* What SDK was this built with? */
            struct {
                u8 sdk_revision;
                u8 sdk_micro;
                u8 sdk_minor;
                u8 sdk_major;
            };
        };
        u8 m_cryptoType2; /* Which keyblob (field 2) */
        u8 m_cryptoType3;
        u8 _0x222[0xE]; /* Padding. */
        FsRightsId m_rightsId; /* Rights ID (for titlekey crypto). */
        NcaSectionEntry fs_table[4]; /* Section entry metadata. */
        SectionHeaderHash fs_header_hash[4]; /* SHA-256 hashes for each section header. */
        u8 m_keys[4 * 0x10]; /* Encrypted key area. */
        u8 _0x340[0xC0]; /* Padding. */
        NcaFsHeader fs_header[4]; /* FS section headers. */
    } NX_PACKED;

    static_assert(sizeof(NcaHeader) == 0xc00, "NcaHeader must be 0xc00");

    struct FileEntry
    {
        std::string name;
        std::vector<u8> data;
    };

    std::string GetNcaIdString(const NcmContentId& ncaId);
    NcmContentId GetNcaIdFromString(std::string ncaIdStr);
    void BuildNcaByHeader(NcaHeader& nca_header, u8 index, const std::vector<FileEntry>& entries, u32 block_size, data::ByteIO& buf);
}
