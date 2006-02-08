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

#include "upcvalidator.h"
#include "isbnvalidator.h"

using Tellico::UPCValidator;

UPCValidator::UPCValidator(QObject* parent_, const char* name_/*=0*/)
    : QValidator(parent_, name_), m_checkISBN(false) {
}

QValidator::State UPCValidator::validate(QString& input_, int& pos_) const {
  // no spaces allowed
  if(input_.contains(' ')) {
    return QValidator::Invalid;
  }

  // no real checking, just if starts with 978, use isbnvalidator
  const uint len = input_.length();
  if(len < 10) {
    m_isbn = false;
  }

  if(!m_checkISBN || (!m_isbn && len < 13)) {
    return QValidator::Intermediate;
  }

  // once it gets converted to an ISBN, remember that, and use it for later
  if(input_.startsWith(QString::fromLatin1("978")) || input_.startsWith(QString::fromLatin1("979"))) {
    ISBNValidator val(0);
    QValidator::State s = val.validate(input_, pos_);
    if(s == QValidator::Acceptable) {
      m_isbn = true;
      // bad hack
      UPCValidator* that = const_cast<UPCValidator*>(this);
      that->signalISBN();
    }
    return s;
  }

  return QValidator::Intermediate;
}

void UPCValidator::fixup(QString& input_) const {
  if(input_.isEmpty()) {
    return;
  }
  input_ = input_.stripWhiteSpace();

  int pos = input_.find(' ');
  if(pos > -1) {
    input_ = input_.left(pos);
  }

  if(!m_checkISBN) {
    return;
  }

  const uint len = input_.length();
  if(len > 12 && (input_.startsWith(QString::fromLatin1("978")) || input_.startsWith(QString::fromLatin1("979")))) {
    QString s = input_;
    ISBNValidator val(0);
    int p = 0;
    int state = val.validate(s, p);
    if(state == QValidator::Acceptable) {
      // bad hack
      UPCValidator* that = const_cast<UPCValidator*>(this);
      that->signalISBN();
      input_ = s;
    }
  }
}

#include "upcvalidator.moc"
