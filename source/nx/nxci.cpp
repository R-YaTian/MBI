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

#include "nx/nxci.hpp"
#include "nx/error.hpp"

namespace nx
{
    XCI::XCI() {}

    void XCI::CommitHeader(std::vector<u8>&& headerBytes, u64 headerOffset)
    {
        m_secureHeaderBytes  = std::move(headerBytes);
        m_secureHeaderOffset = headerOffset;
    }

    const XFS0BaseHeader* XCI::GetBaseHeader()
    {
        if (m_secureHeaderBytes.empty())
        {
            THROW_FORMAT("Cannot retrieve header as header bytes are empty. Have you commited it yet?\n");
        }

        return reinterpret_cast<XFS0BaseHeader*>(m_secureHeaderBytes.data());
    }

    const u64 XCI::GetDataOffset()
    {
        if (m_secureHeaderBytes.empty())
        {
            THROW_FORMAT("Cannot get data offset as header is empty. Have you commited it yet?\n");
        }

        return m_secureHeaderOffset + m_secureHeaderBytes.size();
    }

    const u64 XCI::GetFileEntrySize(const void *fileEntry)
    {
        return ((HFS0FileEntry *)fileEntry)->fileSize;
    }

    const u64 XCI::GetFileEntryOffset(const void *fileEntry)
    {
        return GetDataOffset() + ((HFS0FileEntry *)fileEntry)->dataOffset;
    }

    const void* XCI::GetFileEntry(unsigned int index)
    {
        if (index >= this->GetBaseHeader()->numFiles)
        {
            THROW_FORMAT("File entry index is out of bounds\n");
        }

        return hfs0GetFileEntry(this->GetBaseHeader(), index);
    }

    const char* XCI::GetFileEntryName(const void* fileEntry)
    {
        return hfs0GetFileName(this->GetBaseHeader(), (HFS0FileEntry *)fileEntry);
    }
}
