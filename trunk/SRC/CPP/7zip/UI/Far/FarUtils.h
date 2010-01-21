// FarUtils.h

#ifndef __FARUTILS_H
#define __FARUTILS_H

#ifdef _UNICODE
#include "FarPluginU.h"
#define farChar wchar_t
#define _F(x)     L ## x
#define lstrlenF lstrlenW
#define lstrcpyF lstrcpyW
#define lstrcatF lstrcatW
#define lstrcmpF lstrcmpW
#define FCTL_GETPANELSHORTINFO FCTL_GETPANELINFO
#else
#define kInfoPanelLineSize 80
#include "FarPlugin.h"
#define farChar char
#define _F(x)      x
#define lstrlenF lstrlenA
#define lstrcpyF lstrcpyA
#define lstrcatF lstrcatA
#define lstrcmpF lstrcmpA
#endif

#include "Windows/Registry.h"

namespace NFar {

namespace NFileOperationReturnCode
{
  enum EEnum
  {
    kInterruptedByUser = -1,
    kError = 0,
    kSuccess = 1
  };
}

namespace NEditorReturnCode
{
  enum EEnum
  {
    kOpenError = 0,
    kFileWasChanged = 1,
    kFileWasNotChanged = 2,
    kInterruptedByUser = 3
  };
}

struct MyPluginPanelItem
{
  CSysString strFileName;
  DWORD dwAttributes;
  UINT32 UserData;
};

struct CInitDialogItem
{
  DialogItemTypes Type;
  int X1,Y1,X2,Y2;
  bool Focus;
  bool Selected;
  unsigned int Flags; //FarDialogItemFlags Flags;
  bool DefaultButton;
  int DataMessageId;
  const farChar *DataString;
  const farChar *HistoryName;
  // void InitToFarDialogItem(struct FarDialogItem &anItemDest);
};

class CStartupInfo
{
//Nsky
//  PluginStartupInfo m_Data; //moved to public
//\Nsky
  CSysString m_RegistryPath;

  CSysString GetFullKeyName(const CSysString &keyName) const;
  LONG CreateRegKey(HKEY parentKey,
    const CSysString &keyName, NWindows::NRegistry::CKey &destKey) const;
  LONG OpenRegKey(HKEY parentKey,
    const CSysString &keyName, NWindows::NRegistry::CKey &destKey) const;

public:
  //Nsky
  PluginStartupInfo m_Data;
  FarStandardFunctions m_FSF;
  //\Nsky
  void Init(const PluginStartupInfo &pluginStartupInfo,
      const CSysString &pliginNameForRegestry);
  const farChar *GetMsgString(int messageId);
  int ShowMessage(unsigned int flags, const farChar *helpTopic,
      const farChar **items, int numItems, int numButtons);
  int ShowMessage(const farChar *message);
  int ShowMessage(int messageId);

  //bool SetItemData(const HANDLE &hDlg, DWORD item, CSysString &str );
#ifdef _UNICODE
  int ShowDialog(int X1, int Y1, int X2, int Y2,
    const farChar *helpTopic, struct FarDialogItem *items, int numItems, HANDLE & hDlg);
  int ShowDialog(int sizeX, int sizeY,
    const farChar *helpTopic, struct FarDialogItem *items, int numItems, HANDLE & hDlg);
  BOOL GetItemSelected(const HANDLE &hDlg, DWORD item );
  void DialogFree(HANDLE &hDlg);
#else
  int ShowDialog(int X1, int Y1, int X2, int Y2,
    const farChar *helpTopic, struct FarDialogItem *items, int numItems);
  int ShowDialog(int sizeX, int sizeY,
    const farChar *helpTopic, struct FarDialogItem *items, int numItems);
#endif

  void InitDialogItems(const CInitDialogItem *srcItems,
      FarDialogItem *destItems, int numItems);

  HANDLE SaveScreen(int X1, int Y1, int X2, int Y2);
  HANDLE SaveScreen();
  void RestoreScreen(HANDLE handle);

  void SetRegKeyValue(HKEY parentKey, const CSysString &keyName,
      const LPCTSTR valueName, LPCTSTR value) const;
  void SetRegKeyValue(HKEY hRoot, const CSysString &keyName,
      const LPCTSTR valueName, int value) const;
  void SetRegKeyValue(HKEY hRoot, const CSysString &keyName,
      const LPCTSTR valueName, bool value) const;

  CSysString QueryRegKeyValue(HKEY parentKey, const CSysString &keyName,
      LPCTSTR valueName, const CSysString &valueDefault) const;

  int QueryRegKeyValue(HKEY parentKey, const CSysString &keyName,
      LPCTSTR valueName, int valueDefault) const;

  bool QueryRegKeyValue(HKEY parentKey, const CSysString &keyName,
      LPCTSTR valueName, bool valueDefault) const;

