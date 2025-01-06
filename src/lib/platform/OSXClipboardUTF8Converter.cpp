#include "OSXClipboardUTF8Converter.h"

CFStringRef OSXClipboardUTF8Converter::getOSXFormat() const
{
  return CFSTR("public.utf8-plain-text");
}

std::string OSXClipboardUTF8Converter::doFromIClipboard(const std::string &data) const
{
  return data;
}

std::string OSXClipboardUTF8Converter::doToIClipboard(const std::string &data) const
{
  return data;
}
