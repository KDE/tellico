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

#ifndef __RTF_TOOLS_H__
#define __RTF_TOOLS_H__

#include "common.h"
#include "rtf_keyword.h"

namespace rtf {

template <class InputIter>
void skip_group(InputIter &iter);


/****************************************
function assumes that file pointer points AFTER the opening brace
and that the group is really closed. cs is caller's curchar.
Returns the character that comes after the enclosing brace.
*****************************************/

template <class InputIter>
void skip_group(InputIter &iter)
{
   int cnt=1;
   while (cnt)
   {
      switch (*iter++)
      {
      case '{':
         cnt++;
         break;
      case '}':
         cnt--;
         break;
      case '\\':
      {
         rtf_keyword kw(iter);
         if (!kw.is_control_char() && kw.keyword()==rtf_keyword::rkw_bin
             && kw.parameter()>0)
         {
            std::advance(iter, kw.parameter());
         }
         break;
      }
      }
   }
}

}

#endif
