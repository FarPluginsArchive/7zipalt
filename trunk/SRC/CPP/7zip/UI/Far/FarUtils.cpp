// FarUtils.cpp

#include "StdAfx.h"

#include "FarUtils.h"
#include "Common/DynamicBuffer.h"
#include "Common/StringConvert.h"
#include "Windows/Defs.h"
#include "Windows/Console.h"
#include "Windows/Error.h"
#include "Messages.h"

using namespace NWindows;

namespace NFar {

CStartupInfo g_StartupInfo;

void CStartupInfo::Init(const PluginStartupInfo &pluginStartupInfo,
    const CSysString &pliginNameForRegestry)
{
  m_Data = pluginStartupInfo;
  //Nsky
  m_FSF = *pluginStartupInfo.FSF;
  //\Nsky
  m_RegistryPath = pluginStartupInfo.RootKey;
  m_RegistryPath += '\\';
  m_RegistryPath += pliginNameForRegestry;
}

const farChar *CStartupInfo::GetMsgString(int messageId)
{
  return (const farChar*)m_Data.GetMsg(m_Data.ModuleNumber, messageId);
}

int CStartupInfo::ShowMessage(unsigned int flags,
    const farChar *helpTopic, const farChar **items, int numItems, int numButtons)
{
  return m_Data.Message(m_Data.ModuleNumber, flags, (farChar *)helpTopic,
    (farChar **)items, numItems, numButtons);
}

int CStartupInfo::ShowMessage(const farChar *message)
{
  const farChar *messagesItems[]= { GetMsgString(NMessageID::kError), message,
      GetMsgString(NMessageID::kOk) };
  return ShowMessage(FMSG_WARNING, NULL, messagesItems,
      sizeof(messagesItems) / sizeof(messagesItems[0]), 1);
}
int CStartupInfo::ShowMessage(int messageId)
{
	return ShowMessage(GetMsgString(messageId));
}
static void SplitString(const AString &srcString, AStringVector &destStrings)
{
  destStrings.Clear();
  AString string;
  int len = srcString.Length();
  if (len == 0)
    return;
  for (int i = 0; i < len; i++)
  {
    char c = srcString[i];
    if (c == '\n')
    {
      if (!string.IsEmpty())
      {
        destStrings.Add(string);
        string.Empty();
      }
    }
    else
      string += c;
  }
  if (!string.IsEmpty())
    destStrings.Add(string);
}

#ifdef _UNICODE
int CStartupInfo::ShowDialog(int X1, int Y1, int X2, int Y2,
														 const farChar *helpTopic, struct FarDialogItem *items, int numItems, HANDLE &hDlg)
#else
int CStartupInfo::ShowDialog(int X1, int Y1, int X2, int Y2,
														 const farChar *helpTopic, struct FarDialogItem *items, int numItems)
#endif
{
	int res = 0;
#ifdef _UNICODE
	hDlg = m_Data.DialogInit(m_Data.ModuleNumber, X1, Y1, X2, Y2, (farChar *)helpTopic,	items, numItems, 0, 0, NULL, 0);
	res = m_Data.DialogRun(hDlg);
#else
	res =  m_Data.Dialog(m_Data.ModuleNumber, X1, Y1, X2, Y2, (farChar *)helpTopic,	items, numItems);
#endif

	return res;
}
#ifdef _UNICODE
int CStartupInfo::ShowDialog(int sizeX, int sizeY, const farChar *helpTopic, struct FarDialogItem *items, int numItems, HANDLE & hDlg)
#else
int CStartupInfo::ShowDialog(int sizeX, int sizeY, const farChar *helpTopic, struct FarDialogItem *items, int numItems)
#endif
{
  return ShowDialog(-1, -1, sizeX, sizeY, helpTopic, items, numItems
#ifdef _UNICODE
		, hDlg);
#else
		);
#endif
}

