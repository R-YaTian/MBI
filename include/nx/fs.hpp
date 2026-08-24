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

#include <string>
#include <vector>
#include <switch.h>
#include <filesystem>

namespace nx
{
    enum FsError
    {
        FsError_PathNotFound = 0x202,
        FsError_PathAlreadyExists = 0x402,
        FsError_TargetLocked = 0xE02,
        FsError_UsableSpaceNotEnoughMmcCalibration = 0x4602,
        FsError_UsableSpaceNotEnoughMmcSafe = 0x4802,
        FsError_UsableSpaceNotEnoughMmcUser = 0x4A02,
        FsError_UsableSpaceNotEnoughMmcSystem = 0x4C02,
        FsError_UsableSpaceNotEnoughSdCard = 0x4E02,
        FsError_UnsupportedSdkVersion = 0x6402,
        FsError_MountNameAlreadyExists = 0x7802,
        FsError_PartitionNotFound = 0x7D202,
        FsError_TargetNotFound = 0x7D402,
        FsError_PortSdCardNoDevice = 0xFA202,
        FsError_GameCardCardNotInserted = 0x13B002,
        FsError_GameCardCardNotActivated = 0x13B402,
        FsError_GameCardInvalidSecureAccess = 0x13D802,
        FsError_GameCardInvalidNormalAccess = 0x13DA02,
        FsError_GameCardInvalidAccessAcrossMode = 0x13DC02,
        FsError_GameCardInitialDataMismatch = 0x13E002,
        FsError_GameCardInitialNotFilledWithZero = 0x13E202,
        FsError_GameCardKekIndexMismatch = 0x13E402,
        FsError_GameCardCardHeaderReadFailure = 0x13EE02,
        FsError_GameCardShouldTransitFromInitialToNormal = 0x145002,
        FsError_GameCardShouldTransitFromNormalModeToSecure = 0x145202,
        FsError_GameCardShouldTransitFromNormalModeToDebug = 0x145402,
        FsError_GameCardSendFirmwareFailure = 0x149402,
        FsError_GameCardReceiveCertificateFailure = 0x149A02,
        FsError_GameCardSendSocCertificateFailure = 0x14A002,
        FsError_GameCardReceiveRandomValueFailure = 0x14AA02,
        FsError_GameCardSendRandomValueFailure = 0x14AC02,
        FsError_GameCardReceiveDeviceChallengeFailure = 0x14B602,
        FsError_GameCardRespondDeviceChallengeFailure = 0x14B802,
        FsError_GameCardSendHostChallengeFailure = 0x14BA02,
        FsError_GameCardReceiveChallengeResponseFailure = 0x14BC02,
        FsError_GameCardChallengeAndResponseFailure = 0x14BE02,
        FsError_GameCardSplGenerateRandomBytesFailure = 0x14D802,
        FsError_GameCardReadRegisterFailure = 0x14DE02,
        FsError_GameCardWriteRegisterFailure = 0x14E002,
        FsError_GameCardEnableCardBusFailure = 0x14E202,
        FsError_GameCardGetCardHeaderFailure = 0x14E402,
        FsError_GameCardAsicStatusError = 0x14E602,
        FsError_GameCardChangeGcModeToSecureFailure = 0x14E802,
        FsError_GameCardChangeGcModeToDebugFailure = 0x14EA02,
        FsError_GameCardReadRmaInfoFailure = 0x14EC02,
        FsError_GameCardStateCardSecureModeRequired = 0x150802,
        FsError_GameCardStateCardDebugModeRequired = 0x150A02,
        FsError_GameCardCommandReadId1Failure = 0x155602,
        FsError_GameCardCommandReadId2Failure = 0x155802,
        FsError_GameCardCommandReadId3Failure = 0x155A02,
        FsError_GameCardCommandReadPageFailure = 0x155E02,
        FsError_GameCardCommandWritePageFailure = 0x156202,
        FsError_GameCardCommandRefreshFailure = 0x156402,
        FsError_GameCardCommandReadCrcFailure = 0x156C02,
        FsError_GameCardCommandEraseFailure = 0x156E02,
        FsError_GameCardCommandReadDevParamFailure = 0x157002,
        FsError_GameCardCommandWriteDevParamFailure = 0x157202,
        FsError_GameCardDebugCardReceivedIdMismatch = 0x16B002,
        FsError_GameCardDebugCardId1Mismatch = 0x16B202,
        FsError_GameCardDebugCardId2Mismatch = 0x16B402,
        FsError_GameCardFsCheckHandleInGetStatusFailure = 0x171402,
        FsError_GameCardFsCheckHandleInCreateReadOnlyFailure = 0x172002,
        FsError_GameCardFsCheckHandleInCreateSecureReadOnlyFailure = 0x172202,
        FsError_NotImplemented = 0x177202,
        FsError_AlreadyExists = 0x177602,
        FsError_OutOfRange = 0x177A02,
        FsError_AllocationMemoryFailedInFatFileSystemA = 0x190202,
        FsError_AllocationMemoryFailedInFatFileSystemB = 0x190402,
        FsError_AllocationMemoryFailedInFatFileSystemC = 0x190602,
        FsError_AllocationMemoryFailedInFatFileSystemD = 0x190802,
        FsError_AllocationMemoryFailedInFatFileSystemE = 0x190A02,
        FsError_AllocationMemoryFailedInFatFileSystemF = 0x190C02,
        FsError_AllocationMemoryFailedInFatFileSystemG = 0x190E02,
        FsError_AllocationMemoryFailedInFatFileSystemH = 0x191002,
        FsError_AllocationMemoryFailedInSdCardA = 0x195802,
        FsError_AllocationMemoryFailedInSdCardB = 0x195A02,
        FsError_AllocationMemoryFailedInSystemSaveDataA = 0x195C02,
        FsError_AllocationMemoryFailedInRomFsFileSystemA = 0x195E02,
        FsError_AllocationMemoryFailedInRomFsFileSystemB = 0x196002,
        FsError_AllocationMemoryFailedInRomFsFileSystemC = 0x196202,
        FsError_AllocationMemoryFailedInSdmmcStorageServiceA = 0x1A3E02,
        FsError_AllocationMemoryFailedInBuiltInStorageCreatorA = 0x1A4002,
        FsError_AllocationMemoryFailedInRegisterA = 0x1A4A02,
        FsError_IncorrectSaveDataFileSystemMagicCode = 0x21BC02,
        FsError_InvalidAcidFileSize = 0x234202,
        FsError_InvalidAcidSize = 0x234402,
        FsError_InvalidAcid = 0x234602,
        FsError_AcidVerificationFailed = 0x234802,
        FsError_InvalidNcaSignature = 0x234A02,
        FsError_NcaHeaderSignature1VerificationFailed = 0x234C02,
        FsError_NcaHeaderSignature2VerificationFailed = 0x234E02,
        FsError_NcaFsHeaderHashVerificationFailed = 0x235002,
        FsError_InvalidNcaKeyIndex = 0x235202,
        FsError_InvalidNcaFsHeaderEncryptionType = 0x235602,
        FsError_InvalidNcaPatchInfoIndirectSize = 0x235802,
        FsError_InvalidNcaPatchInfoAesCtrExSize = 0x235A02,
        FsError_InvalidNcaPatchInfoAesCtrExOffset = 0x235C02,
        FsError_InvalidNcaId = 0x235E02,
        FsError_InvalidNcaHeader = 0x236002,
        FsError_InvalidNcaFsHeader = 0x236202,
        FsError_InvalidHierarchicalSha256BlockSize = 0x236802,
        FsError_InvalidHierarchicalSha256LayerCount = 0x236A02,
        FsError_HierarchicalSha256BaseStorageTooLarge = 0x236C02,
        FsError_HierarchicalSha256HashVerificationFailed = 0x236E02,
        FsError_InvalidSha256PartitionHashTarget = 0x244402,
        FsError_Sha256PartitionHashVerificationFailed = 0x244602,
        FsError_PartitionSignatureVerificationFailed = 0x244802,
        FsError_Sha256PartitionSignatureVerificationFailed = 0x244A02,
        FsError_InvalidPartitionEntryOffset = 0x244C02,
        FsError_InvalidSha256PartitionMetaDataSize = 0x244E02,
        FsError_InvalidFatFileNumber = 0x249802,
        FsError_InvalidFatFormatBisUser = 0x249C02,
        FsError_InvalidFatFormatBisSystem = 0x249E02,
        FsError_InvalidFatFormatBisSafe = 0x24A002,
        FsError_InvalidFatFormatBisCalibration = 0x24A202,
        FsError_AesXtsFileSystemFileHeaderCorruptedOnFileOpen = 0x250E02,
        FsError_AesXtsFileSystemFileNoHeaderOnFileOpen = 0x251002,
        FsError_FatFsFormatUnsupportedSize = 0x280202,
        FsError_FatFsFormatInvalidBpb = 0x280402,
        FsError_FatFsFormatInvalidParameter = 0x280602,
        FsError_FatFsFormatIllegalSectorsA = 0x280802,
        FsError_FatFsFormatIllegalSectorsB = 0x280A02,
        FsError_FatFsFormatIllegalSectorsC = 0x280C02,
        FsError_FatFsFormatIllegalSectorsD = 0x280E02,
        FsError_UnexpectedInMountTableA = 0x296A02,
        FsError_TooLongPath = 0x2EE602,
        FsError_InvalidCharacter = 0x2EE802,
        FsError_InvalidPathFormat = 0x2EEA02,
        FsError_DirectoryUnobtainable = 0x2EEC02,
        FsError_InvalidOffset = 0x2F5A02,
        FsError_InvalidSize = 0x2F5C02,
        FsError_NullptrArgument = 0x2F5E02,
        FsError_InvalidAlignment = 0x2F6002,
        FsError_InvalidMountName = 0x2F6202,
        FsError_ExtensionSizeTooLarge = 0x2F6402,
        FsError_ExtensionSizeInvalid = 0x2F6602,
        FsError_FileExtensionWithoutOpenModeAllowAppend = 0x307202,
        FsError_UnsupportedCommitTarget = 0x313A02,
        FsError_UnsupportedSetSizeForNotResizableSubStorage = 0x313C02,
        FsError_UnsupportedSetSizeForResizableSubStorage = 0x313E02,
        FsError_UnsupportedSetSizeForMemoryStorage = 0x314002,
        FsError_UnsupportedOperateRangeForMemoryStorage = 0x314202,
        FsError_UnsupportedOperateRangeForFileStorage = 0x314402,
        FsError_UnsupportedOperateRangeForFileHandleStorage = 0x314602,
        FsError_UnsupportedOperateRangeForSwitchStorage = 0x314802,
        FsError_UnsupportedOperateRangeForStorageServiceObjectAdapter = 0x314A02,
        FsError_UnsupportedWriteForAesCtrCounterExtendedStorage = 0x314C02,
        FsError_UnsupportedSetSizeForAesCtrCounterExtendedStorage = 0x314E02,
        FsError_UnsupportedOperateRangeForAesCtrCounterExtendedStorage = 0x315002,
        FsError_UnsupportedWriteForAesCtrStorageExternal = 0x315202,
        FsError_UnsupportedSetSizeForAesCtrStorageExternal = 0x315402,
        FsError_UnsupportedSetSizeForAesCtrStorage = 0x315602,
        FsError_UnsupportedSetSizeForHierarchicalIntegrityVerificationStorage = 0x315802,
        FsError_UnsupportedOperateRangeForHierarchicalIntegrityVerificationStorage = 0x315A02,
        FsError_UnsupportedSetSizeForIntegrityVerificationStorage = 0x315C02,
        FsError_UnsupportedOperateRangeForWritableIntegrityVerificationStorage = 0x315E02,
        FsError_UnsupportedOperateRangeForIntegrityVerificationStorage = 0x316002,
        FsError_UnsupportedSetSizeForBlockCacheBufferedStorage = 0x316202,
        FsError_UnsupportedOperateRangeForWritableBlockCacheBufferedStorage = 0x316402,
        FsError_UnsupportedOperateRangeForBlockCacheBufferedStorage = 0x316602,
        FsError_UnsupportedWriteForIndirectStorage = 0x316802,
        FsError_UnsupportedSetSizeForIndirectStorage = 0x316A02,
        FsError_UnsupportedOperateRangeForIndirectStorage = 0x316C02,
        FsError_UnsupportedWriteForZeroStorage = 0x316E02,
        FsError_UnsupportedSetSizeForZeroStorage = 0x317002,
        FsError_UnsupportedSetSizeForHierarchicalSha256Storage = 0x317202,
        FsError_UnsupportedWriteForReadOnlyBlockCacheStorage = 0x317402,
        FsError_UnsupportedSetSizeForReadOnlyBlockCacheStorage = 0x317602,
        FsError_UnsupportedSetSizeForIntegrityRomFsStorage = 0x317802,
        FsError_UnsupportedSetSizeForDuplexStorage = 0x317A02,
        FsError_UnsupportedOperateRangeForDuplexStorage = 0x317C02,
        FsError_UnsupportedSetSizeForHierarchicalDuplexStorage = 0x317E02,
        FsError_UnsupportedGetSizeForRemapStorage = 0x318002,
        FsError_UnsupportedSetSizeForRemapStorage = 0x318202,
        FsError_UnsupportedOperateRangeForRemapStorage = 0x318402,
        FsError_UnsupportedSetSizeForIntegritySaveDataStorage = 0x318602,
        FsError_UnsupportedOperateRangeForIntegritySaveDataStorage = 0x318802,
        FsError_UnsupportedSetSizeForJournalIntegritySaveDataStorage = 0x318A02,
        FsError_UnsupportedOperateRangeForJournalIntegritySaveDataStorage = 0x318C02,
        FsError_UnsupportedGetSizeForJournalStorage = 0x318E02,
        FsError_UnsupportedSetSizeForJournalStorage = 0x319002,
        FsError_UnsupportedOperateRangeForJournalStorage = 0x319202,
        FsError_UnsupportedSetSizeForUnionStorage = 0x319402,
        FsError_UnsupportedSetSizeForAllocationTableStorage = 0x319602,
        FsError_UnsupportedReadForWriteOnlyGameCardStorage = 0x319802,
        FsError_UnsupportedSetSizeForWriteOnlyGameCardStorage = 0x319A02,
        FsError_UnsupportedWriteForReadOnlyGameCardStorage = 0x319C02,
        FsError_UnsupportedSetSizeForReadOnlyGameCardStorage = 0x319E02,
        FsError_UnsupportedOperateRangeForReadOnlyGameCardStorage = 0x31A002,
        FsError_UnsupportedSetSizeForSdmmcStorage = 0x31A202,
        FsError_UnsupportedOperateRangeForSdmmcStorage = 0x31A402,
        FsError_UnsupportedOperateRangeForFatFile = 0x31A602,
        FsError_UnsupportedOperateRangeForStorageFile = 0x31A802,
        FsError_UnsupportedSetSizeForInternalStorageConcatenationFile = 0x31AA02,
        FsError_UnsupportedOperateRangeForInternalStorageConcatenationFile = 0x31AC02,
        FsError_UnsupportedQueryEntryForConcatenationFileSystem = 0x31AE02,
        FsError_UnsupportedOperateRangeForConcatenationFile = 0x31B002,
        FsError_UnsupportedSetSizeForZeroBitmapFile = 0x31B202,
        FsError_UnsupportedOperateRangeForFileServiceObjectAdapter = 0x31B402,
        FsError_UnsupportedOperateRangeForAesXtsFile = 0x31B602,
        FsError_UnsupportedWriteForRomFsFileSystem = 0x31B802,
        FsError_UnsupportedCommitProvisionallyForRomFsFileSystem = 0x31BA02,
        FsError_UnsupportedGetTotalSpaceSizeForRomFsFileSystem = 0x31BC02,
        FsError_UnsupportedWriteForRomFsFile = 0x31BE02,
        FsError_UnsupportedOperateRangeForRomFsFile = 0x31C002,
        FsError_UnsupportedWriteForReadOnlyFileSystem = 0x31C202,
        FsError_UnsupportedCommitProvisionallyForReadOnlyFileSystem = 0x31C402,
        FsError_UnsupportedGetTotalSpaceSizeForReadOnlyFileSystem = 0x31C602,
        FsError_UnsupportedWriteForReadOnlyFile = 0x31C802,
        FsError_UnsupportedOperateRangeForReadOnlyFile = 0x31CA02,
        FsError_UnsupportedWriteForPartitionFileSystem = 0x31CC02,
        FsError_UnsupportedCommitProvisionallyForPartitionFileSystem = 0x31CE02,
        FsError_UnsupportedWriteForPartitionFile = 0x31D002,
        FsError_UnsupportedOperateRangeForPartitionFile = 0x31D202,
        FsError_UnsupportedOperateRangeForTmFileSystemFile = 0x31D402,
        FsError_UnsupportedWriteForSaveDataInternalStorageFileSystem = 0x31D602,
        FsError_UnsupportedCommitProvisionallyForApplicationTemporaryFileSystem = 0x31DC02,
        FsError_UnsupportedCommitProvisionallyForSaveDataFileSystem = 0x31DE02,
        FsError_UnsupportedCommitProvisionallyForDirectorySaveDataFileSystem = 0x31E002,
        FsError_UnsupportedWriteForZeroBitmapHashStorageFile = 0x31E202,
        FsError_UnsupportedSetSizeForZeroBitmapHashStorageFile = 0x31E402,
        FsError_NcaExternalKeyUnregisteredDeprecated = 0x326602,
        FsError_FileNotClosed = 0x326E02,
        FsError_DirectoryNotClosed = 0x327002,
        FsError_WriteModeFileNotClosed = 0x327202,
        FsError_AllocatorAlreadyRegistered = 0x327402,
        FsError_DefaultAllocatorAlreadyUsed = 0x327602,
        FsError_AllocatorAlignmentViolation = 0x327A02,
        FsError_UserNotExist = 0x328202,
        FsError_FileNotFound = 0x339402,
        FsError_DirectoryNotFound = 0x339602,
        FsError_MappingTableFull = 0x346402,
        FsError_OpenCountLimit = 0x346A02,
        FsError_MultiCommitFileSystemLimit = 0x346C02,
        FsError_MapFull = 0x353602,
        FsError_NotMounted = 0x35F202,
        FsError_DbmKeyNotFound = 0x3DBC02,
        FsError_DbmFileNotFound = 0x3DBE02,
        FsError_DbmDirectoryNotFound = 0x3DC002,
        FsError_DbmAlreadyExists = 0x3DC402,
        FsError_DbmKeyFull = 0x3DC602,
        FsError_DbmDirectoryEntryFull = 0x3DC802,
        FsError_DbmFileEntryFull = 0x3DCA02,
        FsError_DbmInvalidOperation = 0x3DD402,
    };

namespace fs
{
    using Path = std::filesystem::path;

