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

#include "nx/BufferedPlaceholderWriter.hpp"
#include "nx/error.hpp"
#include <climits>
#include <math.h>

namespace nx::data
{
    constexpr u32 NUM_BUFFER_SEGMENTS = 128;

    BufferedPlaceholderWriter::BufferedPlaceholderWriter(std::shared_ptr<nx::ncm::ContentStorage>& contentStorage, NcmContentId ncaId, size_t totalDataSize) :
        m_totalDataSize(totalDataSize), m_contentStorage(contentStorage), m_ncaId(ncaId), m_writer(ncaId, contentStorage)
    {
        // Though currently the number of segments is fixed, we want them allocated on the heap, not the stack
        m_bufferSegments = std::make_unique<BufferSegment[]>(NUM_BUFFER_SEGMENTS);

        if (m_bufferSegments == nullptr)
            THROW_FORMAT("Failed to allocated buffer segments!\n");

        m_currentFreeSegmentPtr = &m_bufferSegments[m_currentFreeSegment];
        m_currentSegmentToWritePtr = &m_bufferSegments[m_currentSegmentToWrite];
    }

    void BufferedPlaceholderWriter::AppendData(const void* source, size_t length)
    {
        if (m_sizeBuffered + length > m_totalDataSize)
            THROW_FORMAT("Cannot append data as it would exceed the expected total.\n");

        size_t dataSizeRemaining = length;
        u64 sourceOffset = 0;

        while (dataSizeRemaining > 0)
        {
            size_t bufferSegmentSizeRemaining = BUFFER_SEGMENT_DATA_SIZE - m_currentFreeSegmentPtr->writeOffset;

            if (m_currentFreeSegmentPtr->isFinalized.load(std::memory_order_acquire))
                THROW_FORMAT("Current buffer segment is already finalized!\n");

            if (dataSizeRemaining < bufferSegmentSizeRemaining)
            {
                std::memcpy(m_currentFreeSegmentPtr->data + m_currentFreeSegmentPtr->writeOffset, static_cast<const u8*>(source) + sourceOffset, dataSizeRemaining);
                sourceOffset += dataSizeRemaining;
                m_currentFreeSegmentPtr->writeOffset += dataSizeRemaining;
                dataSizeRemaining = 0;
            }
            else
            {
                std::memcpy(m_currentFreeSegmentPtr->data + m_currentFreeSegmentPtr->writeOffset, static_cast<const u8*>(source) + sourceOffset, bufferSegmentSizeRemaining);
                dataSizeRemaining -= bufferSegmentSizeRemaining;
                sourceOffset += bufferSegmentSizeRemaining;
                m_currentFreeSegmentPtr->writeOffset += bufferSegmentSizeRemaining;
                m_currentFreeSegmentPtr->isFinalized.store(true, std::memory_order_release);

                m_currentFreeSegment = (m_currentFreeSegment + 1) % NUM_BUFFER_SEGMENTS;
                m_currentFreeSegmentPtr = &m_bufferSegments[m_currentFreeSegment];
            }
        }

        m_sizeBuffered += length;

        if (m_sizeBuffered == m_totalDataSize)
        {
            m_currentFreeSegmentPtr->isFinalized.store(true, std::memory_order_release);
        }
    }

    bool BufferedPlaceholderWriter::CanAppendData(size_t length)
    {
        if (m_sizeBuffered + length > m_totalDataSize)
            return false;

        if (!this->IsSizeAvailable(length))
            return false;

        return true;
    }

