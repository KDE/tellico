/***************************************************************************
    Copyright (C) 2008-2022 Robby Stephenson <robby@periapsis.org>
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

#include <config.h> // for TELLICO_VERSION

#include "discogsfetcher.h"
#include "../collections/musiccollection.h"
#include "../images/imagefactory.h"
#include "../utils/guiproxy.h"
#include "../utils/mapvalue.h"
#include "../core/filehandler.h"
#include "../gui/combobox.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>
#include <KIO/StoredTransferJob>
#include <KJobUiDelegate>
#include <KJobWidgets>

#include <QLineEdit>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>
#include <QThread>

namespace {
  static const int DISCOGS_MAX_RETURNS_TOTAL = 20;
  static const char* DISCOGS_API_URL = "https://api.discogs.com";
}

using namespace Tellico;
using Tellico::Fetch::DiscogsFetcher;

DiscogsFetcher::DiscogsFetcher(QObject* parent_)
    : Fetcher(parent_)
    , m_limit(DISCOGS_MAX_RETURNS_TOTAL)
    , m_started(false)
    , m_imageSize(LargeImage)
    , m_page(1)
    , m_multiDiscTracks(true) {
}

DiscogsFetcher::~DiscogsFetcher() {
}

QString DiscogsFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool DiscogsFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Title || k == Person || k == Keyword || k == UPC;
}

bool DiscogsFetcher::canFetch(int type) const {
  return type == Data::Collection::Album;
}

void DiscogsFetcher::readConfigHook(const KConfigGroup& config_) {
  QString k = config_.readEntry("API Key");
  if(!k.isEmpty()) {
    m_apiKey = k;
  }
  const int imageSize = config_.readEntry("Image Size", -1);
  if(imageSize > -1) {
    m_imageSize = static_cast<ImageSize>(imageSize);
  }
  // user requested option to maintain pre-4.0 behavior of inserting
  // all tracks into a single track field, regardless of disc count
  m_multiDiscTracks = config_.readEntry("Split Tracks By Disc", true);
}

void DiscogsFetcher::setLimit(int limit_) {
  m_limit = qBound(1, limit_, DISCOGS_MAX_RETURNS_TOTAL);
}

void DiscogsFetcher::search() {
  m_page = 1;
  continueSearch();
}

void DiscogsFetcher::continueSearch() {
  m_started = true;

  QUrl u(QString::fromLatin1(DISCOGS_API_URL));
  u.setPath(QStringLiteral("/database/search"));

  QUrlQuery q;
  switch(request().key()) {
    case Title:
      q.addQueryItem(QStringLiteral("release_title"), request().value());
      q.addQueryItem(QStringLiteral("type"), QStringLiteral("release"));
      break;

    case Person:
      q.addQueryItem(QStringLiteral("artist"), request().value());
      q.addQueryItem(QStringLiteral("type"), QStringLiteral("release"));
      break;

    case Keyword:
      q.addQueryItem(QStringLiteral("q"), request().value());
      q.addQueryItem(QStringLiteral("type"), QStringLiteral("release"));
      break;

    case UPC:
      q.addQueryItem(QStringLiteral("barcode"), request().value());
      break;

    case Raw:
      q.setQuery(request().value());
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      stop();
      return;
  }

  if(m_apiKey.isEmpty()) {
    myDebug() << source() << "- empty API key";
    message(i18n("An access key is required to use this data source.")
            + QLatin1Char(' ') +
            i18n("Those values must be entered in the data source settings."), MessageHandler::Error);
    stop();
    return;
  }

  q.addQueryItem(QStringLiteral("page"), QString::number(m_page));
  q.addQueryItem(QStringLiteral("per_page"), QString::number(m_limit));
  q.addQueryItem(QStringLiteral("token"), m_apiKey);
  u.setQuery(q);

//  myDebug() << "url: " << u.url();

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  m_job->addMetaData(QLatin1String("SendUserAgent"), QLatin1String("true"));
  m_job->addMetaData(QStringLiteral("UserAgent"),
                     QStringLiteral("Tellico/%1").arg(QStringLiteral(TELLICO_VERSION)));
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result, this, &DiscogsFetcher::slotComplete);
}

void DiscogsFetcher::stop() {
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

Tellico::Data::EntryPtr DiscogsFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }

  QString id = entry->field(QStringLiteral("discogs-id"));
  if(!id.isEmpty()) {
    // quiet
    QUrl u(QString::fromLatin1(DISCOGS_API_URL));
    u.setPath(QStringLiteral("/releases/%1").arg(id));
    QByteArray data = FileHandler::readDataFile(u, true);

#if 0
    myWarning() << "Remove data debug from discogsfetcher.cpp";
    QFile f(QString::fromLatin1("/tmp/test-discogs-data.json"));
    if(f.open(QIODevice::WriteOnly)) {
      QTextStream t(&f);
      t << data;
    }
    f.close();
#endif

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    const QVariantMap resultMap = doc.object().toVariantMap();
    if(resultMap.contains(QStringLiteral("message")) && mapValue(resultMap, "id").isEmpty()) {
      const auto& msg = mapValue(resultMap, "message");
      message(msg, MessageHandler::Error);
      myLog() << "DiscogsFetcher -" << msg;
      if(msg.startsWith(QLatin1String("You are making requests too quickly"))) {
        QThread::msleep(2000);
      }
    } else if(error.error == QJsonParseError::NoError) {
      populateEntry(entry, resultMap, true);
    } else {
      myDebug() << "Bad JSON results";
    }
  }

  const QString image_id = entry->field(QStringLiteral("cover"));
  // if it's still a url, we need to load it
  if(image_id.contains(QLatin1Char('/'))) {
    const QString id = ImageFactory::addImage(QUrl::fromUserInput(image_id), true /* quiet */);
    if(id.isEmpty()) {
      myDebug() << "empty id for" << image_id;
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    }
    // empty image ID is ok
    entry->setField(QStringLiteral("cover"), id);
  }

  // don't want to include ID field
  entry->setField(QStringLiteral("discogs-id"), QString());

  return entry;
}

