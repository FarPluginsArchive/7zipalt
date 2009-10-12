// PluginRead.cpp

#include "StdAfx.h"

#include "Plugin.h"

#include "Messages.h"

#include "Common/StringConvert.h"

#include "Windows/FileName.h"
#include "Windows/FileFind.h"
#include "Windows/FileDir.h"
#include "Windows/Defs.h"

#include "../Common/ZipRegistry.h"

#include "ExtractEngine.h"

using namespace NFar;
using namespace NWindows;

static const farChar *kHelpTopicExtrFromSevenZip =  _F("Extract");

static const farChar kDirDelimiter = _F('\\');

static const farChar *kExractPathHistoryName  = _F("7-ZipExtractPath"); 

HRESULT CPlugin::ExtractFiles(
    bool decompressAllItems,
    const UInt32 *indices,
    UINT32 numIndices,
    bool silent,
    NExtract::NPathMode::EEnum pathMode,
    NExtract::NOverwriteMode::EEnum overwriteMode,
    const UString &destPath,
    bool passwordIsDefined, const UString &password)
{
  CScreenRestorer screenRestorer;
  CProgressBox progressBox;

  screenRestorer.Save();

  CProgressBox *progressBoxPointer = &progressBox;
  progressBox.Init(g_StartupInfo.GetMsgString(NMessageID::kExtracting), 48, silent);

  CExtractCallBackImp *extractCallbackSpec = new CExtractCallBackImp;
  CMyComPtr<IFolderArchiveExtractCallback> extractCallback(extractCallbackSpec);
  
  extractCallbackSpec->Init(
      CP_OEMCP,
      progressBoxPointer,
      /*
      GetDefaultName(m_FileName, m_ArchiverInfo.Extension),
      m_FileInfo.MTime, m_FileInfo.Attributes,
      */
      passwordIsDefined, password);

  if (decompressAllItems)
    return m_ArchiveHandler->Extract(pathMode, overwriteMode,
        destPath, BoolToInt(false), extractCallback);
  else
  {
    CMyComPtr<IArchiveFolder> archiveFolder;
    _folder.QueryInterface(IID_IArchiveFolder, &archiveFolder);

    return archiveFolder->Extract(indices, numIndices, pathMode, overwriteMode,
        destPath, BoolToInt(false), extractCallback);
  }
}

#ifdef _UNICODE
NFileOperationReturnCode::EEnum CPlugin::GetFiles(struct PluginPanelItem *panelItems, 
                                            int itemsNumber, int move, const farChar **_aDestPath, int opMode)
#else
NFileOperationReturnCode::EEnum CPlugin::GetFiles(struct PluginPanelItem *panelItems, 
                                                  int itemsNumber, int move, farChar *_aDestPath, int opMode)
#endif
{
  return GetFilesReal(panelItems, itemsNumber, move,
      _aDestPath, opMode, (opMode & OPM_SILENT) == 0);
}

#ifdef _UNICODE
NFileOperationReturnCode::EEnum CPlugin::GetFilesReal(struct PluginPanelItem *panelItems, 
                                                      int itemsNumber, int move, const farChar **_aDestPath, int opMode, bool showBox)
#else
NFileOperationReturnCode::EEnum CPlugin::GetFilesReal(struct PluginPanelItem *panelItems, 
                                                      int itemsNumber, int move, farChar *_aDestPath, int opMode, bool showBox)
#endif

