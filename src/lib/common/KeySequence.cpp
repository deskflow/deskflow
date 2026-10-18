/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "KeySequence.h"
#include <QSettings>

// this table originally comes from Qt sources (gui/kernel/qkeysequence.cpp)
// and is heavily modified for Deskflow
static const struct
{
  int key;
  const char *name;
} keyname[] = {
    {Qt::Key_Space, "Space"},
    {Qt::Key_Escape, "Escape"},
    {Qt::Key_Tab, "Tab"},
    {Qt::Key_Backtab, "LeftTab"},
    {Qt::Key_Backspace, "BackSpace"},
    {Qt::Key_Return, "Return"},
    {Qt::Key_Insert, "Insert"},
    {Qt::Key_Delete, "Delete"},
    {Qt::Key_Pause, "Pause"},
    {Qt::Key_Print, "Print"},
    {Qt::Key_SysReq, "SysReq"},
    {Qt::Key_Home, "Home"},
    {Qt::Key_End, "End"},
    {Qt::Key_Left, "Left"},
    {Qt::Key_Up, "Up"},
    {Qt::Key_Right, "Right"},
    {Qt::Key_Down, "Down"},
    {Qt::Key_PageUp, "PageUp"},
    {Qt::Key_PageDown, "PageDown"},
    {Qt::Key_CapsLock, "CapsLock"},
    {Qt::Key_NumLock, "NumLock"},
    {Qt::Key_ScrollLock, "ScrollLock"},
    {Qt::Key_Menu, "Menu"},
    {Qt::Key_Help, "Help"},
    {Qt::Key_Enter, "KP_Enter"},
    {Qt::Key_Clear, "Clear"},

    {Qt::Key_Back, "WWWBack"},
    {Qt::Key_Forward, "WWWForward"},
    {Qt::Key_Stop, "WWWStop"},
    {Qt::Key_Refresh, "WWWRefresh"},
    {Qt::Key_VolumeDown, "AudioDown"},
    {Qt::Key_VolumeMute, "AudioMute"},
    {Qt::Key_VolumeUp, "AudioUp"},
    {Qt::Key_MediaPlay, "AudioPlay"},
    {Qt::Key_MediaStop, "AudioStop"},
    {Qt::Key_MediaPrevious, "AudioPrev"},
    {Qt::Key_MediaNext, "AudioNext"},
    {Qt::Key_HomePage, "WWWHome"},
    {Qt::Key_Favorites, "WWWFavorites"},
    {Qt::Key_Search, "WWWSearch"},
    {Qt::Key_Standby, "Sleep"},
    {Qt::Key_LaunchMail, "AppMail"},
    {Qt::Key_LaunchMedia, "AppMedia"},
    {Qt::Key_Launch0, "AppUser1"},
    {Qt::Key_Launch1, "AppUser2"},
    {Qt::Key_Select, "Select"},
    {Qt::Key_Plus, "Plus"},
    {Qt::Key_Comma, "Comma"},
    {Qt::Key_Semicolon, "Semicolon"},

    {0, nullptr}
};

bool KeySequence::isMouseButton() const
{
  return !m_Sequence.isEmpty() && m_Sequence.last() < Qt::Key_Space;
}

KeySequence KeySequence::fromString(const QString &text)
{
  KeySequence result;
  if (text.isEmpty()) {
    return result;
  }

  const auto parts = text.split('+', Qt::KeepEmptyParts);
  int modifiers = 0;
  for (qsizetype i = 0; i < parts.size(); ++i) {
    const auto &part = parts.at(i);
    const bool isLast = i == parts.size() - 1;

    int modifierKey = 0;
    int modifier = 0;
    if (part == QStringLiteral("Shift")) {
      modifierKey = Qt::Key_Shift;
      modifier = Qt::ShiftModifier;
    } else if (part == QStringLiteral("Control")) {
      modifierKey = Qt::Key_Control;
      modifier = Qt::ControlModifier;
    } else if (part == QStringLiteral("Alt")) {
      modifierKey = Qt::Key_Alt;
      modifier = Qt::AltModifier;
    } else if (part == QStringLiteral("Meta")) {
      modifierKey = Qt::Key_Meta;
      modifier = Qt::MetaModifier;
    }

    if (modifier != 0) {
      if (isLast) {
        return {};
      }
      modifiers |= modifier;
      result.appendKey(modifierKey, modifiers);
      continue;
    }

    if (!isLast) {
      return {};
    }

    const int key = keyFromString(part);
    if (key == 0 || !result.appendKey(key, modifiers)) {
      return {};
    }
  }

  return result;
}

