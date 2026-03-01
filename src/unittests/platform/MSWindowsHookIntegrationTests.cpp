/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "MSWindowsHookIntegrationTests.h"

#include "platform/MSWindowsHook.h"

#include <Windows.h>

#include <QElapsedTimer>
#include <QList>
#include <QString>
#include <QTest>
#include <cwctype>
#include <initializer_list>
#include <utility>

namespace {

struct ComposePair
{
  UINT deadVK = 0;
  UINT composeVK = 0;
  WCHAR composed = 0;
};

struct DeadKeyAoPair
{
  UINT deadVK = 0;
  WCHAR composedA = 0;
  WCHAR composedO = 0;
};

void flushDeadKeyBuffer(HKL layout)
{
  BYTE emptyState[256] = {0};
  WCHAR unicode[4] = {0};
  while (ToUnicodeEx(VK_SPACE, MapVirtualKeyEx(VK_SPACE, MAPVK_VK_TO_VSC, layout), emptyState, unicode, 4, 0, layout) <
         0) {
  }
}

bool findDeadComposePair(ComposePair &pair)
{
  const HKL layout = GetKeyboardLayout(0);
  if (layout == nullptr) {
    return false;
  }

  const UINT deadCandidates[] = {VK_OEM_1, VK_OEM_2, VK_OEM_3, VK_OEM_4, VK_OEM_5, VK_OEM_6, VK_OEM_7};
  const UINT composeCandidates[] = {'A', 'E', 'I', 'O', 'U', 'N', 'C'};

  for (const UINT deadVK : deadCandidates) {
    flushDeadKeyBuffer(layout);

    BYTE deadState[256] = {0};
    deadState[deadVK] = 0x80;
    WCHAR unicode[4] = {0};
    const UINT deadScan = MapVirtualKeyEx(deadVK, MAPVK_VK_TO_VSC, layout);
    const int deadResult = ToUnicodeEx(deadVK, deadScan, deadState, unicode, 4, 0, layout);
    if (deadResult >= 0) {
      continue;
    }

    for (const UINT composeVK : composeCandidates) {
      BYTE composeState[256] = {0};
      composeState[composeVK] = 0x80;
      WCHAR composedOut[4] = {0};
      const UINT composeScan = MapVirtualKeyEx(composeVK, MAPVK_VK_TO_VSC, layout);
      const int composeResult = ToUnicodeEx(composeVK, composeScan, composeState, composedOut, 4, 0, layout);

      flushDeadKeyBuffer(layout);

      if (composeResult == 1 && composedOut[0] != 0) {
        pair.deadVK = deadVK;
        pair.composeVK = composeVK;
        pair.composed = composedOut[0];
        return true;
      }
    }
  }

  return false;
}

bool composeWithDeadKey(HKL layout, UINT deadVK, UINT composeVK, WCHAR &resultChar)
{
  flushDeadKeyBuffer(layout);

  BYTE deadState[256] = {0};
  deadState[deadVK] = 0x80;
  WCHAR temp[4] = {0};
  const UINT deadScan = MapVirtualKeyEx(deadVK, MAPVK_VK_TO_VSC, layout);
  const int deadResult = ToUnicodeEx(deadVK, deadScan, deadState, temp, 4, 0, layout);
  if (deadResult >= 0) {
    flushDeadKeyBuffer(layout);
    return false;
  }

  BYTE composeState[256] = {0};
  composeState[composeVK] = 0x80;
  WCHAR composedOut[4] = {0};
  const UINT composeScan = MapVirtualKeyEx(composeVK, MAPVK_VK_TO_VSC, layout);
  const int composeResult = ToUnicodeEx(composeVK, composeScan, composeState, composedOut, 4, 0, layout);
  flushDeadKeyBuffer(layout);

  if (composeResult == 1 && composedOut[0] != 0) {
    resultChar = composedOut[0];
    return true;
  }
  return false;
}

bool findDeadKeyForAoComposition(DeadKeyAoPair &pair)
{
  const HKL layout = GetKeyboardLayout(0);
  if (layout == nullptr) {
    return false;
  }

  const UINT deadCandidates[] = {VK_OEM_1, VK_OEM_2, VK_OEM_3, VK_OEM_4, VK_OEM_5, VK_OEM_6, VK_OEM_7};
  for (const UINT deadVK : deadCandidates) {
    WCHAR composedA = 0;
    WCHAR composedO = 0;
    if (!composeWithDeadKey(layout, deadVK, 'A', composedA)) {
      continue;
    }
    if (!composeWithDeadKey(layout, deadVK, 'O', composedO)) {
      continue;
    }

    if (std::towlower(composedA) == L'a' || std::towlower(composedO) == L'o') {
      continue;
    }

    pair.deadVK = deadVK;
    pair.composedA = composedA;
    pair.composedO = composedO;
    return true;
  }

  return false;
}

INPUT makeKeyInput(WORD vk, bool keyUp)
{
  INPUT in = {};
  in.type = INPUT_KEYBOARD;
  in.ki.wVk = vk;
  in.ki.dwFlags = keyUp ? KEYEVENTF_KEYUP : 0;
  return in;
}

void sendKey(WORD vk, bool keyUp)
{
  INPUT in = makeKeyInput(vk, keyUp);
  (void)SendInput(1, &in, sizeof(INPUT));
}

QList<MSG> collectHookMessages(UINT target, int timeoutMs)
{
  QList<MSG> out;
  QElapsedTimer timer;
  timer.start();

  while (timer.elapsed() < timeoutMs) {
    MSG msg;
    bool gotOne = false;
    while (PeekMessage(&msg, nullptr, DESKFLOW_MSG_INPUT_FIRST, DESKFLOW_HOOK_LAST_MSG, PM_REMOVE)) {
      gotOne = true;
      if (msg.message == target) {
        out.push_back(msg);
      }
    }
    if (!gotOne) {
      QTest::qWait(5);
    }
  }

  return out;
}

void clearHookMessageQueue()
{
  MSG pending;
  while (PeekMessage(&pending, nullptr, DESKFLOW_MSG_INPUT_FIRST, DESKFLOW_HOOK_LAST_MSG, PM_REMOVE)) {
  }
}

void runSequence(const std::initializer_list<std::pair<WORD, bool>> &sequence)
{
  for (const auto &event : sequence) {
    sendKey(event.first, event.second);
  }
}

bool containsComposedCharacter(const QList<MSG> &keyMsgs, WCHAR composed)
{
  for (const auto &msg : keyMsgs) {
    const WCHAR wc = static_cast<WCHAR>(LOWORD(msg.wParam));
    if (wc == composed) {
      return true;
    }
  }
  return false;
}

QList<WCHAR> extractKeyDownChars(const QList<MSG> &keyMsgs)
{
  QList<WCHAR> chars;
  for (const auto &msg : keyMsgs) {
    const bool isKeyUp = (msg.lParam & 0x80000000u) != 0;
    if (isKeyUp) {
      continue;
    }
    const WCHAR wc = static_cast<WCHAR>(LOWORD(msg.wParam));
    if (wc != 0) {
      chars.push_back(wc);
    }
  }
  return chars;
}

QString charsToDebugString(const QList<WCHAR> &chars)
{
  QString out;
  for (const auto c : chars) {
    if (!out.isEmpty()) {
      out += ' ';
    }
    out += QStringLiteral("U+%1").arg(static_cast<unsigned int>(c), 4, 16, QLatin1Char('0')).toUpper();
    out += QStringLiteral("('");
    out += QChar(c);
    out += QStringLiteral("')");
  }
  return out;
}

void assertSequenceProducesComposed(
    const ComposePair &pair, const std::initializer_list<std::pair<WORD, bool>> &sequence
)
{
  clearHookMessageQueue();

  MSWindowsHook hook;
  hook.loadLibrary();
  hook.setMode(kHOOK_RELAY_EVENTS);
  const auto installResult = MSWindowsHook::install();
  QVERIFY2(installResult != kHOOK_FAILED, "Failed to install low-level Windows hook");

  runSequence(sequence);

  const auto keyMsgs = collectHookMessages(DESKFLOW_MSG_KEY, 350);
  MSWindowsHook::uninstall();

  const bool composedSeen = containsComposedCharacter(keyMsgs, pair.composed);
  QVERIFY2(composedSeen, "Expected composed character from dead-key sequence was not observed");
}

} // namespace