Tellico::Fetch::FetchRequest DiscogsFetcher::updateRequest(Data::EntryPtr entry_) {
  const QString barcode = entry_->field(QStringLiteral("barcode"));
  if(!barcode.isEmpty()) {
    return FetchRequest(UPC, barcode);
  }

  const QString title = entry_->field(QStringLiteral("title"));
  const QString artist = entry_->field(QStringLiteral("artist"));
  const QString year = entry_->field(QStringLiteral("year"));
  // if any two of those are non-empty, combine them for a keyword search
  const int sum = (title.isEmpty() ? 0:1) + (artist.isEmpty() ? 0:1) + (year.isEmpty() ? 0:1);
  if(sum > 1) {
    QUrlQuery q;
    if(!title.isEmpty()) {
      q.addQueryItem(QStringLiteral("title"), title);
    }
    if(!artist.isEmpty()) {
      q.addQueryItem(QStringLiteral("artist"), artist);
    }
    if(!year.isEmpty()) {
      q.addQueryItem(QStringLiteral("year"), year);
    }
    return FetchRequest(Raw, q.toString());
  }

  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }

  if(!artist.isEmpty()) {
    return FetchRequest(Person, artist);
  }
  return FetchRequest();
}

void DiscogsFetcher::slotComplete(KJob*) {
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

#if 0 // checking remaining discogs rate limit allocation
  const QStringList allHeaders = m_job->queryMetaData(QStringLiteral("HTTP-Headers")).split(QLatin1Char('\n'));
  foreach(const QString& header, allHeaders) {
    if(header.startsWith(QStringLiteral("x-discogs-ratelimit-remaining"))) {
      const int index = header.indexOf(QLatin1Char(':'));
      if(index > 0) {
        myDebug() << "DiscogsFetcher: rate limit remaining:" << header.mid(index + 1);
      }
      break;
    }
  }
#endif
  // see bug 319662. If fetcher is cancelled, job is killed
  // if the pointer is retained, it gets double-deleted
  m_job = nullptr;

#if 0
  myWarning() << "Remove debug from discogsfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test-discogs.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << data;
  }
  f.close();
