// SevenZip/ Messages.h

#ifndef __SEVENZIP_MESSAGES_H
#define __SEVENZIP_MESSAGES_H

namespace NMessageID {

enum EEnum
{
  kOk,
  kCancel,

  kWarning,
  kError,

  kArchiveType,

  kProperties,

  kYes,
  kNo,
  
  kPath,
  kName,
  kExtension,
  kIsFolder,
  kSize,
  kPackSize,
  kAttributes,
  kCTime,
  kATime,
  kMTime,
  kSolid,
  kCommented,
  kEncrypted,
  kSplitBefore,
  kSplitAfter,
  kDictionarySize,
  kCRC,
  kType,
  kAnti,
  kMethod,
  kHostOS,
  kFileSystem,
  kUser,
  kGroup,
  kBlock,
  kComment,
  kPosition,
  kNumSubFolders,
  kNumSubFiles,
  kUnpackVer,
  kVolume,
  kIsVolume,
  kOffset,
  kLinks,
  kNumBlocks,
  kNumVolumes,

  kBit64,
  kBigEndian,
  kCpu,
  kPhySize,
  kHeadersSize,
  kChecksum,
  kCharacts,
  kVa,
  kId,
  kShortName,
  kCreatorApp,
  kSectorSize,
  kPosixAttrib,
  kLink,

  kTotalSize,
  kFreeSpace,
  kClusterSize,
  kLabel,

  kGetPasswordTitle,
  kEnterPasswordForFile,

  kExtractTitle,
  kExtractTo,

  kExtractPathMode,
  kExtractPathFull,
  kExtractPathCurrent,
  kExtractPathNo,

  kExtractOwerwriteMode,
  kExtractOwerwriteAsk,
  kExtractOwerwritePrompt,
  kExtractOwerwriteSkip,
  kExtractOwerwriteAutoRename,
  kExtractOwerwriteAutoRenameExisting,

  kExtractFilesMode,
  kExtractFilesSelected,
  kExtractFilesAll,

  kExtractPassword,
  kExtractPassword2,
  kExtractPasswordsNotSame,

  kExtractExtract,
  kExtractCancel,

  kExtractCanNotOpenOutputFile,

  kExtractUnsupportedMethod,
  kExtractCRCFailed,
  kExtractDataError,
  kExtractCRCFailedEncrypted,
  kExtractDataErrorEncrypted,

  kOverwriteTitle,
  kOverwriteMessage1,
  kOverwriteMessageWouldYouLike,
  kOverwriteMessageWithtTisOne,

  kOverwriteBytes,
  kOverwriteModifiedOn,

  kOverwriteYes,
  kOverwriteYesToAll,
  kOverwriteNo,
  kOverwriteNoToAll,
  kOverwriteAutoRename,
  kOverwriteCancel,

  kUpdateNotSupportedForThisArchive,

  kDeleteTitle,
  kDeleteFile,
  kDeleteFiles,
  kDeleteNumberOfFiles,
  kDeleteDelete,
  kDeleteCancel,

  kUpdateTitle,
  kUpdateAddToArchive,

  kUpdateLevel,
  kUpdateLevelStore,
  kUpdateLevelFastest,
  kUpdateLevelFast,
  kUpdateLevelNormal,
  kUpdateLevelMaximum,
  kUpdateLevelUltra,

  kUpdateMethodLZMA,
  kUpdateMethodLZMA2,
  kUpdateMethodPPMD,

  kUpdateMode,
  kUpdateModeAdd,
  kUpdateModeUpdate,
  kUpdateModeFreshen,
  kUpdateModeSynchronize,

  kAddExtension,

  kUpdateAdd,
  kUpdateSelectArchiver,

  kUpdateSelectArchiverMenuTitle,

  // kArcReadFiles,
  
  kWaitTitle,
  
  kReading,
  kExtracting,
  kDeleting,
  kUpdating,
  
  // kReadingList,

  kMoveIsNotSupported,

  kOpenArchiveMenuString,
  kCreateArchiveMenuString,
  kSelectArchiveFormat,
  kAutoArchiveFormat,
  kDetectArchiveFormat,
  kArchiveFormatSeparator,

  kConfigTitle,

  kConfigPluginEnabled,
  kConfigUseMasks,
  kConfigDisabledFormats,
  kConfigMaxCheckSize1,
  kConfigMaxCheckSize2,
  kConfigDllInfo,
  kConfigDllPath,
  kConfigDllVersion,
  kConfigDllNotFound,
  kConfigAvailableMasks,
  kConfigAvailableFormats,

  kSpecifyDirectoryPath,
  kNoItems,
  kCantLoad7Zip,
  kDirWithSuchName,
  kExistingArchDiffersSpecified,
};

}

#endif