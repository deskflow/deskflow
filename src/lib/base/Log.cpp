/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/Log.h"
#include "base/LogLevel.h"
#include "base/LogOutputters.h"
#include "common/Constants.h"

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>

#if HAVE_FORMAT
#include <format>
#endif

#include <QDateTime>

const int kPriorityPrefixLength = 3;

// names of priorities
static const char *g_priority[] = {"FATAL", "ERROR", "WARNING", "NOTE", "INFO", "DEBUG", "DEBUG1", "DEBUG2"};

// number of priorities
static const int g_numPriority = 8;

// if NDEBUG (not debug) is not specified, i.e. you're building in debug,
// then set default log level to DEBUG, otherwise the max level is INFO.
//
// GOTCHA: if `-DCMAKE_BUILD_TYPE=Debug` isn't set when configuring cmake
// for visual studio, then NDEBUG will be set (even if your VS solution
// config is Debug).
#ifndef NDEBUG
static const LogLevel g_defaultMaxPriority = LogLevel::Debug;
#else
static const LogLevel g_defaultMaxPriority = LogLevel::Info;
#endif

namespace {

LogLevel getPriority(const char *&fmt)
{
  if (strnlen(fmt, SIZE_MAX) < kPriorityPrefixLength) {
    throw std::invalid_argument("invalid format string, too short");
  }

  if (fmt[0] != '%' || fmt[1] != 'z') {
    throw std::invalid_argument("invalid format string, missing priority");
  }

  return static_cast<LogLevel>(fmt[2] - '0');
}

std::vector<char> makeMessage(const char *filename, int lineNumber, const char *message, LogLevel priority)
{

  // base size includes null terminator, colon, space, etc.
  const int baseSize = 10;

  const int priorityMaxSize = 10;
  const auto currentPriority = static_cast<int>(priority);

  auto timeStr = QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString();

  auto sectionName = "IPC";
  if (priority != LogLevel::IPC) {
    sectionName = g_priority[currentPriority];
  }

  size_t priorityLength = strnlen(sectionName, priorityMaxSize);
  size_t messageLength = strnlen(message, SIZE_MAX);
  size_t bufferSize = baseSize + timeStr.length() + priorityLength + messageLength;

  const auto filenameSet = filename != nullptr && filename[0] != '\0';
  if (filenameSet) {
    size_t filenameLength = strnlen(filename, SIZE_MAX);
    size_t lineNumberLength = snprintf(nullptr, 0, "%d", lineNumber);
    bufferSize += filenameLength + lineNumberLength;

    std::vector<char> buffer(bufferSize);
#if HAVE_FORMAT
    std::format_to_n(
        buffer.data(), bufferSize, "[{}] {}: {}\n\t{}:{}", timeStr.c_str(), sectionName, message, filename, lineNumber
    );
#else
    snprintf(
        buffer.data(), bufferSize, "[%s] %s: %s\n\t%s:%d", timeStr.c_str(), sectionName, message, filename, lineNumber
    );
#endif
    return buffer;
  } else {
    std::vector<char> buffer(bufferSize);
#if HAVE_FORMAT
    std::format_to_n(buffer.data(), bufferSize, "[{}] {}: {}", timeStr.c_str(), sectionName, message);
#else
    snprintf(buffer.data(), bufferSize, "[%s] %s: %s", timeStr.c_str(), sectionName, message);
#endif
    return buffer;
  }
}
} // namespace

//
// Log
//

Log *Log::s_log = nullptr;

Log::Log(bool singleton)
{
  if (singleton) {
    assert(s_log == nullptr);
  }

  // other initalization
  m_maxPriority = g_defaultMaxPriority;
  insert(new ConsoleLogOutputter); // NOSONAR - Adopted by `Log`

  if (singleton) {
    s_log = this;
  }
}

Log::Log(Log *src)
{
  s_log = src;
}

Log::~Log()
{
  // clean up
  for (auto index = m_outputters.begin(); index != m_outputters.end(); ++index) {
    delete *index;
  }
  for (auto index = m_alwaysOutputters.begin(); index != m_alwaysOutputters.end(); ++index) {
    delete *index;
  }
}

Log *Log::getInstance()
{
  assert(s_log != nullptr);
  return s_log;
}

const char *Log::getFilterName() const
{
  return getFilterName(getFilter());
}

const char *Log::getFilterName(LogLevel level) const
{
  const auto levelIndex = static_cast<int>(level);
  if (levelIndex < 0) {
    return "Message";
  }
  return g_priority[levelIndex];
}

void Log::print(const char *file, int line, const char *fmt, ...)
{
  const int initBufferSize = 1024;
  const int bufferResizeScale = 2;

  LogLevel priority = getPriority(fmt);
  fmt += kPriorityPrefixLength;

  if (priority > getFilter()) {
    return;
  }

  std::vector<char> buffer(initBufferSize);
  auto length = static_cast<int>(buffer.size());

  while (true) {
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buffer.data(), length, fmt, args);
    va_end(args);

    if (n < 0 || n > length) {
      length *= bufferResizeScale;
      buffer.resize(length);
    } else {
      break;
    }
  }

  if (priority == LogLevel::Print) {
    output(priority, buffer.data());
  } else {
    auto message = makeMessage(file, line, buffer.data(), priority);
    output(priority, message.data());
  }
}

void Log::insert(ILogOutputter *adoptedOutputter, bool alwaysAtHead)
{
  assert(adoptedOutputter != nullptr);

  std::scoped_lock lock{m_mutex};
  if (alwaysAtHead) {
    m_alwaysOutputters.push_front(adoptedOutputter);
  } else {
    m_outputters.push_front(adoptedOutputter);
  }

  adoptedOutputter->open(kAppName);
}

void Log::remove(ILogOutputter *outputter)
{
  std::scoped_lock lock{m_mutex};
  m_outputters.remove(outputter);
  m_alwaysOutputters.remove(outputter);
}

void Log::pop_front(bool alwaysAtHead)
{
  std::scoped_lock lock{m_mutex};
  OutputterList *list = alwaysAtHead ? &m_alwaysOutputters : &m_outputters;
  if (!list->empty()) {
    delete list->front();
    list->pop_front();
  }
}

bool Log::setFilter(const QString &maxPriority)
{
  if (maxPriority.isEmpty()) {
    return false;
  }

  for (int i = 0; i < g_numPriority; ++i) {
    if (maxPriority == QString(g_priority[i])) {
      setFilter(static_cast<LogLevel>(i));
      return true;
    }
  }
  return false;
}

void Log::setFilter(LogLevel maxPriority)
{
  std::scoped_lock lock{m_mutex};
  m_maxPriority = maxPriority;
}

LogLevel Log::getFilter() const
{
  std::scoped_lock lock{m_mutex};
  return m_maxPriority;
}

void Log::output(LogLevel priority, const char *msg)
{
  assert(static_cast<int>(priority) >= -2 && static_cast<int>(priority) < g_numPriority);
  assert(msg != nullptr);
  if (!msg)
    return;

  std::scoped_lock lock{m_mutex};

  OutputterList::const_iterator i;

  for (i = m_alwaysOutputters.begin(); i != m_alwaysOutputters.end(); ++i) {

    // write to outputter
    (*i)->write(priority, msg);
  }

  for (i = m_outputters.begin(); i != m_outputters.end(); ++i) {

    // write to outputter and break out of loop if it returns false
    if (!(*i)->write(priority, msg)) {
      break;
    }
  }
}