    inline bool Exists(const Path& path)
    {
        return std::filesystem::exists(path);
    }

    inline bool Remove(const Path& path)
    {
        return std::filesystem::remove(path);
    }

    inline bool MakeDir(const Path& path)
    {
        return std::filesystem::create_directory(path);
    }

    inline bool MakeDirs(const Path& path)
    {
        return std::filesystem::create_directories(path);
    }

    class IFileSystem;

    class IFile
    {
        friend IFileSystem;

        private:
            FsFile m_file;

            IFile(FsFile& file);

        public:
            // Don't allow copying, or garbage may be closed by the destructor
            IFile& operator=(const IFile&) = delete;
            IFile(const IFile&) = delete;

            ~IFile();

            void Read(u64 offset, void* buf, size_t size);
            void Write(u64 offset, const void* buf, size_t size);
            s64 GetSize();
    };

    class IDirectory
    {
        friend IFileSystem;

        private:
            FsDir m_dir;

            IDirectory(FsDir& dir);

        public:
            // Don't allow copying, or garbage may be closed by the destructor
            IDirectory& operator=(const IDirectory&) = delete;
            IDirectory(const IDirectory&) = delete;

            ~IDirectory();

            void Read(s64 inval, FsDirectoryEntry* buf, size_t numEntries);
            u64 GetEntryCount();
    };

    class IFileSystem
    {
        private:
            FsFileSystem m_fileSystem;

        public:
            // Don't allow copying, or garbage may be closed by the destructor
            IFileSystem& operator=(const IFileSystem&) = delete;
            IFileSystem(const IFileSystem&) = delete;

            IFileSystem();
            ~IFileSystem();

            void OpenFileSystemWithId(std::string path, FsFileSystemType fileSystemType, u64 titleId);
            void CloseFileSystem();

            IFile OpenFile(std::string path, u32 openMode = FsOpenMode_Read);
            IDirectory OpenDirectory(std::string path, u32 flags);
            std::string GetFileNameFromExtension(std::string path, std::string extension);
    };

    s64 GetFreeSpaceSize(FsContentStorageId id);
    s64 GetTotalSpaceSize(FsContentStorageId id);
    std::string FormatSizeString(s64 size);
    std::vector<Path> GetDirectoryFiles(const std::string &dir, const std::vector<std::string> &extensions);
    std::vector<Path> GetDirsAtPath(const std::string &dir);
}
}