#ifdef _UNICODE
CSysString CStartupInfo::GetItemData(const HANDLE &hDlg, DWORD item ) 
{
	FarDialogItem *DialogItem = GetFarDialogItem(hDlg, item);
	if (DialogItem)
	{
		CSysString ret = DialogItem->PtrData;
		HeapFree(GetProcessHeap(), 0, DialogItem);
		return ret;
	}

	return _F("");
}
BOOL CStartupInfo::GetItemSelected(const HANDLE &hDlg, DWORD item ) 
{
	FarDialogItem *DialogItem = GetFarDialogItem(hDlg, item);
	if (DialogItem)
	{
		int ret = DialogItem->Selected;
		HeapFree(GetProcessHeap(), 0, DialogItem);
		return ret;
	}

	return 0;
}
void CStartupInfo::DialogFree( HANDLE &hDlg )
{
	m_Data.DialogFree(hDlg);
	hDlg = 0;
}
#endif

inline static BOOL GetBOOLValue(bool v) { return (v? TRUE: FALSE); }

void CStartupInfo::InitDialogItems(const CInitDialogItem  *srcItems,
    FarDialogItem *destItems, int numItems)
{
  for (int i = 0; i < numItems; i++)
  {
    const CInitDialogItem &srcItem = srcItems[i];
    FarDialogItem &destItem = destItems[i];

    destItem.Type = srcItem.Type;
    destItem.X1 = srcItem.X1;
    destItem.Y1 = srcItem.Y1;
    destItem.X2 = srcItem.X2;
    destItem.Y2 = srcItem.Y2;
    destItem.Focus = GetBOOLValue(srcItem.Focus);
		if(srcItem.HistoryName != NULL)
			destItem.History = srcItem.HistoryName;
		else
			destItem.Selected = GetBOOLValue(srcItem.Selected);
    destItem.Flags = srcItem.Flags;
    destItem.DefaultButton = GetBOOLValue(srcItem.DefaultButton);

#ifdef _UNICODE
		destItem.MaxLen=0;
#endif
		if ((unsigned int)(DWORD_PTR)srcItem.DataMessageId<2000)
#ifndef _UNICODE
			MyStringCopy(destItem.Data, GetMsgString(srcItem.DataMessageId));
#else
			destItem.PtrData = GetMsgString((unsigned int)(DWORD_PTR)srcItem.DataMessageId);
#endif
		else
#ifndef _UNICODE
			MyStringCopy(destItem.Data, srcItem.DataString);
#else
			destItem.PtrData = srcItem.DataString;
#endif
  }
}

// --------------------------------------------

HANDLE CStartupInfo::SaveScreen(int X1, int Y1, int X2, int Y2)
{
  return m_Data.SaveScreen(X1, Y1, X2, Y2);
}

HANDLE CStartupInfo::SaveScreen()
{
  return SaveScreen(0, 0, -1, -1);
}

void CStartupInfo::RestoreScreen(HANDLE handle)
{
  m_Data.RestoreScreen(handle);
}

const char kRegestryKeyDelimiter = '\'';

CSysString CStartupInfo::GetFullKeyName(const CSysString &keyName) const
{
  return (keyName.IsEmpty()) ? m_RegistryPath:
    (m_RegistryPath + kRegestryKeyDelimiter + keyName);
}


LONG CStartupInfo::CreateRegKey(HKEY parentKey,
    const CSysString &keyName, NRegistry::CKey &destKey) const
{
  return destKey.Create(parentKey, GetFullKeyName(keyName));
}

LONG CStartupInfo::OpenRegKey(HKEY parentKey,
    const CSysString &keyName, NRegistry::CKey &destKey) const
{
  return destKey.Open(parentKey, GetFullKeyName(keyName));
}

void CStartupInfo::SetRegKeyValue(HKEY parentKey, const CSysString &keyName,
    LPCTSTR valueName, LPCTSTR value) const
{
  NRegistry::CKey regKey;
  CreateRegKey(parentKey, keyName, regKey);
  regKey.SetValue(valueName, value);
}

void CStartupInfo::SetRegKeyValue(HKEY parentKey, const CSysString &keyName,
    LPCTSTR valueName, UINT32 value) const
{
  NRegistry::CKey regKey;
  CreateRegKey(parentKey, keyName, regKey);
  regKey.SetValue(valueName, value);
}

void CStartupInfo::SetRegKeyValue(HKEY parentKey, const CSysString &keyName,
    LPCTSTR valueName, bool value) const
{
  NRegistry::CKey regKey;
  CreateRegKey(parentKey, keyName, regKey);
  regKey.SetValue(valueName, value);
}

