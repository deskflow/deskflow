#include "OSXClipboardUTF8Converter.h"

CFStringRef OSXClipboardUTF8Converter::getOSXFormat() const
{
  return CFSTR("public.utf8-plain-text");
}

String OSXClipboardUTF8Converter::doFromIClipboard(const String &data) const
{
  return data;
}

String OSXClipboardUTF8Converter::doToIClipboard(const String &data) const
{
  return data;
}
