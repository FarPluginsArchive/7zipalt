// Plugin.cpp

#include "StdAfx.h"

#include "Plugin.h"

#include "Common/IntToString.h"
#include "Common/StringConvert.h"
#include "Common/Wildcard.h"

#include "Windows/FileDir.h"
#include "Windows/FileName.h"
#include "Windows/PropVariantConversions.h"

#include "../Common/PropIDUtils.h"
#include "FarUtils.h"
#include "Messages.h"

using namespace NWindows;
using namespace NFar;

CPlugin::CPlugin(const UString &fileName,
    IInFolderArchive *archiveHandler,
    UString archiveTypeName
    ):
  m_ArchiveHandler(archiveHandler),
  m_FileName(fileName),
  _archiveTypeName(archiveTypeName)
{
  if (!NFile::NFind::FindFile(m_FileName, m_FileInfo))
    throw "error";
  archiveHandler->BindToRootFolder(&_folder);
}

CPlugin::~CPlugin()
{
}

static void MyGetFileTime(IFolderFolder *anArchiveFolder, UInt32 itemIndex,
    PROPID propID, FILETIME &fileTime)
{
  NCOM::CPropVariant prop;
  if (anArchiveFolder->GetProperty(itemIndex, propID, &prop) != S_OK)
    throw 271932;
  if (prop.vt == VT_EMPTY)
  {
    fileTime.dwHighDateTime = 0;
    fileTime.dwLowDateTime = 0;
  }
  else
  {
    if (prop.vt != VT_FILETIME)
      throw 4191730;
    fileTime = prop.filetime;
  }
}

#define kDotsReplaceString _F("[[..]]")
#define kDotsReplaceStringU L"[[..]]"
  
void CPlugin::ReadPluginPanelItem(PluginPanelItem &panelItem, UInt32 itemIndex)
{
  NCOM::CPropVariant prop;
  if (_folder->GetProperty(itemIndex, kpidName, &prop) != S_OK)
    throw 271932;

  if (prop.vt != VT_BSTR)
    throw 272340;

#ifdef _UNICODE
  CSysString oemString = prop.bstrVal;
#else
  CSysString oemString = UnicodeStringToMultiByte(prop.bstrVal, CP_OEMCP);
  const int kFileNameSizeMax = (int)(sizeof(panelItem.FindData.cFileName) / sizeof(panelItem.FindData.cFileName[0]) - 1);
  if (oemString.Length() > kFileNameSizeMax)
    oemString = oemString.Left(kFileNameSizeMax);
#endif

if (oemString == _F(".."))
    oemString = kDotsReplaceString;
#ifdef _UNICODE
  panelItem.FindData.lpwszFileName = (farChar*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, sizeof(farChar)*(oemString.Length() + 1));
  lstrcpy((farChar *)panelItem.FindData.lpwszFileName, (const farChar *)oemString);
#else
  lstrcpy(panelItem.FindData.cFileName, (const farChar *)oemString);
#endif

 
  if (_folder->GetProperty(itemIndex, kpidAttrib, &prop) != S_OK)
    throw 271932;
  if (prop.vt == VT_UI4)
    panelItem.FindData.dwFileAttributes  = prop.ulVal;
  else if (prop.vt == VT_EMPTY)
    panelItem.FindData.dwFileAttributes = m_FileInfo.Attrib;
  else
    throw 21631;
  // leave common attributes only
  panelItem.FindData.dwFileAttributes &= FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM;

  if (_folder->GetProperty(itemIndex, kpidIsDir, &prop) != S_OK)
    throw 271932;
  if (prop.vt == VT_BOOL)
  {
    if (VARIANT_BOOLToBool(prop.boolVal))
      panelItem.FindData.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
  }
  else if (prop.vt != VT_EMPTY)
    throw 21632;

  if (_folder->GetProperty(itemIndex, kpidSize, &prop) != S_OK)
    throw 271932;
  UInt64 length;
  if (prop.vt == VT_EMPTY)
    length = 0;
  else
    length = ::ConvertPropVariantToUInt64(prop);
#ifdef _UNICODE
  panelItem.FindData.nFileSize = length;
#else
  panelItem.FindData.nFileSizeLow = (UInt32)length;
  panelItem.FindData.nFileSizeHigh = (UInt32)(length >> 32);
#endif
  MyGetFileTime(_folder, itemIndex, kpidCTime, panelItem.FindData.ftCreationTime);
  MyGetFileTime(_folder, itemIndex, kpidATime, panelItem.FindData.ftLastAccessTime);
  MyGetFileTime(_folder, itemIndex, kpidMTime, panelItem.FindData.ftLastWriteTime);

  if (panelItem.FindData.ftLastWriteTime.dwHighDateTime == 0 &&
      panelItem.FindData.ftLastWriteTime.dwLowDateTime == 0)
    panelItem.FindData.ftLastWriteTime = m_FileInfo.MTime;

  if (_folder->GetProperty(itemIndex, kpidPackSize, &prop) != S_OK)
    throw 271932;
  if (prop.vt == VT_EMPTY)
    length = 0;
  else
    length = ::ConvertPropVariantToUInt64(prop);
#ifdef _UNICODE
  panelItem.FindData.nPackSize = length;
#else
  panelItem.PackSize = UInt32(length);
  panelItem.PackSizeHigh = UInt32(length >> 32);
#endif
}

