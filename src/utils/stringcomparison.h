/***************************************************************************
    copyright            : (C) 2007-20089 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
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
