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

#include <config.h>
#include "discogsfetcher.h"
#include "../collections/musiccollection.h"
#include "../gui/guiproxy.h"
#include "../core/filehandler.h"
#include "../utils/string_utils.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>
#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <KLineEdit>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QBoxLayout>
#include <QDomDocument>
#include <QTextCodec>

#ifdef HAVE_QJSON
#include <qjson/serializer.h>
#include <qjson/parser.h>
#include <KJobWidgets/KJobWidgets>
#endif

namespace {
  static const int DISCOGS_MAX_RETURNS_TOTAL = 20;
  static const char* DISCOGS_API_URL = "https://api.discogs.com";
}

using namespace Tellico;
using Tellico::Fetch::DiscogsFetcher;

DiscogsFetcher::DiscogsFetcher(QObject* parent_)
    : Fetcher(parent_)
    , m_started(false) {
}

DiscogsFetcher::~DiscogsFetcher() {
}

QString DiscogsFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool DiscogsFetcher::canSearch(FetchKey k) const {
#ifdef HAVE_QJSON
  return k == Title || k == Person || k == Keyword;
#else
  return false;
#endif
}

bool DiscogsFetcher::canFetch(int type) const {
  return type == Data::Collection::Album;
}

void DiscogsFetcher::readConfigHook(const KConfigGroup& config_) {
  QString k = config_.readEntry("API Key");
  if(!k.isEmpty()) {
    m_apiKey = k;
  }
}

void DiscogsFetcher::search() {
#ifdef HAVE_QJSON
  m_started = true;

  if(m_apiKey.isEmpty()) {
    myDebug() << "empty API key";
    message(i18n("Access to data from Discogs.com requires a personal token."),
            MessageHandler::Error);
    stop();
    return;
  }

  QUrl u(QString::fromLatin1(DISCOGS_API_URL));

  switch(request().key) {
    case Title:
      u.setPath(QLatin1String("/database/search"));
      u.addQueryItem(QLatin1String("release_title"), request().value);
      u.addQueryItem(QLatin1String("type"), QLatin1String("release"));
      break;

    case Person:
      u.setPath(QLatin1String("/database/search"));
      u.addQueryItem(QLatin1String("artist"), request().value);
      u.addQueryItem(QLatin1String("type"), QLatin1String("release"));
      break;

    case Keyword:
      u.setPath(QLatin1String("/database/search"));
      u.addQueryItem(QLatin1String("q"), request().value);
      //u.addQueryItem(QLatin1String("type"), QLatin1String("all"));
      break;

    case Raw:
      u.setPath(QLatin1String("/database/search"));
      u.setEncodedQuery(request().value.toUtf8());
      break;

    default:
      myWarning() << "key not recognized:" << request().key;
      stop();
      return;
  }
  u.addQueryItem(QLatin1String("token"), m_apiKey);

//  myDebug() << "url: " << u.url();

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  m_job->addMetaData(QLatin1String("UserAgent"), QString::fromLatin1("Tellico/%1")
                                                                .arg(QLatin1String(TELLICO_VERSION)));
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job, SIGNAL(result(KJob*)), SLOT(slotComplete(KJob*)));
#else
  stop();
#endif
}

void DiscogsFetcher::stop() {
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

Tellico::Data::EntryPtr DiscogsFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }

#ifdef HAVE_QJSON
  QString id = entry->field(QLatin1String("discogs-id"));
  if(!id.isEmpty()) {
    // quiet
    QUrl u(QString::fromLatin1(DISCOGS_API_URL));
    u.setPath(QString::fromLatin1("/releases/%1").arg(id));
    QByteArray data = FileHandler::readDataFile(u, true);

#if 0
    myWarning() << "Remove debug2 from discogsfetcher.cpp";
    QFile f(QString::fromLatin1("/tmp/test2.json"));
    if(f.open(QIODevice::WriteOnly)) {
      QTextStream t(&f);
      t.setCodec("UTF-8");
      t << data;
    }
    f.close();
#endif

    QJson::Parser parser;
    populateEntry(entry, parser.parse(data).toMap(), true);
  }
#endif

  // don't want to include ID field
  entry->setField(QLatin1String("discogs-id"), QString());

  return entry;
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

