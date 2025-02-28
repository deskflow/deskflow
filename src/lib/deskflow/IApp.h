/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2012 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/IInterface.h"

typedef int (*StartupFunc)(int, char **);

namespace deskflow {
class ArgsBase;
class Screen;
} // namespace deskflow

class IEventQueue;

class IApp : public IInterface
{
public:
  virtual void setByeFunc(void (*bye)(int)) = 0;
  virtual deskflow::ArgsBase &argsBase() const = 0;
  virtual int standardStartup(int argc, char **argv) = 0;
  virtual int runInner(int argc, char **argv, StartupFunc startup) = 0;
  virtual void startNode() = 0;
  virtual void bye(int error) = 0;
  virtual int mainLoop() = 0;
  virtual void initApp(int argc, const char **argv) = 0;
  virtual const char *daemonName() const = 0;
  virtual int foregroundStartup(int argc, char **argv) = 0;
  virtual deskflow::Screen *createScreen() = 0;
  virtual IEventQueue *getEvents() const = 0;
};
