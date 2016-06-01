/***************************************************************************
    Copyright (C) 2009 Robby Stephenson <robby@periapsis.org>
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

#include "musicbrainzfetcher.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../images/imagefactory.h"
#include "../utils/guiproxy.h"
#include "../utils/string_utils.h"
#include "../collection.h"
#include "../entry.h"
#include "../utils/datafileregistry.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KIO/Job>
#include <KJobUiDelegate>
#include <KJobWidgets/KJobWidgets>
#include <KConfigGroup>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QDomDocument>
#include <QTextCodec>

namespace {
  static const int MUSICBRAINZ_MAX_RETURNS_TOTAL = 10;
  static const char* MUSICBRAINZ_API_URL = "http://musicbrainz.org/ws/1/";
}

using namespace Tellico;
using Tellico::Fetch::MusicBrainzFetcher;

MusicBrainzFetcher::MusicBrainzFetcher(QObject* parent_)
    : Fetcher(parent_), m_xsltHandler(0),
      m_limit(MUSICBRAINZ_MAX_RETURNS_TOTAL), m_total(-1), m_offset(0),
      m_job(0), m_started(false) {
}

MusicBrainzFetcher::~MusicBrainzFetcher() {
  delete m_xsltHandler;
  m_xsltHandler = 0;
}

QString MusicBrainzFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool MusicBrainzFetcher::canFetch(int type) const {
  return type == Data::Collection::Album;
}

void MusicBrainzFetcher::readConfigHook(const KConfigGroup&) {
}

void MusicBrainzFetcher::search() {
  m_started = true;
  m_total = -1;
  m_offset = 0;
  doSearch();
}

void MusicBrainzFetcher::continueSearch() {
  m_started = true;
  doSearch();
}

void MusicBrainzFetcher::doSearch() {
  QUrl u(QString::fromLatin1(MUSICBRAINZ_API_URL));
  u.addQueryItem(QLatin1String("type"), QLatin1String("xml"));
  u.addQueryItem(QLatin1String("limit"), QString::number(m_limit));
  u.addQueryItem(QLatin1String("offset"), QString::number(m_offset));

  QString queryPath;
  switch(request().key) {
    case Title:
      queryPath = QLatin1String("/release/");
      u.addQueryItem(QLatin1String("title"), request().value);
      break;

    case Person:
      queryPath = QLatin1String("/release/");
      u.addQueryItem(QLatin1String("artist"), request().value);
      break;

    case Keyword:
      queryPath = QLatin1String("/release/");
      u.addQueryItem(QLatin1String("query"), QLatin1String("artist:\"") + request().value + QLatin1String("\" OR ") +
                                             QLatin1String("release:\"") + request().value + QLatin1String("\" OR ")  +
                                             QLatin1String("track:\"") + request().value + QLatin1String("\" OR ") +
                                             QLatin1String("label:\"") + request().value + QLatin1String("\""));
      break;

    case Raw:
      queryPath = QLatin1String("/release/");
      u.addQueryItem(QLatin1String("query"), request().value);
      break;

    default:
      myWarning() << "key not recognized: " << request().key;
      stop();
      return;
  }

  u = u.adjusted(QUrl::StripTrailingSlash);
  u.setPath(u.path() + QLatin1Char('/') + queryPath);

//  myDebug() << "url: " << u.url();

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job, SIGNAL(result(KJob*)),
          SLOT(slotComplete(KJob*)));
}

void MusicBrainzFetcher::stop() {
  if(!m_started) {
    return;
  }
  if(m_job) {
    m_job->kill();
    m_job = 0;
  }
  m_started = false;
  emit signalDone(this);
}

void MusicBrainzFetcher::slotComplete(KJob* ) {
  if(m_job->error()) {
    m_job->ui()->showErrorMessage();
    stop();
    return;
  }

  QByteArray data = m_job->data();
  if(data.isEmpty()) {
    myDebug() << "no data";
    stop();
    return;
  }
  // see bug 319662. If fetcher is cancelled, job is killed
  // if the pointer is retained, it gets double-deleted
  m_job = 0;

#if 0
  myWarning() << "Remove debug from musicbrainzfetcher.cpp";
  QFile f(QLatin1String("/tmp/test.xml"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << data;
  }
  f.close();
#endif

  if(m_total == -1) {
    QDomDocument dom;
    if(!dom.setContent(data, false)) {
      myWarning() << "server did not return valid XML.";
      stop();
      return;
    }
    // total is /resp/fetchresults/@numResults
    QDomNode n = dom.documentElement().namedItem(QLatin1String("release-list"));
    QDomElement e = n.toElement();
    if(!e.isNull()) {
      m_total = e.attribute(QLatin1String("count")).toInt();
//      myDebug() << "total = " << m_total;
    }
  }

  if(!m_xsltHandler) {
    initXSLTHandler();
    if(!m_xsltHandler) { // probably an error somewhere in the stylesheet loading
      stop();
      return;
    }
  }

  // assume always utf-8
  QString str = m_xsltHandler->applyStylesheet(QString::fromUtf8(data.constData(), data.size()));
  Import::TellicoImporter imp(str);
  // be quiet when loading images
  imp.setOptions(imp.options() ^ Import::ImportShowImageErrors);
  Data::CollPtr coll = imp.collection();
  if(!coll) {
    myDebug() << "no collection pointer";
    stop();
    return;
  }

  int count = 0;
  Data::EntryList entries = coll->entries();
  foreach(Data::EntryPtr entry, entries) {
    if(count >= m_limit) {
      break;
    }
    if(!m_started) {
      // might get aborted
      break;
    }

    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, Data::EntryPtr(entry));
    emit signalResultFound(r);
    ++count;
  }

  m_offset += count;
  m_hasMoreResults = m_offset <= m_total;

  stop(); // required
}

Tellico::Data::EntryPtr MusicBrainzFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries[uid_];
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }

  QString mbid = entry->field(QLatin1String("mbid"));
  if(mbid.isEmpty()) {
    return entry;
  }

  QUrl u(QString::fromLatin1(MUSICBRAINZ_API_URL));
  u.setPath(u.path() + QLatin1String("/release/") + mbid);
  u.addQueryItem(QLatin1String("type"), QLatin1String("xml"));
  u.addQueryItem(QLatin1String("inc"), QLatin1String("artist tracks release-events release-groups labels tags url-rels"));

  // quiet
  QString output = FileHandler::readXMLFile(u, true);
#if 0
  myWarning() << "Remove output debug from musicbrainzfetcher.cpp";
  QFile f(QLatin1String("/tmp/test2.xml"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << output;
  }
  f.close();
#endif

  Import::TellicoImporter imp(m_xsltHandler->applyStylesheet(output));
  // be quiet when loading images
  imp.setOptions(imp.options() ^ Import::ImportShowImageErrors);
  Data::CollPtr coll = imp.collection();
//  getTracks(entry);
  if(!coll || coll->entries().isEmpty()) {
    myWarning() << "no collection pointer or no entries";
    return entry;
  }

  if(coll->entryCount() > 1) {
    myDebug() << "weird, more than one entry found";
  }

  // don't want to include id
  coll->removeField(QLatin1String("mbid"));

  entry = coll->entries().front();
  m_entries.insert(uid_, entry); // replaces old value
  return entry;
}

void MusicBrainzFetcher::initXSLTHandler() {
  QString xsltfile = DataFileRegistry::self()->locate(QLatin1String("musicbrainz2tellico.xsl"));
  if(xsltfile.isEmpty()) {
    myWarning() << "can not locate musicbrainz2tellico.xsl.";
    return;
  }

  QUrl u = QUrl::fromLocalFile(xsltfile);

  delete m_xsltHandler;
  m_xsltHandler = new XSLTHandler(u);
  if(!m_xsltHandler->isValid()) {
    myWarning() << "error in musicbrainz2tellico.xsl.";
    delete m_xsltHandler;
    m_xsltHandler = 0;
    return;
  }
}

Tellico::Fetch::FetchRequest MusicBrainzFetcher::updateRequest(Data::EntryPtr entry_) {
//  myDebug();

  const QString title = entry_->field(QLatin1String("title"));
  const QString artist = entry_->field(QLatin1String("artist"));
  if(artist.isEmpty() && !title.isEmpty()) {
    return FetchRequest(Title, title);
  } else if(title.isEmpty() && !artist.isEmpty()) {
    return FetchRequest(Person, artist);
  } else if(!title.isEmpty() && !artist.isEmpty()) {
    return FetchRequest(Raw, QLatin1String("release:") + title + QLatin1String(" AND artist:") + artist);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* MusicBrainzFetcher::configWidget(QWidget* parent_) const {
  return new MusicBrainzFetcher::ConfigWidget(parent_, this);
}

QString MusicBrainzFetcher::defaultName() {
  return QLatin1String("MusicBrainz"); // no translation
}

QString MusicBrainzFetcher::defaultIcon() {
  return favIcon("http://www.musicbrainz.org");
}

MusicBrainzFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const MusicBrainzFetcher*)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();
}

void MusicBrainzFetcher::ConfigWidget::saveConfigHook(KConfigGroup&) {
}

QString MusicBrainzFetcher::ConfigWidget::preferredName() const {
  return MusicBrainzFetcher::defaultName();
}

