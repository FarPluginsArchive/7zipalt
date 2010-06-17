// PluginWrite.cpp

#include "StdAfx.h"

#include "Plugin.h"

#include "Common/Wildcard.h"
#include "Common/StringConvert.h"
#include "Common/IntToString.h"

#include "Windows/FileDir.h"
#include "Windows/FileName.h"
#include "Windows/FileFind.h"
#include "Windows/Defs.h"
#include "Windows/PropVariant.h"

#include "../Common/ZipRegistry.h"
#include "../Common/WorkDir.h"
#include "../Common/OpenArchive.h"
#include "../Common/SetProperties.h"

#include "../Agent/Agent.h"

#include "ProgressBox.h"
#include "Messages.h"
#include "UpdateCallback100.h"

using namespace NWindows;
using namespace NFile;
using namespace NDirectory;
using namespace NFar;

using namespace NUpdateArchive;

static const farChar *kHelpTopic =  _F("Update");

static LPCWSTR kTempArcivePrefix = L"7zA";

static const farChar *kArchiveHistoryKeyName = _F("7-ZipArcName"); 

static UINT32 g_LevelMap[] = { 0, 1, 3, 5, 7, 9 };

static wchar_t* g_MethodMap[] = { L"LZMA", L"LZMA2", L"PPMD" };

static UString ConvertToString(UInt64 num)
{
  wchar_t s[32];
  ConvertUInt64ToString(num, s);
  return s;
}

static HRESULT SetOutProperties(IOutFolderArchive *outArchive, const CCompressionInfo& compressionInfo, bool is_7z)
{
  CProperty prop;
  CObjectVector<CProperty> properties;
  prop.Name = L"x";
  prop.Value = ConvertToString(compressionInfo.Level);
  properties.Add(prop);
  if (is_7z) {
    prop.Name = L"0";
    prop.Value = compressionInfo.Method;
    properties.Add(prop);
  }
  return SetProperties(outArchive, properties);
}

