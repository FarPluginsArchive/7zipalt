// 7zip/Far/Plugin.h

#ifndef __7ZIP_FAR_PLUGIN_H
#define __7ZIP_FAR_PLUGIN_H

#include "Common/MyCom.h"

#include "Windows/COM.h"
#include "Windows/FileFind.h"
#include "Windows/PropVariant.h"

#include "../Agent/IFolderArchive.h"

#include "FarUtils.h"

const UInt32 kNumInfoLinesMax = 64;

struct CPanelMode
{
  int ViewMode;
  int SortMode;
  bool ReverseSort;
  bool NumericSort;
};

class CPlugin
{
  NWindows::NCOM::CComInitializer m_ComInitializer;
  UString m_CurrentDir;

  UString m_PannelTitle;
  
  InfoPanelLine m_InfoLines[kNumInfoLinesMax];

  farChar m_FileNameBuffer[1024];
  farChar m_CurrentDirBuffer[1024];
  farChar m_PannelTitleBuffer[1024];

  CSysString PanelModeColumnTypes;
  CSysString PanelModeColumnWidths;
  PanelMode PanelMode;
  void AddColumn(PROPID aPropID);


  void EnterToDirectory(const UString &aDirName);

  void GetPathParts(UStringVector &aPathParts);
  void GetCurrentDir();
public:
  UString m_FileName;
  // UString m_DefaultName;
  NWindows::NFile::NFind::CFileInfoW m_FileInfo;

  CMyComPtr<IInFolderArchive> m_ArchiveHandler;
  CMyComPtr<IFolderFolder> _folder;
  
  // CArchiverInfo m_ArchiverInfo;
  UString _archiveTypeName;

  bool PasswordIsDefined;
  UString Password;


  CPlugin(const UString &fileName,
        // const UString &aDefaultName,
        IInFolderArchive *archiveHandler,
        UString archiveTypeName
        );
  ~CPlugin();

  void ReadValueSafe(PROPID aPropID, NWindows::NCOM::CPropVariant aPropVariant);
  void ReadPluginPanelItem(PluginPanelItem &aPanelItem, UINT32 anItemIndex);

  int GetFindData(PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode);
  void FreeFindData(PluginPanelItem *PanelItem,int ItemsNumber);
  int SetDirectory(const farChar *aDir, int opMode);
  void GetOpenPluginInfo(struct OpenPluginInfo *anInfo, const CPanelMode& panelMode);

  int DeleteFiles(PluginPanelItem *aPanelItems, int itemsNumber, int opMode);


  HRESULT ExtractFiles(
      bool decompressAllItems,
      const UInt32 *indices,
      UINT32 numIndices,
      bool silent,
      NExtract::NPathMode::EEnum pathMode,
      NExtract::NOverwriteMode::EEnum overwriteMode,
      const UString &destPath,
      bool passwordIsDefined, const UString &password);

#ifdef _UNICODE
  NFar::NFileOperationReturnCode::EEnum GetFiles(struct PluginPanelItem *aPanelItem, int itemsNumber,
    int move, const farChar **destPath, int opMode);
  NFar::NFileOperationReturnCode::EEnum GetFilesReal(struct PluginPanelItem *aPanelItems, 
    int itemsNumber, int move, const farChar **_aDestPath, int opMode, bool aShowBox);
#else
  NFar::NFileOperationReturnCode::EEnum GetFiles(struct PluginPanelItem *aPanelItem, int itemsNumber,
    int move, farChar *destPath, int opMode);
  NFar::NFileOperationReturnCode::EEnum GetFilesReal(struct PluginPanelItem *aPanelItems, 
    int itemsNumber, int move, farChar *_aDestPath, int opMode, bool aShowBox);
#endif

  NFar::NFileOperationReturnCode::EEnum PutFiles(struct PluginPanelItem *aPanelItems, int itemsNumber,
    int move, int opMode);

  HRESULT ShowAttributesWindow();

  int ProcessKey(int aKey, unsigned int aControlState);

  void GetPanelMode(CPanelMode& panelMode);
};

HRESULT CompressFiles(const CObjectVector<NFar::MyPluginPanelItem> &aPluginPanelItems);

#endif
