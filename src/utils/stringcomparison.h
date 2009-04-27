/***************************************************************************
    Copyright (C) 2007-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_STRINGCOMPARISON_H
#define TELLICO_STRINGCOMPARISON_H

#include <QRegExp>

namespace Tellico {

class StringComparison {
public:
  StringComparison();
  virtual ~StringComparison() {}
  virtual int compare(const QString& str1, const QString& str2);
};

class BoolComparison : public StringComparison {
public:
  BoolComparison();
  virtual int compare(const QString& str1, const QString& str2);
};

class TitleComparison : public StringComparison {
public:
  TitleComparison();
  virtual int compare(const QString& str1, const QString& str2);
};

class NumberComparison : public StringComparison {
public:
  NumberComparison();
  virtual int compare(const QString& str1, const QString& str2);
};

class LCCComparison : public StringComparison {
public:
  LCCComparison();
  virtual int compare(const QString& str1, const QString& str2);

private:
  int compareLCC(const QStringList& cap1, const QStringList& cap2) const;
  QRegExp m_regexp;
};

class RatingComparison : public StringComparison {
public:
  RatingComparison();
  virtual int compare(const QString&, const QString&);
};

class ISODateComparison : public StringComparison {
public:
  ISODateComparison();
  virtual int compare(const QString& str1, const QString& str2);
};

}
#endif
