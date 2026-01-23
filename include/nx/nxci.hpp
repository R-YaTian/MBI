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

#include <vector>
#include <memory>
#include <switch/types.h>

#include "nx/xfs0.hpp"
#include "nx/content.hpp"

namespace nx
{
    class XCI : public Content
    {
        private:
            u64 m_secureHeaderOffset;
            std::vector<u8> m_secureHeaderBytes;

        public:
            XCI();

            Type GetType() const override { return Type::XCI; }
            void CommitHeader(std::vector<u8>&& headerBytes, u64 headerOffset);

            const XFS0BaseHeader* GetBaseHeader() override;
            const u64 GetDataOffset() override;
            const u64 GetFileEntrySize(const void *fileEntry) override;
            const u64 GetFileEntryOffset(const void *fileEntry) override;
            const void* GetFileEntry(unsigned int index) override;
            const char* GetFileEntryName(const void* fileEntry) override;
    };
}
