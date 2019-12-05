/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_DEBUG_H
#define TELLICO_DEBUG_H

// some of this was borrowed from amarok/src/debug.h
// which is copyright Max Howell <max.howell@methylblue.com>
// amarok is licensed under the GPL

#include <QDebug>
// std::clock_t
#include <ctime>
#include <unistd.h>

// linux has __GNUC_PREREQ, NetBSD has __GNUC_PREREQ__
#if defined(__GNUC_PREREQ) && !defined(__GNUC_PREREQ__)
#  define __GNUC_PREREQ__ __GNUC_PREREQ
#endif

#if !defined(__GNUC_PREREQ__)
#  if defined __GNUC__
#    define __GNUC_PREREQ__(x, y) \
            ((__GNUC__ == (x) && __GNUC_MINOR__ >= (y)) || \
             (__GNUC__ > (x)))
#  else
#    define __GNUC_PREREQ__(x, y)   0
#  endif
#endif

# if defined __cplusplus ? __GNUC_PREREQ__ (2, 6) : __GNUC_PREREQ__ (2, 4)
#   define MY_FUNCTION  __PRETTY_FUNCTION__
# else
#  if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
#   define MY_FUNCTION  __func__
#  else
#   define MY_FUNCTION  __FILE__ ":" __LINE__
#  endif
# endif

// some logging
#if !defined(KDE_NO_DEBUG_OUTPUT)
#define TELLICO_LOG
#endif

#ifndef DEBUG_PREFIX
  #define FUNC_PREFIX ""
#else
  #define FUNC_PREFIX "[" DEBUG_PREFIX "] "
#endif

#define myDebug()   qDebug()
#define myWarning() qWarning()
#ifdef TELLICO_LOG
#define myLog()     qDebug()
#else
#define myLog()     //qDebug()
#endif

namespace Debug {

class Block {

public:
  Block(const char* label) : m_start(std::clock()), m_label(label) {
    QDebug(QtDebugMsg) << "BEGIN:" << label;
  }

  ~Block() {
    std::clock_t finish = std::clock();
    const double duration = (double) (finish - m_start) / CLOCKS_PER_SEC;
    QDebug(QtDebugMsg) << "  END:" << m_label << "- duration =" << duration;
  }

private :
  std::clock_t m_start;
  const char* m_label;
};

}

/// Standard function announcer
#define DEBUG_FUNC myDebug() << Q_FUNC_INFO;

/// Announce a line
#define DEBUG_LINE myDebug() << "[" << __FILE__ << ":" << __LINE__ << "]";

/// Convenience macro for making a standard Debug::Block
#ifndef WIN32
#define DEBUG_BLOCK Debug::Block uniquelyNamedStackAllocatedStandardBlock( MY_FUNCTION );
#else
#define DEBUG_BLOCK
#endif

#if defined(TELLICO_LOG) && !defined(WIN32)
// see https://www.gnome.org/~federico/news-2006-03.html#timeline-tools
// strace -ttt -f -o /tmp/logfile.strace src/tellico
// plot-timeline.py -o prettygraph.png /tmp/logfile.strace
#define MARK do { \
    char str[128]; \
    ::snprintf(str, 128, "MARK: %s: %s (%d)", metaObject()->className(), MY_FUNCTION, __LINE__); \
    ::access (str, F_OK); \
  } while(false)
#define MARK_MSG(s) do { \
    char str[128]; \
    ::snprintf(str, 128, "MARK: %s: %s (%d)", metaObject()->className(), s, __LINE__); \
    ::access (str, F_OK); \
  } while(false)
#define MARK_LINE do { \
    char str[128]; \
    ::snprintf(str, 128, "MARK: tellico: %s (%d)", __FILE__, __LINE__); \
    ::access (str, F_OK); \
  } while(false)
#else
#define MARK
#define MARK_MSG(s)
#define MARK_LINE
#endif

#endif
