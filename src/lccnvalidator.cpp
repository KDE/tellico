/***************************************************************************
                             lccnvalidator.cpp
                             -------------------
    begin                : Mon Oct 21 2002
    copyright            : (C) 2002 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "lccnvalidator.h"

LCCNValidator::LCCNValidator(QObject* parent_, const char* name_/*=0*/)
    : QRegExpValidator(parent_, name_) {

   // post-2000 format: 2-letter prefix, followed by a hypen and the four-digit year,
   // then up to 6-digit serial number. Arbitrarily decide that years beyond 2999 are
   // not supported
   setRegExp(QRegExp("(?:\\w{2}-)?2\\d{3}-?\\d{1,6}"));
}

QValidator::State LCCNValidator::validate(QString& input_, int& pos_) const {
  // remember if the cursor is at the end
  bool atEnd = (pos_ == input_.length());
  
  fixup(input_);
  if(atEnd) {
    pos_ = input_.length();
  }
  
  return QRegExpValidator::validate(input_, pos_); 
}

void LCCNValidator::fixup(QString& input_) const {
  input_.replace(QRegExp("-"), "");

  QRegExp prefix("^\\w{2}2");
  if(input_.contains(prefix) > 0) {
    input_.insert(2, '-');
  }

  QRegExp year("^(?:\\w{2}-)?2\\d{3}\\d");
  if(input_.contains(year) > 0) {
    input_.find(year);
    input_.insert(year.matchedLength()-1, '-');
  }
}