void MSWindowsHookIntegrationTests::deadKeyIsPreservedAcrossUnrelatedKeyUpUntilNextComposeKeyDown()
{
  ComposePair pair;
  if (!findDeadComposePair(pair)) {
    QSKIP("No suitable dead-key + compose-key pair found for current keyboard layout");
  }
  assertSequenceProducesComposed(
      pair, {{'N', false},
             {static_cast<WORD>(pair.deadVK), false},
             {'N', true},
             {static_cast<WORD>(pair.composeVK), false},
             {static_cast<WORD>(pair.composeVK), true},
             {static_cast<WORD>(pair.deadVK), true}}
  );
}

void MSWindowsHookIntegrationTests::deadKeyCompose_whenDeadReleasedBeforeComposeKeyDown()
{
  ComposePair pair;
  if (!findDeadComposePair(pair)) {
    QSKIP("No suitable dead-key + compose-key pair found for current keyboard layout");
  }
  assertSequenceProducesComposed(
      pair, {{'N', false},
             {static_cast<WORD>(pair.deadVK), false},
             {'N', true},
             {static_cast<WORD>(pair.deadVK), true},
             {static_cast<WORD>(pair.composeVK), false},
             {static_cast<WORD>(pair.composeVK), true}}
  );
}

void MSWindowsHookIntegrationTests::deadKeyCompose_whenDeadReleasedBetweenComposeKeyDownAndUp()
{
  ComposePair pair;
  if (!findDeadComposePair(pair)) {
    QSKIP("No suitable dead-key + compose-key pair found for current keyboard layout");
  }
  assertSequenceProducesComposed(
      pair, {{'N', false},
             {static_cast<WORD>(pair.deadVK), false},
             {'N', true},
             {static_cast<WORD>(pair.composeVK), false},
             {static_cast<WORD>(pair.deadVK), true},
             {static_cast<WORD>(pair.composeVK), true}}
  );
}

