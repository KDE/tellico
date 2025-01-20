/***************************************************************************
    Copyright (C) 2010-2020 Robby Stephenson <robby@periapsis.org>
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

#include "xmlhandler.h"
#include "../tellico_debug.h"

#include <QRegularExpression>
#include <QTextStream>
#include <QXmlStreamReader>
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#include <QTextCodec>
#else
#include <QStringDecoder>
#endif

using Tellico::XMLHandler;

bool XMLHandler::setUtf8XmlEncoding(QString& text_) {
  static const QRegularExpression rx(QLatin1String("encoding\\s*=\\s*\"([\\w-]+)\""));
  QTextStream stream(&text_);
  // the xml header might still indicate an encoding other than utf-8
  // so read the first line and ensure it is set to utf-8
  QString firstLine = stream.readLine();
  QRegularExpressionMatch match = rx.match(firstLine);
  if(match.hasMatch() &&
     match.captured(1).compare(QLatin1String("utf-8"), Qt::CaseInsensitive) != 0) {
    firstLine.replace(rx, QStringLiteral("encoding=\"utf-8\""));
    text_ = firstLine + QLatin1Char('\n') + stream.readAll();
    return true;
  }
  return false;
}

QString XMLHandler::readXMLData(const QByteArray& data_) {
  // need to recognize encoding from the data, like QXmlInputSource::fromRawData() used to do
  QXmlStreamReader reader(data_);
  while(!reader.isStartDocument() && !reader.atEnd()) {
    reader.readNext();
  }
  auto enc = reader.documentEncoding();
  if(enc.isEmpty() || enc.compare(QLatin1String("utf-8"), Qt::CaseInsensitive) == 0) {
    // default to utf8 and no need to parse to change embedded encoding
    return QString::fromUtf8(data_);
  }

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  QTextCodec* codec = QTextCodec::codecForName(enc.toUtf8());
  if(!codec) {
    return QString::fromUtf8(data_);
  }
  QString text = codec->toUnicode(data_);
#else
  const auto encData = enc.toUtf8();
  QStringDecoder decoder(encData);
  if(!decoder.isValid()) {
    return QString::fromUtf8(data_);
  }
  QString text = decoder.decode(data_);
#endif
  // since we always process XML files as utf-8, make sure the embedded encoding is set to utf-8
  if(!setUtf8XmlEncoding(text)) {
    myDebug() << "Found non utf-8 encoding but did not change the embedded declaration" << enc;
    // output up to first 100 characters for debugging purposes
    myDebug() << text.left(100);
  }
  return text;
}
