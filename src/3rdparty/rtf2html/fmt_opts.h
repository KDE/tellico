/*  This is RTF to HTML converter, implemented as a text filter, generally.
    Copyright (C) 2003 Valentin Lavrinenko, vlavrinenko@users.sourceforge.net

    available at http://rtf2html.sf.net

    Original available under the terms of the GNU LGPL2, and according
    to those terms, relicensed under the GNU GPL2 for inclusion in Tellico */

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

#ifndef __FMT_OPTS_H__
#define __FMT_OPTS_H__

#include "common.h"
#include <stack>
#include <vector>
#include <deque>
#include <map>

namespace rtf {

struct color {
   int r, g, b;
   color() : r(-1), g(-1), b(-1) {}
   bool operator==(const color &clr)
   {
      return r==clr.r && g==clr.g && b==clr.b;
   }
   bool operator!=(const color &clr)
   {
      return !(*this==clr);
   }
};

typedef std::vector<color> colorvect;

struct font {
   enum font_family {ff_none, ff_serif, ff_sans_serif, ff_cursive,
                     ff_fantasy, ff_monospace};
   font_family family;
   std::string name;
   int pitch;
   int charset;
   font() : family(ff_none), name(), pitch(0), charset(0) {}
   bool operator==(const font &f)
   {
      return family==f.family && name==f.name;
   }
   bool operator!=(const font &f)
   {
      return !(*this==f);
   }
};

typedef std::map<int, font> fontmap;

struct formatting_options
{
   enum halign {align_left, align_right, align_center, align_justify, align_error};
   enum valign {va_normal, va_sub, va_sup};
   bool chpBold, chpItalic, chpUnderline;
   valign chpVAlign;
   int chpFontSize, chpHighlight;
   color chpFColor, chpBColor;
   font chpFont;
   int papLeft, papRight, papFirst;
   int papBefore, papAfter;
   halign papAlign;
   bool papInTbl;
   formatting_options()
   {
      chpBold=chpItalic=chpUnderline=false;
      chpVAlign=va_normal;
      chpFontSize=chpHighlight=0;
      papLeft=papRight=papFirst=papBefore=papAfter=0;
      papAlign=align_left;
      papInTbl=false;
   }
   bool operator==(const formatting_options &opt) // tests only for character options
   {
      return chpBold==opt.chpBold && chpItalic==opt.chpItalic
             && chpUnderline==opt.chpUnderline && chpVAlign==opt.chpVAlign
             && chpFontSize==opt.chpFontSize
             && chpFColor==opt.chpFColor && chpBColor==opt.chpBColor
             && chpHighlight==opt.chpHighlight && chpFont==opt.chpFont;
   }
   bool operator!=(const formatting_options &opt) // tests only for character options
   {
      return !(*this==opt);
   }
   std::string get_par_str() const;
};

typedef std::stack<formatting_options> fo_stack;

typedef std::deque<formatting_options> fo_deque;

class formatter {
 private:
   fo_deque opt_stack;
 public:
   std::string format(const formatting_options &opt);
   std::string close();
   void clear() { opt_stack.clear(); }
};

class html_text {
 private:
   const formatting_options &opt;
   formatter fmt;
   std::string text;
 public:
   html_text(const formatting_options &_opt) : opt(_opt) {}
   const std::string &str() { return text; }
   template <class T> void write(T s)
   {
      text+=fmt.format(opt)+s;
   }
   std::string close() { return fmt.close(); }
//   void write(char c) { write(std::string()+c); }
   void clear() { text.clear(); fmt.clear(); }
};

}
#endif
