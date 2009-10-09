// OpenArchive.cpp

#include "StdAfx.h"

#include "OpenArchive.h"

#include "Common/Wildcard.h"

#include "Windows/FileName.h"
#include "Windows/FileDir.h"
#include "Windows/Defs.h"
#include "Windows/PropVariant.h"

#include "../../Common/FileStreams.h"
#include "../../Common/StreamUtils.h"

#include "Common/StringConvert.h"

#include "DefaultName.h"

using namespace NWindows;

HRESULT GetArchiveItemPath(IInArchive *archive, UInt32 index, UString &result)
{
  NCOM::CPropVariant prop;
  RINOK(archive->GetProperty(index, kpidPath, &prop));
  if(prop.vt == VT_BSTR)
    result = prop.bstrVal;
  else if (prop.vt == VT_EMPTY)
    result.Empty();
  else
    return E_FAIL;
  return S_OK;
}

HRESULT GetArchiveItemPath(IInArchive *archive, UInt32 index, const UString &defaultName, UString &result)
{
  RINOK(GetArchiveItemPath(archive, index, result));
  if (result.IsEmpty())
  {
    result = defaultName;
    NCOM::CPropVariant prop;
    RINOK(archive->GetProperty(index, kpidExtension, &prop));
    if (prop.vt == VT_BSTR)
    {
      result += L'.';
      result += prop.bstrVal;
    }
    else if (prop.vt != VT_EMPTY)
      return E_FAIL;
  }
  return S_OK;
}

HRESULT GetArchiveItemFileTime(IInArchive *archive, UInt32 index,
    const FILETIME &defaultFileTime, FILETIME &fileTime)
{
  NCOM::CPropVariant prop;
  RINOK(archive->GetProperty(index, kpidMTime, &prop));
  if (prop.vt == VT_FILETIME)
    fileTime = prop.filetime;
  else if (prop.vt == VT_EMPTY)
    fileTime = defaultFileTime;
  else
    return E_FAIL;
  return S_OK;
}

HRESULT IsArchiveItemProp(IInArchive *archive, UInt32 index, PROPID propID, bool &result)
{
  NCOM::CPropVariant prop;
  RINOK(archive->GetProperty(index, propID, &prop));
  if(prop.vt == VT_BOOL)
    result = VARIANT_BOOLToBool(prop.boolVal);
  else if (prop.vt == VT_EMPTY)
    result = false;
  else
    return E_FAIL;
  return S_OK;
}

HRESULT IsArchiveItemFolder(IInArchive *archive, UInt32 index, bool &result)
{
  return IsArchiveItemProp(archive, index, kpidIsDir, result);
}

HRESULT IsArchiveItemAnti(IInArchive *archive, UInt32 index, bool &result)
{
  return IsArchiveItemProp(archive, index, kpidIsAnti, result);
}

// Static-SFX (for Linux) can be big.
const UInt64 kMaxCheckStartPosition = 1 << 22;

HRESULT ReOpenArchive(IInArchive *archive, const UString &fileName, IArchiveOpenCallback *openArchiveCallback)
{
  CInFileStream *inStreamSpec = new CInFileStream;
  CMyComPtr<IInStream> inStream(inStreamSpec);
  inStreamSpec->Open(fileName);
  return archive->Open(inStream, &kMaxCheckStartPosition, openArchiveCallback);
}

#ifndef _SFX
static inline bool TestSignature(const Byte *p1, const Byte *p2, size_t size)
{
  for (size_t i = 0; i < size; i++)
    if (p1[i] != p2[i])
      return false;
  return true;
}
#endif