NFileOperationReturnCode::EEnum CPlugin::PutFiles(
#ifdef _UNICODE
  const wchar_t* srcPath,
#endif
  struct PluginPanelItem *panelItems, int numItems,
  int moveMode, int opMode)
{
  if(moveMode != 0)
  {
    g_StartupInfo.ShowMessage(NMessageID::kMoveIsNotSupported);
    return NFileOperationReturnCode::kError;
  }
  if (numItems == 0)
    return NFileOperationReturnCode::kError;

  const int kYSize = 15;
  const int kXMid = 38;

  CCompressionInfo compressionInfo;
  compressionInfo.Load();

  bool is_7z = _archiveTypeName == L"7z";

  int levelIndex = 0;
  int i;
  for (i = ARRAYSIZE(g_LevelMap) - 1; i >= 0; i--)
    if (compressionInfo.Level >= g_LevelMap[i])
    {
      levelIndex = i;
      break;
    }

    int methodIndex = 0;
    for (i = ARRAYSIZE(g_MethodMap) - 1; i >= 0; i--)
      if (compressionInfo.Method == g_MethodMap[i])
      {
        methodIndex = i;
        break;
      }

  const int kLevelRadioIndex = 2;
  const int kMethodRadioIndex = kLevelRadioIndex + 6;
  const int kModeRadioIndex = kMethodRadioIndex + 4;
  const int kPassword = kModeRadioIndex + 5;
  const int kPassword2 = kPassword + 2;

  UString password;
  UString password2;

  struct CInitDialogItem initItems[]={
    { DI_DOUBLEBOX, 3, 1, 72, kYSize - 2, false, false, 0, false, NMessageID::kUpdateTitle, NULL, NULL },
    { DI_SINGLEBOX, 4, 2, kXMid - 2, 2 + 7, false, false, 0, false, NMessageID::kUpdateLevel, NULL, NULL },
    { DI_RADIOBUTTON, 6, 3, 0, 0, levelIndex == 0, levelIndex == 0,
        DIF_GROUP, false, NMessageID::kUpdateLevelStore, NULL, NULL },
    { DI_RADIOBUTTON, 6, 4, 0, 0, levelIndex == 1, levelIndex == 1,
        0, false, NMessageID::kUpdateLevelFastest, NULL, NULL },
    { DI_RADIOBUTTON, 6, 5, 0, 0, levelIndex == 2, levelIndex == 2,
        0, false, NMessageID::kUpdateLevelFast, NULL, NULL },
    { DI_RADIOBUTTON, 6, 6, 0, 0, levelIndex == 3, levelIndex == 3,
        0, false, NMessageID::kUpdateLevelNormal, NULL, NULL },
    { DI_RADIOBUTTON, 6, 7, 0, 0, levelIndex == 4, levelIndex == 4,
        0, false, NMessageID::kUpdateLevelMaximum, NULL, NULL },
    { DI_RADIOBUTTON, 6, 8, 0, 0, levelIndex == 5, levelIndex == 5,
        0, false, NMessageID::kUpdateLevelUltra, NULL, NULL },

    { DI_RADIOBUTTON, 26, 5, 0, 0, false, methodIndex == 0, DIF_GROUP | (is_7z ? 0 : DIF_HIDDEN), false, NMessageID::kUpdateMethodLZMA, NULL, NULL },
    { DI_RADIOBUTTON, 26, 6, 0, 0, false, methodIndex == 1, is_7z ? 0 : DIF_HIDDEN, false, NMessageID::kUpdateMethodLZMA2, NULL, NULL },
    { DI_RADIOBUTTON, 26, 7, 0, 0, false, methodIndex == 2, is_7z ? 0 : DIF_HIDDEN, false, NMessageID::kUpdateMethodPPMD, NULL, NULL },
    
    { DI_SINGLEBOX, kXMid, 2, 70, 2 + 5, false, false, 0, false, NMessageID::kUpdateMode, NULL, NULL },
    { DI_RADIOBUTTON, kXMid + 2, 3, 0, 0, false, true,
        DIF_GROUP, false, NMessageID::kUpdateModeAdd, NULL, NULL },
    { DI_RADIOBUTTON, kXMid + 2, 4, 0, 0, false, false,
        0, false, NMessageID::kUpdateModeUpdate, NULL, NULL },
    { DI_RADIOBUTTON, kXMid + 2, 5, 0, 0, false, false,
        0, false, NMessageID::kUpdateModeFreshen, NULL, NULL },
    { DI_RADIOBUTTON, kXMid + 2, 6, 0, 0, false, false,
        0, false, NMessageID::kUpdateModeSynchronize, NULL, NULL },

    { DI_TEXT,     5, 10, 0, 0, false, false, 0, false, NMessageID::kExtractPassword, NULL, NULL },  
    { DI_PSWEDIT, 15, 10, 30, 3, true, false, 0, false, -1, NULL, NULL},
    { DI_TEXT,    32, 10, 0, 0, false, false, 0, false, NMessageID::kExtractPassword2, NULL, NULL  },  
    { DI_PSWEDIT, 42, 10, 57, 3, true, false, 0, false, -1, NULL, NULL},

    { DI_TEXT, 3, kYSize - 4, 0, 0, false, false, DIF_BOXCOLOR|DIF_SEPARATOR, false, -1, _F(""), NULL  },  

    { DI_BUTTON, 0, kYSize - 3, 0, 0, false, false, DIF_CENTERGROUP, true, NMessageID::kUpdateAdd, NULL, NULL  },
    { DI_BUTTON, 0, kYSize - 3, 0, 0, false, false, DIF_CENTERGROUP, false, NMessageID::kCancel, NULL, NULL  }
  };
  
  const int kNumDialogItems = sizeof(initItems) / sizeof(initItems[0]);
  const int kOkButtonIndex = kNumDialogItems - 2;
  FarDialogItem dialogItems[kNumDialogItems];
  g_StartupInfo.InitDialogItems(initItems, dialogItems, kNumDialogItems);
#ifdef _UNICODE
  HANDLE hDlg = 0;
  int askCode = g_StartupInfo.ShowDialog(76, kYSize, kHelpTopic, dialogItems, kNumDialogItems, hDlg);
  password =  g_StartupInfo.GetItemData(hDlg, kPassword);
  password2 =  g_StartupInfo.GetItemData(hDlg, kPassword2);
#else
  int askCode = g_StartupInfo.ShowDialog(76, kYSize, kHelpTopic, dialogItems, kNumDialogItems);
  AString p = dialogItems[kPassword].Data;
  password = MultiByteToUnicodeString(p, CP_OEMCP);
  p = dialogItems[kPassword2].Data;
  password2 = MultiByteToUnicodeString(p, CP_OEMCP);
#endif

  if (!password.IsEmpty() && password.Compare(password2) != 0)
  {
    g_StartupInfo.ShowMessage(NMessageID::kExtractPasswordsNotSame);
    return NFileOperationReturnCode::kError;
  }

  if (askCode != kOkButtonIndex)
    return NFileOperationReturnCode::kInterruptedByUser;

  compressionInfo.Level = g_LevelMap[0];
#ifdef _UNICODE
  for (i = 0; i < ARRAYSIZE(g_LevelMap); i++)
    if (g_StartupInfo.GetItemSelected(hDlg, kLevelRadioIndex + i))
      compressionInfo.Level = g_LevelMap[i];

  for (i = 0; i < ARRAYSIZE(g_MethodMap); i++)
    if (g_StartupInfo.GetItemSelected(hDlg, kMethodRadioIndex + i))
      compressionInfo.Method = g_MethodMap[i];

  const CActionSet *actionSet;

  if (g_StartupInfo.GetItemSelected(hDlg, kModeRadioIndex))
    actionSet = &kAddActionSet;
  else if (g_StartupInfo.GetItemSelected(hDlg, kModeRadioIndex + 1))
    actionSet = &kUpdateActionSet;
  else if (g_StartupInfo.GetItemSelected(hDlg, kModeRadioIndex + 2))
    actionSet = &kFreshActionSet;
  else if (g_StartupInfo.GetItemSelected(hDlg, kModeRadioIndex + 3))
    actionSet = &kSynchronizeActionSet;
  else
    throw 51751;

  g_StartupInfo.DialogFree(hDlg);
#else
  for (i = 0; i < ARRAYSIZE(g_LevelMap); i++)
    if (dialogItems[kLevelRadioIndex + i].Selected)
      compressionInfo.Level = g_LevelMap[i];

    for (i = 0; i < ARRAYSIZE(g_MethodMap); i++)
      if (dialogItems[kMethodRadioIndex + i].Selected)
        compressionInfo.Method = g_MethodMap[i];

  const CActionSet *actionSet;

  if (dialogItems[kModeRadioIndex].Selected)
    actionSet = &kAddActionSet;
  else if (dialogItems[kModeRadioIndex + 1].Selected)
    actionSet = &kUpdateActionSet;
  else if (dialogItems[kModeRadioIndex + 2].Selected)
    actionSet = &kFreshActionSet;
  else if (dialogItems[kModeRadioIndex + 3].Selected)
    actionSet = &kSynchronizeActionSet;
  else
    throw 51751;
#endif

  compressionInfo.Save();

  NWorkDir::CInfo workDirInfo;
  ReadWorkDirInfo(workDirInfo);
  UString workDir = GetWorkDir(workDirInfo, m_FileName);
  CreateComplexDirectory(workDir);

  CTempFileW tempFile;
  UString tempFileName;
  if (tempFile.Create(workDir, kTempArcivePrefix, tempFileName) == 0)
    return NFileOperationReturnCode::kError;

  CProgressBox progressBox;
  progressBox.Init(g_StartupInfo.GetMsgString(NMessageID::kUpdating), 48, opMode & OPM_SILENT);
 
  ////////////////////////////
  // Save FolderItem;
  UStringVector aPathVector;
  GetPathParts(aPathVector);

#ifdef _UNICODE
  UString panelPath(srcPath);
  NName::NormalizeDirPathPrefix(panelPath);
#endif

  UStringVector fileNames;
  fileNames.Reserve(numItems);
  for(i = 0; i < numItems; i++)
  {
#ifdef _UNICODE
    fileNames.Add(panelPath + panelItems[i].FindData.lpwszFileName);
#else
    fileNames.Add(MultiByteToUnicodeString(panelItems[i].FindData.cFileName, CP_OEMCP));
#endif
  }
  CRecordVector<const wchar_t *> fileNamePointers;
  fileNamePointers.Reserve(numItems);
  for(i = 0; i < numItems; i++)
    fileNamePointers.Add(fileNames[i]);

  CMyComPtr<IOutFolderArchive> outArchive;
  HRESULT result = m_ArchiveHandler.QueryInterface(IID_IOutFolderArchive, &outArchive);
  if(result != S_OK)
  {
    g_StartupInfo.ShowMessage(NMessageID::kUpdateNotSupportedForThisArchive);
    return NFileOperationReturnCode::kError;
  }
  outArchive->SetFolder(_folder);

  outArchive->SetFiles(L"", &fileNamePointers.Front(), fileNamePointers.Size());
  BYTE actionSetByte[NUpdateArchive::NPairState::kNumValues];
  for (i = 0; i < NUpdateArchive::NPairState::kNumValues; i++)
    actionSetByte[i] = (BYTE)actionSet->StateActions[i];

  CUpdateCallback100Imp *updateCallbackSpec = new CUpdateCallback100Imp;
  CMyComPtr<IFolderArchiveUpdateCallback> updateCallback(updateCallbackSpec );

  updateCallbackSpec->Init(/* m_ArchiveHandler, */ &progressBox, !password.IsEmpty(), password);

  if (SetOutProperties(outArchive, compressionInfo, is_7z) != S_OK)
    return NFileOperationReturnCode::kError;

  result = outArchive->DoOperation2(tempFileName, actionSetByte, NULL, updateCallback);
  updateCallback.Release();
  outArchive.Release();

  if (result != S_OK)
  {
    ShowErrorMessage(result);
    return NFileOperationReturnCode::kError;
  }

  _folder.Release();
  m_ArchiveHandler->Close();

  bool bDeleteRes = DeleteFileAlways(m_FileName);
  if (!bDeleteRes)
    ShowLastErrorMessage();
  else
  {
    tempFile.DisableDeleting();
    bDeleteRes = MyMoveFile(tempFileName, m_FileName);
    if (!bDeleteRes)
      ShowLastErrorMessage();
  }

  result = m_ArchiveHandler->ReOpen(NULL);
  if (result != S_OK)
  {
    ShowErrorMessage(result);
    return NFileOperationReturnCode::kError;
  }
  ////////////////////////////
  // Restore FolderItem;

  m_ArchiveHandler->BindToRootFolder(&_folder);
  for (i = 0; i < aPathVector.Size(); i++)
  {
    CMyComPtr<IFolderFolder> newFolder;
    _folder->BindToFolder(aPathVector[i], &newFolder);
    if(!newFolder  )
      break;
    _folder = newFolder;
  }

  return bDeleteRes?NFileOperationReturnCode::kSuccess:NFileOperationReturnCode::kError;
}