{
  if(move != 0)
  {
    g_StartupInfo.ShowMessage(NMessageID::kMoveIsNotSupported);
    return NFileOperationReturnCode::kError;
  }
#ifdef _UNICODE
  static CSysString destPath;
  destPath = *_aDestPath;
#else
  CSysString destPath = _aDestPath;
#endif
  NFile::NName::NormalizeDirPathPrefix(destPath);

  bool extractSelectedFiles = true;
  
  NExtract::CInfo extractionInfo;
  extractionInfo.PathMode = NExtract::NPathMode::kCurrentPathnames;
  extractionInfo.OverwriteMode = NExtract::NOverwriteMode::kWithoutPrompt;

  bool silent = (opMode & OPM_SILENT) != 0;
  bool decompressAllItems = false;
  UString password = Password;
  bool passwordIsDefined = PasswordIsDefined;

  if (!silent)
  {
    const int kPathIndex = 2;

    ReadExtractionInfo(extractionInfo);

    const int kPathModeRadioIndex = 4;
    const int kOverwriteModeRadioIndex = kPathModeRadioIndex + 4;
    const int kNumOverwriteOptions = 6;
    const int kFilesModeIndex = kOverwriteModeRadioIndex + kNumOverwriteOptions;
    const int kXSize = 76;
    const int kYSize = 19;
    const int kPasswordYPos = 12;
    
    const int kXMid = kXSize / 2;

    CSysString oemPassword;
#ifdef _UNICODE
    oemPassword = password;
#else
    oemPassword = UnicodeStringToMultiByte(password, CP_OEMCP);
#endif
    
    struct CInitDialogItem initItems[]={
      { DI_DOUBLEBOX, 3, 1, kXSize - 4, kYSize - 2, false, false, 0, false, NMessageID::kExtractTitle, NULL, NULL },
      { DI_TEXT, 5, 2, 0, 0, false, false, 0, false, NMessageID::kExtractTo, NULL, NULL },

      { DI_EDIT, 5, 3, kXSize - 6, 3, true, false, DIF_HISTORY, false, -1, destPath, kExractPathHistoryName},

      { DI_SINGLEBOX, 4, 5, kXMid - 2, 5 + 4, false, false, 0, false, NMessageID::kExtractPathMode, NULL, NULL },
      { DI_RADIOBUTTON, 6, 6, 0, 0, false,
          extractionInfo.PathMode == NExtract::NPathMode::kFullPathnames,
          DIF_GROUP, false, NMessageID::kExtractPathFull, NULL, NULL },
      { DI_RADIOBUTTON, 6, 7, 0, 0, false,
          extractionInfo.PathMode == NExtract::NPathMode::kCurrentPathnames,
          0, false, NMessageID::kExtractPathCurrent, NULL, NULL },
      { DI_RADIOBUTTON, 6, 8, 0, 0, false,
          extractionInfo.PathMode == NExtract::NPathMode::kNoPathnames,
          false, 0, NMessageID::kExtractPathNo, NULL, NULL },
      
      { DI_SINGLEBOX, kXMid, 5, kXSize - 6, 5 + kNumOverwriteOptions, false, false, 0, false, NMessageID::kExtractOwerwriteMode, NULL, NULL },
      { DI_RADIOBUTTON, kXMid + 2, 6, 0, 0, false,
          extractionInfo.OverwriteMode == NExtract::NOverwriteMode::kAskBefore,
          DIF_GROUP, false, NMessageID::kExtractOwerwriteAsk, NULL, NULL },
      { DI_RADIOBUTTON, kXMid + 2, 7, 0, 0, false,
          extractionInfo.OverwriteMode == NExtract::NOverwriteMode::kWithoutPrompt,
          0, false, NMessageID::kExtractOwerwritePrompt, NULL, NULL },
      { DI_RADIOBUTTON, kXMid + 2, 8, 0, 0, false,
          extractionInfo.OverwriteMode == NExtract::NOverwriteMode::kSkipExisting,
          0, false, NMessageID::kExtractOwerwriteSkip, NULL, NULL },
      { DI_RADIOBUTTON, kXMid + 2, 9, 0, 0, false,
          extractionInfo.OverwriteMode == NExtract::NOverwriteMode::kAutoRename,
          0, false, NMessageID::kExtractOwerwriteAutoRename, NULL, NULL },
      { DI_RADIOBUTTON, kXMid + 2, 10, 0, 0, false,
          extractionInfo.OverwriteMode == NExtract::NOverwriteMode::kAutoRenameExisting,
          0, false, NMessageID::kExtractOwerwriteAutoRenameExisting, NULL, NULL },
      
      { DI_SINGLEBOX, 4, 10, kXMid- 2, 10 + 3, false, false, 0, false, NMessageID::kExtractFilesMode, NULL, NULL },
      { DI_RADIOBUTTON, 6, 11, 0, 0, false, true, DIF_GROUP, false, NMessageID::kExtractFilesSelected, NULL, NULL },
      { DI_RADIOBUTTON, 6, 12, 0, 0, false, false, 0, false, NMessageID::kExtractFilesAll, NULL, NULL },
      
      { DI_SINGLEBOX, kXMid, kPasswordYPos, kXSize - 6, kPasswordYPos + 2, false, false, 0, false, NMessageID::kExtractPassword, NULL, NULL },
      { DI_PSWEDIT, kXMid + 2, kPasswordYPos + 1, kXSize - 8, 12, false, false, 0, false, -1, oemPassword, NULL},
      
      { DI_TEXT, 3, kYSize - 4, 0, 0, false, false, DIF_BOXCOLOR|DIF_SEPARATOR, false, -1, _F(""), NULL  },  
      
      
      { DI_BUTTON, 0, kYSize - 3, 0, 0, false, false, DIF_CENTERGROUP, true, NMessageID::kExtractExtract, NULL, NULL  },
      { DI_BUTTON, 0, kYSize - 3, 0, 0, false, false, DIF_CENTERGROUP, false, NMessageID::kExtractCancel, NULL, NULL  }
    };
   
    const int kNumDialogItems = sizeof(initItems) / sizeof(initItems[0]);
    const int kOkButtonIndex = kNumDialogItems - 2;
    const int kPasswordIndex = kNumDialogItems - 4;

    FarDialogItem dialogItems[kNumDialogItems];
    g_StartupInfo.InitDialogItems(initItems, dialogItems, kNumDialogItems);
#ifdef _UNICODE
    HANDLE hDlg = 0;
#endif
    for (;;)
    {
#ifdef _UNICODE
      dialogItems[kPathIndex].PtrData = destPath;
      int askCode = g_StartupInfo.ShowDialog(kXSize, kYSize, kHelpTopicExtrFromSevenZip, dialogItems, kNumDialogItems, hDlg);
      if (askCode != kOkButtonIndex)
      {
        g_StartupInfo.DialogFree(hDlg);
        return NFileOperationReturnCode::kInterruptedByUser;
      }
      destPath = g_StartupInfo.GetItemData(hDlg, kPathIndex);
#else
      lstrcpy(dialogItems[kPathIndex].Data, destPath);
      int askCode = g_StartupInfo.ShowDialog(kXSize, kYSize, kHelpTopicExtrFromSevenZip, dialogItems, kNumDialogItems);
      if (askCode != kOkButtonIndex)
        return NFileOperationReturnCode::kInterruptedByUser;
      destPath = dialogItems[kPathIndex].Data;
#endif
      destPath.Trim();
      if (destPath.IsEmpty())
        destPath = _F(".");
#ifdef _UNICODE
      int pos;
      if (g_StartupInfo.GetFullPathName(UString(destPath), destPath, pos))
#endif
        break;
      g_StartupInfo.ShowMessage(NMessageID::kSpecifyDirectoryPath);
#ifdef _UNICODE
      g_StartupInfo.DialogFree(hDlg);
#endif
    }

#ifdef _UNICODE
    if (g_StartupInfo.GetItemSelected(hDlg, kPathModeRadioIndex))
      extractionInfo.PathMode = NExtract::NPathMode::kFullPathnames;
    else if (g_StartupInfo.GetItemSelected(hDlg, kPathModeRadioIndex + 1))
      extractionInfo.PathMode = NExtract::NPathMode::kCurrentPathnames;
    else if (g_StartupInfo.GetItemSelected(hDlg, kPathModeRadioIndex + 2))
      extractionInfo.PathMode = NExtract::NPathMode::kNoPathnames;
    else
      throw 31806;

    if (g_StartupInfo.GetItemSelected(hDlg, kOverwriteModeRadioIndex))
      extractionInfo.OverwriteMode = NExtract::NOverwriteMode::kAskBefore;
    else if (g_StartupInfo.GetItemSelected(hDlg, kOverwriteModeRadioIndex + 1))
      extractionInfo.OverwriteMode = NExtract::NOverwriteMode::kWithoutPrompt;
    else if (g_StartupInfo.GetItemSelected(hDlg, kOverwriteModeRadioIndex + 2))
      extractionInfo.OverwriteMode = NExtract::NOverwriteMode::kSkipExisting;
    else if (g_StartupInfo.GetItemSelected(hDlg, kOverwriteModeRadioIndex + 3))
      extractionInfo.OverwriteMode = NExtract::NOverwriteMode::kAutoRename;
    else if (g_StartupInfo.GetItemSelected(hDlg, kOverwriteModeRadioIndex + 4))
      extractionInfo.OverwriteMode = NExtract::NOverwriteMode::kAutoRenameExisting;
    else
      throw 31806;

    if (g_StartupInfo.GetItemSelected(hDlg, kFilesModeIndex))
      decompressAllItems = false;
    else if (g_StartupInfo.GetItemSelected(hDlg, kFilesModeIndex + 1))
      decompressAllItems = true;
    else
      throw 31806;

    SaveExtractionInfo(extractionInfo);

    if (g_StartupInfo.GetItemSelected(hDlg, kFilesModeIndex))
      extractSelectedFiles = true;
    else if (g_StartupInfo.GetItemSelected(hDlg, kFilesModeIndex + 1))
      extractSelectedFiles = false;
    else
      throw 31806;

    password = g_StartupInfo.GetItemData(hDlg, kPasswordIndex);
    g_StartupInfo.DialogFree(hDlg);
#else
    if (dialogItems[kPathModeRadioIndex].Selected)
      extractionInfo.PathMode = NExtract::NPathMode::kFullPathnames;
    else if (dialogItems[kPathModeRadioIndex + 1].Selected)
      extractionInfo.PathMode = NExtract::NPathMode::kCurrentPathnames;
    else if (dialogItems[kPathModeRadioIndex + 2].Selected)
      extractionInfo.PathMode = NExtract::NPathMode::kNoPathnames;
    else
      throw 31806;

    if (dialogItems[kOverwriteModeRadioIndex].Selected)
      extractionInfo.OverwriteMode = NExtract::NOverwriteMode::kAskBefore;
    else if (dialogItems[kOverwriteModeRadioIndex + 1].Selected)
      extractionInfo.OverwriteMode = NExtract::NOverwriteMode::kWithoutPrompt;
    else if (dialogItems[kOverwriteModeRadioIndex + 2].Selected)
      extractionInfo.OverwriteMode = NExtract::NOverwriteMode::kSkipExisting;
    else if (dialogItems[kOverwriteModeRadioIndex + 3].Selected)
      extractionInfo.OverwriteMode = NExtract::NOverwriteMode::kAutoRename;
    else if (dialogItems[kOverwriteModeRadioIndex + 4].Selected)
      extractionInfo.OverwriteMode = NExtract::NOverwriteMode::kAutoRenameExisting;
    else
      throw 31806;

    if (dialogItems[kFilesModeIndex].Selected)
      decompressAllItems = false;
    else if (dialogItems[kFilesModeIndex + 1].Selected)
      decompressAllItems = true;
    else
      throw 31806;

    SaveExtractionInfo(extractionInfo);

    if (dialogItems[kFilesModeIndex].Selected)
      extractSelectedFiles = true;
    else if (dialogItems[kFilesModeIndex + 1].Selected)
      extractSelectedFiles = false;
    else
      throw 31806;

    oemPassword = dialogItems[kPasswordIndex].Data;
    password = MultiByteToUnicodeString(oemPassword, CP_OEMCP); 
#endif

    passwordIsDefined = !password.IsEmpty();
  }

  NFile::NDirectory::CreateComplexDirectory(destPath);

  CRecordVector<UInt32> indices;
  indices.Reserve(itemsNumber);
  if (!panelItems)
  {
#ifdef _UNICODE
    for (int i = 0; i < itemsNumber; i++)
    {
      indices.Add(g_StartupInfo.GetActivePanelUserData(true, i));
    }
#endif
  }
  else
    for (int i = 0; i < itemsNumber; i++)
      indices.Add((UInt32)panelItems[i].UserData);

#ifdef _UNICODE
  *_aDestPath = destPath;
  HRESULT result = ExtractFiles(decompressAllItems, &indices.Front(), itemsNumber, 
    !showBox, extractionInfo.PathMode, extractionInfo.OverwriteMode, 
    destPath, passwordIsDefined, password);
#else
  _aDestPath[0] = 0;
  lstrcpy(_aDestPath, destPath);
  HRESULT result = ExtractFiles(decompressAllItems, &indices.Front(), itemsNumber, 
    !showBox, extractionInfo.PathMode, extractionInfo.OverwriteMode, 
    MultiByteToUnicodeString(destPath, CP_OEMCP), 
    passwordIsDefined, password);
#endif

  if (result != S_OK)
  {
    if (result == E_ABORT)
      return NFileOperationReturnCode::kInterruptedByUser;
    ShowErrorMessage(result);
    return NFileOperationReturnCode::kError;
  }

  return NFileOperationReturnCode::kSuccess;
}