HRESULT OpenArchive(
    CCodecs *codecs,
    int arcTypeIndex,
    IInStream *inStream,
    const UString &filePath,
    const CIntVector& disabledFormats,
    IInArchive **archiveResult,
    int &formatIndex,
    UString &defaultItemName,
    IArchiveOpenCallback *openArchiveCallback)
{
  *archiveResult = NULL;
  UString extension;
  {
    int dotPos = filePath.ReverseFind(L'.');
    if (dotPos >= 0)
      extension = filePath.Mid(dotPos + 1);
  }
  CIntVector orderIndices;
  if (arcTypeIndex >= 0)
    orderIndices.Add(arcTypeIndex);
  else
  {

  int i;
  int numFound = 0;
  for (i = 0; i < codecs->Formats.Size(); i++)
    if (disabledFormats.Find(i) == -1)
      if (codecs->Formats[i].FindExtension(extension) >= 0)
        orderIndices.Insert(numFound++, i);
      else
        orderIndices.Add(i);
  int PEFormatIndex = codecs->FindFormatForArchiveType(L"PE");
  int ElfFormatIndex = codecs->FindFormatForArchiveType(L"Elf");
  int iPE = orderIndices.Find(PEFormatIndex);
  int iElf = orderIndices.Find(ElfFormatIndex);
  bool probablySFX = ((iPE != -1) && (iPE < numFound)) || ((iElf != -1) && (iElf < numFound));
  #ifndef _SFX
  if ((numFound != 1) || probablySFX)
  {
    CIntVector orderIndices2;
    CByteBuffer byteBuffer;
    const size_t kBufferSize = (1 << 21);
    byteBuffer.SetCapacity(kBufferSize);
    RINOK(inStream->Seek(0, STREAM_SEEK_SET, NULL));
    size_t processedSize = kBufferSize;
    RINOK(ReadStream(inStream, byteBuffer, &processedSize));
    if (processedSize == 0)
      return S_FALSE;

    const Byte *buf = byteBuffer;
    Byte hash[1 << 16];
    memset(hash, 0xFF, 1 << 16);
    Byte prevs[256];
    if (orderIndices.Size() > 255)
      return S_FALSE;
    int i;
    for (i = 0; i < orderIndices.Size(); i++)
    {
      const CArcInfoEx &ai = codecs->Formats[orderIndices[i]];
      const CByteBuffer &sig = ai.StartSignature;
      if (sig.GetCapacity() < 2)
        continue;
      UInt32 v = sig[0] | ((UInt32)sig[1] << 8);
      prevs[i] = hash[v];
      hash[v] = (Byte)i;
    }

    processedSize--;
    for (UInt32 pos = 0; pos < processedSize; pos++)
    {
      for (; pos < processedSize && hash[buf[pos] | ((UInt32)buf[pos + 1] << 8)] == 0xFF; pos++);
      if (pos == processedSize)
        break;
      UInt32 v = buf[pos] | ((UInt32)buf[pos + 1] << 8);
      Byte *ptr = &hash[v];
      int i = *ptr;
      do
      {
        int index = orderIndices[i];
        const CArcInfoEx &ai = codecs->Formats[index];
        const CByteBuffer &sig = ai.StartSignature;
        if (sig.GetCapacity() != 0 && pos + sig.GetCapacity() <= processedSize + 1)
          if (TestSignature(buf + pos, sig, sig.GetCapacity()))
          {
            orderIndices2.Add(index);
            orderIndices[i] = 0xFF;
            *ptr = prevs[i];
          }
        ptr = &prevs[i];
        i = *ptr;
      }
      while (i != 0xFF);
    }
    if (orderIndices2.Size() >= 2)
    {
      int iIso = orderIndices2.Find(codecs->FindFormatForArchiveType(L"iso"));
      int iUdf = orderIndices2.Find(codecs->FindFormatForArchiveType(L"udf"));;
      if (iUdf > iIso && iIso >= 0)
        orderIndices2.Swap(iIso, iUdf);
    }
    int iPE = orderIndices2.Find(PEFormatIndex);
    if (iPE != -1)
    {
      orderIndices2.Delete(iPE);
      orderIndices2.Add(PEFormatIndex);
    }
    int iElf = orderIndices2.Find(ElfFormatIndex);
    if (iElf != -1)
    {
      orderIndices2.Delete(iElf);
      orderIndices2.Add(ElfFormatIndex);
    }
    for (i = 0; i < orderIndices.Size(); i++)
    {
      int val = orderIndices[i];
      if (val != 0xFF)
        orderIndices2.Add(val);
    }
    orderIndices = orderIndices2;
  }
  else {
    int splitFormatIndex = codecs->FindFormatForArchiveType(L"Split");
    if ((orderIndices.Size() >= 2) && (orderIndices[0] == splitFormatIndex))
    {
      CByteBuffer byteBuffer;
      const size_t kBufferSize = (1 << 10);
      byteBuffer.SetCapacity(kBufferSize);
      Byte *buffer = byteBuffer;
      RINOK(inStream->Seek(0, STREAM_SEEK_SET, NULL));
      size_t processedSize = kBufferSize;
      RINOK(ReadStream(inStream, buffer, &processedSize));
      if (processedSize >= 16)
      {
        Byte kRarHeader[] = {0x52 , 0x61, 0x72, 0x21, 0x1a, 0x07, 0x00};
        if (TestSignature(buffer, kRarHeader, 7) && buffer[9] == 0x73 && (buffer[10] & 1) != 0)
        {
          for (int i = 0; i < orderIndices.Size(); i++)
          {
            int index = orderIndices[i];
            const CArcInfoEx &ai = codecs->Formats[index];
            if (ai.Name.CompareNoCase(L"rar") != 0)
              continue;
            orderIndices.Delete(i--);
            orderIndices.Insert(0, index);
            break;
          }
        }
      }
      
      //проверка на форсмажор, если обычным архивам вдруг назначат расширения 000, 001, 002 и т.д.
      int extNum = _wtoi(extension) + 1;
      if ((extNum > 0) && (extNum < 1000))
      {
        wchar_t nextExt[5];
        swprintf(nextExt, ARRAYSIZE(nextExt), L".%.3d", extNum);

        UString testName = filePath;
        int dotPos = testName.ReverseFind(L'.');
        if (dotPos >= 0)
          testName.Delete(dotPos, testName.Length() - dotPos);
        testName += nextExt;

        NFile::NFind::CFileInfoW fileInfo;
        if (NFile::NFind::FindFile(testName, fileInfo) && !fileInfo.IsDir())
        {
          CArchiveLink link;
          CIntVector noSplitFormat;
          noSplitFormat.Add(splitFormatIndex);
          if (OpenArchive(codecs, CIntVector(), testName, noSplitFormat, link, openArchiveCallback) == S_OK)
            orderIndices.Delete(0);
        }
      }
    }
  }
  #endif
  }

  for(int i = 0; i < orderIndices.Size(); i++)
  {
    inStream->Seek(0, STREAM_SEEK_SET, NULL);

    CMyComPtr<IInArchive> archive;

    formatIndex = orderIndices[i];
    RINOK(codecs->CreateInArchive(formatIndex, archive));
    if (!archive)
      continue;

    #ifdef EXTERNAL_CODECS
    {
      CMyComPtr<ISetCompressCodecsInfo> setCompressCodecsInfo;
      archive.QueryInterface(IID_ISetCompressCodecsInfo, (void **)&setCompressCodecsInfo);
      if (setCompressCodecsInfo)
      {
        RINOK(setCompressCodecsInfo->SetCompressCodecsInfo(codecs));
      }
    }
    #endif

    HRESULT result = archive->Open(inStream, &kMaxCheckStartPosition, openArchiveCallback);
    if (result == S_FALSE)
      continue;
    RINOK(result);
    *archiveResult = archive.Detach();
    const CArcInfoEx &format = codecs->Formats[formatIndex];
    if (format.Exts.Size() == 0)
    {
      defaultItemName = GetDefaultName2(ExtractFileNameFromPath(filePath), L"", L"");
    }
    else
    {
      int subExtIndex = format.FindExtension(extension);
      if (subExtIndex < 0)
        subExtIndex = 0;
      defaultItemName = GetDefaultName2(ExtractFileNameFromPath(filePath),
          format.Exts[subExtIndex].Ext,
          format.Exts[subExtIndex].AddExt);
    }
    return S_OK;
  }
  return S_FALSE;
}

