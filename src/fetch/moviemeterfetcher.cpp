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

#include "moviemeterfetcher.h"
#include "../collections/videocollection.h"
#include "../images/imagefactory.h"
#include "../core/filehandler.h"
#include "../utils/guiproxy.h"
#include "../utils/mapvalue.h"
#include "../gui/combobox.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KIO/StoredTransferJob>
#include <KJobUiDelegate>
#include <KJobWidgets>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>

namespace {
  static const char* MOVIEMETER_API_KEY = "t80a06uf736d0yd00jpynpdsgea255yk";
  static const char* MOVIEMETER_API_URL = "http://www.moviemeter.nl/api/film/";
}

using namespace Tellico;
using Tellico::Fetch::MovieMeterFetcher;

MovieMeterFetcher::MovieMeterFetcher(QObject* parent_)
    : Fetcher(parent_)
    , m_started(false)
    , m_imageSize(MediumImage) {
}

MovieMeterFetcher::~MovieMeterFetcher() {
}

QString MovieMeterFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

QString MovieMeterFetcher::attribution() const {
  return QStringLiteral("<a href=\"http://www.moviemeter.nl\">MovieMeter</a>");
}

bool MovieMeterFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Keyword;
}

bool MovieMeterFetcher::canFetch(int type) const {
  return type == Data::Collection::Video;
}

void MovieMeterFetcher::readConfigHook(const KConfigGroup& config_) {
  const int imageSize = config_.readEntry("Image Size", -1);
  if(imageSize > -1) {
    m_imageSize = static_cast<ImageSize>(imageSize);
  }
}

void MovieMeterFetcher::search() {
  m_started = true;

  QUrl u(QString::fromLatin1(MOVIEMETER_API_URL));
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("api_key"), QLatin1String(MOVIEMETER_API_KEY));

  switch(request().key()) {
    case Keyword:
      q.addQueryItem(QStringLiteral("q"), request().value());
      //u.addQueryItem(QLatin1String("type"), QLatin1String("all"));
      break;

    case Raw:
      q.setQuery(request().value());
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      stop();
      return;
  }
  u.setQuery(q);
//  myDebug() << "url: " << u.url();

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result, this, &MovieMeterFetcher::slotComplete);
}

void MovieMeterFetcher::stop() {
  if(!m_started) {
    return;
  }
  if(m_job) {
    m_job->kill();
    m_job = nullptr;
  }
  m_started = false;
  emit signalDone(this);
}

Tellico::Data::EntryPtr MovieMeterFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }

  QString id = entry->field(QStringLiteral("moviemeter-id"));
  if(!id.isEmpty()) {
    QUrl u(QString::fromLatin1(MOVIEMETER_API_URL));
    u.setPath(u.path() + id);
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("api_key"), QLatin1String(MOVIEMETER_API_KEY));
    u.setQuery(q);
    // quiet
    QByteArray data = FileHandler::readDataFile(u, true);

#if 0
    myWarning() << "Remove debug2 from moviemeterfetcher.cpp";
    QFile f(QString::fromLatin1("/tmp/test2.json"));
    if(f.open(QIODevice::WriteOnly)) {
      QTextStream t(&f);
      t << data;
    }
    f.close();
#endif

    QJsonDocument doc = QJsonDocument::fromJson(data);
    populateEntry(entry, doc.object().toVariantMap(), true);
  }

  // image might still be URL
  const QString image_id = entry->field(QStringLiteral("cover"));
  if(image_id.contains(QLatin1Char('/'))) {
    const QString id = ImageFactory::addImage(QUrl::fromUserInput(image_id), true /* quiet */);
    if(id.isEmpty()) {
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    }
    // empty image ID is ok
    entry->setField(QStringLiteral("cover"), id);
  }

  // don't want to include ID field
  entry->setField(QStringLiteral("moviemeter-id"), QString());

  return entry;
}

