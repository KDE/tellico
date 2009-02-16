/***************************************************************************
    copyright            : (C) 2009 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_CSVPARSER_H
#define TELLICO_CSVPARSER_H

#include <QString>

namespace Tellico {

class CSVParser {
public:
  CSVParser(QString str);
  ~CSVParser();

  void setDelimiter(const QString& s);
  void reset(QString str);
  bool hasNext() const;
  void skipLine();

  void addToken(const QString& t);
  void setRowDone(bool b);

  QStringList nextTokens();

private:
  class Private;
  Private* const d;
};

} // namespace

#endif