HRESULT OpenArchive(
    CCodecs *codecs,
    int arcTypeIndex,
    const UString &filePath,
    const CIntVector& disabledFormats,
    IInArchive **archiveResult,
    int &formatIndex,
    UString &defaultItemName,
    IArchiveOpenCallback *openArchiveCallback)
{
  CInFileStream *inStreamSpec = new CInFileStream;
  CMyComPtr<IInStream> inStream(inStreamSpec);
  if (!inStreamSpec->Open(filePath))
    return GetLastError();
  return OpenArchive(codecs, arcTypeIndex, inStream,
    filePath, disabledFormats,
    archiveResult, formatIndex,
    defaultItemName, openArchiveCallback);
}

static void MakeDefaultName(UString &name)
{
  int dotPos = name.ReverseFind(L'.');
  if (dotPos < 0)
    return;
  UString ext = name.Mid(dotPos + 1);
  if (ext.IsEmpty())
    return;
  for (int pos = 0; pos < ext.Length(); pos++)
    if (ext[pos] < L'0' || ext[pos] > L'9')
      return;
  name = name.Left(dotPos);
}

HRESULT OpenArchive(
    CCodecs *codecs,
    const CIntVector &formatIndices,
    const UString &fileName,
    const CIntVector& disabledFormats,
    CObjectVector<CArcInfo>& arcList,
    IArchiveOpenCallback *openArchiveCallback)
{
  if (formatIndices.Size() >= 32)
    return E_NOTIMPL;
  
  CArcInfo arcInfo;
  while (true)
  {
    int arcTypeIndex = -1;
    if (formatIndices.Size())
    {
      if (arcList.Size() >= formatIndices.Size())
        break;
      arcTypeIndex = formatIndices[formatIndices.Size() - arcList.Size() - 1];
    }
    else if (arcList.Size() >= 32)
      break;
    
    if (arcList.IsEmpty())
    {
      HRESULT result = OpenArchive(codecs, arcTypeIndex, fileName, disabledFormats,
        &arcInfo.Archive, arcInfo.FormatIndex, arcInfo.DefaultItemName, openArchiveCallback);
      RINOK(result);
      arcList.Add(arcInfo);
      continue;
    }

    UInt32 mainSubfile;
    {
      NCOM::CPropVariant prop;
      RINOK(arcInfo.Archive->GetArchiveProperty(kpidMainSubfile, &prop));
      if (prop.vt == VT_UI4)
        mainSubfile = prop.ulVal;
      else
        break;
      UInt32 numItems;
      RINOK(arcInfo.Archive->GetNumberOfItems(&numItems));
      if (mainSubfile >= numItems)
        break;
    }


    CMyComPtr<IInArchiveGetStream> getStream;
    HRESULT result = arcInfo.Archive->QueryInterface(IID_IInArchiveGetStream, (void **)&getStream);
    if (result != S_OK || !getStream)
      break;

    CMyComPtr<ISequentialInStream> subSeqStream;
    result = getStream->GetStream(mainSubfile, &subSeqStream);
    if (result != S_OK || !subSeqStream)
      break;

    CMyComPtr<IInStream> subStream;
    result = subSeqStream.QueryInterface(IID_IInStream, &subStream);
    if (result != S_OK || !subStream)
      break;

    UInt32 numItems;
    RINOK(arcInfo.Archive->GetNumberOfItems(&numItems));
    if (numItems < 1)
      break;

    UString subPath;
    RINOK(GetArchiveItemPath(arcInfo.Archive, mainSubfile, subPath))
    if (subPath.IsEmpty())
    {
      MakeDefaultName(arcInfo.DefaultItemName);
      subPath = arcInfo.DefaultItemName;
      const CArcInfoEx &format = codecs->Formats[arcInfo.FormatIndex];
      if (format.Name.CompareNoCase(L"7z") == 0)
      {
        if (subPath.Right(3).CompareNoCase(L".7z") != 0)
          subPath += L".7z";
      }
    }
    else
      subPath = ExtractFileNameFromPath(subPath);

    CMyComPtr<IArchiveOpenSetSubArchiveName> setSubArchiveName;
    openArchiveCallback->QueryInterface(IID_IArchiveOpenSetSubArchiveName, (void **)&setSubArchiveName);
    if (setSubArchiveName)
      setSubArchiveName->SetSubArchiveName(subPath);

    result = OpenArchive(codecs, arcTypeIndex, subStream, subPath, disabledFormats,
        &arcInfo.Archive, arcInfo.FormatIndex, arcInfo.DefaultItemName, openArchiveCallback);
    if (result != S_OK)
      break;
    arcList.Add(arcInfo);
  }
  return S_OK;
}

