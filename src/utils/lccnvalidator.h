/***************************************************************************
    copyright            : (C) 2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_LCCNVALIDATOR_H
#define TELLICO_LCCNVALIDATOR_H

#include <QValidator>

namespace Tellico {

/**
 * Library of Congress Controll Number validator
 *
 * see http://www.loc.gov/marc/lccn_structure.html
 *
 * These are all valid
 * - 89-456
 * - 2001-1114
 * - gm 71-2450
 */
class LCCNValidator : public QRegExpValidator {
Q_OBJECT

public:
  LCCNValidator(QObject* parent);

  /**
   * Returns the formalized version as dictated by LOC search
   * http://catalog.loc.gov/help/number.htm
   */
  static QString formalize(const QString& value);
};

}
#endif
