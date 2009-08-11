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

#ifdef QT_NO_CAST_FROM_ASCII
#define WAS_NO_ASCII
#undef QT_NO_CAST_FROM_ASCII
#endif

#include "amazonrequest.h"
#include "../tellico_debug.h"

#include <config.h>
#ifdef HAVE_QCA2
#include <QtCrypto>
#endif

#ifdef WAS_NO_ASCII
#define QT_NO_CAST_FROM_ASCII
#undef WAS_NO_ASCII
#endif

#include <KCodecs>

using Tellico::Fetch::AmazonRequest;

QCA::Initializer* AmazonRequest::s_initializer = 0;

AmazonRequest::AmazonRequest(const KUrl& site_, const QByteArray& key_) : m_siteUrl(site_), m_key(key_) {
#ifdef HAVE_QCA2
  if(!s_initializer) {
    s_initializer = new QCA::Initializer();
  }
#endif
}

KUrl AmazonRequest::signedRequest(const QMap<QString, QString>& params_) const {
#ifdef HAVE_QCA2
  if(!QCA::isSupported("sha256")) {
    myWarning() << "SHA256 not supported!";
    return KUrl();
  }

  QMap<QString, QString> allParams = params_;
  allParams.insert("Timestamp", QDateTime::currentDateTime().toUTC().toString("yyyy-MM-dd'T'hh:mm:ss'Z'"));

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

  QCA::MessageAuthenticationCode hmac(QLatin1String("hmac(sha256)"), m_key);
  hmac.update(toSign);
  const QByteArray sig = KCodecs::base64Encode(hmac.final().toByteArray()).toPercentEncoding();

  KUrl url = m_siteUrl;
  url.setEncodedQuery(query + "&Signature=" + sig);
  return url;
#else
  Q_UNUSED(params_);
  myWarning() << "SHA256 signing is not available";
  return KUrl();
#endif
}
