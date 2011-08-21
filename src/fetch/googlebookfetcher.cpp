/***************************************************************************
    Copyright (C) 2011 Robby Stephenson <robby@periapsis.org>
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

#include "googlebookfetcher.h"
#include "../tellico_utils.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <KConfigGroup>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QDomDocument>
#include <QTextCodec>

namespace {
  static const int GOOGLEBOOK_MAX_RETURNS_TOTAL = 20;
  static const char* GOOGLEBOOK_API_URL = "http://books.google.com/books/feeds/volumes";
}

using namespace Tellico;
using Tellico::Fetch::GoogleBookFetcher;

GoogleBookFetcher::GoogleBookFetcher(QObject* parent_)
    : XMLFetcher(parent_)
    , m_start(1)
    , m_total(-1) {
  setLimit(GOOGLEBOOK_MAX_RETURNS_TOTAL);
  setXSLTFilename(QLatin1String("googlebook2tellico.xsl"));
}

GoogleBookFetcher::~GoogleBookFetcher() {
}

QString GoogleBookFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool GoogleBookFetcher::canFetch(int type) const {
  return type == Data::Collection::Book;
}

void GoogleBookFetcher::readConfigHook(const KConfigGroup&) {
}

void GoogleBookFetcher::resetSearch() {
  m_total = -1;
}

KUrl GoogleBookFetcher::searchUrl() {
  KUrl u(GOOGLEBOOK_API_URL);
  u.addQueryItem(QLatin1String("start-index"), QString::number(m_start));
  u.addQueryItem(QLatin1String("max-results"), QString::number(limit()));

  switch(request().key) {
    case Keyword:
      u.addQueryItem(QLatin1String("q"), request().value);
      break;

    default:
      myWarning() << "key not recognized: " << request().key;
      return KUrl();
  }

  myDebug() << "url: " << u.url();
  return u;
}

void GoogleBookFetcher::parseData(QByteArray& data_) {
  Q_UNUSED(data_);
#if 0
  if(m_total == -1) {
    QDomDocument dom;
    if(!dom.setContent(data, false)) {
      myWarning() << "server did not return valid XML.";
      return;
    }
    // total is /resp/fetchresults/@numResults
    QDomNode n = dom.documentElement().namedItem(QLatin1String("resp"))
                                      .namedItem(QLatin1String("fetchresults"));
    QDomElement e = n.toElement();
    if(!e.isNull()) {
      m_total = e.attribute(QLatin1String("numResults")).toInt();
      myDebug() << "total = " << m_total;
    }
  }
  m_start = m_entries.count() + 1;
  // not sure how to specify start in the REST url
  //  m_hasMoreResults = m_start <= m_total;
#endif
}

Tellico::Data::EntryPtr GoogleBookFetcher::fetchEntryHookData(Data::EntryPtr entry_) {
  return entry_;
}

Tellico::Fetch::FetchRequest GoogleBookFetcher::updateRequest(Data::EntryPtr entry_) {
  QString title = entry_->field(QLatin1String("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Keyword, title);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* GoogleBookFetcher::configWidget(QWidget* parent_) const {
  return new GoogleBookFetcher::ConfigWidget(parent_, this);
}

QString GoogleBookFetcher::defaultName() {
  return i18n("Google Book Search");
}

QString GoogleBookFetcher::defaultIcon() {
  return favIcon("http://books.google.com");
}

Tellico::StringHash GoogleBookFetcher::allOptionalFields() {
  StringHash hash;
  return hash;
}

GoogleBookFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const GoogleBookFetcher*)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();
}

void GoogleBookFetcher::ConfigWidget::saveConfigHook(KConfigGroup&) {
}

QString GoogleBookFetcher::ConfigWidget::preferredName() const {
  return GoogleBookFetcher::defaultName();
}

#include "googlebookfetcher.moc"