void DiscogsFetcher::slotComplete(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);
#ifdef HAVE_QJSON
//  myDebug();

  if(job->error()) {
    job->ui()->showErrorMessage();
    stop();
    return;
  }

  QByteArray data = job->data();
  if(data.isEmpty()) {
    myDebug() << "no data";
    stop();
    return;
  }
  // see bug 319662. If fetcher is cancelled, job is killed
  // if the pointer is retained, it gets double-deleted
  m_job = 0;

#if 0
  myWarning() << "Remove debug from discogsfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << data;
  }
  f.close();
#endif

  Data::CollPtr coll(new Data::MusicCollection(true));
  // always add ID for fetchEntryHook
  Data::FieldPtr field(new Data::Field(QLatin1String("discogs-id"), QLatin1String("Discogs ID"), Data::Field::Line));
  field->setCategory(i18n("General"));
  coll->addField(field);

  if(optionalFields().contains(QLatin1String("discogs"))) {
    Data::FieldPtr field(new Data::Field(QLatin1String("discogs"), i18n("Discogs Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }
  if(optionalFields().contains(QLatin1String("nationality"))) {
    Data::FieldPtr field(new Data::Field(QLatin1String("nationality"), i18n("Nationality")));
    field->setCategory(i18n("General"));
    field->setFlags(Data::Field::AllowCompletion | Data::Field::AllowMultiple | Data::Field::AllowGrouped);
    field->setFormatType(FieldFormat::FormatPlain);
    coll->addField(field);
  }
  if(optionalFields().contains(QLatin1String("producer"))) {
    Data::FieldPtr field(new Data::Field(QLatin1String("producer"), i18n("Producer")));
    field->setCategory(i18n("General"));
    field->setFlags(Data::Field::AllowCompletion | Data::Field::AllowMultiple | Data::Field::AllowGrouped);
    field->setFormatType(FieldFormat::FormatName);
    coll->addField(field);
  }

  QJson::Parser parser;
  const QVariantMap resultMap = parser.parse(data).toMap();

  if(value(resultMap, "message").startsWith(QLatin1String("Invalid consumer token"))) {
    message(i18n("The Discogs.com server reports a token error."),
            MessageHandler::Error);
    stop();
    return;
  }

  int count = 0;
  foreach(const QVariant& result, resultMap.value(QLatin1String("results")).toList()) {
    if(count >= DISCOGS_MAX_RETURNS_TOTAL) {
      break;
    }

  //  myDebug() << "found result:" << result;

    Data::EntryPtr entry(new Data::Entry(coll));
    populateEntry(entry, result.toMap(), false);

    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
    ++count;
  }

#endif
  stop();
}

void DiscogsFetcher::populateEntry(Data::EntryPtr entry_, const QVariantMap& resultMap_, bool fullData_) {
  entry_->setField(QLatin1String("discogs-id"), value(resultMap_, "id"));
  entry_->setField(QLatin1String("title"), value(resultMap_, "title"));
  entry_->setField(QLatin1String("year"),  value(resultMap_, "year"));
  entry_->setField(QLatin1String("genre"),  value(resultMap_, "genres"));

  QStringList artists;
  foreach(const QVariant& artist, resultMap_.value(QLatin1String("artists")).toList()) {
    artists << value(artist.toMap(), "name");
  }
  entry_->setField(QLatin1String("artist"), artists.join(FieldFormat::delimiterString()));

  QStringList labels;
  foreach(const QVariant& label, resultMap_.value(QLatin1String("labels")).toList()) {
    labels << value(label.toMap(), "name");
  }
  entry_->setField(QLatin1String("label"), labels.join(FieldFormat::delimiterString()));

  // if we only need cursory data, then we're done
  if(!fullData_) {
    return;
  }

  // check the formats, it could have multiple
  // if there is a CD, prefer that in the track list
  bool hasCD = false;
  foreach(const QVariant& format, resultMap_.value(QLatin1String("formats")).toList()) {
    if(value(format.toMap(), "name") == QLatin1String("CD")) {
      entry_->setField(QLatin1String("medium"), i18n("Compact Disc"));
      hasCD = true;
    } else if(value(format.toMap(), "name") == QLatin1String("Vinyl")) {
      entry_->setField(QLatin1String("medium"), i18n("Vinyl"));
    } else if(value(format.toMap(), "name") == QLatin1String("Cassette")) {
      entry_->setField(QLatin1String("medium"), i18n("Cassette"));
    } else if(!hasCD && value(format.toMap(), "name") == QLatin1String("DVD")) {
      // sometimes a CD and DVD both are included. If we're using the CD, ignore the DVD
      entry_->setField(QLatin1String("medium"), i18n("DVD"));
    }
  }

  QStringList tracks;
  foreach(const QVariant& track, resultMap_.value(QLatin1String("tracklist")).toList()) {
    const QVariantMap trackMap = track.toMap();
    if(value(trackMap, "type_") != QLatin1String("track")) {
      continue;
    }

    // Releases might include a CD and a DVD, for example
    // prefer only the tracks on the CD. Allow positionns of just numbers
    if(hasCD && !(value(trackMap, "position").at(0).isNumber() || 
                  value(trackMap, "position").startsWith(QLatin1String("CD")))) {
      continue;
    }

    QStringList trackInfo;
    trackInfo << value(trackMap, "title");
    if(trackMap.contains(QLatin1String("artists"))) {
      QStringList artists;
      foreach(const QVariant& artist, trackMap.value(QLatin1String("artists")).toList()) {
        artists << value(artist.toMap(), "name");
      }
      trackInfo << artists.join(FieldFormat::delimiterString());
    } else {
      trackInfo << entry_->field(QLatin1String("artist"));
    }
    trackInfo << value(trackMap, "duration");
    tracks << trackInfo.join(FieldFormat::columnDelimiterString());
  }
  entry_->setField(QLatin1String("track"), tracks.join(FieldFormat::rowDelimiterString()));

  if(entry_->collection()->hasField(QLatin1String("discogs"))) {
    entry_->setField(QLatin1String("discogs"), value(resultMap_, "uri"));
  }

  if(entry_->collection()->hasField(QLatin1String("nationality"))) {
    entry_->setField(QLatin1String("nationality"), value(resultMap_, "country"));
  }

  if(entry_->collection()->hasField(QLatin1String("producer"))) {
    QStringList producers;
    foreach(const QVariant& extraartist, resultMap_.value(QLatin1String("extraartists")).toList()) {
      if(value(extraartist.toMap(), "role").contains(QLatin1String("Producer"))) {
        producers << value(extraartist.toMap(), "name");
      }
    }
    entry_->setField(QLatin1String("producer"), producers.join(FieldFormat::delimiterString()));
  }

  entry_->setField(QLatin1String("comments"),  value(resultMap_, "notes"));


  /* cover image authentication with personal token does not work yet
  QUrl coverUrl = value(resultMap_, "thumb");
  if(!coverUrl.isEmpty()) {
    // also need authentication
    coverUrl.addQueryItem(QLatin1String("token"), m_apiKey);
    entry_->setField(QLatin1String("cover"), coverUrl.toString());
  }
  */
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
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;
  QLabel* al = new QLabel(i18n("Registration is required for accessing the %1 data source. "
                               "If you agree to the terms and conditions, <a href='%2'>sign "
                               "up for an account</a>, and enter your information below.",
                                preferredName(),
                                QLatin1String("https://www.discogs.com/developers/#page:authentication")),
                          optionsWidget());
  al->setOpenExternalLinks(true);
  al->setWordWrap(true);
  ++row;
  l->addWidget(al, row, 0, 1, 2);
  // richtext gets weird with size
  al->setMinimumWidth(al->sizeHint().width());

  QLabel* label = new QLabel(i18n("User token: "), optionsWidget());
  l->addWidget(label, ++row, 0);

  m_apiKeyEdit = new KLineEdit(optionsWidget());
  connect(m_apiKeyEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_apiKeyEdit, row, 1);
  label->setBuddy(m_apiKeyEdit);

  l->setRowStretch(++row, 10);

  if(fetcher_) {
    m_apiKeyEdit->setText(fetcher_->m_apiKey);
  }

  // now add additional fields widget
  addFieldsWidget(DiscogsFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

void DiscogsFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  QString apiKey = m_apiKeyEdit->text().trimmed();
  if(!apiKey.isEmpty()) {
    config_.writeEntry("API Key", apiKey);
  }
}

QString DiscogsFetcher::ConfigWidget::preferredName() const {
  return DiscogsFetcher::defaultName();
}

// static
QString DiscogsFetcher::value(const QVariantMap& map, const char* name) {
  const QVariant v = map.value(QLatin1String(name));
  if(v.isNull())  {
    return QString();
  } else if(v.canConvert(QVariant::String)) {
    return v.toString();
  } else if(v.canConvert(QVariant::StringList)) {
    return v.toStringList().join(Tellico::FieldFormat::delimiterString());
  } else {
    return QString();
  }
}

