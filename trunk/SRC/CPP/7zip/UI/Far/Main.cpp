// Test Align for updating !!!!!!!!!!!!!!!!!!

#include "StdAfx.h"

// #include <locale.h>
#include <initguid.h>

#include "Plugin.h"

#include "Common/Wildcard.h"
#include "Common/DynamicBuffer.h"
#include "Common/StringConvert.h"
#include "Common/Defs.h"
#include "Common/ComTry.h"
#include "Common/IntToString.h"

#include "Windows/FileFind.h"
#include "Windows/FileIO.h"
#include "Windows/FileDir.h"
#include "Windows/Defs.h"

#include "../../IPassword.h"
#include "../../Common/FileStreams.h"

#include "../Common/DefaultName.h"
#include "../Common/OpenArchive.h"
#include "../Agent/Agent.h"

#include "ProgressBox.h"
#include "FarUtils.h"
#include "Messages.h"

using namespace NWindows;
using namespace NFar;

const farChar *kCommandPrefix = _F("7-zip");

static const farChar *kPluginNameForRegistry = _F("7-ZIP");

const farChar *kRegistryMainKeyName = _F("");

const farChar *kRegistryValueNameEnabled = _F("UsedByDefault3");
const bool kPluginEnabledDefault = true;
const farChar *kRegistryValueNameUseMasks = _F("UseMasks");
const bool kUseMasksDefault = false;
const farChar *kRegistryValueNameMasks = _F("Masks");
const farChar *kRegistryValueNameDisabledFormats = _F("DisabledFormats");
const farChar *kRegistryValueNameViewMode = _F("ViewMode");
const int kViewModeDefault = 2;
const farChar *kRegistryValueNameSortMode = _F("SortMode");
const int kSortModeDefault = SM_NAME;
const farChar *kRegistryValueNameReverseSort = _F("ReverseSort");
const bool kReverseSortDefault = false;
const farChar *kRegistryValueNameNumericSort = _F("NumericSort");
const bool kNumericSortDefault = true;
const farChar *kRegistryValueNameMaxCheckSize = _F("MaxCheckSize");
const int kMaxCheckSizeDefault = 2 * 1024 * 1024;

const farChar *kRegistryValueNameArchiveType = _F("ArchiveType");
const farChar *kArchiveTypeDefault = _F("7z");
const farChar *kRegistryValueNameLevel = _F("Level");
const int kLevelDefault = 7;
const farChar *kRegistryValueNameMethod = _F("Method");
const farChar *kMethodDefault = _F("LZMA");
const farChar *kRegistryValueNameAddExtension = _F("AddExtension");
const bool kAddExtensionDefault = true;

const farChar *kHelpTopicConfig =  _F("Config");

HINSTANCE g_hInstance;
#ifndef _UNICODE
bool g_IsNT = false;
static bool IsItWindowsNT()
{
  OSVERSIONINFO versionInfo;
  versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
  if (!::GetVersionEx(&versionInfo))
    return false;
  return (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
}
#endif

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID)
{
  if (dwReason == DLL_PROCESS_ATTACH)
  {
    g_hInstance = hInstance;
    #ifndef _UNICODE
    g_IsNT = IsItWindowsNT();
    #endif
  }
  return TRUE;
}

// #define  MY_TRY_BEGIN  MY_TRY_BEGIN NCOM::CComInitializer aComInitializer;

CSysString GetMaskList()
{

  UString exts;

  CCodecs *codecs = new CCodecs;
  if (codecs->Load() != S_OK)
    throw g_StartupInfo.GetMsgString(NMessageID::kCantLoad7Zip);
  {
    for (int i = 0; i < codecs->Formats.Size(); i++)
    {
      const CArcInfoEx &arcInfo = codecs->Formats[i];
      for (int j = 0; j < arcInfo.Exts.Size(); j++)
      {
        exts += L"*.";
        exts += arcInfo.Exts[j].Ext;
        exts += L",";
      }
    }
    exts.MakeLower();
    exts.Replace(L".r00", L".r[0-9][0-9]");
    exts.TrimRight(L',');
  }
  codecs->Release();

  return GetSystemString(exts, CP_OEMCP);
}

CSysString GetFormatList()
{
  UString Formats;

  CCodecs *codecs = new CCodecs;
  if (codecs->Load() != S_OK)
    throw g_StartupInfo.GetMsgString(NMessageID::kCantLoad7Zip);
  {
    for (int i = 0; i < codecs->Formats.Size(); i++)
    {
      const CArcInfoEx &arcInfo = codecs->Formats[i];
      if (Formats.Length())
        Formats += L",";
      Formats += arcInfo.Name;
    }
  }
  codecs->Release();

  return GetSystemString(Formats);
}

