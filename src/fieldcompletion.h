/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICOFIELDCOMPLETION_H
#define TELLICOFIELDCOMPLETION_H

#include <KCompletion>

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class FieldCompletion : public KCompletion {
Q_OBJECT

public:
  FieldCompletion(bool multiple);

  void setMultiple(bool m) { m_multiple = m; }
  virtual QString makeCompletion(const QString& string) Q_DECL_OVERRIDE;
  virtual void clear() Q_DECL_OVERRIDE;

protected:
  virtual void postProcessMatch(QString* match) const Q_DECL_OVERRIDE;
  virtual void postProcessMatches(QStringList* matches) const Q_DECL_OVERRIDE;
  virtual void postProcessMatches(KCompletionMatches* matches) const Q_DECL_OVERRIDE;

private:
  bool m_multiple;
  QString m_beginText;
};

} // end namespace
#endif
