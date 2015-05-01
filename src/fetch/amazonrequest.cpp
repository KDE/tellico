/***************************************************************************
    Copyright (C) 2007-2009 Robby Stephenson <robby@periapsis.org>
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

#include "amazonrequest.h"
#include "hmac_sha2.h"
#include "../tellico_debug.h"

#include <KCodecs>

#include <QDateTime>

using Tellico::Fetch::AmazonRequest;

AmazonRequest::AmazonRequest(const QUrl& site_, const QByteArray& key_) : m_siteUrl(site_), m_key(key_) {
}

QUrl AmazonRequest::signedRequest(const QMap<QString, QString>& params_) const {
  QMap<QString, QString> allParams = params_;
  allParams.insert(QLatin1String("Timestamp"),
                   QDateTime::currentDateTime().toUTC().toString(QLatin1String("yyyy-MM-dd'T'hh:mm:ss'Z'")));

  QByteArray query;
  // has to be a map so that the query elements are sorted
  QMapIterator<QString, QString> i(allParams);
  while(i.hasNext()) {
    i.next();

    query.append(i.key().toUtf8().toPercentEncoding());
    query.append('=');
    query.append(i.value().toUtf8().toPercentEncoding());
    if(i.hasNext()) {
      query.append('&');
    }
  }

  const QByteArray toSign = "GET\n"
                          + m_siteUrl.host().toUtf8() + '\n'
                          + m_siteUrl.path().toUtf8() + '\n'
                          + query;

  QByteArray hmac_buffer(SHA256_DIGEST_SIZE, '\0');
  hmac_sha256(reinterpret_cast<unsigned char*>(const_cast<char*>(m_key.data())), m_key.length(),
              reinterpret_cast<unsigned char*>(const_cast<char*>(toSign.data())), toSign.length(),
              reinterpret_cast<unsigned char*>(hmac_buffer.data()), hmac_buffer.length());
  const QByteArray sig = KCodecs::base64Encode(hmac_buffer).toPercentEncoding();
//  myDebug() << sig;

  QUrl url = m_siteUrl;
  url.setEncodedQuery(query + "&Signature=" + sig);
  return url;
}