struct COptions
{
  bool Enabled;
  bool UseMasks;
  CSysString Masks;
  CSysString DisabledFormats;
  CPanelMode PanelMode;
  int MaxCheckSize;
  void Load()
  {
    Enabled = g_StartupInfo.QueryRegKeyValue(HKEY_CURRENT_USER, kRegistryMainKeyName, kRegistryValueNameEnabled, kPluginEnabledDefault);
    UseMasks = g_StartupInfo.QueryRegKeyValue(HKEY_CURRENT_USER, kRegistryMainKeyName, kRegistryValueNameUseMasks, kUseMasksDefault);
    Masks = g_StartupInfo.QueryRegKeyValue(HKEY_CURRENT_USER, kRegistryMainKeyName, kRegistryValueNameMasks, CSysString());
    DisabledFormats = g_StartupInfo.QueryRegKeyValue(HKEY_CURRENT_USER, kRegistryMainKeyName, kRegistryValueNameDisabledFormats, CSysString());
    PanelMode.ViewMode = g_StartupInfo.QueryRegKeyValue(HKEY_CURRENT_USER, kRegistryMainKeyName, kRegistryValueNameViewMode, kViewModeDefault);
    PanelMode.SortMode = g_StartupInfo.QueryRegKeyValue(HKEY_CURRENT_USER, kRegistryMainKeyName, kRegistryValueNameSortMode, kSortModeDefault);
    PanelMode.ReverseSort = g_StartupInfo.QueryRegKeyValue(HKEY_CURRENT_USER, kRegistryMainKeyName, kRegistryValueNameReverseSort, kReverseSortDefault);
    PanelMode.NumericSort = g_StartupInfo.QueryRegKeyValue(HKEY_CURRENT_USER, kRegistryMainKeyName, kRegistryValueNameNumericSort, kNumericSortDefault);
    MaxCheckSize = g_StartupInfo.QueryRegKeyValue(HKEY_CURRENT_USER, kRegistryMainKeyName, kRegistryValueNameMaxCheckSize, kMaxCheckSizeDefault);
    Validate();
  }
  void Save()
  {
    g_StartupInfo.SetRegKeyValue(HKEY_CURRENT_USER, kRegistryMainKeyName, kRegistryValueNameEnabled, Enabled);
    g_StartupInfo.SetRegKeyValue(HKEY_CURRENT_USER, kRegistryMainKeyName, kRegistryValueNameUseMasks, UseMasks);
    g_StartupInfo.SetRegKeyValue(HKEY_CURRENT_USER, kRegistryMainKeyName, kRegistryValueNameMasks, Masks);
    g_StartupInfo.SetRegKeyValue(HKEY_CURRENT_USER, kRegistryMainKeyName, kRegistryValueNameDisabledFormats, DisabledFormats);
    g_StartupInfo.SetRegKeyValue(HKEY_CURRENT_USER, kRegistryMainKeyName, kRegistryValueNameViewMode, PanelMode.ViewMode);
    g_StartupInfo.SetRegKeyValue(HKEY_CURRENT_USER, kRegistryMainKeyName, kRegistryValueNameSortMode, PanelMode.SortMode);
    g_StartupInfo.SetRegKeyValue(HKEY_CURRENT_USER, kRegistryMainKeyName, kRegistryValueNameReverseSort, PanelMode.ReverseSort);
    g_StartupInfo.SetRegKeyValue(HKEY_CURRENT_USER, kRegistryMainKeyName, kRegistryValueNameNumericSort, PanelMode.NumericSort);
    g_StartupInfo.SetRegKeyValue(HKEY_CURRENT_USER, kRegistryMainKeyName, kRegistryValueNameMaxCheckSize, MaxCheckSize);
  }
  void Validate()
  {
    Masks.Trim();
    if (Masks.IsEmpty())
      Masks = GetMaskList();
    DisabledFormats.Trim();
    if ((MaxCheckSize <= 0) || (MaxCheckSize > 4 * 1024 * 1024))
      MaxCheckSize = kMaxCheckSizeDefault;
  }
} g_Options;

void CCompressionInfo::Load()
{
  ArchiveType = GetUnicodeString(g_StartupInfo.QueryRegKeyValue(HKEY_CURRENT_USER, kRegistryMainKeyName, kRegistryValueNameArchiveType, CSysString(kArchiveTypeDefault)));
  Level = g_StartupInfo.QueryRegKeyValue(HKEY_CURRENT_USER, kRegistryMainKeyName, kRegistryValueNameLevel, kLevelDefault);
  Method = GetUnicodeString(g_StartupInfo.QueryRegKeyValue(HKEY_CURRENT_USER, kRegistryMainKeyName, kRegistryValueNameMethod, CSysString(kMethodDefault)));
  AddExtension = g_StartupInfo.QueryRegKeyValue(HKEY_CURRENT_USER, kRegistryMainKeyName, kRegistryValueNameAddExtension, kAddExtensionDefault);
}

void CCompressionInfo::Save()
{
  g_StartupInfo.SetRegKeyValue(HKEY_CURRENT_USER, kRegistryMainKeyName, kRegistryValueNameArchiveType, GetSystemString(ArchiveType));
  g_StartupInfo.SetRegKeyValue(HKEY_CURRENT_USER, kRegistryMainKeyName, kRegistryValueNameLevel, static_cast<int>(Level));
  g_StartupInfo.SetRegKeyValue(HKEY_CURRENT_USER, kRegistryMainKeyName, kRegistryValueNameMethod, GetSystemString(Method));
  g_StartupInfo.SetRegKeyValue(HKEY_CURRENT_USER, kRegistryMainKeyName, kRegistryValueNameAddExtension, AddExtension);
}

