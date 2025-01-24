/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/Stopwatch.h"
#include "arch/Arch.h"

//
// Stopwatch
//

Stopwatch::Stopwatch(bool triggered) : m_mark(0.0), m_triggered(triggered), m_stopped(triggered)
{
  if (!triggered) {
    m_mark = ARCH->time();
  }
}

Stopwatch::~Stopwatch()
{
  // do nothing
}

double Stopwatch::reset()
{
  if (m_stopped) {
    const double dt = m_mark;
    m_mark = 0.0;
    return dt;
  } else {
    const double t = ARCH->time();
    const double dt = t - m_mark;
    m_mark = t;
    return dt;
  }
}

void Stopwatch::stop()
{
  if (m_stopped) {
    return;
  }

  // save the elapsed time
  m_mark = ARCH->time() - m_mark;
  m_stopped = true;
}

void Stopwatch::start()
{
  m_triggered = false;
  if (!m_stopped) {
    return;
  }

  // set the mark such that it reports the time elapsed at stop()
  m_mark = ARCH->time() - m_mark;
  m_stopped = false;
}

void Stopwatch::setTrigger()
{
  stop();
  m_triggered = true;
}

double Stopwatch::getTime()
{
  if (m_triggered) {
    const double dt = m_mark;
    start();
    return dt;
  } else if (m_stopped) {
    return m_mark;
  } else {
    return ARCH->time() - m_mark;
  }
}

Stopwatch::operator double()
{
  return getTime();
}

bool Stopwatch::isStopped() const
{
  return m_stopped;
}

double Stopwatch::getTime() const
{
  if (m_stopped) {
    return m_mark;
  } else {
    return ARCH->time() - m_mark;
  }
}

Stopwatch::operator double() const
{
  return getTime();
}
