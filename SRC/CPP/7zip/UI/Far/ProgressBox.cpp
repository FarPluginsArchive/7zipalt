// ProgressBox.cpp

#include "StdAfx.h"

#include <stdio.h>

#include "ProgressBox.h"
#include "Common/IntToString.h"
#include "FarUtils.h"

static void CopySpaces(farChar *dest, int numSpaces)
{
  int i;
  for (i = 0; i < numSpaces; i++)
    dest[i] = _F(' ');
  dest[i] = _F('\0');
}

void ConvertUInt64ToStringAligned(UInt64 value, farChar *s, int alignSize)
{
  farChar temp[32];
  ConvertUInt64ToString(value, temp);
  int len = (int)lstrlen(temp);
  int numSpaces = 0;
  if (len < alignSize)
  {
    numSpaces = alignSize - len;
    CopySpaces(s, numSpaces);
  }
  lstrcpy(s + numSpaces, temp);
}


// ---------- CMessageBox ----------

static const int kMaxLen = 255;

int CMessageBox::GetMaxWidth()
{
  HANDLE Con = GetStdHandle(STD_OUTPUT_HANDLE);
  if (Con != INVALID_HANDLE_VALUE)
  {
    CONSOLE_SCREEN_BUFFER_INFO ConInfo;
    if (GetConsoleScreenBufferInfo(Con, &ConInfo))
    {
      int ConWidth = ConInfo.srWindow.Right - ConInfo.srWindow.Left + 1;
      if (ConWidth >= 80)
        return ConWidth - 20;
    }
  }
  return 60;
}

void CMessageBox::Init(const CSysString &title, int width)
{
  _title = title;
  _width = MyMin(width, kMaxLen);
  MaxWidth = MyMin(GetMaxWidth(), kMaxLen);
}

void CMessageBox::ShowMessages(const farChar *strings[], int numStrings)
{
  const int kNumStaticStrings = 1;
  const int kNumStringsMax = 10;

  if (numStrings > kNumStringsMax)
    numStrings = kNumStringsMax;

  const farChar *msgItems[kNumStaticStrings + kNumStringsMax];
  msgItems[0] = _title;

  farChar formattedMessages[kNumStringsMax][kMaxLen + 1];

  for (int i = 0; i < numStrings; i++)
  {
    farChar *formattedMessage = formattedMessages[i];
    const farChar *s = strings[i];
    int len = (int)lstrlen(s);
    if (len < MaxWidth)
    {
      _width = MyMax(_width, len);
      int startPos = (_width - len) / 2;
      CopySpaces(formattedMessage, startPos);
      lstrcpy(formattedMessage + startPos, s);
      CopySpaces(formattedMessage + startPos + len, _width - startPos - len);
    }
    else
    {
      _width = MaxWidth;
      lstrcpyn(formattedMessage, s, _width);
      formattedMessage[_width] = 0;
    }
    msgItems[kNumStaticStrings + i] = formattedMessage;
  }
  NFar::g_StartupInfo.ShowMessage(0, NULL, msgItems, kNumStaticStrings + numStrings, 0);
}

CSysString CMessageBox::GetTitle()
{
  return _title;
}

// ---------- CProgressBox ----------

CProgressBox::~CProgressBox() {
  if (_wasShown)
    NFar::g_StartupInfo.SetProgressState(TBPF_NOPROGRESS);
}

void CProgressBox::Init(const CSysString &title, int width, bool lazy)
{
  CMessageBox::Init(title, width);
  _prevMessage.Empty();
  _prevPercentMessage.Empty();
  _wasShown = false;
  unsigned __int64 TmCurr;
  QueryPerformanceCounter((PLARGE_INTEGER) &TmCurr);
  QueryPerformanceFrequency((PLARGE_INTEGER) &TmFreq);
  if (lazy)
    TmNext = TmCurr + TmFreq / 2; // display box after 0.5 sec.
  else
    TmNext = TmCurr; // display box as soon as possible
}

void CProgressBox::Progress(const UInt64 *total, const UInt64 *completed, const CSysString &message)
{
  // update box every 1/4 sec.
  unsigned __int64 TmCurr;
  QueryPerformanceCounter((PLARGE_INTEGER) &TmCurr);
  if (TmCurr < TmNext) return;
  TmNext = TmCurr + TmFreq / 4;

  CSysString percentMessage;
  if (total != 0 && completed != 0)
  {
    UInt64 totalVal = *total;
    if (totalVal == 0)
      totalVal = 1;
    farChar buf[32];
    ConvertUInt64ToStringAligned(*completed * 100 / totalVal, buf, 3);
    lstrcat(buf, _F("%"));
    percentMessage = buf;
  }
  if (message != _prevMessage || percentMessage != _prevPercentMessage || !_wasShown)
  {
    _prevMessage = message;
    _prevPercentMessage = percentMessage;
    const farChar *strings[] = { message, percentMessage };
    ShowMessages(strings, sizeof(strings) / sizeof(strings[0]));
    _wasShown = true;

    CSysString TitleMessage;
    TitleMessage = _F("{") + percentMessage + _F("} ") + _title + _F(": ") + message;
    SetConsoleTitle(TitleMessage);
    NFar::g_StartupInfo.SetProgressState(TBPF_NORMAL);
    NFar::g_StartupInfo.SetProgressValue(*completed, *total);
  }
}
