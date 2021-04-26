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

#include "urlfieldlogic.h"

#include <QDir>

using Tellico::UrlFieldLogic;

UrlFieldLogic::UrlFieldLogic()
  : m_isRelative(false) {
}

void UrlFieldLogic::setRelative(bool relative_) {
  m_isRelative = relative_;
}

void UrlFieldLogic::setBaseUrl(const QUrl& baseUrl_) {
  m_baseUrl = baseUrl_;
}

QString UrlFieldLogic::urlText(const QUrl& url_) const {
  // if it's not relative or if the base URL is not set,
  // then there's nothing to do. Return the URL as-is.
  // Also, if the base URL is not a local file, then ignore it
  if(url_.isEmpty() || !m_isRelative || m_baseUrl.isEmpty() || !m_baseUrl.isLocalFile()) {
    // normalize the url
    return url_.url(QUrl::PrettyDecoded | QUrl::NormalizePathSegments);
  }
  // BUG 410551: use the directory of the base url, not the file itself, in the QDir c'tor
  return QDir(m_baseUrl.adjusted(QUrl::RemoveFilename).path())
             .relativeFilePath(url_.path());
}
