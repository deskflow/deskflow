/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

// -=- wm_hooks.h
//
// Window Message Hooks Dynamic Link library
//
// This interface is used by the WMHooks class in rfb_win32 to hook the
// windows on the desktop and receive notifications of changes in their
// state.

#ifndef __WM_HOOKS_H__
#define __WM_HOOKS_H__

#include <windows.h>

#define DLLEXPORT __declspec(dllexport)

extern "C"
{
  //
  // -=- Display hook message types
  //

  DLLEXPORT UINT WM_Hooks_WindowChanged();
  DLLEXPORT UINT WM_Hooks_WindowBorderChanged();
  DLLEXPORT UINT WM_Hooks_WindowClientAreaChanged();
  DLLEXPORT UINT WM_Hooks_RectangleChanged();
  DLLEXPORT UINT WM_Hooks_CursorChanged();

  //
  // -=- Display update hooks
  //

  // - WM_Hooks_Install
  // Add the current thread to the list of threads that will receive
  // notifications of changes to the display.
  // If thread is NULL then the entire display will be hooked.
  // If thread is !NULL and then the specified
  // thread will be hooked.
  // Each thread may only register one hook at a time.
  // The call will fail (return FALSE) if the thread already has hooks
  // set, or if the hooks cannot be set, or some other error occurs.

  DLLEXPORT BOOL WM_Hooks_Install(DWORD owner, DWORD thread);

  // - WM_Hooks_Remove
  // Removes any hook set by the current thread.
  // The return indicates whether anything went wrong removing the hooks,
  // that might cause problems later.

  DLLEXPORT BOOL WM_Hooks_Remove(DWORD owner);

  //
  // -=- User input hooks
  //

  // - WM_Hooks_EnableRealInputs
  // If TRUE is passed, then "real" input is enabled, otherwise it is disabled.

  DLLEXPORT BOOL WM_Hooks_EnableRealInputs(BOOL pointer, BOOL keyboard);

  // - WM_Hooks_EnableSynthInputs
  // If TRUE is passed, then synthetic inputs are enabled, otherwise disabled.

  DLLEXPORT BOOL WM_Hooks_EnableSynthInputs(BOOL pointer, BOOL keyboard);

  //
  // -=- Cursor shape hooking
  //

  // - WM_Hooks_EnableCursorShape
  // If TRUE is passed, then hooks will produce notifications when cursor shape
  // changes.

  DLLEXPORT BOOL WM_Hooks_EnableCursorShape(BOOL enable);

#ifdef _DEBUG
  // - WM_Hooks_SetDiagnosticRange
  // Select a range of messages that will be reported while hooks are active
  DLLEXPORT void WM_Hooks_SetDiagnosticRange(UINT min, UINT max);
  DLLEXPORT UINT WM_Hooks_Diagnostic();
#endif

}

#endif // __WM_HOOKS_H__
