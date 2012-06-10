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

// -=- LogWriter.cxx - client-side logging interface

#include <string.h>
#ifdef WIN32
#define strcasecmp _stricmp
#endif

#include <rfb/LogWriter.h>
#include <rfb/Configuration.h>
#include <rfb/util.h>
#include <stdlib.h>

rfb::LogParameter rfb::logParams;

using namespace rfb;


LogWriter::LogWriter(const char* name) : m_name(name), m_level(0), m_log(0), m_next(log_writers) {
  log_writers = this;
}

LogWriter::~LogWriter() {
  // *** Should remove this logger here!
}

void LogWriter::setLog(Logger *logger) {
  m_log = logger;
}

void LogWriter::setLevel(int level) {
  m_level = level;
}

void
LogWriter::listLogWriters(int width) {
  // *** make this respect width...
  LogWriter* current = log_writers;
  fprintf(stderr, "  ");
  while (current) {
    fprintf(stderr, "%s", current->m_name);
    current = current->m_next;
    if (current) fprintf(stderr, ", ");
  }
  fprintf(stderr, "\n");
}

LogWriter* LogWriter::log_writers;

LogWriter*
LogWriter::getLogWriter(const char* name) {
  LogWriter* current = log_writers;
  while (current) {
    if (strcasecmp(name, current->m_name) == 0) return current;
      current = current->m_next;
    }
  return 0;
}

bool LogWriter::setLogParams(const char* params) {
  CharArray logwriterName, loggerName, logLevel;
  if (!strSplit(params, ':', &logwriterName.buf, &loggerName.buf) ||
    !strSplit(loggerName.buf, ':', &loggerName.buf, &logLevel.buf)) {
    fprintf(stderr,"failed to parse log params:%s\n",params);
    return false;
  }
  int level = atoi(logLevel.buf);
  Logger* logger = 0;
  if (strcmp("", loggerName.buf) != 0) {
    logger = Logger::getLogger(loggerName.buf);
    if (!logger) fprintf(stderr,"no logger found! %s\n",loggerName.buf);
  }
  if (strcmp("*", logwriterName.buf) == 0) {
    LogWriter* current = log_writers;
    while (current) {
      current->setLog(logger);
      current->setLevel(level);
      current = current->m_next;
    }
    return true;
  } else {
    LogWriter* logwriter = getLogWriter(logwriterName.buf);
    if (!logwriter) {
      fprintf(stderr,"no logwriter found! %s\n",logwriterName.buf);
    } else {
      logwriter->setLog(logger);
      logwriter->setLevel(level);
      return true;
    }
  }
  return false;
}


LogParameter::LogParameter()
  : StringParameter("Log",
    "Specifies which log output should be directed to "
    "which target logger, and the level of output to log. "
    "Format is <log>:<target>:<level>[, ...].",
    "") {
}

bool LogParameter::setParam(const char* v) {
  if (immutable) return true;
  LogWriter::setLogParams("*::0");
  StringParameter::setParam(v);
  CharArray logParam;
  CharArray params(getData());
  while (params.buf) {
    strSplit(params.buf, ',', &logParam.buf, &params.buf);
    if (strlen(logParam.buf) && !LogWriter::setLogParams(logParam.buf))
      return false;
  }
  return true;
}

void LogParameter::setDefault(const char* d) {
  def_value = d;
  setParam(def_value);
}