CSysString CStartupInfo::QueryRegKeyValue(HKEY parentKey, const CSysString &keyName,
    LPCTSTR valueName, const CSysString &valueDefault) const
{
  NRegistry::CKey regKey;
  if (OpenRegKey(parentKey, keyName, regKey) != ERROR_SUCCESS)
    return valueDefault;

  CSysString value;
  if(regKey.QueryValue(valueName, value) != ERROR_SUCCESS)
    return valueDefault;

  return value;
}

UINT32 CStartupInfo::QueryRegKeyValue(HKEY parentKey, const CSysString &keyName,
    LPCTSTR valueName, UINT32 valueDefault) const
{
  NRegistry::CKey regKey;
  if (OpenRegKey(parentKey, keyName, regKey) != ERROR_SUCCESS)
    return valueDefault;

  UINT32 value;
  if(regKey.QueryValue(valueName, value) != ERROR_SUCCESS)
    return valueDefault;

  return value;
}

bool CStartupInfo::QueryRegKeyValue(HKEY parentKey, const CSysString &keyName,
    LPCTSTR valueName, bool valueDefault) const
{
  NRegistry::CKey regKey;
  if (OpenRegKey(parentKey, keyName, regKey) != ERROR_SUCCESS)
    return valueDefault;

  bool value;
  if(regKey.QueryValue(valueName, value) != ERROR_SUCCESS)
    return valueDefault;

  return value;
}

bool CStartupInfo::Control(HANDLE pluginHandle, int command, int param1, LONG_PTR param2)
{
#ifdef _UNICODE
  return BOOLToBool(m_Data.Control(pluginHandle, command, param1, (LONG_PTR)param2));
#else
	return BOOLToBool(m_Data.Control(pluginHandle, command, (void*)param2));
#endif
}
LONG_PTR CStartupInfo::Control2(HANDLE pluginHandle, int command, int param1, LONG_PTR param2)
{
#ifdef _UNICODE
	return m_Data.Control(pluginHandle, command, param1, (LONG_PTR)param2);
#else
	return m_Data.Control(pluginHandle, command, (void*)param2);
#endif
}
bool CStartupInfo::ControlRequestActivePanel(int command, int param1, LONG_PTR param2)
{
#ifdef _UNICODE
	return Control(PANEL_ACTIVE, command, param1, param2);
#else
	return Control(INVALID_HANDLE_VALUE, command, param1, param2);
#endif
}
int CStartupInfo::ControlRequestActivePanel2(int command, int param1, LONG_PTR param2)
{
#ifdef _UNICODE
	return Control2(PANEL_ACTIVE, command, param1, param2);
#else
	return Control2(INVALID_HANDLE_VALUE, command, param1, param2);
#endif
}
#ifdef _UNICODE
FarDialogItem * CStartupInfo::GetFarDialogItem(const HANDLE &hDlg, DWORD item)
{
	if (hDlg)
	{
		LONG_PTR size = m_Data.SendDlgMessage(hDlg,DM_GETDLGITEM, item, 0);
		if (size > 0)
		{
			BYTE *buf = (BYTE *)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, sizeof(BYTE)*size);
			if (hDlg)
			{
				m_Data.SendDlgMessage(hDlg,DM_GETDLGITEM, item, (LONG_PTR)buf);
				return (FarDialogItem *)buf;
			}
			else
				HeapFree(GetProcessHeap(), 0, buf);
		}
	}
	return NULL;
}
PluginPanelItem * CStartupInfo::GetFarPluginPanelItem(HANDLE hPanel, int FCTL, int i )
{
	LONG_PTR size = Control2(hPanel, FCTL, i, 0);
	if (size > 0)
	{
		BYTE *buf = (BYTE *)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, sizeof(BYTE)*size);
		Control(hPanel, FCTL, i, (LONG_PTR)buf);
		return (PluginPanelItem *)buf;
	}
	return NULL;
}
bool CStartupInfo::ControlUpdateActivePanel(int param)
{
	return Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, param, NULL);
}
#else
bool CStartupInfo::ControlUpdateActivePanel(LONG_PTR param)
{
	return Control(INVALID_HANDLE_VALUE, FCTL_UPDATEPANEL, 0, param);
}
#endif
#ifdef _UNICODE
bool CStartupInfo::ControlUpdatePassivePanel(int param)
{
	return Control(PANEL_PASSIVE, FCTL_UPDATEPANEL, param, NULL);
}
#else
bool CStartupInfo::ControlUpdatePassivePanel(LONG_PTR param)
{
	return Control(INVALID_HANDLE_VALUE, FCTL_UPDATEANOTHERPANEL, 0, param);
}
#endif
bool CStartupInfo::ControlRedrawActivePanel(LONG_PTR param)
{
#ifdef _UNICODE
	return Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, param);
