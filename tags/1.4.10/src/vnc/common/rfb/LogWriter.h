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

// -=- LogWriter.h - The Log writer class.

#ifndef __RFB_LOG_WRITER_H__
#define __RFB_LOG_WRITER_H__

#include <stdarg.h>
#include <rfb/Logger.h>
#include <rfb/Configuration.h>

// Each log writer instance has a unique textual name,
// and is attached to a particular Log instance and
// is assigned a particular log level.

#define DEF_LOGFUNCTION(name, level) \
  inline void name(const char* fmt, ...) { \
    if (m_log && (level <= m_level)) {     \
      va_list ap; va_start(ap, fmt);       \
      m_log->write(level, m_name, fmt, ap);\
      va_end(ap);                          \
    }                                      \
  }

namespace rfb {

  class LogWriter;

  class LogWriter {
  public:
    LogWriter(const char* name);
    ~LogWriter();

    const char *getName() {return m_name;}

    void setLog(Logger *logger);
    void setLevel(int level);

    inline void write(int level, const char* format, ...) {
      if (m_log && (level <= m_level)) {
        va_list ap;
        va_start(ap, format);
        m_log->write(level, m_name, format, ap);
        va_end(ap);
      }
    }

    DEF_LOGFUNCTION(error, 0)
    DEF_LOGFUNCTION(status, 10)
    DEF_LOGFUNCTION(info, 30)
    DEF_LOGFUNCTION(debug, 100)

    // -=- DIAGNOSTIC & HELPER ROUTINES

    static void listLogWriters(int width=79);

    // -=- CLASS FIELDS & FUNCTIONS

    static LogWriter* log_writers;

    static LogWriter* getLogWriter(const char* name);

    static bool setLogParams(const char* params);

  private:
    const char* m_name;
    int m_level;
    Logger* m_log;
    LogWriter* m_next;
  };

  class LogParameter : public StringParameter {
  public:
    LogParameter();
    virtual bool setParam(const char* v);

    // Call this to set a suitable default value.
    // Can't use the normal default mechanism for
    // this because there is no guarantee on C++
    // constructor ordering - some LogWriters may
    // not exist when LogParameter gets constructed.
    // NB: The default value must exist for the
    //     lifetime of the process!
    void setDefault(const char* v);
  };
  extern LogParameter logParams;

};

#endif // __RFB_LOG_WRITER_H__