static void SetCallback(const UString &archiveName,
    IOpenCallbackUI *openCallbackUI,
    IArchiveOpenCallback *reOpenCallback,
    CMyComPtr<IArchiveOpenCallback> &openCallback)
{
  COpenCallbackImp *openCallbackSpec = new COpenCallbackImp;
  openCallback = openCallbackSpec;
  openCallbackSpec->Callback = openCallbackUI;
  openCallbackSpec->ReOpenCallback = reOpenCallback;

  UString fullName;
  int fileNamePartStartIndex;
  NFar::g_StartupInfo.GetFullPathName((LPCWSTR)archiveName, fullName, fileNamePartStartIndex);
  openCallbackSpec->Init(
      fullName.Left(fileNamePartStartIndex),
      fullName.Mid(fileNamePartStartIndex));
}

HRESULT MyOpenArchive(
    CCodecs *codecs,
    int arcTypeIndex,
    const UString &archiveName,
    IInArchive **archive, UString &defaultItemName, IOpenCallbackUI *openCallbackUI)
{
  CMyComPtr<IArchiveOpenCallback> openCallback;
  SetCallback(archiveName, openCallbackUI, NULL, openCallback);
  int formatInfo;
  return OpenArchive(codecs, arcTypeIndex, archiveName, CIntVector(), archive, formatInfo, defaultItemName, openCallback);
}