int CPlugin::GetFindData(PluginPanelItem **panelItems,
    int *itemsNumber, int opMode)
{
  // CScreenRestorer screenRestorer;
  /*
  if ((opMode & OPM_SILENT) == 0 && (opMode & OPM_FIND ) == 0)
  {
    screenRestorer.Save();
    const char *msgItems[]=
    {
      g_StartupInfo.GetMsgString(NMessageID::kWaitTitle),
        g_StartupInfo.GetMsgString(NMessageID::kReadingList)
    };
    g_StartupInfo.ShowMessage(0, NULL, msgItems,
      sizeof(msgItems) / sizeof(msgItems[0]), 0);
  }
  */

  UInt32 numItems = 0;
  if (_folder)
    _folder->GetNumberOfItems(&numItems);

  *panelItems = NULL;
  if (numItems > 0)
  {
    *panelItems = (PluginPanelItem*) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, sizeof(PluginPanelItem)*numItems);
    try
    {
      for (UInt32 i = 0; i < numItems; i++)
      {
        PluginPanelItem &panelItem = (*panelItems)[i];
        ReadPluginPanelItem(panelItem, i);
        panelItem.UserData = i;
      }
    }
    catch(...)
    {
      HeapFree(GetProcessHeap(),0,*panelItems);
      throw;
    }
  }
  *itemsNumber = numItems;
  return(TRUE);
}

void CPlugin::FreeFindData(PluginPanelItem *panelItems,
    int itemsNumber)
{
  for (int i = 0; i < itemsNumber; i++)
  {
#ifdef _UNICODE
    if (panelItems[i].FindData.lpwszFileName != NULL)
    {
      HeapFree(GetProcessHeap(),0,(LPVOID)panelItems[i].FindData.lpwszFileName);
      panelItems[i].FindData.lpwszFileName = 0;
    }
#endif
    if (panelItems[i].Description != NULL)
      HeapFree(GetProcessHeap(),0,(LPVOID)panelItems[i].Description);
  }

  if (panelItems)
    HeapFree(GetProcessHeap(),0,panelItems);
}


void CPlugin::EnterToDirectory(const UString &dirName)
{
  CMyComPtr<IFolderFolder> newFolder;
  UString s = dirName;
  if (dirName == kDotsReplaceStringU)
    s = L"..";
  _folder->BindToFolder(s, &newFolder);
  if (newFolder == NULL)
    if (dirName.IsEmpty())
      return;
    else
      throw 40325;
  _folder = newFolder;
}

