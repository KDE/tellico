/*  This is RTF to HTML converter, implemented as a text filter, generally.
    Copyright (C) 2003 Valentin Lavrinenko, vlavrinenko@users.sourceforge.net

    available at http://rtf2html.sf.net

    Original available under the terms of the GNU LGPL2, and according
    to those terms, relicensed under the GNU GPL2 for inclusion in Tellico */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef __COMMON_H__
#define __COMMON_H__

#include <string>
#include <sstream>
#include <iomanip>

inline std::string from_int(int value)
{
   std::ostringstream buf;
   buf<<value;
   return buf.str();
}

inline std::string hex(unsigned int value)
{
   std::ostringstream buf;
   buf<<std::setw(2)<<std::setfill('0')<<std::hex<<value;
   return buf.str();
}

#endif
