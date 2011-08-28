/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2011 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#ifdef synxinhk_EXPORTS
#define synxinhk_API __declspec(dllexport)
#else
#define synxinhk_API __declspec(dllimport)
#endif

synxinhk_API LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam);
synxinhk_API BOOL InstallXInputHook();
synxinhk_API void RemoveXInputHook();
synxinhk_API void SetXInputButtons(DWORD userIndex, WORD buttons);
synxinhk_API void SetXInputSticks(DWORD userIndex, SHORT lx, SHORT ly, SHORT rx, SHORT ry);
synxinhk_API void SetXInputTriggers(DWORD userIndex, BYTE left, BYTE right);
synxinhk_API void QueueXInputTimingReq();
synxinhk_API BOOL DequeueXInputTimingResp();
synxinhk_API WORD GetXInputFakeFreqMillis();
synxinhk_API BOOL DequeueXInputFeedback(WORD* leftMotor, WORD* rightMotor);

#ifdef SYNERGY_EXPORT_XINPUT_HOOKS

synxinhk_API DWORD WINAPI HookXInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState);
synxinhk_API DWORD WINAPI HookXInputSetState(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration);
synxinhk_API DWORD WINAPI HookXInputGetCapabilities(DWORD userIndex, DWORD flags, XINPUT_CAPABILITIES* capabilities);

#endif
