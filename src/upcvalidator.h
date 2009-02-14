/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef UPCVALIDATOR_H
#define UPCVALIDATOR_H

#include <qvalidator.h>

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class UPCValidator : public QValidator {
Q_OBJECT

public:
  UPCValidator(QObject* parent, const char* name=0);

  virtual QValidator::State validate(QString& input, int& pos) const;
  virtual void fixup(QString& input) const;

  void setCheckISBN(bool b) { m_checkISBN = b; }

signals:
  void signalISBN();

private:
  bool m_checkISBN : 1;
  mutable bool m_isbn : 1;
};

class CueCat {
public:
  static QValidator::State decode(QString& str);
};

} // end namespace
#endif
