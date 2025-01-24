/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/common.h"

#if defined(__cplusplus)
extern "C"
{
#endif

  void *screenSaverUtilCreatePool();
  void screenSaverUtilReleasePool(void *);

  void *screenSaverUtilCreateController();
  void screenSaverUtilReleaseController(void *);
  void screenSaverUtilEnable(void *);
  void screenSaverUtilDisable(void *);
  void screenSaverUtilActivate(void *);
  void screenSaverUtilDeactivate(void *, int isEnabled);
  int screenSaverUtilIsActive(void *);

#if defined(__cplusplus)
}
#endif
