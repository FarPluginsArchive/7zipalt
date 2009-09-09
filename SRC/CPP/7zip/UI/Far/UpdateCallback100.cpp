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
STDMETHODIMP CUpdateCallback100Imp::CryptoGetTextPassword2(Int32 *passwordIsDefined, BSTR *password)
{
	*passwordIsDefined = BoolToInt(m_PasswordIsDefined);
	if (!m_PasswordIsDefined)
	{
		return S_OK;
	}
	*passwordIsDefined = BoolToInt(m_PasswordIsDefined);
	return StringToBstr(m_Password, password);
}
STDMETHODIMP CUpdateCallback100Imp::CryptoGetTextPassword(BSTR *password)
{
	if (!m_PasswordIsDefined)
		return S_FALSE;
	return StringToBstr(m_Password, password);
}