#ifdef _UNICODE
int WINAPI _export GetMinFarVersionW(void)
{
  return MAKEFARVERSION(2,0,1309);//тут был breaking change
}
#endif

UStringVector SplitString(const UString& Str, wchar_t SplitCh)
{
  UStringVector List;
  int Pos = 0;
  while (Pos < Str.Length())
  {
    int Pos2 = Str.Find(SplitCh, Pos);
    if (Pos2 == -1)
      Pos2 = Str.Length();
    if (Pos2 != Pos)
    {
      UString SubStr = Str.Mid(Pos, Pos2 - Pos);
      SubStr.Trim();
      List.Add(SubStr);
    }
    Pos = Pos2 + 1;
  }
  return List;
}

#ifdef _UNICODE
void WINAPI SetStartupInfoW(const struct PluginStartupInfo *info)
#else
void WINAPI SetStartupInfo(struct PluginStartupInfo *info)
#endif
{
  MY_TRY_BEGIN;
  g_StartupInfo.Init(*info, kPluginNameForRegistry);
  g_Options.Load();
  MY_TRY_END1(_F("SetStartupInfo"));
}

class COpenArchiveCallback:
  public IArchiveOpenCallback,
  public IArchiveOpenVolumeCallback,
  public IProgress,
  public ICryptoGetTextPassword,
  public CMyUnknownImp
{
  DWORD m_StartTickValue;
  bool m_MessageBoxIsShown;

  CProgressBox _progressBox;

  UInt64 _numFilesTotal;
  UInt64 _numFilesCur;
  UInt64 _numBytesTotal;
  UInt64 _numBytesCur;

  bool _numFilesTotalDefined;
  bool _numFilesCurDefined;
  bool _numBytesTotalDefined;
  bool _numBytesCurDefined;

  NWindows::NFile::NFind::CFileInfoW _fileInfo;
public:
  bool PasswordIsDefined;
  UString Password;

  UString _folderPrefix;

public:
  MY_UNKNOWN_IMP3(
     IArchiveOpenVolumeCallback,
     IProgress,
     ICryptoGetTextPassword
    )

  // IProgress
  STDMETHOD(SetTotal)(UINT64 total);
  STDMETHOD(SetCompleted)(const UINT64 *aCompleteValue);

  // IArchiveOpenCallback
  STDMETHOD(SetTotal)(const UINT64 *numFiles, const UINT64 *numBytes);
  STDMETHOD(SetCompleted)(const UINT64 *numFiles, const UINT64 *numBytes);

  // IArchiveOpenVolumeCallback
  STDMETHOD(GetProperty)(PROPID propID, PROPVARIANT *value);
  STDMETHOD(GetStream)(const wchar_t *name, IInStream **inStream);

  // ICryptoGetTextPassword
  STDMETHOD(CryptoGetTextPassword)(BSTR *password);

  void Init()
  {
    PasswordIsDefined = false;

    _numFilesTotalDefined = false;
    _numFilesCurDefined = false;
    _numBytesTotalDefined = false;
    _numBytesCurDefined = false;

    m_MessageBoxIsShown = false;
  }
  void ShowMessage();

  void LoadFileInfo(const UString &folderPrefix,
      const UString &fileName)
  {
    _folderPrefix = folderPrefix;
    if (!NWindows::NFile::NFind::FindFile(_folderPrefix + fileName, _fileInfo))
      throw 1;
  }
};

void COpenArchiveCallback::ShowMessage()
{
  DWORD currentTime = GetTickCount();
  if (!m_MessageBoxIsShown)
  {
    _progressBox.Init(g_StartupInfo.GetMsgString(NMessageID::kReading), 48, true);

    m_MessageBoxIsShown = true;
  }
  UInt64 total = 0, cur = 0;
  bool curIsDefined = false, totalIsDefined = false;

  farChar message[256] = { 0 };
  if (_numFilesCurDefined)
    ConvertUInt64ToStringAligned(_numFilesCur, message, 5);

  if (_numFilesTotalDefined)
  {
    lstrcat(message, _F(" / "));
    ConvertUInt64ToStringAligned(_numFilesTotal, message + lstrlen(message), 5);
    total = _numFilesTotal;
    totalIsDefined = true;
    if (_numFilesCurDefined)
    {
      cur = _numFilesCur;
      curIsDefined = true;
    }
  }
  else if (_numBytesTotalDefined)
  {
    total = _numBytesTotal;
    totalIsDefined = true;
    if (_numBytesCurDefined)
    {
      cur = _numBytesCur;
      curIsDefined = true;
    }
  }
  _progressBox.Progress(
      totalIsDefined ? &total: NULL,
      curIsDefined ? &cur: NULL,
      message);
}