#else
	return Control(INVALID_HANDLE_VALUE, FCTL_REDRAWPANEL, 0, param);
#endif
}
bool CStartupInfo::ControlRedrawPassivePanel(LONG_PTR param)
{
#ifdef _UNICODE
	return Control(PANEL_PASSIVE, FCTL_REDRAWPANEL, 0, param);
#else
	return Control(INVALID_HANDLE_VALUE, FCTL_REDRAWANOTHERPANEL, 0, param);
#endif
}
bool CStartupInfo::ControlGetActivePanelInfo(PanelInfo &panelInfo)
{
  return ControlRequestActivePanel(FCTL_GETPANELINFO, 0, (LONG_PTR)&panelInfo);
}
bool CStartupInfo::ControlSetSelection(const PanelInfo &panelInfo)
{
  return ControlRequestActivePanel(FCTL_SETSELECTION, 0, (LONG_PTR)&panelInfo);
}
bool CStartupInfo::ControlGetActivePanelCurrentItemInfo(
    PluginPanelItem &pluginPanelItem, PanelInfo &panelInfo)
{
  if(!ControlGetActivePanelInfo(panelInfo))
    return false;
  if(panelInfo.ItemsNumber <= 0)
		throw g_StartupInfo.GetMsgString(NMessageID::kNoItems);
#ifdef _UNICODE
	return Control(PANEL_ACTIVE, FCTL_GETCURRENTPANELITEM, 0, (LONG_PTR)&pluginPanelItem);
#else
	pluginPanelItem = panelInfo.PanelItems[panelInfo.CurrentItem];

	return true;
#endif
}
CSysString CStartupInfo::GetActivePanelCurrentItemName()
{
#ifdef _UNICODE
	PluginPanelItem * ppi = GetFarPluginPanelItem(PANEL_ACTIVE, FCTL_GETCURRENTPANELITEM, 0);
	if (ppi)
	{
		CSysString ret = ppi->FindData.lpwszFileName;
		HeapFree(GetProcessHeap(), 0, ppi);
		return ret;
	}
#else
	PanelInfo pi;
	ControlRequestActivePanel(FCTL_GETPANELINFO, 0, (LONG_PTR)&pi);

	return GetSystemString(pi.PanelItems[pi.CurrentItem].FindData.cFileName, CP_OEMCP);
#endif
	return _F("");
}
DWORD CStartupInfo::GetActivePanelCurrentItemAtt()
{
#ifdef _UNICODE
	PluginPanelItem * ppi = GetFarPluginPanelItem(PANEL_ACTIVE, FCTL_GETCURRENTPANELITEM, 0);
	if (ppi)
	{
		DWORD ret = ppi->FindData.dwFileAttributes;
		HeapFree(GetProcessHeap(), 0, ppi);
		return ret;
	}
#else
	PanelInfo pi;
	ControlRequestActivePanel(FCTL_GETPANELINFO, 0, (LONG_PTR)&pi);
	return pi.PanelItems[pi.CurrentItem].FindData.dwFileAttributes;
#endif
	return 0;
}
DWORD_PTR CStartupInfo::GetActivePanelCurrentItemData()
{
#ifdef _UNICODE
	PluginPanelItem * ppi = GetFarPluginPanelItem(PANEL_ACTIVE, FCTL_GETCURRENTPANELITEM, 0);
	if (ppi)
	{
		UINT32 ret = ppi->UserData;
		HeapFree(GetProcessHeap(), 0, ppi);
		return ret;
	}
#else
	PanelInfo pi;
	ControlRequestActivePanel(FCTL_GETPANELINFO, 0, (LONG_PTR)&pi);
	return pi.PanelItems[pi.CurrentItem].UserData;
#endif
	return 0;
}
DWORD_PTR CStartupInfo::GetActivePanelUserData(bool bSelected, int i)
{
#ifdef _UNICODE
	PluginPanelItem * ppi = GetFarPluginPanelItem(PANEL_ACTIVE, bSelected?FCTL_GETSELECTEDPANELITEM:FCTL_GETPANELITEM, i);
	if (ppi)
	{
		DWORD_PTR ret = ppi->UserData;
		HeapFree(GetProcessHeap(), 0, ppi);
		return ret;
	}
#else
	PanelInfo pi;
	ControlRequestActivePanel(FCTL_GETPANELINFO, 0, (LONG_PTR)&pi);
	if (bSelected)
		return pi.SelectedItems[i].UserData;
	else
		return pi.PanelItems[i].UserData;
#endif
	return 0;
}
CSysString CStartupInfo::GetActivePanelName(bool bSelected, int i)
{
#ifdef _UNICODE
	PluginPanelItem * ppi = GetFarPluginPanelItem(PANEL_ACTIVE, bSelected?FCTL_GETSELECTEDPANELITEM:FCTL_GETPANELITEM, i);
	if (ppi)
	{
		CSysString ret = ppi->FindData.lpwszFileName;
		HeapFree(GetProcessHeap(), 0, ppi);
		return ret;
	}
#else
	PanelInfo pi;
	ControlRequestActivePanel(FCTL_GETPANELINFO, 0, (LONG_PTR)&pi);
	if (bSelected)
		return GetSystemString(pi.SelectedItems[i].FindData.cFileName, CP_OEMCP);
	else
		return GetSystemString(pi.PanelItems[i].FindData.cFileName, CP_OEMCP);
#endif
	return _F("");
}
DWORD CStartupInfo::GetActivePanelAtt(bool bSelected, int i)
{
#ifdef _UNICODE
	PluginPanelItem * ppi = GetFarPluginPanelItem(PANEL_ACTIVE, bSelected?FCTL_GETSELECTEDPANELITEM:FCTL_GETPANELITEM, i);
	if (ppi)
	{
		DWORD ret = ppi->FindData.dwFileAttributes;
		HeapFree(GetProcessHeap(), 0, ppi);
		return ret;
	}
#else
	PanelInfo pi;
	ControlRequestActivePanel(FCTL_GETPANELINFO, 0, (LONG_PTR)&pi);
	if (bSelected)
		return pi.SelectedItems[i].FindData.dwFileAttributes;
	else
		return pi.PanelItems[i].FindData.dwFileAttributes;
#endif
	return 0;
}
bool CStartupInfo::ControlGetActivePanelSelectedOrCurrentItems(
    CObjectVector<MyPluginPanelItem> &pluginPanelItems)
{
  pluginPanelItems.Clear();
	PanelInfo panelInfo;
  if(!ControlGetActivePanelInfo(panelInfo))
    return false;
  if(panelInfo.ItemsNumber <= 0)
    throw g_StartupInfo.GetMsgString(NMessageID::kNoItems);
  
	if (panelInfo.SelectedItemsNumber == 0)
	{
		MyPluginPanelItem pluginPanelItem;
		pluginPanelItem.strFileName = GetActivePanelCurrentItemName();
		pluginPanelItem.dwAttributes = GetActivePanelCurrentItemAtt();
		pluginPanelItem.UserData = GetActivePanelCurrentItemData();
		pluginPanelItems.Add(pluginPanelItem);
	}
	else
	{
		for (int i = 0; i < panelInfo.SelectedItemsNumber; i++)
		{
			MyPluginPanelItem pluginPanelItem;
			pluginPanelItem.strFileName = GetActivePanelName(true, i);
			pluginPanelItem.dwAttributes = GetActivePanelAtt(true, i);
			pluginPanelItem.UserData = GetActivePanelUserData(true, i);
			pluginPanelItems.Add(pluginPanelItem);
		}
	}

  return true;
}

