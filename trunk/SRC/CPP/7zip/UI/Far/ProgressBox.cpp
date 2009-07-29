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

void CMessageBox::Init(const CSysString &title, int width)
{
  _title = title;
  _width = MyMin(width, kMaxLen);
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
    if (len < kMaxLen)
    {
      int size = MyMax(_width, len);
      int startPos = (size - len) / 2;
      CopySpaces(formattedMessage, startPos);
			lstrcpy(formattedMessage + startPos, s);
      //MyStringCopy(formattedMessage + startPos, s);
      CopySpaces(formattedMessage + startPos + len, size - startPos - len);
    }
    else
    {
      lstrcpyn(formattedMessage, s, kMaxLen);
      formattedMessage[kMaxLen] = 0;
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

void CProgressBox::Init(const CSysString &title, int width)
{
  CMessageBox::Init(title, width);
  _prevMessage.Empty();
  _prevPercentMessage.Empty();
  _wasShown = false;
}

void CProgressBox::Progress(const UInt64 *total, const UInt64 *completed, const CSysString &message)
{
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
  }
}