  bool Control(HANDLE plugin, int command, int param1, LONG_PTR param2);
  LONG_PTR Control2(HANDLE pluginHandle, int command, int param1, LONG_PTR param2);
  bool ControlRequestActivePanel(int command, int param1, LONG_PTR param);
  int  ControlRequestActivePanel2(int command, int param1, LONG_PTR param);
#ifdef _UNICODE
  CSysString GetItemData(const HANDLE &hDlg, DWORD item );
  FarDialogItem * GetFarDialogItem(const HANDLE &hDlg, DWORD item);
  PluginPanelItem * GetFarPluginPanelItem(HANDLE hPanel, int FCTL, int i );
  bool ControlUpdateActivePanel(int param);
  bool ControlUpdatePassivePanel(int param);
#else
  bool ControlUpdateActivePanel(LONG_PTR param);
  bool ControlUpdatePassivePanel(LONG_PTR param);
#endif
  bool ControlRedrawActivePanel(LONG_PTR param);
  bool ControlRedrawPassivePanel(LONG_PTR param);
  bool ControlGetActivePanelInfo(PanelInfo &panelInfo);
  DWORD_PTR GetActivePanelUserData(bool bSelected, int i);
  CSysString GetActivePanelName(bool bSelected, int i);
  DWORD GetActivePanelAtt(bool bSelected, int i);

  CSysString GetActivePanelCurrentItemName();
  DWORD GetActivePanelCurrentItemAtt();
  DWORD_PTR GetActivePanelCurrentItemData();
  bool ControlGetActivePanelCurrentItemInfo(PluginPanelItem &pluginPanelItem, PanelInfo &panelInfo);
  bool ControlGetActivePanelSelectedOrCurrentItems(
      CObjectVector<MyPluginPanelItem> &pluginPanelItems);

  bool ControlClearPanelSelection();

  int Menu(
      int x,
      int y,
      int maxHeight,
      unsigned int flags,
      const farChar *title,
      const farChar *aBottom,
      const farChar *helpTopic,
      int *breakKeys,
      int *breakCode,
      FarMenuItem *items,
      int numItems);
  int Menu(
      unsigned int flags,
      const farChar *title,
      const farChar *helpTopic,
      FarMenuItem *items,
      int numItems);

  int Menu(
      unsigned int flags,
      const farChar *title,
      const farChar *helpTopic,
      const CSysStringVector &items,
      int selectedItem);

  bool GetCurrentDirectory(HANDLE hPanel, UString &resultPath);
  bool GetFullPathName(LPCWSTR fileName, UString &resultPath, int &fnStartIndex);
  int Editor(const farChar *fileName, const farChar *title,
      int X1, int Y1, int X2, int Y2, DWORD flags, int startLine, int startChar)
      { return m_Data.Editor((farChar *)fileName, (farChar *)title, X1, Y1, X2, Y2,
        flags, startLine, startChar
#ifdef _UNICODE
      , CP_AUTODETECT
#endif
        ); }
  int Editor(const farChar *fileName)
      { return Editor(fileName, NULL, 0, 0, -1, -1, 0, -1, -1); }

  int Viewer(const farChar *fileName, const farChar *title,
      int X1, int Y1, int X2, int Y2, DWORD flags)
      { return m_Data.Viewer((farChar *)fileName, (farChar *)title, X1, Y1, X2, Y2, flags
#ifdef _UNICODE
      , CP_AUTODETECT
#endif
      ); }
  int Viewer(const farChar *fileName)
      { return Viewer(fileName, NULL, 0, 0, -1, -1, VF_NONMODAL); }
  void SetProgressState(TBPFLAG state);
  void SetProgressValue(unsigned __int64 completed, unsigned __int64 total);
};

class CScreenRestorer
{
  bool m_Saved;
  HANDLE m_HANDLE;
public:
  CScreenRestorer(): m_Saved(false){};
  ~CScreenRestorer();
  void Save();
  void Restore();
};


extern CStartupInfo g_StartupInfo;

void PrintErrorMessage(const farChar *message, int code);
void PrintErrorMessage(const farChar *message, const farChar *aText);

#define  MY_TRY_BEGIN   try\
  {

#define  MY_TRY_END1(x)     }\
  catch(int n) { PrintErrorMessage(x, n);  return; }\
  catch(const CSysString &s) { PrintErrorMessage(x, s); return; }\
  catch(const farChar *s) { PrintErrorMessage(x, s); return; }\
  catch(...) { g_StartupInfo.ShowMessage(x);  return; }

#define  MY_TRY_END2(x, y)     }\
  catch(int n) { PrintErrorMessage(x, n); return y; }\
  catch(const CSysString &s) { PrintErrorMessage(x, s); return y; }\
  catch(const farChar *s) { PrintErrorMessage(x, s); return y; }\
  catch(...) { g_StartupInfo.ShowMessage(x); return y; }

bool WasEscPressed();
void ShowErrorMessage(DWORD errorCode);
void ShowLastErrorMessage();

}

#endif
