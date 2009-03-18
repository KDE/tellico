/***************************************************************************
    copyright            : (C) 2005-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_UPCVALIDATOR_H
#define TELLICO_UPCVALIDATOR_H

#include <QValidator>

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class UPCValidator : public QValidator {
Q_OBJECT

public:
  UPCValidator(QObject* parent);

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
