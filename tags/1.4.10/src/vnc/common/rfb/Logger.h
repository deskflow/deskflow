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

// -=- Logger.h - The Logger class.

#ifndef __RFB_LOGGER_H__
#define __RFB_LOGGER_H__

#include <stdarg.h>
#include <stdio.h>

// Each log writer instance has a unique textual name,
// and is attached to a particular Logger instance and
// is assigned a particular log level.

namespace rfb {

  class Logger {
  public:

    // -=- Create / Destroy a logger

    Logger(const char* name);
    virtual ~Logger();

    // -=- Get the name of a logger

    const char *getName() {return m_name;}

    // -=- Write data to a log

    virtual void write(int level, const char *logname, const char *text) = 0;
    void write(int level, const char *logname, const char* format, va_list ap);

    // -=- Register a logger

    void registerLogger();

    // -=- CLASS FIELDS & FUNCTIONS

    static Logger* loggers;

    static Logger* getLogger(const char* name);

    static void listLoggers();

  private:
    bool registered;
    const char *m_name;
    Logger *m_next;
  };

};

#endif // __RFB_LOGGER_H__
