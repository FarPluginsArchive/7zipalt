// PluginDelete.cpp

#include "StdAfx.h"

#include <stdio.h>

#include "Plugin.h"
#include "Messages.h"
#include "UpdateCallback100.h"

#include "Windows/FileDir.h"

#include "../../Common/FileStreams.h"

#include "Common/StringConvert.h"

#include "../Common/ZipRegistry.h"
#include "../Common/WorkDir.h"

using namespace NFar;
using namespace NWindows;
using namespace NFile;
using namespace NDirectory;

static LPCWSTR kTempArchivePrefix = L"7zA";

int CPlugin::DeleteFiles(PluginPanelItem *panelItems, int numItems,
    int opMode)
{
  if (numItems == 0)
    return FALSE;
  /*
  if (!m_ArchiverInfo.UpdateEnabled)
  {
    g_StartupInfo.ShowMessage(NMessageID::kUpdateNotSupportedForThisArchive);
    return FALSE;
  }
  */
  if ((opMode & OPM_SILENT) == 0)
  {
    const farChar *msgItems[]=
    {
      g_StartupInfo.GetMsgString(NMessageID::kDeleteTitle),
      g_StartupInfo.GetMsgString(NMessageID::kDeleteFiles),
      g_StartupInfo.GetMsgString(NMessageID::kDeleteDelete),
      g_StartupInfo.GetMsgString(NMessageID::kDeleteCancel)
    };
    farChar msg[1024];
    if (numItems == 1)
    {
#ifdef _UNICODE
      g_StartupInfo.m_FSF.sprintf(msg, g_StartupInfo.GetMsgString(NMessageID::kDeleteFile), panelItems[0].FindData.lpwszFileName);
#else
      g_StartupInfo.m_FSF.sprintf(msg, g_StartupInfo.GetMsgString(NMessageID::kDeleteFile), panelItems[0].FindData.cFileName);
#endif
      msgItems[1] = msg;
    }
    else if (numItems > 1)
    {
      g_StartupInfo.m_FSF.sprintf(msg, g_StartupInfo.GetMsgString(NMessageID::kDeleteNumberOfFiles), numItems);
      msgItems[1] = msg;
    }
    if (g_StartupInfo.ShowMessage(FMSG_WARNING, NULL, msgItems, 
        sizeof(msgItems) / sizeof(msgItems[0]), 2) != 0)
      return (FALSE);
  }

  CProgressBox progressBox;
  progressBox.Init(g_StartupInfo.GetMsgString(NMessageID::kDeleting), 48, opMode & OPM_SILENT);

  NWorkDir::CInfo workDirInfo;
  ReadWorkDirInfo(workDirInfo);

  UString workDir = GetWorkDir(workDirInfo, m_FileName);
  CreateComplexDirectory(workDir);

  CTempFileW tempFile;
  UString tempFileName;
  if (tempFile.Create(workDir, kTempArchivePrefix, tempFileName) == 0)
    return FALSE;


  CRecordVector<UINT32> indices;
  indices.Reserve(numItems);
  int i;
  for(i = 0; i < numItems; i++)
    indices.Add(panelItems[i].UserData);

  ////////////////////////////
  // Save _folder;

  UStringVector pathVector;
  GetPathParts(pathVector);
  
  CMyComPtr<IOutFolderArchive> outArchive;
  HRESULT result = m_ArchiveHandler.QueryInterface(
      IID_IOutFolderArchive, &outArchive);
  if(result != S_OK)
  {
    g_StartupInfo.ShowMessage(NMessageID::kUpdateNotSupportedForThisArchive);
    return FALSE;
  }
  outArchive->SetFolder(_folder);

  CUpdateCallback100Imp *updateCallbackSpec = new CUpdateCallback100Imp;
  CMyComPtr<IFolderArchiveUpdateCallback> updateCallback(updateCallbackSpec );
  
  UString pass;
  //CrOm: ������� �������� ������ �� ��������
  updateCallbackSpec->Init(/* m_ArchiveHandler, */ &progressBox, false, pass);


  result = outArchive->DeleteItems(
      tempFileName,
      &indices.Front(), indices.Size(),
      updateCallback);
  updateCallback.Release();
  outArchive.Release();

  if (result != S_OK)
  {
    ShowErrorMessage(result);
    return FALSE;
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
    return FALSE;
  }

 
  ////////////////////////////
  // Restore _folder;

  m_ArchiveHandler->BindToRootFolder(&_folder);
  for (i = 0; i < pathVector.Size(); i++)
  {
    CMyComPtr<IFolderFolder> newFolder;
    _folder->BindToFolder(pathVector[i], &newFolder);
    if(!newFolder  )
      break;
    _folder = newFolder;
  }
  GetCurrentDir();

  return(bDeleteRes?TRUE:FALSE);
}
