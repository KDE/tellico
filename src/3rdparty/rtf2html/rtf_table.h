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

#ifndef __RTF_H__
#define __RTF_H__

#include "common.h"
#include <vector>
#include <cmath>
#include <list>
#include <cstdlib>

namespace rtf {

struct table_cell
{
   int Rowspan;
   std::string Text;
   table_cell() : Rowspan(0) {}
};

struct table_cell_def
{
   enum valign {valign_top, valign_bottom, valign_center};
   bool BorderTop, BorderBottom, BorderLeft, BorderRight;
   bool *ActiveBorder;
   int Right, Left;
   bool Merged, FirstMerged;
   valign VAlign;
   table_cell_def()
   {
      BorderTop=BorderBottom=BorderLeft=BorderRight=Merged=FirstMerged=false;
      ActiveBorder=NULL;
      Right=Left=0;
      VAlign=valign_top;
   }
   bool right_equals(int x) { return x==Right; }
   bool left_equals(int x) { return x==Left; }
};

template <class T>
class killing_ptr_vector : public std::vector<T*>
{
 public:
   ~killing_ptr_vector()
   {
      for (typename killing_ptr_vector<T>::iterator i=this->begin(); i!=this->end(); ++i)
         delete *i;
   }
};

typedef killing_ptr_vector<table_cell> table_cells;
typedef killing_ptr_vector<table_cell_def> table_cell_defs;

typedef std::list<table_cell_defs> table_cell_defs_list;

struct table_row
{
   table_cells Cells;
   table_cell_defs_list::iterator CellDefs;
   int Height;
   int Left;
   table_row() : Height(-1000),  Left(-1000) {}
};

class table : public killing_ptr_vector<table_row>
{
 private:
   typedef killing_ptr_vector<table_row> base_class;
 public:
   table() : base_class() {}
   std::string make();
};

}

#endif
