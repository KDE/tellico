/***************************************************************************
                          lccnvalidator.h  -  description
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

#ifndef LCCNVALIDATOR_H
#define LCCNVALIDATOR_H

#include <qvalidator.h>

/**
 * @author Robby Stephenson
 * @version $Id: lccnvalidator.h,v 1.3 2002/11/10 00:38:29 robby Exp $
 */
class LCCNValidator : public QRegExpValidator {
public: 
  LCCNValidator(QObject* parent, const char* name=0);
  
  void fixup(QString& input) const;
  QValidator::State validate(QString& input, int& pos) const;
};

#endif
