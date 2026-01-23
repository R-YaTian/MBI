#pragma once

#include <switch/types.h>

#define MAGIC_HFS0 0x30534648

namespace nx
{
    struct PFS0FileEntry
    {
        u64 dataOffset;
        u64 fileSize;
        u32 stringTableOffset;
        u32 padding;
    } NX_PACKED;

    static_assert(sizeof(PFS0FileEntry) == 0x18, "PFS0FileEntry must be 0x18");

    struct HFS0FileEntry
    {
        u64 dataOffset;
        u64 fileSize;
        u32 stringTableOffset;
        u32 hashedSize;
        u64 padding;
        unsigned char hash[0x20];
    } NX_PACKED;

    static_assert(sizeof(HFS0FileEntry) == 0x40, "HFS0FileEntry must be 0x18");

    struct XFS0BaseHeader
    {
        u32 magic;
        u32 numFiles;
        u32 stringTableSize;
        u32 reserved;
    } NX_PACKED;

    static_assert(sizeof(XFS0BaseHeader) == 0x10, "XFS0BaseHeader must be 0x10");

    NX_INLINE const HFS0FileEntry *hfs0GetFileEntry(const XFS0BaseHeader *header, u32 i)
    {
        if (i >= header->numFiles)
            return NULL;
        return (const HFS0FileEntry*)(header + 0x1 + i * 0x4);
    }

    NX_INLINE const char *hfs0GetStringTable(const XFS0BaseHeader *header)
    {
        return (const char*)(header + 0x1 + header->numFiles * 0x4);
    }

    NX_INLINE const char *hfs0GetFileName(const XFS0BaseHeader *header, const HFS0FileEntry *entry)
    {
        return hfs0GetStringTable(header) + entry->stringTableOffset;
    }
}
