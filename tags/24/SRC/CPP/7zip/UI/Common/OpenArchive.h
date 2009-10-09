// OpenArchive.h

#ifndef __OPENARCHIVE_H
#define __OPENARCHIVE_H

#include "Common/MyString.h"
#include "Windows/FileFind.h"

#include "../../Archive/IArchive.h"
#include "LoadCodecs.h"
#include "ArchiveOpenCallback.h"

HRESULT GetArchiveItemPath(IInArchive *archive, UInt32 index, UString &result);
HRESULT GetArchiveItemPath(IInArchive *archive, UInt32 index, const UString &defaultName, UString &result);
HRESULT GetArchiveItemFileTime(IInArchive *archive, UInt32 index,
    const FILETIME &defaultFileTime, FILETIME &fileTime);
HRESULT IsArchiveItemProp(IInArchive *archive, UInt32 index, PROPID propID, bool &result);
HRESULT IsArchiveItemFolder(IInArchive *archive, UInt32 index, bool &result);
HRESULT IsArchiveItemAnti(IInArchive *archive, UInt32 index, bool &result);

struct ISetSubArchiveName
{
  virtual void SetSubArchiveName(const wchar_t *name) = 0;
};

struct CArcInfo
{
  CMyComPtr<IInArchive> Archive;
  UString DefaultItemName;
  int FormatIndex;
};

HRESULT OpenArchive(
    CCodecs *codecs,
    int arcTypeIndex,
    IInStream *inStream,
    const UString &filePath,
    const CIntVector& disabledFormats,
    IInArchive **archiveResult,
    int &formatIndex,
    UString &defaultItemName,
    IArchiveOpenCallback *openArchiveCallback);

HRESULT OpenArchive(
    CCodecs *codecs,
    int arcTypeIndex,
    const UString &filePath,
    const CIntVector& disabledFormats,
    IInArchive **archive,
    int &formatIndex,
    UString &defaultItemName,
    IArchiveOpenCallback *openArchiveCallback);

HRESULT OpenArchive(
    CCodecs *codecs,
    const CIntVector &formatIndices,
    const UString &filePath,
    const CIntVector& disabledFormats,
    CObjectVector<CArcInfo>& arcList,
    IArchiveOpenCallback *openArchiveCallback);

HRESULT ReOpenArchive(IInArchive *archive, const UString &fileName, IArchiveOpenCallback *openArchiveCallback);

struct CArchiveLink
{
  CObjectVector<CArcInfo> ArcList;

  UStringVector VolumePaths;

  bool IsOpen;
  UInt64 VolumesSize;

  int GetNumLevels() const { return ArcList.Size(); }

  CArchiveLink(): IsOpen(false), VolumesSize(0) {};
  ~CArchiveLink() { Close(); }

  IInArchive *GetArchive() { return ArcList.Size() ? ArcList.Back().Archive : NULL; }
  UString GetDefaultItemName()  { return ArcList.Back().DefaultItemName; }
  int GetArchiverIndex() const { return ArcList.Back().FormatIndex; }
  HRESULT Close();
  void Release();
};

HRESULT OpenArchive(
    CCodecs *codecs,
    const CIntVector &formatIndices,
    const UString &archiveName,
    const CIntVector& disabledFormats,
    CArchiveLink &archiveLink,
    IArchiveOpenCallback *openCallback);

HRESULT MyOpenArchive(
    CCodecs *codecs,
    const CIntVector &formatIndices,
    const UString &archiveName,
    CArchiveLink &archiveLink,
    IOpenCallbackUI *openCallbackUI);

HRESULT ReOpenArchive(
    CCodecs *codecs,
    CArchiveLink &archiveLink,
    const UString &fileName,
    IArchiveOpenCallback *openCallback);

HRESULT DetectArchiveType(CCodecs *codecs, const UString &filePath, CObjectVector<CIntVector>& arcIndices, IArchiveOpenCallback *openArchiveCallback);

#endif