namespace NPathType
{
  enum EEnum
  {
    kLocal,
    kUNC
  };
  EEnum GetPathType(const UString &path);
}

struct CParsedPath
{
  UString Prefix; // Disk or UNC with slash
  UStringVector PathParts;
  void ParsePath(const UString &path);
  UString MergePath() const;
};

static const wchar_t kDirDelimiter = WCHAR_PATH_SEPARATOR;
static const wchar_t kDiskDelimiter = L':';

namespace NPathType
{
  EEnum GetPathType(const UString &path)
  {
    if (path.Length() <= 2)
      return kLocal;
    if (path[0] == kDirDelimiter && path[1] == kDirDelimiter)
      return kUNC;
    return kLocal;
  }
}

void CParsedPath::ParsePath(const UString &path)
{
  int curPos = 0;
  switch (NPathType::GetPathType(path))
  {
    case NPathType::kLocal:
    {
      int posDiskDelimiter = path.Find(kDiskDelimiter);
      if(posDiskDelimiter >= 0)
      {
        curPos = posDiskDelimiter + 1;
        if (path.Length() > curPos)
          if(path[curPos] == kDirDelimiter)
            curPos++;
      }
      break;
    }
    case NPathType::kUNC:
    {
      int curPos = path.Find(kDirDelimiter, 2);
      if(curPos < 0)
        curPos = path.Length();
      else
        curPos++;
    }
  }
  Prefix = path.Left(curPos);
  SplitPathToParts(path.Mid(curPos), PathParts);
}

