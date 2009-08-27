// UpdateCallback.h

#ifndef __UPDATECALLBACK100_H
#define __UPDATECALLBACK100_H

#include "Common/MyCom.h"

#include "../Agent/IFolderArchive.h"

#include "ProgressBox.h"

class CUpdateCallback100Imp:
  public IFolderArchiveUpdateCallback,
  public CMyUnknownImp
{
  // CMyComPtr<IInFolderArchive> _archiveHandler;
  CProgressBox *m_progressBox;
  UInt64 _total;

  CSysString m_message;
  bool m_PasswordIsDefined;
  UString m_Password;

public:
  MY_UNKNOWN_IMP;

  INTERFACE_IProgress(;)
  INTERFACE_IFolderArchiveUpdateCallback(;)

  //STDMETHOD(CryptoGetTextPassword2)(Int32 *passwordIsDefined, BSTR *password);

  CUpdateCallback100Imp(): _total(0) {}
  void Init(/* IInFolderArchive *archiveHandler, */ CProgressBox *progressBox,
    bool passwordIsDefined,
    const UString &password)
  {
    // _archiveHandler = archiveHandler;
    m_PasswordIsDefined = passwordIsDefined;
    m_Password = password;
    m_progressBox = progressBox;
  }
};



#endif