QString KeySequence::toString() const
{
  QString result;

  for (int i = 0; i < m_Sequence.size(); i++) {
    result += keyToString(m_Sequence[i]);

    if (i != m_Sequence.size() - 1)
      result += "+";
  }

  return result;
}

bool KeySequence::appendMouseButton(int button)
{
  return appendKey(button, 0);
}

bool KeySequence::appendKey(int key, int modifiers)
{
  if (m_Sequence.size() == 4)
    return true;

  switch (key) {
  case Qt::Key_AltGr:
    return false;

  case Qt::Key_Control:
  case Qt::Key_Alt:
  case Qt::Key_Shift:
  case Qt::Key_Meta:
  case Qt::Key_Menu: {
    int mod = modifiers & (~m_Modifiers);
    if (mod) {
      m_Sequence.append(mod);
      m_Modifiers |= mod;
    }
  } break;

  default:
    // see if we can handle this key, if not, don't accept it
    if (keyToString(key).isEmpty())
      break;

    m_Sequence.append(key);
    setValid(true);
    return true;
  }

  return false;
}

void KeySequence::loadSettings(QSettings &settings)
{
  sequence().clear();
  int num = settings.beginReadArray("keys");
  for (int i = 0; i < num; i++) {
    settings.setArrayIndex(i);
    sequence().append(settings.value("key", 0).toInt());
  }
  settings.endArray();

  setModifiers(0);
  setValid(true);
}

void KeySequence::saveSettings(QSettings &settings) const
{
  settings.beginWriteArray("keys");
  for (int i = 0; i < sequence().size(); i++) {
    settings.setArrayIndex(i);
    settings.setValue("key", sequence()[i]);
  }
  settings.endArray();
}

QString KeySequence::keyToString(int key)
{
  // nothing there?
  if (key == 0)
    return "";

  // a hack to handle mouse buttons as if they were keys
  if (key < Qt::Key_Space) {
    switch (key) {
    case Qt::LeftButton:
      return "1";
    case Qt::RightButton:
      return "2";
    case Qt::MiddleButton:
      return "3";
    default:
      qDebug() << "unknown key" << key;
      break;
    }

    return "4"; // qt only knows three mouse buttons, so assume it's an unknown
                // fourth one
  }

  // modifiers?
  if (key & Qt::ShiftModifier)
    return "Shift";

  if (key & Qt::ControlModifier)
    return "Control";

  if (key & Qt::AltModifier)
    return "Alt";

  if (key & Qt::MetaModifier)
    return "Meta";

  // treat key pad like normal keys (FIXME: we should have another lookup table
  // for keypad keys instead)
  key &= ~Qt::KeypadModifier;

  // a special key? (check before printable to handle comma/semicolon)
  int i = 0;
  while (keyname[i].name) {
    if (key == keyname[i].key)
      return QString::fromUtf8(keyname[i].name);
    i++;
  }

  // a printable 7 bit character?
  if (key < 0x80)
    return QChar(key & 0x7f).toLower();

  // a function key?
  if (key >= Qt::Key_F1 && key <= Qt::Key_F35)
    return QString::fromUtf8("F%1").arg(key - Qt::Key_F1 + 1);

  // representable in ucs2?
  if (key < 0x10000) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    const uint16_t keyHex = QChar(key).toLower().unicode();
    return QString("\\u%1").arg(keyHex, kStrSize, kBase, kFillChar);
#else
    return QString("\\u%1").arg(QChar(key).toLower().unicode(), kStrSize, kBase, kFillChar);
#endif
  }

  // give up, deskflow probably won't handle this
  return "";
}

int KeySequence::keyFromString(const QString &text)
{
  for (int i = 0; keyname[i].name != nullptr; ++i) {
    if (text == QString::fromUtf8(keyname[i].name)) {
      return keyname[i].key;
    }
  }

  if (text.size() == 1 && text.front().unicode() < 0x80) {
    return text.front().unicode();
  }

  if (text.startsWith(QLatin1Char('F'))) {
    bool ok = false;
    const int number = text.sliced(1).toInt(&ok);
    if (ok && number >= 1 && number <= 35) {
      return Qt::Key_F1 + number - 1;
    }
  }

  if (text.size() == 6 && text.startsWith(QStringLiteral("\\u"))) {
    bool ok = false;
    const int key = text.sliced(2).toInt(&ok, 16);
    if (ok) {
      return key;
    }
  }

  return 0;
}
