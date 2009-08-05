// Test Align for updating !!!!!!!!!!!!!!!!!!

#include "StdAfx.h"

// #include <locale.h>
#include <initguid.h>

#include "Plugin.h"

#include "Common/Wildcard.h"
#include "Common/DynamicBuffer.h"
#include "Common/StringConvert.h"
#include "Common/Defs.h"

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

static const farChar *kCommandPrefix = _F("7-zip");

static const farChar *kRegistryMainKeyName = _F("");

static const farChar *kRegistryValueNameEnabled = _F("UsedByDefault3");
static bool kPluginEnabledDefault = true;
//Nsky
static const farChar *kRegistryValueNameUseMasks = _F("UseMasks");
static bool kUseMasksDefault = false;
static const farChar *kRegistryValueNameMasks = _F("Masks");
//static const farChar *kMasksDefault = _F("*.7z,*.bz,*.arj,*.bz2,*.cab,*.gz,*.lzh,*.rar,*.r[0-9][0-9],*.tar,*.z,*.zip,*.tbz,*.tbz2,*.tgz,*.taz,*.rpm,*.cpio,*.deb,*.001,*.jar,*.xpi,*.chm");
//\Nsky
static const farChar *kRegistryValueNameDisabledFormats = _F("DisabledFormats");

static const farChar *kHelpTopicConfig =  _F("Config");

extern "C"
{
#ifndef _UNICODE
	void WINAPI SetStartupInfo(const struct PluginStartupInfo *Info);
	HANDLE WINAPI OpenFilePlugin(char *Name,const unsigned char *Data,int DataSize);
	HANDLE WINAPI OpenPlugin(int OpenFrom,INT_PTR Item);
	void WINAPI ClosePlugin(HANDLE plugin);
	int WINAPI GetFindData(HANDLE plugin, struct PluginPanelItem **panelItems,
		int *itemsNumber, int OpMode);
	void WINAPI FreeFindData(HANDLE plugin, struct PluginPanelItem *panelItems,
		int itemsNumber);
	int WINAPI GetFiles(HANDLE plugin, struct PluginPanelItem *panelItems,
		int itemsNumber, int move, char *destPath, int opMode);
	int WINAPI SetDirectory(HANDLE plugin, const char *dir, int opMode);
	void WINAPI GetPluginInfo(struct PluginInfo *info);
	int WINAPI Configure(int itemNumber);
	void WINAPI GetOpenPluginInfo(HANDLE plugin, struct OpenPluginInfo *info);
	int WINAPI PutFiles(HANDLE plugin, struct PluginPanelItem *panelItems,
		int itemsNumber, int move, int opMode);
	int WINAPI DeleteFiles(HANDLE plugin, PluginPanelItem *panelItems,
		int itemsNumber, int opMode);
	int WINAPI ProcessKey(HANDLE plugin, int key, unsigned int controlState);
#else
	int WINAPI GetMinFarVersionW(void);
	void WINAPI SetStartupInfoW(const struct PluginStartupInfo *info);
	HANDLE WINAPI OpenFilePluginW(const wchar_t *name,const unsigned char *Data,int DataSize,int OpMode);
	HANDLE WINAPI OpenPluginW(int openFrom, INT_PTR item);
	void WINAPI ClosePluginW(HANDLE plugin);
	int WINAPI GetFindDataW(HANDLE plugin, struct PluginPanelItem **panelItems,	int *itemsNumber, int OpMode);
	void WINAPI FreeFindDataW(HANDLE plugin, struct PluginPanelItem *panelItems,	int itemsNumber);
	int WINAPI GetFilesW(HANDLE plugin, struct PluginPanelItem *panelItems,	int itemsNumber, int move, const wchar_t **destPath, int opMode);
	int WINAPI SetDirectoryW(HANDLE plugin,const wchar_t *dir, int opMode);
	void WINAPI GetPluginInfoW(struct PluginInfo *info);
	int WINAPI ConfigureW(int itemNumber);
	void WINAPI GetOpenPluginInfoW(HANDLE plugin, struct OpenPluginInfo *info);
	int WINAPI PutFilesW(HANDLE plugin, struct PluginPanelItem *panelItems,	int itemsNumber, int move, int opMode);
	int WINAPI DeleteFilesW(HANDLE plugin, PluginPanelItem *panelItems,	int itemsNumber, int opMode);
	int WINAPI ProcessKeyW(HANDLE plugin, int key, unsigned int controlState);
#endif
};

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

static struct COptions
{
  bool Enabled;
//Nsky
  bool UseMasks;
  CSysString Masks;
//\Nsky
  CSysString DisabledFormats;
} g_Options;

static const farChar *kPluginNameForRegistry = _F("7-ZIP");

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

#ifdef _UNICODE
int WINAPI _export GetMinFarVersionW(void)
{
	return MAKEFARVERSION(2,0,992);//��� ��� breaking change
}
#endif