Tellico::Fetch::FetchRequest MovieMeterFetcher::updateRequest(Data::EntryPtr entry_) {
  const QString title = entry_->field(QStringLiteral("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Keyword, title);
  }
  return FetchRequest();
}

void MovieMeterFetcher::slotComplete(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);
//  myDebug();

  if(job->error()) {
    job->uiDelegate()->showErrorMessage();
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
  m_job = nullptr;

#if 0
  myWarning() << "Remove debug from moviemeterfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << data;
  }
  f.close();
#endif

  Data::CollPtr coll(new Data::VideoCollection(true));
  // always add ID for fetchEntryHook
  Data::FieldPtr field(new Data::Field(QStringLiteral("moviemeter-id"), QStringLiteral("MovieMeter ID"), Data::Field::Line));
  field->setCategory(i18n("General"));
  coll->addField(field);

  if(optionalFields().contains(QStringLiteral("moviemeter"))) {
    Data::FieldPtr field(new Data::Field(QStringLiteral("moviemeter"), i18n("MovieMeter Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }
  if(optionalFields().contains(QStringLiteral("alttitle"))) {
    Data::FieldPtr field(new Data::Field(QStringLiteral("alttitle"), i18n("Alternative Titles"), Data::Field::Table));
    field->setFormatType(FieldFormat::FormatTitle);
    coll->addField(field);
  }

  QJsonDocument doc = QJsonDocument::fromJson(data);
  QJsonArray array = doc.array();
  for(int i = 0; i < array.count(); i++) {
    Data::EntryPtr entry(new Data::Entry(coll));
    populateEntry(entry, array.at(i).toObject().toVariantMap(), false);

    FetchResult* r = new FetchResult(this, entry);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
  }

  stop();
}

void MovieMeterFetcher::populateEntry(Data::EntryPtr entry_, const QVariantMap& resultMap_, bool fullData_) {
  entry_->setField(QStringLiteral("moviemeter-id"), mapValue(resultMap_, "id"));
  entry_->setField(QStringLiteral("title"), mapValue(resultMap_, "title"));
  entry_->setField(QStringLiteral("year"),  mapValue(resultMap_, "year"));

  // if we only need cursory data, then we're done
  if(!fullData_) {
    return;
  }

  entry_->setField(QStringLiteral("genre"),  mapValue(resultMap_, "genres"));
  entry_->setField(QStringLiteral("plot"),  mapValue(resultMap_, "plot"));
  entry_->setField(QStringLiteral("running-time"),  mapValue(resultMap_, "duration"));
  entry_->setField(QStringLiteral("director"),  mapValue(resultMap_, "directors"));
  entry_->setField(QStringLiteral("nationality"),  mapValue(resultMap_, "countries"));

  QStringList castList;
  foreach(const QVariant& actor, resultMap_.value(QLatin1String("actors")).toList()) {
    castList << mapValue(actor.toMap(), "name");
  }
  entry_->setField(QStringLiteral("cast"), castList.join(FieldFormat::rowDelimiterString()));

  if(entry_->collection()->hasField(QStringLiteral("moviemeter"))) {
    entry_->setField(QStringLiteral("moviemeter"), mapValue(resultMap_, "url"));
  }

  if(entry_->collection()->hasField(QStringLiteral("alttitle"))) {
    entry_->setField(QStringLiteral("alttitle"), mapValue(resultMap_, "alternative_title"));
  }

  const QString coverField(QStringLiteral("cover"));
  switch(m_imageSize) {
    case SmallImage:
      entry_->setField(coverField, mapValue(resultMap_.value(QStringLiteral("posters")).toMap(), "small"));
      break;
    case MediumImage:
      entry_->setField(coverField, mapValue(resultMap_.value(QStringLiteral("posters")).toMap(), "regular"));
      break;
    case LargeImage:
      entry_->setField(coverField, mapValue(resultMap_.value(QStringLiteral("posters")).toMap(), "large"));
      break;
    case NoImage:
      break;
  }
}

Tellico::Fetch::ConfigWidget* MovieMeterFetcher::configWidget(QWidget* parent_) const {
  return new MovieMeterFetcher::ConfigWidget(parent_, this);
}

QString MovieMeterFetcher::defaultName() {
  return QStringLiteral("MovieMeter"); // no translation
}

QString MovieMeterFetcher::defaultIcon() {
  return favIcon("https://www.moviemeter.nl");
}

Tellico::StringHash MovieMeterFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("moviemeter")] = i18n("MovieMeter Link");
  hash[QStringLiteral("alttitle")]   = i18n("Alternative Titles");
  return hash;
}

MovieMeterFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const MovieMeterFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;
  auto label = new QLabel(i18n("&Image size: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_imageCombo = new GUI::ComboBox(optionsWidget());
  m_imageCombo->addItem(i18n("Small Image"), SmallImage);
  m_imageCombo->addItem(i18n("Medium Image"), MediumImage); // no medium right now, either thumbnail (small) or large
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
    m_imageCombo->setCurrentData(fetcher_->m_imageSize);
  } else { // defaults
    m_imageCombo->setCurrentData(MediumImage);
  }

  // now add additional fields widget
  addFieldsWidget(MovieMeterFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

void MovieMeterFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  const int n = m_imageCombo->currentData().toInt();
  config_.writeEntry("Image Size", n);
}

QString MovieMeterFetcher::ConfigWidget::preferredName() const {
  return MovieMeterFetcher::defaultName();
}