int CPlugin::SetDirectory(const farChar *aszDir, int /* opMode */)
{
#ifdef _UNICODE
  UString path = aszDir;
#else
  UString path = MultiByteToUnicodeString(aszDir, CP_OEMCP);
#endif
  if (path == WSTRING_PATH_SEPARATOR)
  {
    _folder.Release();
    m_ArchiveHandler->BindToRootFolder(&_folder);
  }
  else if (path == L"..")
  {
    CMyComPtr<IFolderFolder> newFolder;
    _folder->BindToParentFolder(&newFolder);
    if (newFolder == NULL)
      throw 40312;
    _folder = newFolder;
  }
  else if (path.IsEmpty())
    EnterToDirectory(path);
  else
  {
    if (path[0] == WCHAR_PATH_SEPARATOR)
    {
      _folder.Release();
      m_ArchiveHandler->BindToRootFolder(&_folder);
      path = path.Mid(1);
    }
    UStringVector pathParts;
    SplitPathToParts(path, pathParts);
    for (int i = 0; i < pathParts.Size(); i++)
      EnterToDirectory(pathParts[i]);
  }
  GetCurrentDir();
  return TRUE;
}

void CPlugin::GetPathParts(UStringVector &pathParts)
{
  pathParts.Clear();
  CMyComPtr<IFolderFolder> folderItem = _folder;
  for (;;)
  {
    CMyComPtr<IFolderFolder> newFolder;
    folderItem->BindToParentFolder(&newFolder);
    if (newFolder == NULL)
      break;
    NCOM::CPropVariant prop;
    if (folderItem->GetFolderProperty(kpidName, &prop) == S_OK)
      if (prop.vt == VT_BSTR)
        pathParts.Insert(0, (const wchar_t *)prop.bstrVal);
    folderItem = newFolder;
  }
}

void CPlugin::GetCurrentDir()
{
  m_CurrentDir.Empty();
  UStringVector pathParts;
  GetPathParts(pathParts);
  for (int i = 0; i < pathParts.Size(); i++)
  {
    m_CurrentDir += WCHAR_PATH_SEPARATOR;
    m_CurrentDir += pathParts[i];
  }
}

static farChar *kPluginFormatName = _F("7-ZIP");


struct CPROPIDToName
{
  PROPID PropID;
  int PluginID;
};

static CPROPIDToName kPROPIDToName[] =
{
  { kpidName, NMessageID::kName },
  { kpidExtension, NMessageID::kExtension },
  { kpidIsDir, NMessageID::kIsFolder },
  { kpidSize, NMessageID::kSize },
  { kpidPackSize, NMessageID::kPackSize },
  { kpidAttrib, NMessageID::kAttributes },
  { kpidCTime, NMessageID::kCTime },
  { kpidATime, NMessageID::kATime },
  { kpidMTime, NMessageID::kMTime },
  { kpidSolid, NMessageID::kSolid },
  { kpidCommented, NMessageID::kCommented },
  { kpidEncrypted, NMessageID::kEncrypted },
  { kpidSplitBefore, NMessageID::kSplitBefore },
  { kpidSplitAfter, NMessageID::kSplitAfter },
  { kpidDictionarySize, NMessageID::kDictionarySize },
  { kpidCRC, NMessageID::kCRC },
  { kpidType, NMessageID::kType },
  { kpidIsAnti, NMessageID::kAnti },
  { kpidMethod, NMessageID::kMethod },
  { kpidHostOS, NMessageID::kHostOS },
  { kpidFileSystem, NMessageID::kFileSystem },
  { kpidUser, NMessageID::kUser },
  { kpidGroup, NMessageID::kGroup },
  { kpidBlock, NMessageID::kBlock },
  { kpidComment, NMessageID::kComment },
  { kpidPosition, NMessageID::kPosition },
  { kpidNumSubDirs, NMessageID::kNumSubFolders },
  { kpidNumSubFiles, NMessageID::kNumSubFiles },
  { kpidUnpackVer, NMessageID::kUnpackVer },
  { kpidVolume, NMessageID::kVolume },
  { kpidIsVolume, NMessageID::kIsVolume },
  { kpidOffset, NMessageID::kOffset },
  { kpidLinks, NMessageID::kLinks },
  { kpidNumBlocks, NMessageID::kNumBlocks },
  { kpidNumVolumes, NMessageID::kNumVolumes },
  
  { kpidBit64, NMessageID::kBit64 },
  { kpidBigEndian, NMessageID::kBigEndian },
  { kpidCpu, NMessageID::kCpu },
  { kpidPhySize, NMessageID::kPhySize },
  { kpidHeadersSize, NMessageID::kHeadersSize },
  { kpidChecksum, NMessageID::kChecksum },
  { kpidCharacts, NMessageID::kCharacts },
  { kpidVa, NMessageID::kVa }
};