#endif

  Data::CollPtr coll(new Data::MusicCollection(true));
  // always add ID for fetchEntryHook
  Data::FieldPtr field(new Data::Field(QStringLiteral("discogs-id"), QStringLiteral("Discogs ID"), Data::Field::Line));
  field->setCategory(i18n("General"));
  coll->addField(field);

  if(optionalFields().contains(QStringLiteral("discogs"))) {
    Data::FieldPtr field(new Data::Field(QStringLiteral("discogs"), i18n("Discogs Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }
  if(optionalFields().contains(QStringLiteral("nationality"))) {
    Data::FieldPtr field(new Data::Field(QStringLiteral("nationality"), i18n("Nationality")));
    field->setCategory(i18n("General"));
    field->setFlags(Data::Field::AllowCompletion | Data::Field::AllowMultiple | Data::Field::AllowGrouped);
    field->setFormatType(FieldFormat::FormatPlain);
    coll->addField(field);
  }
  if(optionalFields().contains(QStringLiteral("producer"))) {
    Data::FieldPtr field(new Data::Field(QStringLiteral("producer"), i18n("Producer")));
    field->setCategory(i18n("General"));
    field->setFlags(Data::Field::AllowCompletion | Data::Field::AllowMultiple | Data::Field::AllowGrouped);
    field->setFormatType(FieldFormat::FormatName);
    coll->addField(field);
  }
  if(optionalFields().contains(QStringLiteral("barcode"))) {
    Data::FieldPtr field(new Data::Field(QStringLiteral("barcode"), i18n("Barcode")));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }
  if(optionalFields().contains(QStringLiteral("catno"))) {
    Data::FieldPtr field(new Data::Field(QStringLiteral("catno"), i18n("Catalog Number")));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }

  QJsonDocument doc = QJsonDocument::fromJson(data);
//  const QVariantMap resultMap = doc.object().toVariantMap().value(QStringLiteral("feed")).toMap();
  const QVariantMap resultMap = doc.object().toVariantMap();

  if(mapValue(resultMap, "message").startsWith(QLatin1String("Invalid consumer token"))) {
    message(i18n("The Discogs.com server reports a token error."),
            MessageHandler::Error);
    stop();
    return;
  }

  const int totalPages = mapValue(resultMap, "pagination", "pages").toInt();
  m_hasMoreResults = m_page < totalPages;
  ++m_page;

  int count = 0;
  foreach(const QVariant& result, resultMap.value(QLatin1String("results")).toList()) {
    if(count >= m_limit) {
      break;
    }

  //  myDebug() << "found result:" << result;

    Data::EntryPtr entry(new Data::Entry(coll));
    populateEntry(entry, result.toMap(), false);

    FetchResult* r = new FetchResult(this, entry);
    m_entries.insert(r->uid, entry);
    Q_EMIT signalResultFound(r);
    ++count;
  }

  stop();
}

void DiscogsFetcher::populateEntry(Data::EntryPtr entry_, const QVariantMap& resultMap_, bool fullData_) {
  entry_->setField(QStringLiteral("discogs-id"), mapValue(resultMap_, "id"));
  entry_->setField(QStringLiteral("title"), mapValue(resultMap_, "title"));
  const QString year = mapValue(resultMap_, "year");
  if(year != QLatin1String("0")) {
    entry_->setField(QStringLiteral("year"), year);
  }
  entry_->setField(QStringLiteral("genre"),  mapValue(resultMap_, "genres"));

  QStringList artists;
  foreach(const QVariant& artist, resultMap_.value(QLatin1String("artists")).toList()) {
    artists << mapValue(artist.toMap(), "name");
  }
  artists.removeDuplicates(); // sometimes the same value is repeated
  entry_->setField(QStringLiteral("artist"), artists.join(FieldFormat::delimiterString()));

  QStringList labels, catnos;
  foreach(const QVariant& label, resultMap_.value(QLatin1String("labels")).toList()) {
    const QVariantMap labelMap = label.toMap();
    labels << mapValue(labelMap, "name");
    catnos << mapValue(labelMap, "catno");
  }
  entry_->setField(QStringLiteral("label"), labels.join(FieldFormat::delimiterString()));
  if(entry_->collection()->hasField(QStringLiteral("catno"))) {
    entry_->setField(QStringLiteral("catno"), catnos.join(FieldFormat::delimiterString()));
  }

  if(m_imageSize != NoImage) {
    /* cover value is not always in the full data, so go ahead and set it now */
    QString coverUrl;
    if(m_imageSize == SmallImage) {
      coverUrl = mapValue(resultMap_, "thumb");
    } else { // no medium, only other size is large
      const auto imageList = resultMap_.value(QStringLiteral("images")).toList();
      // find either the "primary" one or just the first in the list
      for(const auto& imageItem : imageList) {
        const auto map = imageItem.toMap();
        if(mapValue(map, "type") == QLatin1String("primary")) {
          coverUrl = mapValue(map, "resource_url");
          break;
        }
      }
      if(coverUrl.isEmpty() && !imageList.isEmpty()) {
        const auto map = imageList.at(0).toMap();
        coverUrl = mapValue(map, "resource_url");
      }
      if(coverUrl.isEmpty()) {
        coverUrl = mapValue(resultMap_, "cover_image");
      }
    }
    if(!coverUrl.isEmpty()) {
      entry_->setField(QStringLiteral("cover"), coverUrl);
    }
  }

  // if we only need cursory data, then we're done
  if(!fullData_) {
    return;
  }

  // check the formats, it could have multiple
  // if there is a CD, prefer that in the track list
  bool hasCD = false;
  foreach(const QVariant& format, resultMap_.value(QLatin1String("formats")).toList()) {
    const QString formatName = mapValue(format.toMap(), "name");
    if(formatName == QLatin1String("CD")) {
      entry_->setField(QStringLiteral("medium"), i18n("Compact Disc"));
      hasCD = true;
    } else if(formatName == QLatin1String("Vinyl")) {
      entry_->setField(QStringLiteral("medium"), i18n("Vinyl"));
    } else if(formatName == QLatin1String("Cassette")) {
      entry_->setField(QStringLiteral("medium"), i18n("Cassette"));
    } else if(!hasCD && formatName == QLatin1String("DVD")) {
      // sometimes a CD and DVD both are included. If we're using the CD, ignore the DVD
      entry_->setField(QStringLiteral("medium"), i18n("DVD"));
    }
  }

  static const QRegularExpression discSplit(QStringLiteral("[-. ]"));
  static const QRegularExpression nonDigits(QStringLiteral("[\\D]"));
  QList<QStringList> discs; // list of tracks per disc
  foreach(const QVariant& track, resultMap_.value(QLatin1String("tracklist")).toList()) {
    const QVariantMap trackMap = track.toMap();
    if(mapValue(trackMap, "type_") != QLatin1String("track")) {
      continue;
    }

    // Releases might include a CD and a DVD, for example
    // prefer only the tracks on the CD. Allow positions of just numbers
    if(hasCD && !(mapValue(trackMap, "position").at(0).isNumber() ||
                  mapValue(trackMap, "position").startsWith(QLatin1String("CD")))) {
      continue;
    }

    QStringList trackInfo;
    trackInfo << mapValue(trackMap, "title");
    if(trackMap.contains(QStringLiteral("artists"))) {
      QStringList artists;
      foreach(const QVariant& artist, trackMap.value(QLatin1String("artists")).toList()) {
        artists << mapValue(artist.toMap(), "name");
      }
      trackInfo << artists.join(FieldFormat::delimiterString());
    } else {
      trackInfo << entry_->field(QStringLiteral("artist"));
    }
    trackInfo << mapValue(trackMap, "duration");

    // determine whether the track is in a multi-disc album
    int disc = 1;
    const QString trackNum = mapValue(trackMap, "position");
    if(trackNum.contains(discSplit)) {
      disc = trackNum.section(discSplit, 0, 0).remove(nonDigits).toInt();
      if(disc == 0) disc = 1;
    }
    while(discs.size() < disc) discs << QStringList();
    discs[disc-1] << trackInfo.join(FieldFormat::columnDelimiterString());
  }
  bool changeTrackTitle = true;
  for(int disc = 0; disc < discs.count(); ++disc) {
    QString trackField = QStringLiteral("track");
    if(disc > 0) {
      trackField.append(QString::number(disc+1));
      Data::FieldPtr f2(new Data::Field(trackField,
                                        i18n("Tracks (Disc %1)", disc+1),
                                        Data::Field::Table));
      f2->setFormatType(FieldFormat::FormatTitle);
      f2->setProperty(QStringLiteral("columns"), QStringLiteral("3"));
      f2->setProperty(QStringLiteral("column1"), i18n("Title"));
      f2->setProperty(QStringLiteral("column2"), i18n("Artist"));
      f2->setProperty(QStringLiteral("column3"), i18n("Length"));
      entry_->collection()->addField(f2);
      // also change the title of the first track field
      if(changeTrackTitle) {
        Data::FieldPtr f1 = entry_->collection()->fieldByName(QStringLiteral("track"));
        f1->setTitle(i18n("Tracks (Disc %1)", 1));
        entry_->collection()->modifyField(f1);
        changeTrackTitle = false;
      }
    }
    if(m_multiDiscTracks) {
      entry_->setField(trackField, discs.at(disc).join(FieldFormat::rowDelimiterString()));
    } else {
      // combine tracks from all discs into the single track field
      QStringList allTracks;
      for(const auto& discTracks : std::as_const(discs)) {
        allTracks += discTracks;
      }
      entry_->setField(trackField, allTracks.join(FieldFormat::rowDelimiterString()));
      break;
    }
  }

  if(entry_->collection()->hasField(QStringLiteral("discogs"))) {
    entry_->setField(QStringLiteral("discogs"), mapValue(resultMap_, "uri"));
  }

  if(entry_->collection()->hasField(QStringLiteral("nationality"))) {
    entry_->setField(QStringLiteral("nationality"), mapValue(resultMap_, "country"));
  }

  if(entry_->collection()->hasField(QStringLiteral("barcode"))) {
    foreach(const QVariant& identifier, resultMap_.value(QLatin1String("identifiers")).toList()) {
      const QVariantMap idMap = identifier.toMap();
      if(mapValue(idMap, "type") == QLatin1String("Barcode")) {
        entry_->setField(QStringLiteral("barcode"), mapValue(idMap, "value"));
        break;
      }
    }
  }

  if(entry_->collection()->hasField(QStringLiteral("producer"))) {
    QStringList producers;
    foreach(const QVariant& extraArtist, resultMap_.value(QLatin1String("extraartists")).toList()) {
      const QVariantMap extraArtistMap = extraArtist.toMap();
      if(mapValue(extraArtistMap, "role").contains(QStringLiteral("Producer"))) {
        producers << mapValue(extraArtistMap, "name");
      }
    }
    entry_->setField(QStringLiteral("producer"), producers.join(FieldFormat::delimiterString()));
  }

  entry_->setField(QStringLiteral("comments"), mapValue(resultMap_, "notes"));
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
  hash[QStringLiteral("producer")] = i18n("Producer");
  hash[QStringLiteral("nationality")] = i18n("Nationality");
  hash[QStringLiteral("discogs")] = i18n("Discogs Link");
  hash[QStringLiteral("barcode")] = i18n("Barcode");
  hash[QStringLiteral("catno")] = i18n("Catalog Number");
  return hash;
}

DiscogsFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const DiscogsFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;
  QLabel* al = new QLabel(i18n("Registration is required for accessing this data source. "
                               "If you agree to the terms and conditions, <a href='%1'>sign "
                               "up for an account</a>, and enter your information below.",
                                QLatin1String("https://www.discogs.com/developers/#page:authentication")),
                          optionsWidget());
  al->setOpenExternalLinks(true);
  al->setWordWrap(true);
  ++row;
  l->addWidget(al, row, 0, 1, 2);
  // richtext gets weird with size
  al->setMinimumWidth(al->sizeHint().width());

  QLabel* label = new QLabel(i18n("User token:"), optionsWidget());
  l->addWidget(label, ++row, 0);

  m_apiKeyEdit = new QLineEdit(optionsWidget());
  connect(m_apiKeyEdit, &QLineEdit::textChanged, this, &ConfigWidget::slotSetModified);
  l->addWidget(m_apiKeyEdit, row, 1);
  label->setBuddy(m_apiKeyEdit);

  label = new QLabel(i18n("&Image size: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_imageCombo = new GUI::ComboBox(optionsWidget());
  m_imageCombo->addItem(i18n("Small Image"), SmallImage);
//  m_imageCombo->addItem(i18n("Medium Image"), MediumImage); // no medium right now, either thumbnail (small) or large
  m_imageCombo->addItem(i18n("Large Image"), LargeImage);
  m_imageCombo->addItem(i18n("No Image"), NoImage);
  void (GUI::ComboBox::* activatedInt)(int) = &GUI::ComboBox::activated;
  connect(m_imageCombo, activatedInt, this, &ConfigWidget::slotSetModified);
  l->addWidget(m_imageCombo, row, 1);
  QString w = i18n("The cover image may be downloaded as well. However, too many large images in the "
                   "collection may degrade performance.");
  label->setWhatsThis(w);
  m_imageCombo->setWhatsThis(w);
  label->setBuddy(m_imageCombo);

  l->setRowStretch(++row, 10);

  if(fetcher_) {
    m_apiKeyEdit->setText(fetcher_->m_apiKey);
    m_imageCombo->setCurrentData(fetcher_->m_imageSize);
    m_multiDiscTracks = fetcher_->m_multiDiscTracks;
  } else { // defaults
    m_imageCombo->setCurrentData(LargeImage);
    m_multiDiscTracks = true;
  }

  // now add additional fields widget
  addFieldsWidget(DiscogsFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

void DiscogsFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  QString apiKey = m_apiKeyEdit->text().trimmed();
  if(!apiKey.isEmpty()) {
    config_.writeEntry("API Key", apiKey);
  }
  const int n = m_imageCombo->currentData().toInt();
  config_.writeEntry("Image Size", n);
  config_.writeEntry("Split Tracks By Disc", m_multiDiscTracks);
}

QString DiscogsFetcher::ConfigWidget::preferredName() const {
  return DiscogsFetcher::defaultName();
}
