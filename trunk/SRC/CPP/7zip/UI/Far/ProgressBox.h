// ProgressBox.h

#ifndef __PROGRESSBOX_H
#define __PROGRESSBOX_H

#include "Common/MyString.h"
#include "Common/Types.h"

#ifdef _UNICODE
void ConvertUInt64ToStringAligned(UInt64 value, wchar_t *s, int alignSize);
#else
void ConvertUInt64ToStringAligned(UInt64 value, char *s, int alignSize);
#endif

class CMessageBox
{
protected:
  CSysString _title;
  int _width;
  int MaxWidth;
  int GetMaxWidth();
public:
  void Init(const CSysString &title, int width);
	CSysString GetTitle();
#ifdef _UNICODE
	void ShowMessages(const wchar_t *strings[], int numStrings);
#else
	void ShowMessages(const char *strings[], int numStrings);
#endif
};

class CProgressBox: public CMessageBox
{
  CSysString _prevMessage;
  CSysString _prevPercentMessage;
  bool _wasShown;
  unsigned __int64 TmNext;
  unsigned __int64 TmFreq;
public:
  void Init(const CSysString &title, int width);
  void Progress(const UInt64 *total, const UInt64 *completed, const CSysString &message);
};

#endif