static const int kNumPROPIDToName = sizeof(kPROPIDToName) /  sizeof(kPROPIDToName[0]);

static int FindPropertyToName(PROPID propID)
{
  for (int i = 0; i < kNumPROPIDToName; i++)
    if (kPROPIDToName[i].PropID == propID)
      return i;
  return -1;
}

/*
struct CPropertyIDInfo
{
  PROPID PropID;
  const char *FarID;
  int Width;
  // char CharID;
};

static CPropertyIDInfo kPropertyIDInfos[] =
{
  { kpidName, "N", 0},
  { kpidSize, "S", 8},
  { kpidPackedSize, "P", 8},
  { kpidAttrib, "A", 0},
  { kpidCTime, "DC", 14},
  { kpidATime, "DA", 14},
  { kpidMTime, "DM", 14},
  
  { kpidSolid, NULL, 0, 'S'},
  { kpidEncrypted, NULL, 0, 'P'}

  { kpidDictionarySize, IDS_PROPERTY_DICTIONARY_SIZE },
  { kpidSplitBefore, NULL, 'B'},
  { kpidSplitAfter, NULL, 'A'},
  { kpidComment, , NULL, 'C'},
  { kpidCRC, IDS_PROPERTY_CRC }
  // { kpidType, L"Type" }
};

static const int kNumPropertyIDInfos = sizeof(kPropertyIDInfos) /
    sizeof(kPropertyIDInfos[0]);

static int FindPropertyInfo(PROPID propID)
{
  for (int i = 0; i < kNumPropertyIDInfos; i++)
    if (kPropertyIDInfos[i].PropID == propID)
      return i;
  return -1;
}
*/

// char *g_Titles[] = { "a", "f", "v" };
/*
static void SmartAddToString(AString &destString, const char *srcString)
{
  if (!destString.IsEmpty())
    destString += ',';
  destString += srcString;
}
*/

/*
void CPlugin::AddColumn(PROPID propID)
{
  int index = FindPropertyInfo(propID);
  if (index >= 0)
  {
    for (int i = 0; i < m_ProxyHandler->m_InternalProperties.Size(); i++)
    {
      const CArchiveItemProperty &aHandlerProperty = m_ProxyHandler->m_InternalProperties[i];
      if (aHandlerProperty.ID == propID)
        break;
    }
    if (i == m_ProxyHandler->m_InternalProperties.Size())
      return;

    const CPropertyIDInfo &propertyIDInfo = kPropertyIDInfos[index];
    SmartAddToString(PanelModeColumnTypes, propertyIDInfo.FarID);
    char tmp[32];
    itoa(propertyIDInfo.Width, tmp, 10);
    SmartAddToString(PanelModeColumnWidths, tmp);
    return;
  }
}
*/

static CSysString GetNameOfProp(PROPID propID, const wchar_t *name)
{
  int index = FindPropertyToName(propID);
  if (index < 0)
  {
    if (name)
#ifdef _UNICODE
      return name;
#else
      return UnicodeStringToMultiByte((const wchar_t *)name, CP_OEMCP);
#endif
    farChar s[32];
    ConvertUInt64ToString(propID, s);
    return s;
  }
  return g_StartupInfo.GetMsgString(kPROPIDToName[index].PluginID);
}