HRESULT MyOpenArchive(
    CCodecs *codecs,
    const CIntVector &formatIndices,
    const UString &archiveName,
    CObjectVector<CArcInfo>& arcList,
    UStringVector &volumePaths,
    UInt64 &volumesSize,
    IOpenCallbackUI *openCallbackUI)
{
  volumesSize = 0;
  COpenCallbackImp *openCallbackSpec = new COpenCallbackImp;
  CMyComPtr<IArchiveOpenCallback> openCallback = openCallbackSpec;
  openCallbackSpec->Callback = openCallbackUI;

  UString fullName;
  int fileNamePartStartIndex;
  NFar::g_StartupInfo.GetFullPathName((LPCWSTR)archiveName, fullName, fileNamePartStartIndex);
  UString prefix = fullName.Left(fileNamePartStartIndex);
  UString name = fullName.Mid(fileNamePartStartIndex);
  openCallbackSpec->Init(prefix, name);

  RINOK(OpenArchive(codecs, formatIndices, archiveName, CIntVector(),
      arcList,
      openCallback));
  volumePaths.Add(prefix + name);
  for (int i = 0; i < openCallbackSpec->FileNames.Size(); i++)
    volumePaths.Add(prefix + openCallbackSpec->FileNames[i]);
  volumesSize = openCallbackSpec->TotalSize;
  return S_OK;
}

HRESULT CArchiveLink::Close()
{
  if (IsOpen)
  {
    for (int i = ArcList.Size() - 1; i >= 0; i--)
      RINOK(ArcList[i].Archive->Close());
    IsOpen = false;
  }
  return S_OK;
}

void CArchiveLink::Release()
{
  IsOpen = false;
  for (int i = ArcList.Size() - 1; i >= 0; i--)
    ArcList[i].Archive.Release();
}

HRESULT OpenArchive(
    CCodecs *codecs,
    const CIntVector &formatIndices,
    const UString &archiveName,
    const CIntVector& disabledFormats,
    CArchiveLink &archiveLink,
    IArchiveOpenCallback *openCallback)
{
  HRESULT res = OpenArchive(codecs, formatIndices, archiveName, disabledFormats,
    archiveLink.ArcList,
    openCallback);
  archiveLink.IsOpen = (res == S_OK);
  return res;
}

