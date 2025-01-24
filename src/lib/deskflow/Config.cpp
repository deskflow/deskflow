/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "Config.h"

#include "base/Log.h"

#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

// Use .h for fallback with 3.3.0
#include <toml++/toml.h>

namespace deskflow {

Config::Config(const std::string &filename, const std::string &section) : m_filename(filename), m_section(section)
{
}

const char *const *Config::argv() const
{
  return m_argv.data();
}

int Config::argc() const
{
  return static_cast<int>(m_argv.size());
}

bool Config::load(const std::string &firstArg)
{

  if (!firstArg.empty()) {
    m_args.push_back(firstArg);
  }

  if (m_filename.empty()) {
    throw NoConfigFilenameError();
  }

  if (!std::filesystem::exists(m_filename)) {
    LOG((CLOG_ERR "config file not found: %s", m_filename.c_str()));
    return false;
  }

  toml::table configTable;
  try {
    LOG((CLOG_INFO "loading config file: %s", m_filename.c_str()));
    configTable = toml::parse_file(m_filename);

  } catch (const toml::parse_error &err) {
    LOG((CLOG_ERR "toml parse error: %s", err.what()));
    throw ParseError();
  } catch (const std::exception &err) {
    LOG((CLOG_ERR "unknown parse error: %s", err.what()));
    throw ParseError();
  }

  if (!configTable.contains(m_section)) {
    LOG((CLOG_WARN "no %s section found in config file", m_section.c_str()));
    return false;
  }

  const auto &section = configTable[m_section];
  const auto args = section["args"];
  if (!args.is_table()) {
    LOG((CLOG_WARN "no args table found in config file"));
    return false;
  }

  std::string specialLastArg = "";
  const auto &table = *(args.as_table());
  for (const auto &pair : table) {
    const auto &key = pair.first;
    if (key.str() == "_last") {
      specialLastArg = pair.second.as_string()->get();
      continue;
    }

    m_args.push_back("--" + std::string(key.str()));

    if (pair.second.is_string()) {
      const auto value = pair.second.as_string()->get();
      m_args.push_back(value);
    }
  }

  if (!specialLastArg.empty()) {
    m_args.push_back(specialLastArg);
  }

  if (m_args.empty()) {
    LOG((CLOG_WARN "no args loaded from config file"));
    return false;
  }

  for (const auto &arg : m_args) {
    m_argv.push_back(arg.c_str());
  }

  return true;
}

} // namespace deskflow
