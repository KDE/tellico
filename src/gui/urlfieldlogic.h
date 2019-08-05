/***************************************************************************
    Copyright (C) 2019 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_URLFIELDLOGIC_H
#define TELLICO_URLFIELDLOGIC_H

#include <QUrl>

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class UrlFieldLogic {
public:
  UrlFieldLogic();

  void setRelative(bool relative);
  bool isRelative() const { return m_isRelative; }

  void setBaseUrl(const QUrl& baseUrl);
  QUrl baseUrl() const { return m_baseUrl; }

  QString urlText(const QUrl& url) const;

private:
  bool m_isRelative;
  QUrl m_baseUrl;
};

} // end namespace
#endif
