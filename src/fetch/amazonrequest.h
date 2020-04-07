/***************************************************************************
    Copyright (C) 2007-2020 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_FETCH_AMAZONREQUEST_H
#define TELLICO_FETCH_AMAZONREQUEST_H

#include <QUrl>
#include <QMap>

class AmazonFetcherTest;

namespace Tellico {
  namespace Fetch {

/**
 * @author Robby Stephenson
 */
class AmazonRequest {

friend class ::AmazonFetcherTest;

public:
  enum Operation {
    SearchItems,
    GetItems
  };

  AmazonRequest(const QString& accessKey_, const QString& secretKey_);

  void setHost(const QByteArray& host);
  void setRegion(const QByteArray& region);
  void setOperation(int op);
  QMap<QByteArray, QByteArray> headers(const QByteArray& payload);

private:
  QByteArray prepareCanonicalRequest(const QByteArray& payload) const;
  QByteArray prepareStringToSign(const QByteArray& canonicalUrl) const;
  QByteArray calculateSignature(const QByteArray& stringToSign) const;
  QByteArray buildAuthorizationString(const QByteArray& signature) const;
  QByteArray toHexHash(const QByteArray& data) const;
  QByteArray targetOperation() const;

  QMap<QByteArray, QByteArray> m_headers;
  QByteArray m_accessKey;
  QByteArray m_secretKey;
  QByteArray m_method;
  QByteArray m_service;
  QByteArray m_host;
  QByteArray m_region;
  QByteArray m_path;
  QByteArray m_amzDate;
  Operation m_operation;
  mutable QByteArray m_signedHeaders;
};

  } // end Fetch namespace
} // end Tellico namespace

#endif