void MSWindowsHookIntegrationTests::deadKeyDoesNotLeakToNextCharacter_afterCompose()
{
  DeadKeyAoPair pair;
  if (!findDeadKeyForAoComposition(pair)) {
    QSKIP("No suitable dead-key that composes with both A and O in current keyboard layout");
  }

  clearHookMessageQueue();

  MSWindowsHook hook;
  hook.loadLibrary();
  hook.setMode(kHOOK_RELAY_EVENTS);
  const auto installResult = MSWindowsHook::install();
  QVERIFY2(installResult != kHOOK_FAILED, "Failed to install low-level Windows hook");

  runSequence({
      {'N', false},
      {static_cast<WORD>(pair.deadVK), false},
      {'N', true},
      {'A', false},
      {'A', true},
      {'O', false},
      {'O', true},
      {static_cast<WORD>(pair.deadVK), true},
  });

  const auto keyMsgs = collectHookMessages(DESKFLOW_MSG_KEY, 350);
  MSWindowsHook::uninstall();

  const auto keyDownChars = extractKeyDownChars(keyMsgs);
  const WCHAR composedA = static_cast<WCHAR>(std::towlower(pair.composedA));
  const WCHAR composedO = static_cast<WCHAR>(std::towlower(pair.composedO));

  int indexComposedA = -1;
  for (int i = 0; i < keyDownChars.size(); ++i) {
    if (std::towlower(keyDownChars[i]) == composedA) {
      indexComposedA = i;
      break;
    }
  }
  QVERIFY2(indexComposedA >= 0, "Expected composed A not observed (relay mode)");

  bool plainOAfterCompose = false;
  bool composedOAfterCompose = false;
  for (int i = indexComposedA + 1; i < keyDownChars.size(); ++i) {
    const WCHAR c = static_cast<WCHAR>(std::towlower(keyDownChars[i]));
    if (c == L'o') {
      plainOAfterCompose = true;
    }
    if (c == composedO) {
      composedOAfterCompose = true;
    }
  }

  QVERIFY2(plainOAfterCompose, "Expected plain 'o' not observed (relay mode)");
  QVERIFY2(!composedOAfterCompose, "Dead key leaked to O (relay mode)");
}

void MSWindowsHookIntegrationTests::deadKeyDoesNotLeak_whenNextKeyDownBeforeComposeKeyUp()
{
  DeadKeyAoPair pair;
  if (!findDeadKeyForAoComposition(pair)) {
    QSKIP("No suitable dead-key that composes with both A and O in current keyboard layout");
  }

  clearHookMessageQueue();

  MSWindowsHook hook;
  hook.loadLibrary();
  hook.setMode(kHOOK_RELAY_EVENTS);
  const auto installResult = MSWindowsHook::install();
  QVERIFY2(installResult != kHOOK_FAILED, "Failed to install low-level Windows hook");

  runSequence({
      {'N', false},
      {static_cast<WORD>(pair.deadVK), false},
      {'N', true},
      {'A', false},
      {static_cast<WORD>(pair.deadVK), true},
      {'O', false},
      {'A', true},
      {'O', true},
  });

  const auto keyMsgs = collectHookMessages(DESKFLOW_MSG_KEY, 350);
  MSWindowsHook::uninstall();

  const auto keyDownChars = extractKeyDownChars(keyMsgs);
  const WCHAR composedA = static_cast<WCHAR>(std::towlower(pair.composedA));
  const WCHAR composedO = static_cast<WCHAR>(std::towlower(pair.composedO));

  int indexComposedA = -1;
  for (int i = 0; i < keyDownChars.size(); ++i) {
    if (std::towlower(keyDownChars[i]) == composedA) {
      indexComposedA = i;
      break;
    }
  }
  QVERIFY2(indexComposedA >= 0, "Expected composed A not observed (relay mode)");

  bool plainOAfterCompose = false;
  bool composedOAfterCompose = false;
  for (int i = indexComposedA + 1; i < keyDownChars.size(); ++i) {
    const WCHAR c = static_cast<WCHAR>(std::towlower(keyDownChars[i]));
    if (c == L'o') {
      plainOAfterCompose = true;
    }
    if (c == composedO) {
      composedOAfterCompose = true;
    }
  }

  const auto debugChars = charsToDebugString(keyDownChars);
  QVERIFY2(
      !composedOAfterCompose,
      qPrintable(QStringLiteral("Dead key leaked in fast-overlap sequence (relay mode). Captured: %1").arg(debugChars))
  );
  if (!plainOAfterCompose) {
    QWARN(qPrintable(
        QStringLiteral("No plain 'o' observed in overlap sequence (relay mode). Captured: %1").arg(debugChars)
    ));
  }
}

QTEST_MAIN(MSWindowsHookIntegrationTests)