static CSysString GetNameOfProp2(PROPID propID, const wchar_t *name)
{
  CSysString s = GetNameOfProp(propID, name);
#ifndef _UNICODE
  if (s.Length() > (kInfoPanelLineSize - 1))
    s = s.Left(kInfoPanelLineSize - 1);
#endif
  return s;
}

static CSysString ConvertSizeToString(UInt64 value)
{
  farChar s[32];
  ConvertUInt64ToString(value, s);
  int i = MyStringLen(s);
  int pos = sizeof(s) / sizeof(s[0]);
  s[--pos] = L'\0';
  while (i > 3)
  {
    s[--pos] = s[--i];
    s[--pos] = s[--i];
    s[--pos] = s[--i];
    s[--pos] = _F(' ');
  }
  while (i > 0)
    s[--pos] = s[--i];
  return s + pos;
}

static CSysString PropToString(const NCOM::CPropVariant &prop, PROPID propID)
{
  CSysString s;
  if (prop.vt == VT_BSTR)
    s = GetSystemString(prop.bstrVal, CP_OEMCP);

  else if (prop.vt == VT_BOOL)
  {
    int messageID = VARIANT_BOOLToBool(prop.boolVal) ?
      NMessageID::kYes : NMessageID::kNo;
    return g_StartupInfo.GetMsgString(messageID);
  }
  else if (prop.vt != VT_EMPTY)
  {
    if ((
        propID == kpidSize ||
        propID == kpidPackSize ||
        propID == kpidNumSubDirs ||
        propID == kpidNumSubFiles ||
        propID == kpidNumBlocks ||
        propID == kpidPhySize ||
        propID == kpidHeadersSize ||
        propID == kpidClusterSize
        ) && (prop.vt == VT_UI8 || prop.vt == VT_UI4))
      s = ConvertSizeToString(ConvertPropVariantToUInt64(prop));
    else
#ifdef _UNICODE
      s = ConvertPropertyToString(prop, propID);
#else
      s = UnicodeStringToMultiByte(ConvertPropertyToString(prop, propID), CP_OEMCP);
#endif
  }
  s.Replace((farChar)0xA, _F(' '));
  s.Replace((farChar)0xD, _F(' '));
  return s;
}

static CSysString PropToString2(const NCOM::CPropVariant &prop, PROPID propID)
{
  CSysString s = PropToString(prop, propID);
#ifndef _UNICODE
  if (s.Length() > (kInfoPanelLineSize - 1))
    s = s.Left(kInfoPanelLineSize - 1);
#endif // _UNICODE
  return s;
}

