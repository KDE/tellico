/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_DEBUG_H
#define TELLICO_DEBUG_H

// most of this is borrowed from amarok/src/debug.h
// which is copyright Max Howell <max.howell@methylblue.com>
// amarok is licensed under the GPL

#include <kdebug.h>
// std::clock_t
#include <ctime>

// linux has __GNUC_PREREQ, NetBSD has __GNUC_PREREQ__
#if defined(__GNUC_PREREQ) && !defined(__GNUC_PREREQ__)
#define __GNUC_PREREQ__ __GNUC_PREREQ
#endif

#if !defined(__GNUC_PREREQ__)
#if defined __GNUC__
#define __GNUC_PREREQ__(x, y) \
        ((__GNUC__ == (x) && __GNUC_MINOR__ >= (y)) || \
         (__GNUC__ > (x)))
#else
#define __GNUC_PREREQ__(x, y)   0
#endif
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
#ifndef NDEBUG
#define TELLICO_LOG
#endif

#ifndef NDEBUG
#define TELLICO_DEBUG
#endif

namespace Debug {
  typedef kndbgstream NoDebugStream;
#ifndef TELLICO_DEBUG
  typedef kndbgstream DebugStream;
  static inline DebugStream log()     { return DebugStream(); }
  static inline DebugStream debug()   { return DebugStream(); }
  static inline DebugStream warning() { return DebugStream(); }
  static inline DebugStream error()   { return DebugStream(); }
  static inline DebugStream fatal()   { return DebugStream(); }

#else
  #ifndef DEBUG_PREFIX
    #define FUNC_PREFIX ""
  #else
    #define FUNC_PREFIX "[" DEBUG_PREFIX "] "
  #endif

//from kdebug.h
/*
  enum DebugLevels {
    KDEBUG_INFO  = 0,
    KDEBUG_WARN  = 1,
    KDEBUG_ERROR = 2,
    KDEBUG_FATAL = 3
  };
*/

  typedef kdbgstream DebugStream;
#ifdef TELLICO_LOG
  static inline DebugStream log()   { return kdDebug(); }
#else
  static inline kndbgstream log()   { return NoDebugStream(); }
#endif
  static inline DebugStream debug()   { return kdDebug()   << FUNC_PREFIX; }
  static inline DebugStream warning() { return kdWarning() << FUNC_PREFIX << "[WARNING!] "; }
  static inline DebugStream error()   { return kdError()   << FUNC_PREFIX << "[ERROR!] "; }
  static inline DebugStream fatal()   { return kdFatal()   << FUNC_PREFIX; }

  #undef FUNC_PREFIX
#endif

class Block {

public:
  Block(const char* label) : m_start(std::clock()), m_label(label) {
    Debug::debug() << "BEGIN: " << label << endl;
  }

  ~Block() {
    std::clock_t finish = std::clock();
    const double duration = (double) (finish - m_start) / CLOCKS_PER_SEC;
    Debug::debug() << "  END: " << m_label << " - duration = " << duration << endl;
  }

private :
  std::clock_t m_start;
  const char* m_label;
};

}

#define myDebug() Debug::debug()
#define myWarning() Debug::warning()
#define myLog() Debug::log()

/// Standard function announcer
#define DEBUG_FUNC_INFO myDebug() << k_funcinfo << endl;

/// Announce a line
#define DEBUG_LINE_INFO myDebug() << k_funcinfo << "Line: " << __LINE__ << endl;

/// Convenience macro for making a standard Debug::Block
#define DEBUG_BLOCK Debug::Block uniquelyNamedStackAllocatedStandardBlock( __func__ );

#ifdef TELLICO_LOG
// see http://www.gnome.org/~federico/news-2006-03.html#timeline-tools
#define MARK do { \
    char str[128]; \
    ::snprintf(str, 128, "MARK: %s: %s (%d)", className(), MY_FUNCTION, __LINE__); \
    ::access (str, F_OK); \
  } while(false)
#define MARK_MSG(s) do { \
    char str[128]; \
    ::snprintf(str, 128, "MARK: %s: %s (%d)", className(), s, __LINE__); \
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
