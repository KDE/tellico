/***************************************************************************
    Copyright (C) 2006-2009 Robby Stephenson <robby@periapsis.org>
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

#include "dcimporter.h"
#include "../collections/bookcollection.h"
#include "tellico_xml.h"
#include "../tellico_debug.h"

using Tellico::Import::DCImporter;

DCImporter::DCImporter(const KUrl& url_) : XMLImporter(url_) {
}

DCImporter::DCImporter(const QString& text_) : XMLImporter(text_) {
}

DCImporter::DCImporter(const QDomDocument& dom_) : XMLImporter(dom_) {
}

Tellico::Data::CollPtr DCImporter::collection() {
  const QString& dc = XML::nsDublinCore;
  const QString& zing = XML::nsZing;

  Data::CollPtr c(new Data::BookCollection(true));

  QDomDocument doc = domDocument();

  QRegExp authorDateRX(QLatin1String(",?(\\s+\\d{4}-?(?:\\d{4})?\\.?)(.*)$"));
  QRegExp dateRX(QLatin1String("\\d{4}"));

  QDomNodeList recordList = doc.elementsByTagNameNS(zing, QLatin1String("recordData"));
  myDebug() << "number of records: " << recordList.count();

  enum { UnknownNS, UseNS, NoNS } useNS = UnknownNS;

#define GETELEMENTS(s) (useNS == NoNS) \
                         ? elem.elementsByTagName(QLatin1String(s)) \
                         : elem.elementsByTagNameNS(dc, QLatin1String(s))

  for(int i = 0; i < recordList.count(); ++i) {
    Data::EntryPtr e(new Data::Entry(c));

    QDomElement elem = recordList.item(i).toElement();

    QDomNodeList nodeList = GETELEMENTS("title");
    if(nodeList.count() == 0) { // no title, skip
      if(useNS == UnknownNS) {
        nodeList = elem.elementsByTagName(QLatin1String("title"));
        if(nodeList.count() > 0) {
          useNS = NoNS;
        } else {
          myDebug() << "no title, skipping";
          continue;
        }
      } else {
        myDebug() << "no title, skipping";
        continue;
      }
    } else if(useNS == UnknownNS) {
      useNS = UseNS;
    }
    QString s = nodeList.item(0).toElement().text();
    s.replace(QLatin1Char('\n'), QLatin1Char(' '));
    s = s.simplified();
    e->setField(QLatin1String("title"), s);

    nodeList = GETELEMENTS("creator");
    QStringList creators;
    for(int j = 0; j < nodeList.count(); ++j) {
      QString s = nodeList.item(j).toElement().text();
      if(authorDateRX.indexIn(s) > -1) {
      // check if anything after date like [publisher]
        if(authorDateRX.cap(2).trimmed().isEmpty()) {
          s.remove(authorDateRX);
          s = s.simplified();
          creators << s;
        } else {
          myDebug() << "weird creator, skipping: " << s;
        }
      } else {
        creators << s;
      }
    }
    e->setField(QLatin1String("author"), creators.join(QLatin1String("; ")));

    nodeList = GETELEMENTS("publisher");
    QStringList publishers;
    for(int j = 0; j < nodeList.count(); ++j) {
      publishers << nodeList.item(j).toElement().text();
    }
    e->setField(QLatin1String("publisher"), publishers.join(QLatin1String("; ")));

    nodeList = GETELEMENTS("subject");
    QStringList keywords;
    for(int j = 0; j < nodeList.count(); ++j) {
      keywords << nodeList.item(j).toElement().text();
    }
    e->setField(QLatin1String("keyword"), keywords.join(QLatin1String("; ")));

    nodeList = GETELEMENTS("date");
    if(nodeList.count() > 0) {
      QString s = nodeList.item(0).toElement().text();
      if(dateRX.indexIn(s) > -1) {
        e->setField(QLatin1String("pub_year"), dateRX.cap());
      }
    }

    nodeList = GETELEMENTS("description");
    if(nodeList.count() > 0) { // no title, skip
      e->setField(QLatin1String("comments"), nodeList.item(0).toElement().text());
    }

    c->addEntries(e);
  }
#undef GETELEMENTS

  return c;
}
