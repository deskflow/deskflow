/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef COSXSCREENSAVERUTIL_H
#define COSXSCREENSAVERUTIL_H

#include "common.h"

#if defined(__cplusplus)
extern "C" {
#endif

void*					screenSaverUtilCreatePool();
void					screenSaverUtilReleasePool(void*);

void*					screenSaverUtilCreateController();
void					screenSaverUtilReleaseController(void*);
void					screenSaverUtilEnable(void*);
void					screenSaverUtilDisable(void*);
void					screenSaverUtilActivate(void*);
void					screenSaverUtilDeactivate(void*, int isEnabled);
int						screenSaverUtilIsActive(void*);

#if defined(__cplusplus)
}
#endif

#endif