    void BufferedPlaceholderWriter::WriteSegmentToPlaceholder()
    {
        if (m_sizeWrittenToPlaceholder >= m_totalDataSize)
            THROW_FORMAT("Cannot write segment as end of data has already been reached!\n");

        if (!m_currentSegmentToWritePtr->isFinalized)
            THROW_FORMAT("Cannot write segment as it hasn't been finalized!\n");

        // NOTE: The final segment will have leftover data from previous writes, however
        // this will be accounted for by this size
        size_t sizeToWriteToPlaceholder = std::min(m_totalDataSize - m_sizeWrittenToPlaceholder, BUFFER_SEGMENT_DATA_SIZE);
        m_writer.write(m_currentSegmentToWritePtr->data, sizeToWriteToPlaceholder);

        m_currentSegmentToWritePtr->writeOffset = 0;
        m_currentSegmentToWritePtr->isFinalized.store(false, std::memory_order_release);
        m_currentSegmentToWrite = (m_currentSegmentToWrite + 1) % NUM_BUFFER_SEGMENTS;
        m_currentSegmentToWritePtr = &m_bufferSegments[m_currentSegmentToWrite];
        m_sizeWrittenToPlaceholder += sizeToWriteToPlaceholder;
    }

    bool BufferedPlaceholderWriter::CanWriteSegmentToPlaceholder()
    {
        if (m_sizeWrittenToPlaceholder >= m_totalDataSize)
            return false;

        if (!m_currentSegmentToWritePtr->isFinalized.load(std::memory_order_acquire))
            return false;

        return true;
    }

    u32 BufferedPlaceholderWriter::CalcNumSegmentsRequired(size_t size)
    {
        if (m_currentFreeSegmentPtr->isFinalized.load(std::memory_order_acquire))
        {
            return UINT_MAX;
        }

        size_t bufferSegmentSizeRemaining = BUFFER_SEGMENT_DATA_SIZE - m_currentFreeSegmentPtr->writeOffset;

        if (size <= bufferSegmentSizeRemaining)
        {
            return 1;
        }
        else
        {
            double numSegmentsReq = 1 + static_cast<double>(size - bufferSegmentSizeRemaining) / static_cast<double>(BUFFER_SEGMENT_DATA_SIZE);
            return std::ceil(numSegmentsReq);
        }
    }

    bool BufferedPlaceholderWriter::IsSizeAvailable(size_t size)
    {
        u32 numSegmentsRequired = this->CalcNumSegmentsRequired(size);

        if (numSegmentsRequired > NUM_BUFFER_SEGMENTS)
        {
            return false;
        }

        for (u32 i = 0; i < numSegmentsRequired; i++)
        {
            u32 segmentIndex = m_currentFreeSegment + i;
            BufferSegment* bufferSegment = &m_bufferSegments[segmentIndex % NUM_BUFFER_SEGMENTS];

            if (bufferSegment->isFinalized.load(std::memory_order_acquire))
            {
                return false;
            }

            if (i != 0 && bufferSegment->writeOffset != 0)
            {
                THROW_FORMAT("Unexpected non-zero write offset at segment %u (write offset = %lu)\n", segmentIndex, bufferSegment->writeOffset);
            }
        }

        return true;
    }

    bool BufferedPlaceholderWriter::IsBufferDataComplete()
    {
        if (m_sizeBuffered > m_totalDataSize)
            THROW_FORMAT("Size buffered cannot exceed total data size!\n");

        return m_sizeBuffered == m_totalDataSize;
    }

    bool BufferedPlaceholderWriter::IsPlaceholderComplete()
    {
        if (m_sizeWrittenToPlaceholder > m_totalDataSize)
            THROW_FORMAT("Size written to placeholder cannot exceed total data size!\n");

        return m_sizeWrittenToPlaceholder == m_totalDataSize;
    }

    size_t BufferedPlaceholderWriter::GetTotalDataSize()
    {
        return m_totalDataSize;
    }

    size_t BufferedPlaceholderWriter::GetSizeBuffered()
    {
        return m_sizeBuffered;
    }

    size_t BufferedPlaceholderWriter::GetSizeWrittenToPlaceholder()
    {
        return m_sizeWrittenToPlaceholder;
    }

    std::vector<u8> BufferedPlaceholderWriter::Finalize()
    {
        std::vector<u8> hash(SHA256_HASH_SIZE);
        m_writer.close(hash.data());
        return hash;
    }
}