bool CStartupInfo::ControlClearPanelSelection()
{
  PanelInfo panelInfo;
  if(!ControlGetActivePanelInfo(panelInfo))
    return false;
  for (int i = 0; i < panelInfo.ItemsNumber; i++)
#ifdef _UNICODE
		Control(PANEL_ACTIVE, FCTL_SETSELECTION, i, NULL);

	return true;
#else
		panelInfo.PanelItems[i].Flags &= ~PPIF_SELECTED;

	return ControlSetSelection(panelInfo);
#endif
}

////////////////////////////////////////////////
// menu function

int CStartupInfo::Menu(
    int x,
    int y,
    int maxHeight,
    unsigned int flags,
    const farChar *title,
    const farChar *aBottom,
    const farChar *helpTopic,
    int *breakKeys,
    int *breakCode,
    struct FarMenuItem *items,
    int numItems)
{
  return m_Data.Menu(m_Data.ModuleNumber, x, y, maxHeight, flags, (farChar *)title,
      (farChar *)aBottom, (farChar *)helpTopic, breakKeys, breakCode, items, numItems);
}

int CStartupInfo::Menu(
    unsigned int flags,
    const farChar *title,
    const farChar *helpTopic,
    struct FarMenuItem *items,
    int numItems)
{
  return Menu(-1, -1, 0, flags, title, NULL, helpTopic, NULL,
      NULL, items, numItems);
}