void CPlugin::GetOpenPluginInfo(struct OpenPluginInfo *info, const CPanelMode& panelMode)
{
  info->StructSize = sizeof(*info);
  info->Flags = OPIF_USEFILTER | OPIF_USESORTGROUPS| OPIF_USEHIGHLIGHTING|
              OPIF_ADDDOTS | OPIF_COMPAREFATTIME;

  UINT codePage = ::AreFileApisANSI() ? CP_ACP : CP_OEMCP;

#ifdef _UNICODE
  lstrcpy(m_FileNameBuffer, (const farChar*)m_FileName);
  lstrcpy(m_CurrentDirBuffer, (const farChar*)m_CurrentDir);
#else
  MyStringCopy(m_FileNameBuffer, (const char *)UnicodeStringToMultiByte(m_FileName, codePage));
  MyStringCopy(m_CurrentDirBuffer, (const char *)UnicodeStringToMultiByte(m_CurrentDir, CP_OEMCP));
#endif
  info->HostFile = m_FileNameBuffer;
  info->CurDir = m_CurrentDirBuffer;
  info->Format = kPluginFormatName;

  UString name;
  {
    UString fullName;
    int index;
    g_StartupInfo.GetFullPathName((LPCWSTR)m_FileName, fullName, index);
    name = fullName.Mid(index);
  }

  m_PannelTitle =
      UString(L' ') +
      _archiveTypeName +
      UString(L':') +
      name +
      UString(L' ');
  if (!m_CurrentDir.IsEmpty())
  {
    // m_PannelTitle += '\\';
    m_PannelTitle += m_CurrentDir;
  }

#ifdef _UNICODE
  lstrcpy(m_PannelTitleBuffer, (const farChar *)m_PannelTitle);
#else
  MyStringCopy(m_PannelTitleBuffer, (const char *)UnicodeStringToMultiByte(m_PannelTitle, CP_OEMCP));
#endif

  info->PanelTitle = m_PannelTitleBuffer;

  memset(m_InfoLines, 0, sizeof(m_InfoLines));

#ifdef _UNICODE
  m_InfoLines[0].Text = _F("");
  m_InfoLines[0].Separator = TRUE;

  m_InfoLines[1].Text = g_StartupInfo.GetMsgString(NMessageID::kArchiveType);
  m_InfoLines[1].Data = _archiveTypeName;
#else
  MyStringCopy(m_InfoLines[0].Text, _F(""));
  m_InfoLines[0].Separator = TRUE;

  MyStringCopy(m_InfoLines[1].Text, g_StartupInfo.GetMsgString(NMessageID::kArchiveType));
  MyStringCopy(m_InfoLines[1].Data, (const farChar *)UnicodeStringToMultiByte(_archiveTypeName, CP_OEMCP));
#endif

#ifdef _UNICODE
  static CSysStringVector pNames;
  static CSysStringVector pData;
  pNames.Clear();
  pData.Clear();
#endif

  int numItems = 2;

  CMyComPtr<IFolderProperties> folderProperties;
  if (_folder)
    _folder.QueryInterface(IID_IFolderProperties, &folderProperties);
  if (folderProperties)
  {
    UInt32 numProps;
    if (folderProperties->GetNumberOfFolderProperties(&numProps) == S_OK)
    {
      for (UInt32 i = 0; i < numProps && numItems < kNumInfoLinesMax; i++)
      {
        CMyComBSTR name;
        PROPID propID;
        VARTYPE vt;
        if (folderProperties->GetFolderPropertyInfo(i, &name, &propID, &vt) != S_OK)
          continue;
        NCOM::CPropVariant prop;
        if (_folder->GetFolderProperty(propID, &prop) != S_OK || prop.vt == VT_EMPTY)
          continue;

        InfoPanelLine &item = m_InfoLines[numItems++];
#ifdef _UNICODE
        pNames.Add(GetNameOfProp2(propID, name));
        item.Text = pNames.Back();
        pData.Add(PropToString2(prop, propID));
        item.Data = pData.Back();
#else
        MyStringCopy(item.Text, (const farChar *)GetNameOfProp2(propID, name));
        MyStringCopy(item.Data, (const farChar *)PropToString2(prop, propID));
#endif
      }
    }
  }

  if (numItems < kNumInfoLinesMax)
  {
    InfoPanelLine &item = m_InfoLines[numItems++];
#ifdef _UNICODE
    pNames.Add(_F(""));
    item.Text = pNames.Back();
    pData.Add(_F(""));
    item.Data = pData.Back();
#else
    MyStringCopy(item.Text, "");
    MyStringCopy(item.Data, "");
#endif
    item.Separator = TRUE;
  }

  {
    CMyComPtr<IGetFolderArchiveProperties> getFolderArchiveProperties;
    if (_folder)
      _folder.QueryInterface(IID_IGetFolderArchiveProperties, &getFolderArchiveProperties);
    if (getFolderArchiveProperties)
    {
      CMyComPtr<IFolderArchiveProperties> getProps;
      getFolderArchiveProperties->GetFolderArchiveProperties(&getProps);
      if (getProps)
      {
        UInt32 numProps;
        if (getProps->GetNumberOfArchiveProperties(&numProps) == S_OK)
        {
          for (UInt32 i = 0; i < numProps && numItems < kNumInfoLinesMax; i++)
          {
            CMyComBSTR name;
            PROPID propID;
            VARTYPE vt;
            if (getProps->GetArchivePropertyInfo(i, &name, &propID, &vt) != S_OK)
              continue;
            InfoPanelLine &item = m_InfoLines[numItems++];

            NCOM::CPropVariant prop;
            if (getProps->GetArchiveProperty(propID, &prop) != S_OK || prop.vt == VT_EMPTY)
              continue;

#ifndef _UNICODE            
            MyStringCopy(item.Text, (const farChar *)GetNameOfProp2(propID, name));
            MyStringCopy(item.Data, (const farChar *)PropToString2(prop, propID));
#else
            pNames.Add(GetNameOfProp2(propID, name));
            item.Text = pNames.Back();
            pData.Add(PropToString2(prop, propID));
            item.Data = pData.Back();
#endif
          }
        }
      }
    }
  }
  info->InfoLines = m_InfoLines;
  info->InfoLinesNumber = numItems;

  PanelModeColumnTypes.Empty();
  PanelModeColumnWidths.Empty();

  info->StartPanelMode = '0' + panelMode.ViewMode;
  info->StartSortMode = panelMode.SortMode;
  info->StartSortOrder = panelMode.ReverseSort ? 1 : 0;
}