STDMETHODIMP COpenArchiveCallback::SetTotal(const UInt64 *numFiles, const UInt64 *numBytes)
{
  if (WasEscPressed())
    return E_ABORT;
  
  _numFilesTotalDefined = (numFiles != NULL);
  if (_numFilesTotalDefined)
    _numFilesTotal = *numFiles;

  _numBytesTotalDefined = (numBytes != NULL);
  if (_numBytesTotalDefined)
    _numBytesTotal = *numBytes;

  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::SetCompleted(const UInt64 *numFiles, const UInt64 *numBytes)
{
  if (WasEscPressed())
    return E_ABORT;

  _numFilesCurDefined = (numFiles != NULL);
  if (_numFilesCurDefined)
    _numFilesCur = *numFiles;

  _numBytesCurDefined = (numBytes != NULL);
  if (_numBytesCurDefined)
    _numBytesCur = *numBytes;

  // if (*numFiles % 100 != 0)
  //   return S_OK;
  ShowMessage();
  return S_OK;
}


STDMETHODIMP COpenArchiveCallback::SetTotal(const UInt64 /* total */)
{
  if (WasEscPressed())
    return E_ABORT;
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::SetCompleted(const UInt64 *completed)
{
  if (WasEscPressed())
    return E_ABORT;
  if (completed == NULL)
    return S_OK;
  ShowMessage();
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::GetStream(const wchar_t *name,
    IInStream **inStream)
{
  if (WasEscPressed())
    return E_ABORT;
  *inStream = NULL;
  UString fullPath = _folderPrefix + name;
  if (!NWindows::NFile::NFind::FindFile(fullPath, _fileInfo))
    return S_FALSE;
  if (_fileInfo.IsDir())
    return S_FALSE;
  CInFileStream *inFile = new CInFileStream;
  CMyComPtr<IInStream> inStreamTemp = inFile;
  if (!inFile->Open(fullPath))
    return ::GetLastError();
  *inStream = inStreamTemp.Detach();
  return S_OK;
}


STDMETHODIMP COpenArchiveCallback::GetProperty(PROPID propID, PROPVARIANT *value)
{
  NWindows::NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidName:  prop = GetUnicodeString(_fileInfo.Name, CP_OEMCP); break;
    case kpidIsDir:  prop = _fileInfo.IsDir(); break;
    case kpidSize:  prop = _fileInfo.Size; break;
    case kpidAttrib:  prop = (UInt32)_fileInfo.Attrib; break;
    case kpidCTime:  prop = _fileInfo.CTime; break;
    case kpidATime:  prop = _fileInfo.ATime; break;
    case kpidMTime:  prop = _fileInfo.MTime; break;
  }
  prop.Detach(value);
  return S_OK;
}

HRESULT GetPassword(UString &password)
{
  if (WasEscPressed())
    return E_ABORT;
  password.Empty();
  CInitDialogItem initItems[]=
  {
    { DI_DOUBLEBOX, 3, 1, 72, 4, false, false, 0, false,  NMessageID::kGetPasswordTitle, NULL, NULL },
    { DI_TEXT, 5, 2, 0, 0, false, false, DIF_SHOWAMPERSAND, false, NMessageID::kEnterPasswordForFile, NULL, NULL },
    { DI_PSWEDIT, 5, 3, 70, 3, true, false, 0, true, -1, _F(""), NULL }
  };

  const int kNumItems = sizeof(initItems)/sizeof(initItems[0]);
  FarDialogItem dialogItems[kNumItems];
  g_StartupInfo.InitDialogItems(initItems, dialogItems, kNumItems);

  // sprintf(DialogItems[1].Data,GetMsg(MGetPasswordForFile),FileName);
#ifdef _UNICODE
  HANDLE hDlg;
  if (g_StartupInfo.ShowDialog(76, 6, NULL, dialogItems, kNumItems, hDlg) < 0)
    return (E_ABORT);

  password = g_StartupInfo.GetItemData(hDlg, 2);
  g_StartupInfo.DialogFree(hDlg);
#else
  if (g_StartupInfo.ShowDialog(76, 6, NULL, dialogItems, kNumItems) < 0)
    return (E_ABORT);
  AString oemPassword = dialogItems[2].Data;
  password = MultiByteToUnicodeString(oemPassword, CP_OEMCP);
#endif

  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::CryptoGetTextPassword(BSTR *password)
{
  if (!PasswordIsDefined)
  {
    RINOK(GetPassword(Password));
    PasswordIsDefined = true;
  }
  return StringToBstr(Password, password);
}

struct FormatInfo
{
  int index;
  CSysString name;
};

int CompareFormatInfo(void *const *a1, void *const *a2, void * /* param */)
{
  return g_StartupInfo.m_FSF.LStricmp((**(const FormatInfo**)a1).name, (**(const FormatInfo**)a2).name);
}

STDMETHODIMP CAgent::Open(const wchar_t *filePath, BSTR *archiveType, IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  _archiveFilePath = filePath;
  maxCheckSize = g_Options.MaxCheckSize;
  NFile::NFind::CFileInfoW fileInfo;
  if (!NFile::NFind::FindFile(_archiveFilePath, fileInfo))
    return ::GetLastError();
  if (fileInfo.IsDir())
    return E_FAIL;

  _compressCodecsInfo.Release();
  _codecs = new CCodecs;
  _compressCodecsInfo = _codecs;
  RINOK(_codecs->Load());

  CIntVector formats;
  int menuIndex;
  if (showFormatMenu)
  {
    CObjectVector<FormatInfo> formatInfos;
    CSysStringVector formatMenuStrings;
    for (int i = 0; i < _codecs->Formats.Size(); i++)
    {
      FormatInfo format;
      format.index = i;
      format.name = GetSystemString(_codecs->Formats[i].Name);
      formatInfos.Add(format);
    }
    formatInfos.Sort(CompareFormatInfo, NULL);
    formatMenuStrings.Add(g_StartupInfo.GetMsgString(NMessageID::kAutoArchiveFormat));
    formatMenuStrings.Add(g_StartupInfo.GetMsgString(NMessageID::kDetectArchiveFormat));
    for (int i = 0; i < formatInfos.Size(); i++)
    {
      const farChar* prefFormats[] = { _F("7z"), _F("bzip2"), _F("Cab"), _F("gzip"), _F("Iso"), _F("Nsis"), _F("Rar"), _F("tar"), _F("Udf"), _F("zip") };
      CSysString name = formatInfos[i].name;
      for (int j = 0; j < ARRAYSIZE(prefFormats); j++)
        if (name.CompareNoCase(prefFormats[j]) == 0)
          name.Insert(0, _F("&"));
      formatMenuStrings.Add(name);
    }
    menuIndex = g_StartupInfo.Menu(FMENU_AUTOHIGHLIGHT, g_StartupInfo.GetMsgString(NMessageID::kSelectArchiveFormat), NULL, formatMenuStrings, 0);
    if (menuIndex == -1)
      return E_ABORT;
    if (menuIndex == 1)
    {
      CObjectVector<CIntVector> arcIndices;
      DetectArchiveType(_codecs, _archiveFilePath, g_Options.MaxCheckSize, arcIndices, openArchiveCallback);
      if (arcIndices.Size() == 0)
        return S_FALSE;
      CSysStringVector formatNames;
      for (int i = 0; i < arcIndices.Size(); i++)
      {
        CSysString strFormat;
        for (int j = 0; j < arcIndices[i].Size(); j++)
        {
          if (j) strFormat += g_StartupInfo.GetMsgString(NMessageID::kArchiveFormatSeparator);
          strFormat += GetSystemString(_codecs->Formats[arcIndices[i][arcIndices[i].Size() - 1 - j]].Name);
        }
        formatNames.Add(strFormat);
      }
      int index = g_StartupInfo.Menu(FMENU_AUTOHIGHLIGHT, g_StartupInfo.GetMsgString(NMessageID::kSelectArchiveFormat), NULL, formatNames, 0);
      if (index == -1)
        return E_ABORT;
      formats = arcIndices[index];
    }
    else if (menuIndex != 0)
      formats.Add(formatInfos[menuIndex - 2].index);
  }

  CIntVector disabledFormats;
  if (!showFormatMenu || (menuIndex == 0))
  {
    UStringVector disabledFormatsStr = SplitString(GetUnicodeString(g_Options.DisabledFormats), L',');
    for (int i = 0; i < disabledFormatsStr.Size(); i++)
      disabledFormats.Add(_codecs->FindFormatForArchiveType(disabledFormatsStr[i]));
  }

  RINOK(OpenArchive(_codecs, formats, _archiveFilePath, g_Options.MaxCheckSize, disabledFormats, _archiveLink, openArchiveCallback));
  DefaultName = _archiveLink.GetDefaultItemName();
  const CArcInfoEx &ai = _codecs->Formats[_archiveLink.GetArchiverIndex()];

  DefaultTime = fileInfo.MTime;
  DefaultAttrib = fileInfo.Attrib;
  ArchiveType = ai.Name;
  if (archiveType == 0)
    return S_OK;
  return StringToBstr(ArchiveType, archiveType);
  COM_TRY_END
}

#define CL_FILE    1
#define CL_CMDLINE 2
#define CL_MENU    3

static HANDLE MyOpenFilePlugin(const UString name, int CallLocation)
{
  UString normalizedName = name;
  normalizedName.Trim();
  UString fullName;
  int fileNamePartStartIndex;
  g_StartupInfo.GetFullPathName((LPCWSTR)normalizedName, fullName, fileNamePartStartIndex);
  NFile::NFind::CFileInfoW fileInfo;
  if (!NFile::NFind::FindFile(fullName, fileInfo))
    return INVALID_HANDLE_VALUE;
  if (fileInfo.IsDir())
     return INVALID_HANDLE_VALUE;
  //Nsky
  if (CallLocation==CL_FILE && g_Options.UseMasks) {
    if (!g_StartupInfo.m_FSF.ProcessName(g_Options.Masks,(farChar*)(const farChar*)GetSystemString(name, CP_OEMCP),
#ifdef _UNICODE
      0, 
#endif
      PN_CMPNAMELIST | PN_SKIPPATH))
      return INVALID_HANDLE_VALUE;
  }
  //\Nsky

  CMyComPtr<IInFolderArchive> archiveHandler;

  COpenArchiveCallback *openArchiveCallbackSpec = new COpenArchiveCallback;
  CMyComPtr<IArchiveOpenCallback> openArchiveCallback = openArchiveCallbackSpec;

  openArchiveCallbackSpec->Init();
  openArchiveCallbackSpec->LoadFileInfo(
      fullName.Left(fileNamePartStartIndex),
      fullName.Mid(fileNamePartStartIndex));

  CAgent* agent = new CAgent;
  agent->showFormatMenu = CallLocation == CL_MENU;
  archiveHandler = agent;
  CMyComBSTR archiveType;
#ifdef _UNICODE
  HRESULT result = archiveHandler->Open(fullName, &archiveType, openArchiveCallback);
#else
  HRESULT result = archiveHandler->Open(GetUnicodeString(fullName, CP_OEMCP), &archiveType, openArchiveCallback);
#endif
  if (result != S_OK)
  {
    if (result == E_HANDLE)
      g_StartupInfo.ShowMessage(NMessageID::kCantLoad7Zip);
    return INVALID_HANDLE_VALUE;
  }

  CPlugin *plugin = new CPlugin(
      fullName,
      archiveHandler,
      (const wchar_t *)archiveType
      );
  if (plugin == NULL)
    return(INVALID_HANDLE_VALUE);
  plugin->PasswordIsDefined = openArchiveCallbackSpec->PasswordIsDefined;
  plugin->Password = openArchiveCallbackSpec->Password;

  return (HANDLE)(plugin);
}

#ifdef _UNICODE
HANDLE WINAPI OpenFilePluginW(const wchar_t *name,const unsigned char *Data,int DataSize,int OpMode)
#else
HANDLE WINAPI OpenFilePlugin(char *name,const unsigned char * /* data */, unsigned int /* dataSize */)
#endif
{
  MY_TRY_BEGIN;
  if (name == NULL || (!g_Options.Enabled))
  {
    // if (!Opt.ProcessShiftF1)
      return(INVALID_HANDLE_VALUE);
  }
//Nsky
  return MyOpenFilePlugin(GetUnicodeString(name, CP_OEMCP), CL_FILE);
//\Nsky
  MY_TRY_END2(_F("OpenFilePlugin"), INVALID_HANDLE_VALUE);
}

#ifdef _UNICODE
HANDLE WINAPI OpenPluginW(int openFrom, INT_PTR item)
#else
HANDLE WINAPI OpenPlugin(int openFrom, int item)
#endif
{
  MY_TRY_BEGIN;
  if(openFrom == OPEN_COMMANDLINE)
  {
    CSysString fileName = (const farChar *)(UINT_PTR)item;
    if(fileName.IsEmpty())
      return INVALID_HANDLE_VALUE;
    if (fileName.Length() >= 2 &&
        fileName[0] == '\"' && fileName[fileName.Length() - 1] == '\"')
      fileName = fileName.Mid(1, fileName.Length() - 2);

//Nsky
    return MyOpenFilePlugin(GetUnicodeString(fileName, CP_OEMCP), CL_CMDLINE);
//\Nsky
  }
  if(openFrom == OPEN_PLUGINSMENU)
  {
    switch(item)
    {
      case 0:
      {
//Nsky
        return MyOpenFilePlugin(GetUnicodeString(g_StartupInfo.GetActivePanelCurrentItemName(), CP_OEMCP), CL_MENU);
//\Nsky
      }
      case 1:
      {
        CObjectVector<MyPluginPanelItem> pluginPanelItem;
        if(!g_StartupInfo.ControlGetActivePanelSelectedOrCurrentItems(pluginPanelItem))
          throw 142134;
        if (CompressFiles(pluginPanelItem) == S_OK)
        {
          g_StartupInfo.ControlClearPanelSelection();
          g_StartupInfo.ControlUpdateActivePanel(NULL);
          g_StartupInfo.ControlRedrawActivePanel(NULL);
          g_StartupInfo.ControlUpdatePassivePanel(1);
          g_StartupInfo.ControlRedrawPassivePanel(NULL);
        }
        return INVALID_HANDLE_VALUE;
      }
      default:
        throw 4282215;
    }
  }
  return INVALID_HANDLE_VALUE;
  MY_TRY_END2(_F("OpenPlugin"), INVALID_HANDLE_VALUE);
}

#ifdef _UNICODE
void WINAPI ClosePluginW(HANDLE plugin)
#else
void WINAPI ClosePlugin(HANDLE plugin)
#endif
{
  MY_TRY_BEGIN;
  ((CPlugin *)plugin)->GetPanelMode(g_Options.PanelMode);
  g_Options.Save();
  delete (CPlugin *)plugin;
  MY_TRY_END1(_F("ClosePlugin"));
}

#ifdef _UNICODE
int WINAPI GetFindDataW(HANDLE plugin, struct PluginPanelItem **panelItems,
#else
int WINAPI GetFindData(HANDLE plugin, struct PluginPanelItem **panelItems,
#endif
    int *itemsNumber,int opMode)
{
  MY_TRY_BEGIN;
  return(((CPlugin *)plugin)->GetFindData(panelItems, itemsNumber, opMode));
  MY_TRY_END2(_F("GetFindData"), FALSE);
}

#ifdef _UNICODE
void WINAPI FreeFindDataW(HANDLE plugin, struct PluginPanelItem *panelItems,
#else
void WINAPI FreeFindData(HANDLE plugin, struct PluginPanelItem *panelItems,
#endif
    int itemsNumber)
{
  MY_TRY_BEGIN;
  ((CPlugin *)plugin)->FreeFindData(panelItems, itemsNumber);
  MY_TRY_END1(_F("FreeFindData"));
}

#ifdef _UNICODE
int WINAPI GetFilesW(HANDLE plugin, struct PluginPanelItem *panelItems, int itemsNumber, int move, const wchar_t **destPath, int opMode)
#else
int WINAPI GetFiles(HANDLE plugin, struct PluginPanelItem *panelItems, int itemsNumber, int move, char *destPath, int opMode)
#endif
{
  MY_TRY_BEGIN;
  return(((CPlugin *)plugin)->GetFiles(panelItems, itemsNumber, move, destPath, opMode));
  MY_TRY_END2(_F("GetFiles"), NFileOperationReturnCode::kError);
}

#ifdef _UNICODE
int WINAPI SetDirectoryW(HANDLE plugin, const wchar_t *dir, int opMode)
#else
int WINAPI SetDirectory(HANDLE plugin, const char *dir, int opMode)
#endif
{
  MY_TRY_BEGIN;
  return(((CPlugin *)plugin)->SetDirectory(dir, opMode));
  MY_TRY_END2(_F("SetDirectory"), FALSE);
}

#ifdef _UNICODE
void WINAPI GetPluginInfoW(struct PluginInfo *info)
#else
void WINAPI GetPluginInfo(struct PluginInfo *info)
#endif
{
  MY_TRY_BEGIN;

  info->StructSize = sizeof(*info);
  info->Flags = 0;
  info->DiskMenuStrings = NULL;
  info->DiskMenuNumbers = NULL;
  info->DiskMenuStringsNumber = 0;
  static const farChar *pluginMenuStrings[2];
  pluginMenuStrings[0] = g_StartupInfo.GetMsgString(NMessageID::kOpenArchiveMenuString);
  pluginMenuStrings[1] = g_StartupInfo.GetMsgString(NMessageID::kCreateArchiveMenuString);
  info->PluginMenuStrings = (farChar **)pluginMenuStrings;
  info->PluginMenuStringsNumber = 2;
  static const farChar *pluginCfgStrings[1];
  pluginCfgStrings[0] = g_StartupInfo.GetMsgString(NMessageID::kOpenArchiveMenuString);
  info->PluginConfigStrings = (farChar **)pluginCfgStrings;
  info->PluginConfigStringsNumber = sizeof(pluginCfgStrings) / sizeof(pluginCfgStrings[0]);
  info->CommandPrefix = (farChar *)kCommandPrefix;
  MY_TRY_END1(_F("GetPluginInfo"));
}

#ifdef _UNICODE
int WINAPI ConfigureW(int /* itemNumber */)
#else
int WINAPI Configure(int /* itemNumber */)
#endif
{
  MY_TRY_BEGIN;

  const int kEnabledCheckBoxIndex = 1;
  const int kUseMasksCheckBoxIndex = 2;
  const int kMasksIndex = 3;
  const int kDisabledFormatsIndex = 5;
  const int kMaxCheckSizeIndex = 7;

  const int kYSize = 17;
  const int kXSize = 76;

  CSysString AvailableMasks = GetMaskList();
  CSysString AvailableFormats = GetFormatList();

  farChar s[32];
  ConvertUInt64ToString(g_Options.MaxCheckSize / 1024, s);
  CSysString MaxCheckSize = s;
  int MaxCheckSizePos = 5 + lstrlenF(g_StartupInfo.GetMsgString(NMessageID::kConfigMaxCheckSize1));

  struct CInitDialogItem initItems[]=
  {
    { DI_DOUBLEBOX, 3, 1, kXSize - 4, kYSize - 2, false, false, 0, false, NMessageID::kConfigTitle, NULL, NULL },
    { DI_CHECKBOX, 5, 2, 0, 0, true, g_Options.Enabled, 0, false, NMessageID::kConfigPluginEnabled, NULL, NULL },
    { DI_CHECKBOX, 5, 3, 0, 0, false, g_Options.UseMasks, 0, false, NMessageID::kConfigUseMasks, NULL, NULL },
    { DI_EDIT, 5, 4, kXSize - 6, 0, false, false, 0, false, -1, g_Options.Masks, NULL },
    { DI_TEXT, 5, 5, 0, 0, false, false, 0, false, NMessageID::kConfigDisabledFormats, NULL, NULL },
    { DI_EDIT, 5, 6, kXSize - 6, 0, false, false, 0, false, -1, g_Options.DisabledFormats, NULL },
    { DI_TEXT, 5, 7, 0, 0, false, false, 0, false, NMessageID::kConfigMaxCheckSize1, NULL, NULL },
    { DI_EDIT, MaxCheckSizePos, 7, MaxCheckSizePos + 4, 0, false, false, 0, false, -1, MaxCheckSize, NULL },
    { DI_TEXT, MaxCheckSizePos + 4 + 2, 7, 0, 0, false, false, 0, false, NMessageID::kConfigMaxCheckSize2, NULL, NULL },
    { DI_TEXT, 5, 8, 0, 0, false, false, DIF_BOXCOLOR | DIF_SEPARATOR, false, -1, _F(""), NULL },
    { DI_TEXT, 5, 9, 0, 0, false, false, 0, false, NMessageID::kConfigAvailableMasks, NULL, NULL },
    { DI_EDIT, 5, 10, kXSize - 6, 0, false, false, DIF_READONLY, false, -1, AvailableMasks, NULL },
    { DI_TEXT, 5, 11, 0, 0, false, false, 0, false, NMessageID::kConfigAvailableFormats, NULL, NULL },
    { DI_EDIT, 5, 12, kXSize - 6, 0, false, false, DIF_READONLY, false, -1, AvailableFormats, NULL },
    { DI_TEXT, 5, 13, 0, 0, false, false, DIF_BOXCOLOR | DIF_SEPARATOR, false, -1, _F(""), NULL },
    { DI_BUTTON, 0, kYSize - 3, 0, 0, false, false, DIF_CENTERGROUP, true, NMessageID::kOk, NULL, NULL },
    { DI_BUTTON, 0, kYSize - 3, 0, 0, false, false, DIF_CENTERGROUP, false, NMessageID::kCancel, NULL, NULL },
  };

  const int kNumDialogItems = sizeof(initItems) / sizeof(initItems[0]);
  const int kOkButtonIndex = kNumDialogItems - 2;

  FarDialogItem dialogItems[kNumDialogItems];
  g_StartupInfo.InitDialogItems(initItems, dialogItems, kNumDialogItems);

#ifdef _UNICODE
  HANDLE hDlg = 0;
  int askCode = g_StartupInfo.ShowDialog(kXSize, kYSize, kHelpTopicConfig, dialogItems, kNumDialogItems, hDlg);

  if (askCode == kOkButtonIndex)
  {
    g_Options.Enabled = BOOLToBool(g_StartupInfo.GetItemSelected(hDlg, kEnabledCheckBoxIndex));
    g_Options.UseMasks = BOOLToBool(g_StartupInfo.GetItemSelected(hDlg, kUseMasksCheckBoxIndex));
    g_Options.Masks = g_StartupInfo.GetItemData(hDlg, kMasksIndex);
    g_Options.DisabledFormats = g_StartupInfo.GetItemData(hDlg, kDisabledFormatsIndex);
    MaxCheckSize = g_StartupInfo.GetItemData(hDlg, kMaxCheckSizeIndex);
  }

  g_StartupInfo.DialogFree(hDlg);
#else
  int askCode = g_StartupInfo.ShowDialog(kXSize, kYSize, kHelpTopicConfig, dialogItems, kNumDialogItems);
#endif

  if (askCode != kOkButtonIndex)
    return (FALSE);

#ifndef _UNICODE
  g_Options.Enabled = BOOLToBool(dialogItems[kEnabledCheckBoxIndex].Selected);
  g_Options.UseMasks = BOOLToBool(dialogItems[kUseMasksCheckBoxIndex].Selected);
  g_Options.Masks = dialogItems[kMasksIndex].Data;
  g_Options.DisabledFormats = dialogItems[kDisabledFormatsIndex].Data;
  MaxCheckSize = dialogItems[kMaxCheckSizeIndex].Data;
#endif

  g_Options.MaxCheckSize = g_StartupInfo.m_FSF.atoi(MaxCheckSize) * 1024;
  g_Options.Validate();
  g_Options.Save();
  return(TRUE);
  MY_TRY_END2(_F("Configure"), FALSE);
}

#ifdef _UNICODE
void WINAPI GetOpenPluginInfoW(HANDLE plugin,struct OpenPluginInfo *info)
#else
void WINAPI GetOpenPluginInfo(HANDLE plugin,struct OpenPluginInfo *info)
#endif
{
  MY_TRY_BEGIN;
  ((CPlugin *)plugin)->GetOpenPluginInfo(info, g_Options.PanelMode);
  MY_TRY_END1(_F("GetOpenPluginInfo"));
}

#ifdef _UNICODE
int WINAPI PutFilesW(HANDLE plugin, struct PluginPanelItem *panelItems, int itemsNumber, int move, const wchar_t *srcPath, int opMode)
#else
int WINAPI PutFiles(HANDLE plugin, struct PluginPanelItem *panelItems, int itemsNumber, int move, int opMode)
#endif
{
  MY_TRY_BEGIN;
#ifdef _UNICODE
  return(((CPlugin *)plugin)->PutFiles(srcPath, panelItems, itemsNumber, move, opMode));
#else
  return(((CPlugin *)plugin)->PutFiles(panelItems, itemsNumber, move, opMode));
#endif
  MY_TRY_END2(_F("PutFiles"), NFileOperationReturnCode::kError);
}

#ifdef _UNICODE
int WINAPI DeleteFilesW(HANDLE plugin, PluginPanelItem *panelItems,
#else
int WINAPI DeleteFiles(HANDLE plugin, PluginPanelItem *panelItems,
#endif
    int itemsNumber, int opMode)
{
  MY_TRY_BEGIN;
  return(((CPlugin *)plugin)->DeleteFiles(panelItems, itemsNumber, opMode));
  MY_TRY_END2(_F("DeleteFiles"), FALSE);
}

#ifdef _UNICODE
int WINAPI ProcessKeyW(HANDLE plugin, int key, unsigned int controlState)
#else
int WINAPI ProcessKey(HANDLE plugin, int key, unsigned int controlState)
#endif
{
  MY_TRY_BEGIN;
  return (((CPlugin *)plugin)->ProcessKey(key, controlState));
  MY_TRY_END2(_F("ProcessKey"), FALSE);
}
