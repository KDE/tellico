/***************************************************************************
    Copyright (C) 2008-2009 Robby Stephenson <robby@periapsis.org>
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

#include "discogsfetcher.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../gui/guiproxy.h"
#include "../tellico_utils.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <KConfigGroup>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QBoxLayout>
#include <QDomDocument>
#include <QTextCodec>

//#define DISCOGS_TEST

namespace {
  static const int DISCOGS_MAX_RETURNS_TOTAL = 20;
  static const char* DISCOGS_API_URL = "http://api.discogs.com";
}

using namespace Tellico;
using Tellico::Fetch::DiscogsFetcher;

DiscogsFetcher::DiscogsFetcher(QObject* parent_)
    : XMLFetcher(parent_) {
  setLimit(DISCOGS_MAX_RETURNS_TOTAL);
  setXSLTFilename(QLatin1String("discogs2tellico.xsl"));
}

DiscogsFetcher::~DiscogsFetcher() {
}

QString DiscogsFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool DiscogsFetcher::canFetch(int type) const {
  return type == Data::Collection::Album;
}

void DiscogsFetcher::readConfigHook(const KConfigGroup& config_) {
  Q_UNUSED(config_);
}

void DiscogsFetcher::resetSearch() {
  m_total = -1;
}

KUrl DiscogsFetcher::searchUrl() {
  KUrl u(DISCOGS_API_URL);
  u.addQueryItem(QLatin1String("f"), QLatin1String("xml"));

  switch(request().key) {
    case Title:
      u.setPath(QLatin1String("/search"));
      u.addQueryItem(QLatin1String("q"), QLatin1String("title:") + request().value);
      u.addQueryItem(QLatin1String("type"), QLatin1String("releases"));
      break;

    case Person:
      u.setPath(QString::fromLatin1("/artist/%1").arg(request().value));
      u.addQueryItem(QLatin1String("releases"), QLatin1String("1"));
      break;

    case Keyword:
      u.setPath(QLatin1String("/search"));
      u.addQueryItem(QLatin1String("q"), request().value);
      u.addQueryItem(QLatin1String("type"), QLatin1String("all"));
      break;

    default:
      myWarning() << "key not recognized: " << request().key;
      return KUrl();
  }

#ifdef DISCOGS_TEST
  u = KUrl("/home/robby/discogs-results.xml");
#endif
//  myDebug() << "url: " << u.url();
  return u;
}

void DiscogsFetcher::parseData(QByteArray& data_) {
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

Tellico::Data::EntryPtr DiscogsFetcher::fetchEntryHookData(Data::EntryPtr entry_) {
  Q_ASSERT(entry_);

  QString release = entry_->field(QLatin1String("discogs-id"));
  if(release.isEmpty()) {
    myDebug() << "no discogs release found";
    return entry_;
  }

#ifdef DISCOGS_TEST
  KUrl u("/home/robby/discogs-release.xml");
#else
  KUrl u(DISCOGS_API_URL);
  u.setPath(QString::fromLatin1("/release/%1").arg(release));
  u.addQueryItem(QLatin1String("f"), QLatin1String("xml"));
#endif
//  myDebug() << "url: " << u;

  // quiet
  QString output = FileHandler::readXMLFile(u, true);

#if 0
  myWarning() << "Remove output debug from discogsfetcher.cpp";
  QFile f(QLatin1String("/tmp/test-discogs2.xml"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << output;
  }
  f.close();
#endif

  Import::TellicoImporter imp(xsltHandler()->applyStylesheet(output));
  // be quiet when loading images
  imp.setOptions(imp.options() ^ Import::ImportShowImageErrors);
  Data::CollPtr coll = imp.collection();
//  getTracks(entry);
  if(!coll) {
    myWarning() << "no collection pointer";
    return entry_;
  }

  if(coll->entryCount() > 1) {
    myDebug() << "weird, more than one entry found";
  }

  // don't want to include id
  coll->removeField(QLatin1String("discogs-id"));
  return coll->entries().front();
}

Tellico::Fetch::FetchRequest DiscogsFetcher::updateRequest(Data::EntryPtr entry_) {
  QString title = entry_->field(QLatin1String("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }

  QString artist = entry_->field(QLatin1String("artist"));
  if(!artist.isEmpty()) {
    return FetchRequest(Person, artist);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* DiscogsFetcher::configWidget(QWidget* parent_) const {
  return new DiscogsFetcher::ConfigWidget(parent_, this);
}

QString DiscogsFetcher::defaultName() {
  return i18n("Discogs Audio Search");
}

QString DiscogsFetcher::defaultIcon() {
  return favIcon("http://www.discogs.com");
}

Tellico::StringHash DiscogsFetcher::allOptionalFields() {
  StringHash hash;
  hash[QLatin1String("producer")] = i18n("Producer");
  hash[QLatin1String("nationality")] = i18n("Nationality");
  hash[QLatin1String("discogs")] = i18n("Discogs Link");
  return hash;
}

DiscogsFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const DiscogsFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  // now add additional fields widget
  addFieldsWidget(DiscogsFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

QString DiscogsFetcher::ConfigWidget::preferredName() const {
  return DiscogsFetcher::defaultName();
}

#include "discogsfetcher.moc"
