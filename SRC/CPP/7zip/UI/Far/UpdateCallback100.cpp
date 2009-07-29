// UpdateCallback.h

#include "StdAfx.h"

#include "UpdateCallback100.h"

#include "Common/Defs.h"
#include "Common/StringConvert.h"
#include "FarUtils.h"

using namespace NFar;

STDMETHODIMP CUpdateCallback100Imp::SetNumFiles(UInt64 /* numFiles */)
{
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::SetTotal(UInt64 size)
{
  _total = size;
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::SetCompleted(const UInt64 *completeValue)
{
  if (WasEscPressed())
    return E_ABORT;
  if (m_progressBox != 0)
	{
    m_progressBox->Progress(&_total, completeValue, m_message);

		farChar tmp[256];
		g_StartupInfo.m_FSF.sprintf(tmp, _F("{%u%%} "), *completeValue * 100 / _total);
		m_titlemessage.Empty();
		m_titlemessage += tmp;
		m_titlemessage += m_progressBox->GetTitle();
		m_titlemessage += _F(": ");
		m_titlemessage += m_message;
		SetConsoleTitle(m_titlemessage);
	}
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::CompressOperation(const wchar_t*  name )
{
	m_message = GetSystemString(name, CP_OEMCP);
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::DeleteOperation(const wchar_t*  name )
{
	m_message = GetSystemString(name, CP_OEMCP);
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::OperationResult(Int32 /* opRes */)
{
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::UpdateErrorMessage(const wchar_t *message)
{
#ifdef _UNICODE
	CSysString s = message;
#else
	CSysString s = UnicodeStringToMultiByte(message, CP_OEMCP);
#endif
  if (g_StartupInfo.ShowMessage(s) == -1)
    return E_ABORT;
  return S_OK;
}

/*STDMETHODIMP CUpdateCallback100Imp::CryptoGetTextPassword2(Int32 *passwordIsDefined, BSTR *password)
{
	COM_TRY_BEGIN
		return Callback->CryptoGetTextPassword2(passwordIsDefined, password);
	COM_TRY_END
}*/