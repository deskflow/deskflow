/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchFile.h"

#define ARCH_FILE ArchFileUnix

//! Unix implementation of IArchFile
class ArchFileUnix : public IArchFile
{
public:
  ArchFileUnix();
  virtual ~ArchFileUnix();

  // IArchFile overrides
  virtual const char *getBasename(const char *pathname);
  virtual std::string getUserDirectory();
  virtual std::string getSystemDirectory();
  virtual std::string getInstalledDirectory();
  virtual std::string getLogDirectory();
  virtual std::string getPluginDirectory();
  virtual std::string getProfileDirectory();
  virtual std::string concatPath(const std::string &prefix, const std::string &suffix);
  virtual void setProfileDirectory(const std::string &s);
  virtual void setPluginDirectory(const std::string &s);

private:
  std::string m_profileDirectory;
  std::string m_pluginDirectory;
};
