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

// -=- Logger_file - log to a file

#ifndef __RFB_LOGGER_FILE_H__
#define __RFB_LOGGER_FILE_H__

#include <time.h>
#include <rfb/Logger.h>

namespace rfb {

  class Logger_File : public Logger {
  public:
    Logger_File(const char* loggerName);
    ~Logger_File();

    virtual void write(int level, const char *logname, const char *message);
    void setFilename(const char* filename);
    void setFile(FILE* file);

    int indent;
    int width;

  protected:
    void closeFile();
    char* m_filename;
    FILE* m_file;
    time_t m_lastLogTime;
  };

  bool initFileLogger(const char* filename);
};

#endif