struct CArchiveItemProperty
{
  CSysString Name;
  PROPID ID;
  VARTYPE Type;
};

HRESULT CPlugin::ShowAttributesWindow()
{
  if (g_StartupInfo.GetActivePanelCurrentItemName() == _F("..") &&
      NFile::NFind::NAttributes::IsDir(g_StartupInfo.GetActivePanelCurrentItemAtt()))
    return S_FALSE;

  DWORD_PTR itemIndex = g_StartupInfo.GetActivePanelCurrentItemData();

  CObjectVector<CArchiveItemProperty> properties;
  UInt32 numProps;
  RINOK(_folder->GetNumberOfProperties(&numProps));
  int i;
  for (i = 0; i < (int)numProps; i++)
  {
    CMyComBSTR name;
    PROPID propID;
    VARTYPE vt;
    RINOK(_folder->GetPropertyInfo(i, &name, &propID, &vt));
    CArchiveItemProperty prop;
    prop.Type = vt;
    prop.ID = propID;
    if (prop.ID  == kpidPath)
      prop.ID  = kpidName;
    prop.Name = GetNameOfProp(propID, name);
    properties.Add(prop);
  }

  int size = 2;
  CRecordVector<CInitDialogItem> initDialogItems;
  
  int xSize = 70;
  CInitDialogItem initDialogItem = 
  { DI_DOUBLEBOX, 3, 1, xSize - 4, size - 2, false, false, 0, false, NMessageID::kProperties, NULL, NULL };
  initDialogItems.Add(initDialogItem);
  CSysStringVector values;

  for (i = 0; i < properties.Size(); i++)
  {
    const CArchiveItemProperty &property = properties[i];

    CInitDialogItem initDialogItem = 
      { DI_TEXT, 5, 3 + i, 0, 0, false, false, 0, false, 0, NULL, NULL };
    int index = FindPropertyToName(property.ID);
    if (index < 0)
    {
      initDialogItem.DataMessageId = -1;
      initDialogItem.DataString = property.Name;
    }
    else
      initDialogItem.DataMessageId = kPROPIDToName[index].PluginID;
    initDialogItems.Add(initDialogItem);

    NCOM::CPropVariant prop;
    RINOK(_folder->GetProperty((UInt32)itemIndex, property.ID, &prop));
    CSysString s = PropToString(prop, property.ID);
    values.Add(s);
    {
      CInitDialogItem initDialogItem = 
      { DI_TEXT, 30, 3 + i, 0, 0, false, false, 0, false, -1, NULL, NULL };
      initDialogItems.Add(initDialogItem);
    }
  }

  int numLines = values.Size();
  for (i = 0; i < numLines; i++)
  {
    CInitDialogItem &initDialogItem = initDialogItems[1 + i * 2 + 1];
    initDialogItem.DataString = values[i];
  }
  
  int numDialogItems = initDialogItems.Size();
  
  CRecordVector<FarDialogItem> dialogItems;
  dialogItems.Reserve(numDialogItems);
  for (i = 0; i < numDialogItems; i++)
    dialogItems.Add(FarDialogItem());
  g_StartupInfo.InitDialogItems(&initDialogItems.Front(), 
      &dialogItems.Front(), numDialogItems);
  
  int maxLen = 0;
  for (i = 0; i < numLines; i++)
  {
    FarDialogItem &dialogItem = dialogItems[1 + i * 2];
#ifdef _UNICODE
    int len = (int)lstrlenF(dialogItem.PtrData);
#else
    int len = (int)lstrlenF(dialogItem.Data);
#endif
   if (len > maxLen)
      maxLen = len;
  }
  int maxLen2 = 0;
  const int kSpace = 10;
  for (i = 0; i < numLines; i++)
  {
    FarDialogItem &dialogItem = dialogItems[1 + i * 2 + 1];
#ifdef _UNICODE
    int len = (int)lstrlenF(dialogItem.PtrData);
#else
    int len = (int)lstrlenF(dialogItem.Data);
#endif
    if (len > maxLen2)
      maxLen2 = len;
    dialogItem.X1 = maxLen + kSpace;
  }
  size = numLines + 6;
  xSize = maxLen + kSpace + maxLen2 + 5;
  FarDialogItem &firstDialogItem = dialogItems.Front();
  firstDialogItem.Y2 = size - 2;
  firstDialogItem.X2 = xSize - 4;
  
#ifdef _UNICODE
  HANDLE hDlg = 0;
  g_StartupInfo.ShowDialog(xSize, size, NULL, &dialogItems.Front(), numDialogItems, hDlg);
  g_StartupInfo.DialogFree(hDlg);
#else
  g_StartupInfo.ShowDialog(xSize, size, NULL, &dialogItems.Front(), numDialogItems);
#endif


  return S_OK;
}