int CStartupInfo::Menu(
    unsigned int flags,
    const farChar *title,
    const farChar *helpTopic,
    const CSysStringVector &items,
    int selectedItem)
{
  CRecordVector<FarMenuItem> farMenuItems;
  for(int i = 0; i < items.Size(); i++)
  {
    FarMenuItem item;
    item.Checked = 0;
    item.Separator = 0;
    item.Selected = (i == selectedItem);
#ifdef _UNICODE
		item.Text = (const farChar *)items[i];
#else
		CSysString reducedString = items[i].Left(sizeof(item.Text) / sizeof(item.Text[0]) - 1);
		MyStringCopy(item.Text, (const farChar *)reducedString);
#endif
    farMenuItems.Add(item);
  }
  return Menu(flags, title, helpTopic, &farMenuItems.Front(), farMenuItems.Size());
}

//////////////////////////////////
// CScreenRestorer

CScreenRestorer::~CScreenRestorer()
{
  Restore();
}
void CScreenRestorer::Save()
{
  if(m_Saved)
    return;
  m_HANDLE = g_StartupInfo.SaveScreen();
  m_Saved = true;
}

void CScreenRestorer::Restore()
{
  if(m_Saved)
  {
    g_StartupInfo.RestoreScreen(m_HANDLE);
    m_Saved = false;
  }
};

static CSysString DWORDToString(DWORD number)
{
  farChar buffer[32];
	g_StartupInfo.m_FSF.itoa(number, buffer, 10);
  return buffer;
}

void PrintErrorMessage(const farChar *message, int code)
{
  CSysString tmp = message;
  tmp += _F(" #");
  tmp += DWORDToString(code);
  g_StartupInfo.ShowMessage(tmp);
}

void PrintErrorMessage(const farChar *message, const farChar *text)
{
  CSysString tmp = message;
  tmp += _F(": ");
  tmp += text;
  g_StartupInfo.ShowMessage(tmp);
}

bool WasEscPressed()
{
  NConsole::CIn inConsole;
  HANDLE handle = ::GetStdHandle(STD_INPUT_HANDLE);
  if(handle == INVALID_HANDLE_VALUE)
    return true;
  inConsole.Attach(handle);
  for (;;)
  {
    DWORD numEvents;
    if(!inConsole.GetNumberOfEvents(numEvents))
      return true;
    if(numEvents == 0)
      return false;

    INPUT_RECORD event;
    if(!inConsole.ReadEvent(event, numEvents))
      return true;
    if (event.EventType == KEY_EVENT &&
        event.Event.KeyEvent.bKeyDown &&
        event.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)
      return true;
  }
}

void ShowErrorMessage(DWORD errorCode)
{
  CSysString message;
  NError::MyFormatMessage(errorCode, message);
  message.Replace(_F("\r"), _F(""));
  message.Replace(_F("\n"), _F(" "));
#ifdef _UNICODE
	g_StartupInfo.ShowMessage(message);
#else
	g_StartupInfo.ShowMessage(SystemStringToOemString(message));
#endif
}

void ShowLastErrorMessage()
{
  ShowErrorMessage(::GetLastError());
}

}
