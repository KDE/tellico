/***************************************************************************
    Copyright (C) 2009-2022 Robby Stephenson <robby@periapsis.org>
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
#include "../collection.h"
#include "../entry.h"
#include "../utils/guiproxy.h"
#include "../utils/string_utils.h"
#include "../utils/datafileregistry.h"
#include "../utils/xmlhandler.h"
#include "../utils/tellico_utils.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KIO/StoredTransferJob>
#include <KJobUiDelegate>
#include <KJobWidgets>
#include <KConfigGroup>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QDomDocument>
#include <QUrlQuery>
#include <QThread>

namespace {
  static const int MUSICBRAINZ_MAX_RETURNS_TOTAL = 10;
  static const char* MUSICBRAINZ_API_URL = "https://musicbrainz.org/ws/2/";
}

using namespace Tellico;
using Tellico::Fetch::MusicBrainzFetcher;

MusicBrainzFetcher::MusicBrainzFetcher(QObject* parent_)
    : Fetcher(parent_), m_xsltHandler(nullptr),
      m_limit(MUSICBRAINZ_MAX_RETURNS_TOTAL), m_total(-1), m_offset(0), m_multiDiscTracks(true),
      m_mbidSearch(false), m_job(nullptr), m_started(false) {
}

MusicBrainzFetcher::~MusicBrainzFetcher() {
  delete m_xsltHandler;
  m_xsltHandler = nullptr;
}

QString MusicBrainzFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool MusicBrainzFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Title || k == Person || k == Keyword || k == UPC;
}

bool MusicBrainzFetcher::canFetch(int type) const {
  return type == Data::Collection::Album;
}

void MusicBrainzFetcher::readConfigHook(const KConfigGroup& config_) {
  // user requested option to maintain pre-4.0 behavior of inserting
  // all tracks into a single track field, regardless of disc count
  m_multiDiscTracks = config_.readEntry("Split Tracks By Disc", true);
}

void MusicBrainzFetcher::setLimit(int limit_) {
  m_limit = qBound(1, limit_, MUSICBRAINZ_MAX_RETURNS_TOTAL);
}

void MusicBrainzFetcher::search() {
  m_started = true;
  m_mbidSearch = false;
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
  // all searches are for musical releases since Tellico only tracks albums
  u.setPath(u.path() + QStringLiteral("release"));

  QString queryString;
  switch(request().key()) {
    case Title:
      queryString = QStringLiteral("release:\"%1\"").arg(request().value());
      break;

    case Person:
      queryString = QStringLiteral("artist:\"%1\"").arg(request().value());
      break;

    case UPC:
      queryString = QStringLiteral("barcode:\"%1\"").arg(request().value());
      break;

    case Keyword:
    case Raw:
      queryString = request().value();
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      stop();
      return;
  }
  QUrlQuery q;
  // special case for searching for MBID directly
  if(request().key() == Fetch::Keyword || request().key() == Fetch::Raw) {
    static const QRegularExpression mbrx(QStringLiteral("^\\w{8}-\\w{4}-\\w{4}-\\w{4}-\\w{12}$"));
    if(request().value().startsWith(QLatin1StringView("reid:")) ||
       request().value().startsWith(QLatin1StringView("mbid:"))) {
      u.setPath(u.path() + QLatin1Char('/') + request().value().mid(5));
      q.addQueryItem(QStringLiteral("inc"), QStringLiteral("artists+recordings+release-groups+labels+url-rels"));
      m_mbidSearch = true;
    } else if(request().value().contains(mbrx)) {
      u.setPath(u.path() + QLatin1Char('/') + request().value());
      q.addQueryItem(QStringLiteral("inc"), QStringLiteral("artists+recordings+release-groups+labels+url-rels"));
      m_mbidSearch = true;
    }
  } else {
    q.addQueryItem(QStringLiteral("query"), queryString);
    q.addQueryItem(QStringLiteral("limit"), QString::number(m_limit));
    q.addQueryItem(QStringLiteral("offset"), QString::number(m_offset));
  }
  q.addQueryItem(QStringLiteral("fmt"), QStringLiteral("xml"));
  u.setQuery(q);
//  myDebug() << "url: " << u.url();

  m_requestTimer.start();
  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  // see https://musicbrainz.org/doc/XML_Web_Service/Rate_Limiting#Provide_meaningful_User-Agent_strings
  Tellico::addUserAgent(m_job);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result,
          this, &MusicBrainzFetcher::slotComplete);
}

void MusicBrainzFetcher::stop() {
  if(!m_started) {
    return;
  }
  if(m_job) {
    m_job->kill();
    m_job = nullptr;
  }
  m_started = false;
  Q_EMIT signalDone(this);
}

void MusicBrainzFetcher::slotComplete(KJob* ) {
  if(m_job->error()) {
    m_job->uiDelegate()->showErrorMessage();
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
  m_job = nullptr;

#if 0
  myWarning() << "Remove debug from musicbrainzfetcher.cpp";
  QFile f(QStringLiteral("/tmp/test.xml"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << data;
  }
  f.close();
#endif

  if(m_total == -1) {
    QDomDocument dom;
    if(!dom.setContent(data, QDomDocument::ParseOption::Default)) {
      myWarning() << "server did not return valid XML:" << data;
      stop();
      return;
    }
    // total is /metadata/release-list/@count
    QDomNode n = dom.documentElement().namedItem(QStringLiteral("release-list"));
    QDomElement e = n.toElement();
    if(!e.isNull()) {
      m_total = e.attribute(QStringLiteral("count")).toInt();
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

    FetchResult* r = new FetchResult(this, entry);
    m_entries.insert(r->uid, entry);
    Q_EMIT signalResultFound(r);
    ++count;
  }

  m_offset += count;
  m_hasMoreResults = (m_total > 0 && m_offset <= m_total);

  stop(); // required
}

Tellico::Data::EntryPtr MusicBrainzFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries[uid_];
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }

  const QString mbid = entry->field(QStringLiteral("mbid"));
  if(mbid.isEmpty() || m_mbidSearch) { // no further searching if it was already by MBID
    return entry;
  }

  QUrl u(QString::fromLatin1(MUSICBRAINZ_API_URL));
  u.setPath(u.path() + QStringLiteral("release/") + mbid);
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("fmt"), QStringLiteral("xml"));
  q.addQueryItem(QStringLiteral("inc"), QStringLiteral("artists+recordings+release-groups+labels+url-rels"));
  u.setQuery(q);
//  myDebug() << u;

  // limit to one request per second
  while(m_requestTimer.elapsed() < 1000) {
    QThread::msleep(300);
  }
  m_requestTimer.start();

  KIO::StoredTransferJob* dataJob = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  Tellico::addUserAgent(dataJob);
  if(!dataJob->exec()) {
    myDebug() << "Failed to load" << u;
    return entry;
  }
  const QString output = XMLHandler::readXMLData(dataJob->data());
#if 0
  myWarning() << "Remove output debug from musicbrainzfetcher.cpp";
  QFile f(QStringLiteral("/tmp/test2.xml"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << output;
  }
  f.close();
#endif

  Import::TellicoImporter imp(m_xsltHandler->applyStylesheet(output));
  // be quiet when loading images
  imp.setOptions(imp.options() ^ Import::ImportShowImageErrors);
  Data::CollPtr coll = imp.collection();
  if(!coll || coll->entries().isEmpty()) {
    myWarning() << "no collection pointer or no entries";
    return entry;
  }

  if(coll->entryCount() > 1) {
    myDebug() << "weird, more than one entry found";
  }

  // don't want to include id
  coll->removeField(QStringLiteral("mbid"));

  entry = coll->entries().front();
  // revert to pre-4.0 approach of having all tracks in a single field
  if(!m_multiDiscTracks) {
    const QString trackField(QStringLiteral("track"));
    auto f = coll->fieldByName(trackField);
    f->setTitle(i18n("Tracks")); // revert to default field title
    coll->modifyField(f);
    QString allTracks = entry->field(trackField);
    for(int disc = 2; true; ++disc) {
      const QString t = trackField + QString::number(disc);
      if(!coll->hasField(t)) {
        break;
      }
      allTracks += FieldFormat::rowDelimiterString();
      allTracks += entry->field(t);
      coll->removeField(t);
    }
    entry->setField(trackField, allTracks);
  }

  m_entries.insert(uid_, entry); // replaces old value
  return entry;
}

void MusicBrainzFetcher::initXSLTHandler() {
  QString xsltfile = DataFileRegistry::self()->locate(QStringLiteral("musicbrainz2tellico.xsl"));
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
    m_xsltHandler = nullptr;
    return;
  }
}

Tellico::Fetch::FetchRequest MusicBrainzFetcher::updateRequest(Data::EntryPtr entry_) {
  const QString barcode = entry_->field(QStringLiteral("barcode"));
  if(!barcode.isEmpty()) {
    return FetchRequest(UPC, barcode);
  }

  QString mbid = entry_->field(QStringLiteral("musicbrainz"));
  if(!mbid.isEmpty()) {
    mbid = mbid.section(QLatin1Char('/'), -1);
    return FetchRequest(Raw, QStringLiteral("reid:") + mbid);
  }

  const QString title = entry_->field(QStringLiteral("title"));
  const QString artist = entry_->field(QStringLiteral("artist"));
  if(artist.isEmpty() && !title.isEmpty()) {
    return FetchRequest(Title, title);
  } else if(title.isEmpty() && !artist.isEmpty()) {
    return FetchRequest(Person, artist);
  } else if(!title.isEmpty() && !artist.isEmpty()) {
    return FetchRequest(Raw, QStringLiteral("release:\"%1\" AND artist:\"%2\"").arg(title, artist));
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* MusicBrainzFetcher::configWidget(QWidget* parent_) const {
  return new MusicBrainzFetcher::ConfigWidget(parent_, this);
}

QString MusicBrainzFetcher::defaultName() {
  return QStringLiteral("MusicBrainz"); // no translation
}

QString MusicBrainzFetcher::defaultIcon() {
  return favIcon("https://musicbrainz.org");
}

Tellico::StringHash MusicBrainzFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("barcode")] = i18n("Barcode");
  hash[QStringLiteral("musicbrainz")] = i18n("MusicBrainz Link");
  return hash;
}

MusicBrainzFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const MusicBrainzFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  // now add additional fields widget
  addFieldsWidget(MusicBrainzFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());

  if(fetcher_) {
    m_multiDiscTracks = fetcher_->m_multiDiscTracks;
  } else {
    m_multiDiscTracks = true;
  }
}

void MusicBrainzFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  config_.writeEntry("Split Tracks By Disc", m_multiDiscTracks);
}

QString MusicBrainzFetcher::ConfigWidget::preferredName() const {
  return MusicBrainzFetcher::defaultName();
}
