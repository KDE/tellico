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

namespace rtf {

template <class T>
class dbg_iter_mixin : public virtual T
{
 public:
   int offset;
   dbg_iter_mixin(const T& t) : T(t)
   {}
   T& operator=(const T& t)
   {
      return T::operator=(t);
   }
   dbg_iter_mixin& operator++ ()
   {
      ++offset;
      T::operator++();
      return *this;
   }
   dbg_iter_mixin operator++ (int i)
   {
      ++offset;
      return T::operator++(i);
   }
   char operator *() const
   {
      T::value_type c=T::operator*();
//      std::cerr<<offset<<":"<<c<<std::endl;
      return c;
   }
};

template <class T>
class dbg_iter : public dbg_iter_mixin<T>
{
 public:
   dbg_iter(const T& t) : dbg_iter_mixin<T>(t)
   {}
};

template<class T>
class dbg_iter<std::istreambuf_iterator<T> > :
   public virtual std::istreambuf_iterator<T>,
   public dbg_iter_mixin<std::istreambuf_iterator<T> >
{
 public:
   dbg_iter(std::basic_streambuf<T> *buf) : std::istreambuf_iterator<T>(buf)
   {}
};

}