HRESULT MyOpenArchive(CCodecs *codecs,
    const CIntVector &formatIndices,
    const UString &archiveName,
    CArchiveLink &archiveLink,
    IOpenCallbackUI *openCallbackUI)
{
  HRESULT res = MyOpenArchive(codecs, formatIndices, archiveName,
    archiveLink.ArcList,
    archiveLink.VolumePaths,
    archiveLink.VolumesSize,
    openCallbackUI);
  archiveLink.IsOpen = (res == S_OK);
  return res;
}

HRESULT ReOpenArchive(CCodecs *codecs, CArchiveLink &archiveLink, const UString &fileName,
    IArchiveOpenCallback *openCallback)
{
  if (archiveLink.GetNumLevels() > 1)
    return E_NOTIMPL;

  if (archiveLink.GetNumLevels() == 0)
    return MyOpenArchive(codecs, CIntVector(), fileName, archiveLink, 0);

  CMyComPtr<IArchiveOpenCallback> openCallbackNew;
  SetCallback(fileName, NULL, openCallback, openCallbackNew);

  HRESULT res = ReOpenArchive(archiveLink.GetArchive(), fileName, openCallbackNew);
  archiveLink.IsOpen = (res == S_OK);
  return res;
}

struct CArcIndex
{
  int index;
  CObjectVector<CArcIndex> subIndices;
};

HRESULT DetectArchiveType(CCodecs *codecs, IInStream *inStream, CArcIndex& parentArcIndex, IArchiveOpenCallback *openArchiveCallback)
{
  for (int i = 0; i < codecs->Formats.Size(); i++)
  {
    inStream->Seek(0, STREAM_SEEK_SET, NULL);

    CMyComPtr<IInArchive> archive;
    if ((codecs->CreateInArchive(i, archive) != S_OK) || !archive)
      continue;

    if (archive->Open(inStream, &kMaxCheckStartPosition, openArchiveCallback) != S_OK)
      continue;

    CArcIndex arcIndex;
    arcIndex.index = i;
    parentArcIndex.subIndices.Add(arcIndex);

    NCOM::CPropVariant prop;
    if ((archive->GetArchiveProperty(kpidMainSubfile, &prop) != S_OK) || (prop.vt != VT_UI4))
      continue;
    UInt32 mainSubfile = prop.ulVal;
    UInt32 numItems;
    if ((archive->GetNumberOfItems(&numItems) != S_OK) || (mainSubfile >= numItems))
      continue;

    CMyComPtr<IInArchiveGetStream> getStream;
    if ((archive->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void **>(&getStream)) != S_OK) || !getStream)
      continue;

    CMyComPtr<ISequentialInStream> subSeqStream;
    if ((getStream->GetStream(mainSubfile, &subSeqStream) != S_OK) || !subSeqStream)
      continue;

    CMyComPtr<IInStream> subStream;
    if ((subSeqStream.QueryInterface(IID_IInStream, &subStream) != S_OK) || !subStream)
      continue;

    DetectArchiveType(codecs, subStream, parentArcIndex.subIndices.Back(), openArchiveCallback);
  }
  return S_OK;
}

void addSubIndices(CObjectVector<CIntVector>& arcIndices, const CArcIndex& arcIndex, const CIntVector& arcs)
{
  for (int i = 0; i < arcIndex.subIndices.Size(); i++)
  {
    CIntVector subArcs(arcs);
    subArcs.Insert(0, arcIndex.subIndices[i].index);
    arcIndices.Add(subArcs);
    addSubIndices(arcIndices, arcIndex.subIndices[i], subArcs);
  }
}

HRESULT DetectArchiveType(CCodecs *codecs, const UString &filePath, CObjectVector<CIntVector>& arcIndices, IArchiveOpenCallback *openArchiveCallback)
{
  CInFileStream *inStreamSpec = new CInFileStream;
  CMyComPtr<IInStream> inStream(inStreamSpec);
  if (!inStreamSpec->Open(filePath))
    return GetLastError();
  CArcIndex rootIndex;
  HRESULT result = DetectArchiveType(codecs, inStream, rootIndex, openArchiveCallback);
  addSubIndices(arcIndices, rootIndex, CIntVector());
  return result;
}