/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/IKeyState.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>

//
// IKeyState
//

IKeyState::IKeyState(IEventQueue *events)
{
}

//
// IKeyState::KeyInfo
//

IKeyState::KeyInfo *IKeyState::KeyInfo::alloc(KeyID id, KeyModifierMask mask, KeyButton button, int32_t count)
{
  KeyInfo *info = (KeyInfo *)malloc(sizeof(KeyInfo));
  info->m_key = id;
  info->m_mask = mask;
  info->m_button = button;
  info->m_count = count;
  info->m_screens = NULL;
  info->m_screensBuffer[0] = '\0';
  return info;
}

IKeyState::KeyInfo *IKeyState::KeyInfo::alloc(
    KeyID id, KeyModifierMask mask, KeyButton button, int32_t count, const std::set<std::string> &destinations
)
{
  std::string screens = join(destinations);
  const char *buffer = screens.c_str();

  // build structure
  KeyInfo *info = (KeyInfo *)malloc(sizeof(KeyInfo) + screens.size());
  info->m_key = id;
  info->m_mask = mask;
  info->m_button = button;
  info->m_count = count;
  info->m_screens = info->m_screensBuffer;
  std::copy(buffer, buffer + screens.size() + 1, info->m_screensBuffer);
  return info;
}

IKeyState::KeyInfo *IKeyState::KeyInfo::alloc(const KeyInfo &x)
{
  auto bufferLen = strnlen(x.m_screensBuffer, SIZE_MAX);
  auto info = (KeyInfo *)malloc(sizeof(KeyInfo) + bufferLen);
  info->m_key = x.m_key;
  info->m_mask = x.m_mask;
  info->m_button = x.m_button;
  info->m_count = x.m_count;
  info->m_screens = x.m_screens ? info->m_screensBuffer : NULL;
  memcpy(info->m_screensBuffer, x.m_screensBuffer, bufferLen + 1);
  return info;
}

bool IKeyState::KeyInfo::isDefault(const char *screens)
{
  return (screens == NULL || screens[0] == '\0');
}

bool IKeyState::KeyInfo::contains(const char *screens, const std::string &name)
{
  // special cases
  if (isDefault(screens)) {
    return false;
  }
  if (screens[0] == '*') {
    return true;
  }

  // search
  std::string match;
  match.reserve(name.size() + 2);
  match += ":";
  match += name;
  match += ":";
  return (strstr(screens, match.c_str()) != NULL);
}

bool IKeyState::KeyInfo::equal(const KeyInfo *a, const KeyInfo *b)
{
  return (
      a->m_key == b->m_key && a->m_mask == b->m_mask && a->m_button == b->m_button && a->m_count == b->m_count &&
      strcmp(a->m_screensBuffer, b->m_screensBuffer) == 0
  );
}

std::string IKeyState::KeyInfo::join(const std::set<std::string> &destinations)
{
  // collect destinations into a string.  names are surrounded by ':'
  // which makes searching easy.  the string is empty if there are no
  // destinations and "*" means all destinations.
  std::string screens;
  for (auto i = destinations.begin(); i != destinations.end(); ++i) {
    if (*i == "*") {
      screens = "*";
      break;
    } else {
      if (screens.empty()) {
        screens = ":";
      }
      screens += *i;
      screens += ":";
    }
  }
  return screens;
}

void IKeyState::KeyInfo::split(const char *screens, std::set<std::string> &dst)
{
  dst.clear();
  if (isDefault(screens)) {
    return;
  }
  if (screens[0] == '*') {
    dst.insert("*");
    return;
  }

  const char *i = screens + 1;
  while (*i != '\0') {
    const char *j = strchr(i, ':');
    dst.insert(std::string(i, j - i));
    i = j + 1;
  }
}