int CPlugin::ProcessKey(int key, unsigned int controlState)
{
  if (controlState == PKF_CONTROL && key == _F('A'))
  {
    HRESULT result = ShowAttributesWindow();
    if (result == S_OK)
      return TRUE;
    if (result == S_FALSE)
      return FALSE;
    throw _F("Error");
  }
  if ((controlState & PKF_ALT) != 0 && key == VK_F6)
  {
    UString folderPath;
    int index;
    if (!g_StartupInfo.GetFullPathName((LPCWSTR)m_FileName, folderPath, index))
      return FALSE;
    folderPath = folderPath.Left(index);
    PanelInfo panelInfo;
    g_StartupInfo.ControlGetActivePanelInfo(panelInfo);
#ifdef _UNICODE
    const wchar_t * wc = folderPath;
    GetFilesReal(NULL, panelInfo.SelectedItemsNumber, FALSE, &wc, OPM_SILENT, true); 
#else
    char wc[MAX_PATH];
    wc[0] = 0;
    lstrcpy(wc, UnicodeStringToMultiByte(folderPath, CP_OEMCP));
    GetFilesReal(panelInfo.SelectedItems, panelInfo.SelectedItemsNumber, FALSE, wc, OPM_SILENT, true);
#endif
    g_StartupInfo.ControlUpdateActivePanel(NULL);
    g_StartupInfo.ControlRedrawActivePanel(NULL);
    g_StartupInfo.ControlUpdatePassivePanel(1);
    g_StartupInfo.ControlRedrawPassivePanel(NULL);
    return TRUE;
  }

  return FALSE;
}

void CPlugin::GetPanelMode(CPanelMode& panelMode)
{
  PanelInfo pi;
  if (g_StartupInfo.Control(this, FCTL_GETPANELSHORTINFO, 0, reinterpret_cast<LONG_PTR>(&pi)))
  {
    panelMode.ViewMode = pi.ViewMode;
    panelMode.SortMode = pi.SortMode;
    panelMode.ReverseSort = (pi.Flags & PFLAGS_REVERSESORTORDER) != 0;
    panelMode.NumericSort = (pi.Flags & PFLAGS_NUMERICSORT) != 0;
  }
}