UString CParsedPath::MergePath() const
{
  UString result = Prefix;
  for(int i = 0; i < PathParts.Size(); i++)
  {
    if (i != 0)
      result += kDirDelimiter;
    result += PathParts[i];
  }
  return result;
}

HRESULT CompressFiles(const CObjectVector<MyPluginPanelItem> &pluginPanelItems)
{
  if (pluginPanelItems.Size() == 0)
    return E_FAIL;

  UStringVector fileNames;
  bool bSingleDir = pluginPanelItems.Size() == 1 && NFind::NAttributes::IsDir(pluginPanelItems[0].dwAttributes);

  int i;
  for(i = 0; i < pluginPanelItems.Size(); i++)
  {
    const MyPluginPanelItem &panelItem = pluginPanelItems[i];
    UString fullName;
    if (panelItem.strFileName == _F("..") && 
        NFind::NAttributes::IsDir(panelItem.dwAttributes))
      return E_FAIL;
    if (panelItem.strFileName == _F(".") && 
        NFind::NAttributes::IsDir(panelItem.dwAttributes))
      return E_FAIL;
    int index;
    if (!g_StartupInfo.GetFullPathName((LPCWSTR)GetUnicodeString(panelItem.strFileName, CP_OEMCP), fullName, index))
      return E_FAIL;
    fileNames.Add(fullName);
  }

  CCompressionInfo compressionInfo;
  compressionInfo.Load();

  bool is_7z;
  int archiverIndex = 0;

  CCodecs *codecs = new CCodecs;
  CMyComPtr<ICompressCodecsInfo> compressCodecsInfo = codecs;
  if (codecs->Load() != S_OK || codecs->Formats.Size() == 0)
    throw g_StartupInfo.GetMsgString(NMessageID::kCantLoad7Zip);
  {
    for (int i = 0; i < codecs->Formats.Size(); i++)
    {
      const CArcInfoEx &arcInfo = codecs->Formats[i];
      if (arcInfo.UpdateEnabled)
      {
        if (archiverIndex == -1)
          archiverIndex = i;
        if (arcInfo.Name.CompareNoCase(compressionInfo.ArchiveType) == 0)
          archiverIndex = i;
      }
    }
  }


  UString resultPath;
  {
    CParsedPath parsedPath;
    parsedPath.ParsePath(fileNames.Front());
    if(parsedPath.PathParts.Size() == 0)
      return E_FAIL;
    if (fileNames.Size() == 1 || parsedPath.PathParts.Size() == 1)
      resultPath = parsedPath.PathParts.Back();
    else
    {
      parsedPath.PathParts.DeleteBack();
      resultPath = parsedPath.PathParts.Back();
    }
  }
  UString archiveNameSrc = resultPath;
  UString archiveName = archiveNameSrc;

  const CArcInfoEx &arcInfo = codecs->Formats[archiverIndex];
  int prevFormat = archiverIndex;
 
  if (!arcInfo.KeepName && !bSingleDir)
  {
    int dotPos = archiveName.ReverseFind('.');
    int slashPos = MyMax(archiveName.ReverseFind('\\'), archiveName.ReverseFind('/'));
    if (dotPos > slashPos)
      archiveName = archiveName.Left(dotPos);
  }
  archiveName += L'.';
  archiveName += arcInfo.GetMainExt();
  
  const CActionSet *actionSet = &kAddActionSet;
  UString password;
  UString password2;

  for (;;)
  {
#ifdef _UNICODE
    UString archiveNameA = archiveName;
#else
    AString archiveNameA = UnicodeStringToMultiByte(archiveName, CP_OEMCP);
#endif
    const int kYSize = 19;
    const int kXMid = 38;
  
    const int kArchiveNameIndex = 2;
    const int kLevelRadioIndex = kArchiveNameIndex + 2;
    const int kMethodRadioIndex = kLevelRadioIndex + 6;
    const int kModeRadioIndex = kMethodRadioIndex + 4;
    const int kAddExtensionCheck = kModeRadioIndex + 4;

    const int kPassword = kAddExtensionCheck + 2;
    const int kPassword2 = kPassword + 2;

    const CArcInfoEx &arcInfo = codecs->Formats[archiverIndex];

    farChar updateAddToArchiveString[512];
    const CSysString s = GetSystemString(arcInfo.Name, CP_OEMCP);

    g_StartupInfo.m_FSF.sprintf(updateAddToArchiveString, 
        g_StartupInfo.GetMsgString(NMessageID::kUpdateAddToArchive), (const farChar *)s);

    is_7z = codecs->Formats[archiverIndex].Name == L"7z";

    int levelIndex = 0;
    int i;
    for (i = ARRAYSIZE(g_LevelMap) - 1; i >= 0; i--)
      if (compressionInfo.Level >= g_LevelMap[i])
      {
        levelIndex = i;
        break;
      }

    int methodIndex = 0;
    for (i = ARRAYSIZE(g_MethodMap) - 1; i >= 0; i--)
      if (compressionInfo.Method == g_MethodMap[i])
      {
        methodIndex = i;
        break;
      }

    struct CInitDialogItem initItems[]=
    {
      { DI_DOUBLEBOX, 3, 1, 72, kYSize - 2, false, false, 0, false, NMessageID::kUpdateTitle, NULL, NULL },

      { DI_TEXT, 5, 2, 0, 0, false, false, 0, false, -1, updateAddToArchiveString, NULL },

      { DI_EDIT, 5, 3, 70, 3, true, false, DIF_HISTORY, false, -1, archiveNameA, kArchiveHistoryKeyName},

      { DI_SINGLEBOX, 4, 4, kXMid - 2, 4 + 7, false, false, 0, false, NMessageID::kUpdateLevel, NULL, NULL },
      { DI_RADIOBUTTON, 6, 5, 0, 0, false, levelIndex == 0,
          DIF_GROUP, false, NMessageID::kUpdateLevelStore, NULL, NULL },
      { DI_RADIOBUTTON, 6, 6, 0, 0, false, levelIndex == 1,
          0, false, NMessageID::kUpdateLevelFastest, NULL, NULL },
      { DI_RADIOBUTTON, 6, 7, 0, 0, false, levelIndex == 2,
          0, false, NMessageID::kUpdateLevelFast, NULL, NULL },
      { DI_RADIOBUTTON, 6, 8, 0, 0, false, levelIndex == 3,
          0, false, NMessageID::kUpdateLevelNormal, NULL, NULL },
      { DI_RADIOBUTTON, 6, 9, 0, 0, false, levelIndex == 4,
          false, 0, NMessageID::kUpdateLevelMaximum, NULL, NULL },
      { DI_RADIOBUTTON, 6, 10, 0, 0, false, levelIndex == 5,
          false, 0, NMessageID::kUpdateLevelUltra, NULL, NULL },

      { DI_RADIOBUTTON, 26, 5, 0, 0, false, methodIndex == 0, DIF_GROUP | (is_7z ? 0 : DIF_HIDDEN), false, NMessageID::kUpdateMethodLZMA, NULL, NULL },
      { DI_RADIOBUTTON, 26, 6, 0, 0, false, methodIndex == 1, is_7z ? 0 : DIF_HIDDEN, false, NMessageID::kUpdateMethodLZMA2, NULL, NULL },
      { DI_RADIOBUTTON, 26, 7, 0, 0, false, methodIndex == 2, is_7z ? 0 : DIF_HIDDEN, false, NMessageID::kUpdateMethodPPMD, NULL, NULL },

      { DI_SINGLEBOX, kXMid, 4, 70, 4 + 5, false, false, 0, false, NMessageID::kUpdateMode, NULL, NULL },
      { DI_RADIOBUTTON, kXMid + 2, 5, 0, 0, false,
          actionSet == &kAddActionSet,
          DIF_GROUP, false, NMessageID::kUpdateModeAdd, NULL, NULL },
      { DI_RADIOBUTTON, kXMid + 2, 6, 0, 0, false,
          actionSet == &kUpdateActionSet,
          0, false, NMessageID::kUpdateModeUpdate, NULL, NULL },
      { DI_RADIOBUTTON, kXMid + 2, 7, 0, 0, false,
          actionSet == &kFreshActionSet,
          0, false, NMessageID::kUpdateModeFreshen, NULL, NULL },
      { DI_RADIOBUTTON, kXMid + 2, 8, 0, 0, false,
          actionSet == &kSynchronizeActionSet,
          0, false, NMessageID::kUpdateModeSynchronize, NULL, NULL },

      { DI_CHECKBOX, 5, 12, 0, 0, false, compressionInfo.AddExtension, 0, false, NMessageID::kAddExtension, NULL, NULL },

      { DI_TEXT,     5, 14, 0, 0, false, false, 0, false, NMessageID::kExtractPassword, NULL, NULL },  
      { DI_PSWEDIT, 15, 14, 30, 3, true, false, 0, false, -1, NULL, NULL},
      { DI_TEXT,    32, 14, 0, 0, false, false, 0, false, NMessageID::kExtractPassword2, NULL, NULL  },  
      { DI_PSWEDIT, 42, 14, 57, 3, true, false, 0, false, -1, NULL, NULL},

      { DI_TEXT, 3, kYSize - 4, 0, 0, false, false, DIF_BOXCOLOR|DIF_SEPARATOR, false, -1, _F(""), NULL  },  

      { DI_BUTTON, 0, kYSize - 3, 0, 0, false, false, DIF_CENTERGROUP, true, NMessageID::kUpdateAdd, NULL, NULL  },
      { DI_BUTTON, 0, kYSize - 3, 0, 0, false, false, DIF_CENTERGROUP, false, NMessageID::kUpdateSelectArchiver, NULL, NULL  },
      { DI_BUTTON, 0, kYSize - 3, 0, 0, false, false, DIF_CENTERGROUP, false, NMessageID::kCancel, NULL, NULL  }
    };

    const int kNumDialogItems = sizeof(initItems) / sizeof(initItems[0]);
    
    const int kOkButtonIndex = kNumDialogItems - 3;
    const int kSelectarchiverButtonIndex = kNumDialogItems - 2;

    FarDialogItem dialogItems[kNumDialogItems];
    g_StartupInfo.InitDialogItems(initItems, dialogItems, kNumDialogItems);

#ifdef _UNICODE
    HANDLE hDlg = 0;
    int askCode = g_StartupInfo.ShowDialog(76, kYSize, kHelpTopic, dialogItems, kNumDialogItems, hDlg);

    archiveNameA =  g_StartupInfo.GetItemData(hDlg, kArchiveNameIndex);
    archiveNameA.Trim();
    archiveName = archiveNameA;
    password =  g_StartupInfo.GetItemData(hDlg, kPassword);
    password2 =  g_StartupInfo.GetItemData(hDlg, kPassword2);
#else
    int askCode = g_StartupInfo.ShowDialog(76, kYSize, kHelpTopic, dialogItems, kNumDialogItems);

    archiveNameA = dialogItems[kArchiveNameIndex].Data;
    archiveNameA.Trim();
    archiveName = MultiByteToUnicodeString(archiveNameA, CP_OEMCP);

    AString p = dialogItems[kPassword].Data;
    password = MultiByteToUnicodeString(p, CP_OEMCP);
    p = dialogItems[kPassword2].Data;
    password2 = MultiByteToUnicodeString(p, CP_OEMCP);
#endif

    if (!password.IsEmpty() && password.Compare(password2) != 0)
    {
      g_StartupInfo.ShowMessage(NMessageID::kExtractPasswordsNotSame);
      continue;
    }

    compressionInfo.Level = g_LevelMap[0];
#ifdef _UNICODE
    for (i = 0; i < ARRAYSIZE(g_LevelMap); i++)
      if (g_StartupInfo.GetItemSelected(hDlg, kLevelRadioIndex + i))
        compressionInfo.Level = g_LevelMap[i];

    for (i = 0; i < ARRAYSIZE(g_MethodMap); i++)
      if (g_StartupInfo.GetItemSelected(hDlg, kMethodRadioIndex + i))
        compressionInfo.Method = g_MethodMap[i];

    if (g_StartupInfo.GetItemSelected(hDlg, kModeRadioIndex))
      actionSet = &kAddActionSet;
    else if (g_StartupInfo.GetItemSelected(hDlg, kModeRadioIndex + 1))
      actionSet = &kUpdateActionSet;
    else if (g_StartupInfo.GetItemSelected(hDlg, kModeRadioIndex + 2))
      actionSet = &kFreshActionSet;
    else if (g_StartupInfo.GetItemSelected(hDlg, kModeRadioIndex + 3))
      actionSet = &kSynchronizeActionSet;
    else
      throw 51751;

    compressionInfo.AddExtension = g_StartupInfo.GetItemSelected(hDlg, kAddExtensionCheck)?true:false;

    g_StartupInfo.DialogFree(hDlg);
#else
    for (i = 0; i < ARRAYSIZE(g_LevelMap); i++)
      if (dialogItems[kLevelRadioIndex + i].Selected)
        compressionInfo.Level = g_LevelMap[i];

    for (i = 0; i < ARRAYSIZE(g_MethodMap); i++)
      if (dialogItems[kMethodRadioIndex + i].Selected)
        compressionInfo.Method = g_MethodMap[i];

    if (dialogItems[kModeRadioIndex].Selected)
      actionSet = &kAddActionSet;
    else if (dialogItems[kModeRadioIndex + 1].Selected)
      actionSet = &kUpdateActionSet;
    else if (dialogItems[kModeRadioIndex + 2].Selected)
      actionSet = &kFreshActionSet;
    else if (dialogItems[kModeRadioIndex + 3].Selected)
      actionSet = &kSynchronizeActionSet;
    else
      throw 51751;

    compressionInfo.AddExtension = dialogItems[kAddExtensionCheck].Selected?true:false;
#endif

    if (askCode == kSelectarchiverButtonIndex)
    {
      CIntVector indices;
      CSysStringVector archiverNames;
      for(int i = 0; i < codecs->Formats.Size(); i++)
      {
        const CArcInfoEx &arc = codecs->Formats[i];
        if (arc.UpdateEnabled)
        {
          indices.Add(i);
          archiverNames.Add(GetSystemString(arc.Name, CP_OEMCP));
        }
      }
    
      int index = g_StartupInfo.Menu(FMENU_AUTOHIGHLIGHT,
          g_StartupInfo.GetMsgString(NMessageID::kUpdateSelectArchiverMenuTitle),
          NULL, archiverNames, archiverIndex);
      if(index >= 0)
      {
        const CArcInfoEx &prevArchiverInfo = codecs->Formats[prevFormat];
        if (prevArchiverInfo.KeepName)
        {
          const UString &prevExtension = prevArchiverInfo.GetMainExt();
          const int prevExtensionLen = prevExtension.Length();
          if (archiveName.Right(prevExtensionLen).CompareNoCase(prevExtension) == 0)
          {
            int pos = archiveName.Length() - prevExtensionLen;
            if (pos > 1)
            {
              int dotPos = archiveName.ReverseFind('.');
              if (dotPos == pos - 1)
                archiveName = archiveName.Left(dotPos);
            }
          }
        }

        archiverIndex = indices[index];
        const CArcInfoEx &arcInfo = codecs->Formats[archiverIndex];
        prevFormat = archiverIndex;
        
        if (arcInfo.KeepName || bSingleDir)
          archiveName = archiveNameSrc;
        else
        {
          int dotPos = archiveName.ReverseFind('.');
          int slashPos = MyMax(archiveName.ReverseFind('\\'), archiveName.ReverseFind('/'));
          if (dotPos > slashPos)
            archiveName = archiveName.Left(dotPos);
        }
        archiveName += L'.';
        archiveName += arcInfo.GetMainExt();
      }
      continue;
    }

    if (askCode != kOkButtonIndex)
      return E_ABORT;

    //CrOm: автоматическое добавление расширения файлу (если стоит крыжик).
    if (compressionInfo.AddExtension)
    {
      int dotPos = archiveName.ReverseFind('.');
      int slashPos = MyMax(archiveName.ReverseFind('\\'), archiveName.ReverseFind('/'));
      if (dotPos > slashPos || dotPos == -1)
      {
        UString archiveExt = archiveName.Mid(dotPos + 1);
        if (archiveExt.CompareNoCase(arcInfo.GetMainExt()))
        {
          archiveName += L'.';
          archiveName += arcInfo.GetMainExt();
        }
      }
    }

    break;
  }

  const CArcInfoEx &archiverInfoFinal = codecs->Formats[archiverIndex];
  compressionInfo.ArchiveType = archiverInfoFinal.Name;
  compressionInfo.Save();

  NWorkDir::CInfo workDirInfo;
  ReadWorkDirInfo(workDirInfo);

  UString fullArchiveName;
  int index;
  if (!g_StartupInfo.GetFullPathName((LPCWSTR)archiveName, fullArchiveName, index))
    return E_FAIL;
   
  UString workDir = GetWorkDir(workDirInfo, fullArchiveName);
  CreateComplexDirectory(workDir);

  CTempFileW tempFile;
  UString tempFileName;
  if (tempFile.Create(workDir, kTempArcivePrefix, tempFileName) == 0)
    return E_FAIL;


  CProgressBox progressBox;
  CProgressBox *progressBoxPointer = &progressBox;
  progressBox.Init(g_StartupInfo.GetMsgString(NMessageID::kUpdating), 48, false);

  NFind::CFileInfoW fileInfo;

  CMyComPtr<IOutFolderArchive> outArchive;

  CMyComPtr<IInFolderArchive> archiveHandler;
  if(NFind::FindFile(fullArchiveName, fileInfo))
  {
    if (fileInfo.IsDir())
      throw g_StartupInfo.GetMsgString(NMessageID::kDirWithSuchName);

    CAgent *agentSpec = new CAgent;
    archiveHandler = agentSpec;
    // CLSID realClassID;
    CMyComBSTR archiveType;
    RINOK(agentSpec->Open(
        fullArchiveName,
        // &realClassID,
        &archiveType,
        NULL));

    if (archiverInfoFinal.Name.CompareNoCase((const wchar_t *)archiveType) != 0)
      throw g_StartupInfo.GetMsgString(NMessageID::kExistingArchDiffersSpecified);
    HRESULT result = archiveHandler.QueryInterface(
        IID_IOutFolderArchive, &outArchive);
    if(result != S_OK)
    {
      g_StartupInfo.ShowMessage(NMessageID::kUpdateNotSupportedForThisArchive);
      return E_FAIL;
    }
  }
  else
  {
    CAgent *agentSpec = new CAgent;
    outArchive = agentSpec;
  }

  CRecordVector<const wchar_t *> fileNamePointers;
  fileNamePointers.Reserve(fileNames.Size());
  for(i = 0; i < fileNames.Size(); i++)
    fileNamePointers.Add(fileNames[i]);

  outArchive->SetFolder(NULL);
  outArchive->SetFiles(L"", &fileNamePointers.Front(), fileNamePointers.Size());
  BYTE actionSetByte[NUpdateArchive::NPairState::kNumValues];
  for (i = 0; i < NUpdateArchive::NPairState::kNumValues; i++)
    actionSetByte[i] = (BYTE)actionSet->StateActions[i];

  CUpdateCallback100Imp *updateCallbackSpec = new CUpdateCallback100Imp;
  CMyComPtr<IFolderArchiveUpdateCallback> updateCallback(updateCallbackSpec );

  updateCallbackSpec->Init(/* archiveHandler, */ &progressBox, !password.IsEmpty(), password);

  RINOK(SetOutProperties(outArchive, compressionInfo, is_7z));

  HRESULT result = outArchive->DoOperation(
      codecs, archiverIndex,
      tempFileName, actionSetByte,
      NULL, updateCallback);
  updateCallback.Release();
  outArchive.Release();

  if (result != S_OK)
  {
    ShowErrorMessage(result);
    return result;
  }
 
  if(archiveHandler)
  {
    archiveHandler->Close();
    if (!DeleteFileAlways(fullArchiveName))
    {
      ShowLastErrorMessage();
      return NFileOperationReturnCode::kError;
    }
  }
  tempFile.DisableDeleting();
  if (!MyMoveFile(tempFileName, fullArchiveName))
  {
    ShowLastErrorMessage();
    return E_FAIL;
  }
  
  return S_OK;
}

