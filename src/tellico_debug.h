/***************************************************************************
    copyright            : (C) 2003-2005 by Robby Stephenson
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
#include <ctime>       //std::clock_t

#ifndef NDEBUG
#define TELLICO_DEBUG
#endif

namespace Debug {
#ifndef TELLICO_DEBUG
  typedef kndbgstream DebugStream;
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
  enum DebugLevels {
    KDEBUG_INFO  = 0,
    KDEBUG_WARN  = 1,
    KDEBUG_ERROR = 2,
    KDEBUG_FATAL = 3
  };

  typedef kdbgstream DebugStream;
  static inline DebugStream debug()   { return kdDebug()   << FUNC_PREFIX; }
  static inline DebugStream warning() { return kdWarning() << FUNC_PREFIX << "[WARNING!] "; }
  static inline DebugStream error()   { return kdError()   << FUNC_PREFIX << "[ERROR!] "; }
  static inline DebugStream fatal()   { return kdFatal()   << FUNC_PREFIX; }

  #undef FUNC_PREFIX
#endif

  typedef kndbgstream NoDebugStream;

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

#define myDebug() Debug::debug()

/// Standard function announcer
#define DEBUG_FUNC_INFO myDebug() << k_funcinfo << endl;

/// Announce a line
#define DEBUG_LINE_INFO myDebug() << k_funcinfo << "Line: " << __LINE__ << endl;

/// Convenience macro for making a standard Debug::Block
#define DEBUG_BLOCK Debug::Block uniquelyNamedStackAllocatedStandardBlock( __PRETTY_FUNCTION__ );

}

#endif
