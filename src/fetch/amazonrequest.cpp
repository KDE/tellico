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

#include <kmdcodec.h>

#include <qdatetime.h>

using Tellico::Fetch::AmazonRequest;

AmazonRequest::AmazonRequest(const KURL& site_, const QString& key_) : m_siteUrl(site_), m_key(key_) {
}

KURL AmazonRequest::signedRequest(const QMap<QString, QString>& params_) const {
  QMap<QString, QString> allParams = params_;
  allParams.insert(QString::fromLatin1("Timestamp"),
                   QDateTime::currentDateTime(Qt::UTC).toString(Qt::ISODate));

  QString query;
  // has to be a map so that the query elements are sorted
  QMap<QString, QString>::Iterator it;
  for ( it = allParams.begin(); it != allParams.end(); ++it ) {
    query += KURL::encode_string(it.key());
    query += '=';
    query += KURL::encode_string(it.data());
    query += '&';
  }
  // remove last '&'
  query.truncate(query.length()-1);

  const QCString toSign = "GET\n"
                          + m_siteUrl.host().utf8() + '\n'
                          + m_siteUrl.path().utf8() + '\n'
                          + query.latin1();

  QByteArray hmac_buffer;
  hmac_buffer.fill('\0', SHA256_DIGEST_SIZE);
  // subtract one from string size for toSign, not exactly sure why
  hmac_sha256(reinterpret_cast<unsigned char*>(const_cast<char*>(m_key.latin1())), m_key.length(),
              reinterpret_cast<unsigned char*>(toSign.data()), toSign.size()-1,
              reinterpret_cast<unsigned char*>(hmac_buffer.data()), hmac_buffer.size());
  const QString sig = KURL::encode_string(KCodecs::base64Encode(hmac_buffer));
//  myDebug() << sig << endl;

  KURL url = m_siteUrl;
  url.setQuery(query + "&Signature=" + sig);
  return url;
}
