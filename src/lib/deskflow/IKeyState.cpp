/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/IKeyState.h"

#include <cstdint>
#include <cstring>

//
// IKeyState
//

IKeyState::IKeyState(const IEventQueue *)
{
  // do nothing
}

//
// IKeyState::KeyInfo
//

IKeyState::KeyInfo *IKeyState::KeyInfo::alloc(KeyID id, KeyModifierMask mask, KeyButton button, int32_t count)
{
  auto *info = new KeyInfo();
  info->m_key = id;
  info->m_mask = mask;
  info->m_button = button;
  info->m_count = count;
  return info;
}

IKeyState::KeyInfo *IKeyState::KeyInfo::alloc(
    KeyID id, KeyModifierMask mask, KeyButton button, int32_t count, const std::set<std::string> &destinations
)
{
  auto *info = new KeyInfo();
  info->m_key = id;
  info->m_mask = mask;
  info->m_button = button;
  info->m_count = count;
  info->m_computers = join(destinations);
  return info;
}

IKeyState::KeyInfo *IKeyState::KeyInfo::alloc(const KeyInfo &x)
{
  auto *info = new KeyInfo();

  info->m_key = x.m_key;
  info->m_mask = x.m_mask;
  info->m_button = x.m_button;
  info->m_count = x.m_count;
  info->m_computers = x.m_computers;
  return info;
}

bool IKeyState::KeyInfo::isDefault(const char *computers)
{
  return (computers == nullptr || computers[0] == '\0');
}

bool IKeyState::KeyInfo::contains(const char *computers, const std::string_view &name)
{
  // special cases
  if (isDefault(computers)) {
    return false;
  }
  if (computers[0] == '*') {
    return true;
  }

  // search
  std::string match;
  match.reserve(name.size() + 2);
  match += ":";
  match += name;
  match += ":";
  return (strstr(computers, match.c_str()) != nullptr);
}

bool IKeyState::KeyInfo::equal(const KeyInfo *a, const KeyInfo *b)
{
  return (
      a->m_key == b->m_key && a->m_mask == b->m_mask && a->m_button == b->m_button && a->m_count == b->m_count &&
      a->m_computers == b->m_computers
  );
}

std::string IKeyState::KeyInfo::join(const std::set<std::string> &destinations)
{
  // collect destinations into a string.  names are surrounded by ':'
  // which makes searching easy.  the string is empty if there are no
  // destinations and "*" means all destinations.
  std::string computers;
  for (const auto &i : destinations) {
    if (i == "*") {
      computers = "*";
      break;
    } else {
      if (computers.empty()) {
        computers = ":";
      }
      computers += i;
      computers += ":";
    }
  }
  return computers;
}

void IKeyState::KeyInfo::split(const char *computers, std::set<std::string> &dst)
{
  dst.clear();
  if (isDefault(computers)) {
    return;
  }
  if (computers[0] == '*') {
    dst.emplace("*");
    return;
  }

  const char *i = computers + 1;
  while (*i != '\0') {
    const char *j = strchr(i, ':');
    dst.emplace(i, j - i);
    i = j + 1;
  }
}