CSysStringVector SplitString(const CSysString& Str, farChar SplitCh)
{
  CSysStringVector List;
  int Pos = 0;
  while (Pos < Str.Length())
  {
    int Pos2 = Str.Find(SplitCh, Pos);
    if (Pos2 == -1)
      Pos2 = Str.Length();
    CSysString SubStr = Str.Mid(Pos, Pos2 - Pos);
    SubStr.Trim();
    List.Add(SubStr);
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
  g_Options.Enabled = g_StartupInfo.QueryRegKeyValue(
      HKEY_CURRENT_USER, kRegistryMainKeyName,
      kRegistryValueNameEnabled, kPluginEnabledDefault);
//Nsky
  g_Options.UseMasks = g_StartupInfo.QueryRegKeyValue(
      HKEY_CURRENT_USER, kRegistryMainKeyName,
      kRegistryValueNameUseMasks, kUseMasksDefault);
  g_Options.Masks = g_StartupInfo.QueryRegKeyValue(
      HKEY_CURRENT_USER, kRegistryMainKeyName,
      kRegistryValueNameMasks, (CSysString)_F(""));//kMasksDefault);
  if (g_Options.Masks == _F("")) g_Options.Masks = GetMaskList();
//\Nsky
  g_Options.DisabledFormats = g_StartupInfo.QueryRegKeyValue(HKEY_CURRENT_USER, kRegistryMainKeyName, kRegistryValueNameDisabledFormats, (CSysString)_F(""));
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

  DWORD m_PrevTickCount;

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
    m_PrevTickCount = GetTickCount();
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
    if (currentTime - m_PrevTickCount < 400)
      return;
    _progressBox.Init(
        // g_StartupInfo.GetMsgString(NMessageID::kWaitTitle),
        g_StartupInfo.GetMsgString(NMessageID::kReading), 48);

    m_MessageBoxIsShown = true;
  }
  else
  {
    if (currentTime - m_PrevTickCount < 200)
      return;
  }
  m_PrevTickCount = currentTime;
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

//Nsky
#define CL_FILE    1
#define CL_CMDLINE 2
#define CL_MENU    3

static HANDLE MyOpenFilePlugin(const UString name, int CallLocation)
//\Nsky
{
	UString normalizedName = name;
  normalizedName.Trim();
  UString fullName;
  int fileNamePartStartIndex;
  NFile::NDirectory::MyGetFullPathName(normalizedName, fullName, fileNamePartStartIndex);
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

  CScreenRestorer screenRestorer;
  screenRestorer.Save();

  COpenArchiveCallback *openArchiveCallbackSpec = new COpenArchiveCallback;
  CMyComPtr<IArchiveOpenCallback> openArchiveCallback = openArchiveCallbackSpec;

  openArchiveCallbackSpec->Init();
  openArchiveCallbackSpec->LoadFileInfo(
      fullName.Left(fileNamePartStartIndex),
      fullName.Mid(fileNamePartStartIndex));

  archiveHandler = new CAgent;
  CMyComBSTR archiveType;
#ifdef _UNICODE
	HRESULT result = archiveHandler->Open(fullName, &archiveType, openArchiveCallback);
#else
	HRESULT result = archiveHandler->Open(GetUnicodeString(fullName, CP_OEMCP), &archiveType, openArchiveCallback);
#endif
  if (result != S_OK)
  {
    if (result == E_ABORT)
      return (HANDLE)-2;
		if (result == E_HANDLE)
			g_StartupInfo.ShowMessage(NMessageID::kCantLoad7Zip);
    return INVALID_HANDLE_VALUE;
  }

  if (SplitString(g_Options.DisabledFormats, _F(',')).Find(GetSystemString(UString(archiveType))) != -1)
  {
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
          g_StartupInfo.ControlUpdatePassivePanel(NULL);
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
int WINAPI GetFilesW(HANDLE plugin, struct PluginPanelItem *panelItems,	int itemsNumber, int move, const wchar_t **destPath, int opMode)
#else
int WINAPI GetFiles(HANDLE plugin, struct PluginPanelItem *panelItems,	int itemsNumber, int move, char *destPath, int opMode)
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

  const int kYSize = 16;
  const int kXSize = 76;

  CSysString AvailableMasks = GetMaskList();
  CSysString AvailableFormats = GetFormatList();

  struct CInitDialogItem initItems[]=
  {
    { DI_DOUBLEBOX, 3, 1, kXSize - 4, kYSize - 2, false, false, 0, false, NMessageID::kConfigTitle, NULL, NULL },
    { DI_CHECKBOX, 5, 2, 0, 0, true, g_Options.Enabled, 0, false, NMessageID::kConfigPluginEnabled, NULL, NULL },
    { DI_CHECKBOX, 5, 3, 0, 0, false, g_Options.UseMasks, 0, false, NMessageID::kConfigUseMasks, NULL, NULL },
    { DI_EDIT, 5, 4, kXSize - 6, 0, false, false, 0, false, -1, g_Options.Masks, NULL },
    { DI_TEXT, 5, 5, 0, 0, false, false, 0, false, NMessageID::kConfigDisabledFormats, NULL, NULL },
    { DI_EDIT, 5, 6, kXSize - 6, 0, false, false, 0, false, -1, g_Options.DisabledFormats, NULL },
    { DI_TEXT, 5, 7, 0, 0, false, false, DIF_BOXCOLOR | DIF_SEPARATOR, false, -1, _F(""), NULL },
    { DI_TEXT, 5, 8, 0, 0, false, false, 0, false, NMessageID::kConfigAvailableMasks, NULL, NULL },
    { DI_EDIT, 5, 9, kXSize - 6, 0, false, false, DIF_READONLY, false, -1, AvailableMasks, NULL },
    { DI_TEXT, 5, 10, 0, 0, false, false, 0, false, NMessageID::kConfigAvailableFormats, NULL, NULL },
    { DI_EDIT, 5, 11, kXSize - 6, 0, false, false, DIF_READONLY, false, -1, AvailableFormats, NULL },
    { DI_TEXT, 5, 12, 0, 0, false, false, DIF_BOXCOLOR | DIF_SEPARATOR, false, -1, _F(""), NULL },
    { DI_BUTTON, 0, kYSize - 3, 0, 0, false, false, DIF_CENTERGROUP, true, NMessageID::kOk, NULL, NULL },
    { DI_BUTTON, 0, kYSize - 3, 0, 0, false, false, DIF_CENTERGROUP, false, NMessageID::kCancel, NULL, NULL },
  };

  const int kNumDialogItems = sizeof(initItems) / sizeof(initItems[0]);
  const int kOkButtonIndex = kNumDialogItems - 2;

  FarDialogItem dialogItems[kNumDialogItems];
  g_StartupInfo.InitDialogItems(initItems, dialogItems, kNumDialogItems);

#ifdef _UNICODE
	//Nsky
	HANDLE hDlg = 0;
	int askCode = g_StartupInfo.ShowDialog(kXSize, kYSize,
		kHelpTopicConfig, dialogItems, kNumDialogItems, hDlg);
	//\Nsky

  if (askCode == kOkButtonIndex)
  {
    g_Options.Enabled = BOOLToBool(g_StartupInfo.GetItemSelected(hDlg, kEnabledCheckBoxIndex));
    //Nsky
    g_Options.UseMasks = BOOLToBool(g_StartupInfo.GetItemSelected(hDlg, kUseMasksCheckBoxIndex));
    g_Options.Masks = g_StartupInfo.GetItemData(hDlg, kMasksIndex);
    //\Nsky
  	g_Options.DisabledFormats = g_StartupInfo.GetItemData(hDlg, kDisabledFormatsIndex);
  }

	g_StartupInfo.DialogFree(hDlg);
#else
	//Nsky
	int askCode = g_StartupInfo.ShowDialog(kXSize, kYSize,
		kHelpTopicConfig, dialogItems, kNumDialogItems);
	//\Nsky
#endif

  if (askCode != kOkButtonIndex)
    return (FALSE);

#ifndef _UNICODE
	g_Options.Enabled = BOOLToBool(dialogItems[kEnabledCheckBoxIndex].Selected);
	//Nsky
	g_Options.UseMasks = BOOLToBool(dialogItems[kUseMasksCheckBoxIndex].Selected);
	g_Options.Masks = dialogItems[kMasksIndex].Data;
	//\Nsky
	g_Options.DisabledFormats = dialogItems[kDisabledFormatsIndex].Data;
#endif

	g_Options.Masks.Trim();
	if (g_Options.Masks.Length() == 0)
		g_Options.Masks = GetMaskList(); //kMasksDefault;
	g_Options.DisabledFormats.Trim();

  g_StartupInfo.SetRegKeyValue(HKEY_CURRENT_USER, kRegistryMainKeyName,
      kRegistryValueNameEnabled, g_Options.Enabled);
//Nsky
  g_StartupInfo.SetRegKeyValue(HKEY_CURRENT_USER, kRegistryMainKeyName,
      kRegistryValueNameUseMasks, g_Options.UseMasks);
  g_StartupInfo.SetRegKeyValue(HKEY_CURRENT_USER, kRegistryMainKeyName,
      kRegistryValueNameMasks, g_Options.Masks);
//Nsky
  g_StartupInfo.SetRegKeyValue(HKEY_CURRENT_USER, kRegistryMainKeyName, kRegistryValueNameDisabledFormats, g_Options.DisabledFormats);
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
  ((CPlugin *)plugin)->GetOpenPluginInfo(info);
  MY_TRY_END1(_F("GetOpenPluginInfo"));
}

#ifdef _UNICODE
int WINAPI PutFilesW(HANDLE plugin, struct PluginPanelItem *panelItems,
#else
int WINAPI PutFiles(HANDLE plugin, struct PluginPanelItem *panelItems,
#endif
                   int itemsNumber, int move, int opMode)
{
  MY_TRY_BEGIN;
  return(((CPlugin *)plugin)->PutFiles(panelItems, itemsNumber, move, opMode));
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