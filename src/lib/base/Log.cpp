/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.    If not, see <http://www.gnu.org/licenses/>.
 */

#include "base/Log.h"
#include "arch/Arch.h"
#include "base/log_outputters.h"
#include "common/Version.h"

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>

const int kPriorityPrefixLength = 3;

// names of priorities
static const char *g_priority[] = {"FATAL",  "ERROR",  "WARNING", "NOTE",
                                   "INFO",   "DEBUG",  "DEBUG1",  "DEBUG2",
                                   "DEBUG3", "DEBUG4", "DEBUG5"};

// number of priorities
static const int g_numPriority =
    (int)(sizeof(g_priority) / sizeof(g_priority[0]));

// if NDEBUG (not debug) is not specified, i.e. you're building in debug,
// then set default log level to DEBUG, otherwise the max level is INFO.
//
// GOTCHA: if `-DCMAKE_BUILD_TYPE=Debug` isn't set when configuring cmake
// for visual studio, then NDEBUG will be set (even if your VS solution
// config is Debug).
#ifndef NDEBUG
static const int g_defaultMaxPriority = kDEBUG;
#else
static const int g_defaultMaxPriority = kINFO;
#endif

namespace {

ELevel getPriority(const char *&fmt) {
  if (strnlen(fmt, SIZE_MAX) < kPriorityPrefixLength) {
    throw std::invalid_argument("invalid format string, too short");
  }

  if (fmt[0] != '%' || fmt[1] != 'z') {
    throw std::invalid_argument("invalid format string, missing %z");
  }

  return static_cast<ELevel>(fmt[2] - '0');
}

std::vector<char> makeMessage(const char *filename, int line,
                              const char *message, ELevel priority,
                              bool debug) {
  const int timeBufferSize = 50;
  const int yearOffset = 1900;
  const int monthOffset = 1;
  const int baseSize = 10;

  char timestamp[timeBufferSize];
  time_t t;
  time(&t);
  struct tm tm;

#if WINAPI_MSWINDOWS
  localtime_s(&tm, &t);
#else
  localtime_r(&t, &tm);
#endif

  snprintf(timestamp, sizeof(timestamp), "%04i-%02i-%02iT%02i:%02i:%02i",
           tm.tm_year + yearOffset, tm.tm_mon + monthOffset, tm.tm_mday,
           tm.tm_hour, tm.tm_min, tm.tm_sec);

  size_t timestampLength = strnlen(timestamp, sizeof(timestamp));
  size_t priorityLength = strnlen(g_priority[priority], SIZE_MAX);
  size_t messageLength = strnlen(message, SIZE_MAX);

  size_t size = baseSize + timestampLength + priorityLength + messageLength;

  if (debug) {
    std::cout << "*** trace start ***" << std::endl;
    size_t filenameLength = strnlen(filename, SIZE_MAX);
    std::cout << "*** trace end ***" << std::endl;
    const int debugFileOffset = 6;
    size += filenameLength + debugFileOffset;
    std::vector<char> logMessage(size);
    snprintf(logMessage.data(), size, "[%s] %s: %s\n\t%s:%d", timestamp,
             g_priority[priority], message, filename, line);
    return logMessage;
  } else {
    std::vector<char> logMessage(size);
    snprintf(logMessage.data(), size, "[%s] %s: %s", timestamp,
             g_priority[priority], message);
    return logMessage;
  }
}
} // namespace

//
// Log
//

Log *Log::s_log = NULL;

Log::Log(bool singleton, bool debug) : m_debug(debug) {
  if (singleton) {
    assert(s_log == NULL);
  }

  // create mutex for multithread safe operation
  m_mutex = ARCH->newMutex();

  // other initalization
  m_maxPriority = g_defaultMaxPriority;
  insert(new ConsoleLogOutputter);

  if (singleton) {
    s_log = this;
  }
}

Log::Log(Log *src) { s_log = src; }

Log::~Log() {
  // clean up
  for (OutputterList::iterator index = m_outputters.begin();
       index != m_outputters.end(); ++index) {
    delete *index;
  }
  for (OutputterList::iterator index = m_alwaysOutputters.begin();
       index != m_alwaysOutputters.end(); ++index) {
    delete *index;
  }
  ARCH->closeMutex(m_mutex);
}

Log *Log::getInstance() {
  assert(s_log != NULL);
  return s_log;
}

const char *Log::getFilterName() const { return getFilterName(getFilter()); }

const char *Log::getFilterName(int level) const {
  if (level < 0) {
    return "Message";
  }
  return g_priority[level];
}

void Log::print(const char *file, int line, const char *fmt, ...) {
  const int initBufferSize = 1024;
  const int bufferResizeScale = 2;

  ELevel priority = getPriority(fmt);
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

  if (priority != kPRINT) {
    auto message = makeMessage(file, line, buffer.data(), priority, m_debug);
    output(priority, message.data());
  } else {
    output(priority, buffer.data());
  }
}

void Log::insert(ILogOutputter *outputter, bool alwaysAtHead) {
  assert(outputter != NULL);

  ArchMutexLock lock(m_mutex);
  if (alwaysAtHead) {
    m_alwaysOutputters.push_front(outputter);
  } else {
    m_outputters.push_front(outputter);
  }

  outputter->open(kAppVersion);

  // Issue 41
  // don't show log unless user requests it, as some users find this
  // feature irritating (i.e. when they lose network connectivity).
  // in windows the log window can be displayed by selecting "show log"
  // from the synergy system tray icon.
  // if this causes problems for other architectures, then a different
  // work around should be attempted.
  // outputter->show(false);
}

void Log::remove(ILogOutputter *outputter) {
  ArchMutexLock lock(m_mutex);
  m_outputters.remove(outputter);
  m_alwaysOutputters.remove(outputter);
}

void Log::pop_front(bool alwaysAtHead) {
  ArchMutexLock lock(m_mutex);
  OutputterList *list = alwaysAtHead ? &m_alwaysOutputters : &m_outputters;
  if (!list->empty()) {
    delete list->front();
    list->pop_front();
  }
}

bool Log::setFilter(const char *maxPriority) {
  if (maxPriority != NULL) {
    for (int i = 0; i < g_numPriority; ++i) {
      if (strcmp(maxPriority, g_priority[i]) == 0) {
        setFilter(i);
        return true;
      }
    }
    return false;
  }
  return true;
}

void Log::setFilter(int maxPriority) {
  ArchMutexLock lock(m_mutex);
  m_maxPriority = maxPriority;
}

int Log::getFilter() const {
  ArchMutexLock lock(m_mutex);
  return m_maxPriority;
}

void Log::output(ELevel priority, char *msg) {
  assert(priority >= -1 && priority < g_numPriority);
  assert(msg != NULL);
  if (!msg)
    return;

  ArchMutexLock lock(m_mutex);

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
