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

#include "amazonrequest.h"
#include "../tellico_debug.h"

#include <QDateTime>
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>

namespace {
  static const char* AMAZON_REQUEST = "aws4_request";
  static const char* AMAZON_REQUEST_ALGORITHM = "AWS4-HMAC-SHA256";
  static const char* AMAZON_REQUEST_NAMESPACE = "com.amazon.paapi5";
  static const char* AMAZON_REQUEST_VERSION = "v1";
  static const char* AMAZON_REQUEST_SERVICE = "ProductAdvertisingAPI";
}

using Tellico::Fetch::AmazonRequest;

AmazonRequest::AmazonRequest(const QString& accessKey_, const QString& secretKey_)
    : m_accessKey(accessKey_.toUtf8())
    , m_secretKey(secretKey_.toUtf8())
    , m_method("POST")
    , m_service(AMAZON_REQUEST_SERVICE)
    , m_operation(SearchItems) {
  m_amzDate = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd'T'hhmmss'Z'")).toUtf8();
}

void AmazonRequest::setHost(const QByteArray& host_) {
  m_host = host_;
}

void AmazonRequest::setPath(const QByteArray& path_) {
  m_path = path_;
}

void AmazonRequest::setRegion(const QByteArray& region_) {
  m_region = region_;
}

void AmazonRequest::setOperation(int op_) {
  m_operation = static_cast<Operation>(op_);
}

QMap<QByteArray, QByteArray> AmazonRequest::headers(const QByteArray& payload_) {
  m_headers.insert("content-encoding", "amz-1.0");
  m_headers.insert("content-type", "application/json; charset=utf-8");
  m_headers.insert("host", m_host);
  m_headers.insert("x-amz-date", m_amzDate);
  m_headers.insert("x-amz-target", targetOperation());

  const QByteArray canonicalURL = prepareCanonicalRequest(payload_);
  const QByteArray stringToSign = prepareStringToSign(canonicalURL);
  const QByteArray signature = calculateSignature(stringToSign);
  m_headers.insert("Authorization", buildAuthorizationString(signature));
  return m_headers;
}

QByteArray AmazonRequest::prepareCanonicalRequest(const QByteArray& payload_) const {
  Q_ASSERT(!m_method.isEmpty());
  Q_ASSERT(!m_path.isEmpty());
  Q_ASSERT(!m_headers.isEmpty());
  QByteArray req = m_method + '\n'
                 + m_path + '\n' + '\n';

  m_signedHeaders.clear();
  // a map so that the query elements are sorted
  QMapIterator<QByteArray, QByteArray> i(m_headers);
  while(i.hasNext()) {
    i.next();

    m_signedHeaders.append(i.key());
    m_signedHeaders.append(';');

    req.append(i.key());
    req.append(':');
    req.append(i.value());
    req.append('\n');
  }
  req.append('\n');

  m_signedHeaders.chop(1); // remove final ';'
  req.append(m_signedHeaders);
  req.append('\n');

  req.append(toHexHash(payload_));
  return req;
}

QByteArray AmazonRequest::prepareStringToSign(const QByteArray& canonicalUrl_) const {
  QByteArray stringToSign(AMAZON_REQUEST_ALGORITHM);
  stringToSign += '\n';
  stringToSign += m_amzDate + '\n';
  stringToSign += m_amzDate.left(8) + '/' + m_region + '/' + m_service + '/' + AMAZON_REQUEST + '\n';
  stringToSign += toHexHash(canonicalUrl_);
  return stringToSign;
}

QByteArray AmazonRequest::calculateSignature(const QByteArray& stringToSign_) const {
  QByteArray signatureKey;
  signatureKey = QMessageAuthenticationCode::hash(m_amzDate.left(8), QByteArray("AWS4" + m_secretKey), QCryptographicHash::Sha256);
  signatureKey = QMessageAuthenticationCode::hash(m_region, signatureKey, QCryptographicHash::Sha256);
  signatureKey = QMessageAuthenticationCode::hash(m_service, signatureKey, QCryptographicHash::Sha256);
  signatureKey = QMessageAuthenticationCode::hash(AMAZON_REQUEST, signatureKey, QCryptographicHash::Sha256);
  // '0' says no separators between hex encoded characters
  return QMessageAuthenticationCode::hash(stringToSign_, signatureKey, QCryptographicHash::Sha256).toHex(0);
}

QByteArray AmazonRequest::buildAuthorizationString(const QByteArray& signature_) const {
  Q_ASSERT(!m_accessKey.isEmpty());
  Q_ASSERT(!m_region.isEmpty());
  Q_ASSERT(!m_service.isEmpty());
  Q_ASSERT(!m_signedHeaders.isEmpty());
  QByteArray authString(AMAZON_REQUEST_ALGORITHM);
  authString += ' ';
  authString += "Credential=" + m_accessKey;
  authString += '/' + m_amzDate.left(8) + '/' + m_region + '/' + m_service + "/" + AMAZON_REQUEST + ", ";
  authString += "SignedHeaders=" + m_signedHeaders + ", " + "Signature=" + signature_;
  return authString;
}

QByteArray AmazonRequest::toHexHash(const QByteArray& data_) const {
  const QByteArray hash = QCryptographicHash::hash(data_, QCryptographicHash::Sha256);
  return hash.toHex(0); // '0' says no separators between hex encoded characters
}

QByteArray AmazonRequest::targetOperation() const {
  QByteArray target(AMAZON_REQUEST_NAMESPACE);
  target.append('.');
  target.append(AMAZON_REQUEST_VERSION);
  target.append('.');
  target.append(m_service);
  target.append(AMAZON_REQUEST_VERSION);
  target.append('.');
  switch(m_operation) {
    case SearchItems:
      target.append("SearchItems");
      break;
    case GetItems:
      target.append("GetItems");
      break;
  }
  return target;
}
