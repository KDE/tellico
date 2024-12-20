/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
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

  virtual QValidator::State validate(QString& input, int& pos) const override;
  virtual void fixup(QString& input) const override;

  void setCheckISBN(bool b) { m_checkISBN = b; }

Q_SIGNALS:
  void signalISBN() const; // clazy:exclude=const-signal-or-slot

private:
  bool m_checkISBN;
  mutable bool m_isbn;
};

class CueCat {
public:
  static QValidator::State decode(QString& str);
};

} // end namespace
#endif
