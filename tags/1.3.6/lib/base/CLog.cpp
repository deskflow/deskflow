/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CLog.h"
#include "CString.h"
#include "CStringUtil.h"
#include "LogOutputters.h"
#include "CArch.h"
#include "Version.h"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <ctime> 

// names of priorities
static const char*    g_priority[] = {
                "FATAL",
                "ERROR",
                "WARNING",
                "NOTE",
                "INFO",
                "DEBUG",
                "DEBUG1",
                "DEBUG2"
              };

// number of priorities
static const int    g_numPriority = (int)(sizeof(g_priority) /
                      sizeof(g_priority[0]));

// the default priority
#if defined(NDEBUG)
static const int    g_defaultMaxPriority = 4;
#else
static const int    g_defaultMaxPriority = 5;
#endif

// length of longest string in g_priority
static const int    g_maxPriorityLength = 7;

// length of suffix string (": ")
static const int    g_prioritySuffixLength = 2;

// amount of padded required to fill in the priority prefix
static const int    g_priorityPad = g_maxPriorityLength +
                    g_prioritySuffixLength;


//
// CLog
//

CLog*         CLog::s_log = NULL;

CLog::CLog()
{
  assert(s_log == NULL);

  // create mutex for multithread safe operation
  m_mutex            = ARCH->newMutex();

  // other initalization
  m_maxPriority      = g_defaultMaxPriority;
  m_maxNewlineLength = 0;
  insert(new CConsoleLogOutputter);
}

CLog::~CLog()
{
  // clean up
  for (COutputterList::iterator index  = m_outputters.begin();
                  index != m_outputters.end(); ++index) {
    delete *index;
  }
  for (COutputterList::iterator index  = m_alwaysOutputters.begin();
                  index != m_alwaysOutputters.end(); ++index) {
    delete *index;
  }
  ARCH->closeMutex(m_mutex);
  s_log = NULL;
}

CLog*
CLog::getInstance()
{
  // note -- not thread safe;  client must initialize log safely
  if (s_log == NULL) {
    s_log = new CLog;
  }
  return s_log;
}

void
CLog::print(const char* file, int line, const char* fmt, ...) const
{
  // check if fmt begins with a priority argument
  int priority = 4;
  if (fmt[0] == '%' && fmt[1] == 'z') {
    priority = fmt[2] - '\060';
    fmt += 3;
  }

  // done if below priority threshold
  if (priority > getFilter()) {
    return;
  }

  // compute prefix padding length
  char stack[1024];

  // compute suffix padding length
  int sPad = m_maxNewlineLength;

  // print to buffer, leaving space for a newline at the end and prefix
  // at the beginning.
  char* buffer = stack;
  int len      = (int)(sizeof(stack) / sizeof(stack[0]));
  while (true) {
    // try printing into the buffer
    va_list args;
    va_start(args, fmt);
    int n = ARCH->vsnprintf(buffer, len  - sPad, fmt, args);
    va_end(args);

    // if the buffer wasn't big enough then make it bigger and try again
    if (n < 0 || n > (int)len) {
      if (buffer != stack) {
        delete[] buffer;
      }
      len   *= 2;
      buffer = new char[len];
    }

    // if the buffer was big enough then continue
    else {
      break;
    }
  }

  // print the prefix to the buffer.  leave space for priority label.
  if (file != NULL) {
      char message[2048];
	  struct tm *tm;
	  char tmp[220];
	  time_t t;
	  time(&t);
	  tm = localtime(&t);
	  sprintf(tmp, "%04i-%02i-%02iT%02i:%02i:%02i", tm->tm_year + 1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	  //strcpy(msg, tmp);

	  
	  sprintf(message, "%s %s: %s\n\t%s,%d", tmp, g_priority[priority], buffer, file, line);
//    buffer[pPad - 1] = ' ';

    // discard file and line if priority < 0
    /*if (priority < 0) {
      message += pPad - g_priorityPad;
    }
	*/
	  // output buffer
	  output(priority, message);
  } else {
	  output(priority, buffer);
  }

  // clean up
  if (buffer != stack) {
    delete[] buffer;
  }
}

void
CLog::insert(ILogOutputter* outputter, bool alwaysAtHead)
{
  assert(outputter               != NULL);
  assert(outputter->getNewline() != NULL);

  CArchMutexLock lock(m_mutex);
  if (alwaysAtHead) {
    m_alwaysOutputters.push_front(outputter);
  }
  else {
    m_outputters.push_front(outputter);
  }
  int newlineLength = (int)strlen(outputter->getNewline());
  if (newlineLength > m_maxNewlineLength) {
    m_maxNewlineLength = newlineLength;
  }
  outputter->open(kAppVersion);

  // Issue 41
  // don't show log unless user requests it, as some users find this
  // feature irritating (i.e. when they lose network connectivity).
  // in windows the log window can be displayed by selecting "show log"
  // from the synergy system tray icon.
  // if this causes problems for other architectures, then a different
  // work around should be attempted.
  //outputter->show(false);
}

void
CLog::remove(ILogOutputter* outputter)
{
  CArchMutexLock lock(m_mutex);
  m_outputters.remove(outputter);
  m_alwaysOutputters.remove(outputter);
}

void
CLog::pop_front(bool alwaysAtHead)
{
  CArchMutexLock lock(m_mutex);
  COutputterList* list = alwaysAtHead ? &m_alwaysOutputters : &m_outputters;
  if (!list->empty()) {
    delete list->front();
    list->pop_front();
  }
}

bool
CLog::setFilter(const char* maxPriority)
{
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

void
CLog::setFilter(int maxPriority)
{
  CArchMutexLock lock(m_mutex);
  m_maxPriority = maxPriority;
}

int
CLog::getFilter() const
{
  CArchMutexLock lock(m_mutex);
  return m_maxPriority;
}

void
CLog::output(int priority, char* msg) const
{
  assert(priority >= -1 && priority < g_numPriority);
  assert(msg != NULL);

  // insert priority label
  //int n = -g_prioritySuffixLength;
  /*
  if (priority >= 0) {

	  
    n = strlen(g_priority[priority]);
    strcpy(msg + g_maxPriorityLength - n, g_priority[priority]);
    msg[g_maxPriorityLength + 0] = ':';
    msg[g_maxPriorityLength + 1] = ' ';
    msg[g_maxPriorityLength + 1] = ' ';

    
  }
*/
  // find end of message
  //char* end = msg + g_priorityPad + strlen(msg + g_priorityPad);
  int len = (int)strlen(msg);
  char* tmp = new char[len+m_maxNewlineLength+1];
  char* end = tmp + len;
  strcpy(tmp, msg);

  // write to each outputter
  CArchMutexLock lock(m_mutex);
  for (COutputterList::const_iterator index  = m_alwaysOutputters.begin();
                    index != m_alwaysOutputters.end();
                    ++index) {
    // get outputter
    ILogOutputter* outputter = *index;
	
	// put an appropriate newline at the end
	strcpy(end, outputter->getNewline());

    // write message
    outputter->write(static_cast<ILogOutputter::ELevel>(priority),
              tmp /*+ g_maxPriorityLength - n*/);
  }
  for (COutputterList::const_iterator index  = m_outputters.begin();
                    index != m_outputters.end(); ++index) {
    // get outputter
    ILogOutputter* outputter = *index;

    // put an appropriate newline at the end
    strcpy(end, outputter->getNewline());

    // write message and break out of loop if it returns false
    if (!outputter->write(static_cast<ILogOutputter::ELevel>(priority),
              tmp /*+ g_maxPriorityLength - n*/)) {
      break;
    }
  }

  delete[] tmp;
}
