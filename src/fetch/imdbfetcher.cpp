/***************************************************************************
    Copyright (C) 2004-2009 Robby Stephenson <robby@periapsis.org>
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

#include "imdbfetcher.h"
#include "../collections/videocollection.h"
#include "../entry.h"
#include "../field.h"
#include "../fieldformat.h"
#include "../images/imagefactory.h"
#include "../utils/objvalue.h"
#include "../utils/guiproxy.h"
#include "../gui/combobox.h"
#include "../gui/lineedit.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>
#include <KIO/StoredTransferJob>
#include <KJobUiDelegate>
#include <KAcceleratorManager>
#include <KJobWidgets>

#include <QSpinBox>
#include <QFile>
#include <QMap>
#include <QLabel>
#include <QRadioButton>
#include <QGroupBox>
#include <QButtonGroup>
#include <QGridLayout>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QRegularExpression>

namespace {
  static const uint IMDB_DEFAULT_CAST_SIZE = 10;
}

using namespace Tellico;
using Tellico::Fetch::IMDBFetcher;
using namespace Qt::Literals::StringLiterals;

IMDBFetcher::IMDBFetcher(QObject* parent_) : Fetcher(parent_),
    m_job(nullptr), m_started(false), m_imageSize(MediumImage),
    m_numCast(IMDB_DEFAULT_CAST_SIZE), m_useSystemLocale(true) {
}

IMDBFetcher::~IMDBFetcher() = default;

QString IMDBFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool IMDBFetcher::canFetch(int type) const {
  return type == Data::Collection::Video;
}

// imdb can search title only
bool IMDBFetcher::canSearch(Fetch::FetchKey k) const {
  // Raw searches are intended to be the imdb url
  return k == Title || k == Raw;
}

void IMDBFetcher::readConfigHook(const KConfigGroup& config_) {
  m_numCast = config_.readEntry("Max Cast", IMDB_DEFAULT_CAST_SIZE);
  const int imageSize = config_.readEntry("Image Size", -1);
  if(imageSize > -1) {
    m_imageSize = static_cast<ImageSize>(imageSize);
  }
  m_useSystemLocale = config_.readEntry("System Locale", true);
  m_customLocale = config_.readEntry("Custom Locale");
}

// multiple values not supported
void IMDBFetcher::search() {
  m_started = true;
  m_matches.clear();

  QString operationName, query;
  QJsonObject vars;
  switch(request().key()) {
    case Title:
      operationName = QLatin1String("Search");
      query = searchQuery();
      vars.insert(QLatin1String("searchTerms"), request().value());
      break;

    case Raw:
      {
        // expect a url that ends with the tt id
        QRegularExpression ttEndRx(QStringLiteral("/(tt\\d+)/?$"));
        auto match = ttEndRx.match(request().value());
        if(match.hasMatch()) {
          operationName = QLatin1String("TitleFull");
          query = titleQuery();
          vars.insert(QLatin1String("id"), match.captured(1));
        } else {
          // fallback to a general search
          myDebug() << "bad url";
          operationName = QLatin1String("Search");
          query = searchQuery();
          vars.insert(QLatin1String("searchTerms"), request().value());
        }
      }
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      stop();
      return;
  }

  QJsonObject payload;
  payload.insert(QLatin1String("operationName"), operationName);
  payload.insert(QLatin1String("query"), query);
  payload.insert(QLatin1String("variables"), vars);

  m_job = KIO::storedHttpPost(QJsonDocument(payload).toJson(),
                              QUrl(QLatin1String("https://api.graphql.imdb.com")),
                              KIO::HideProgressInfo);
  configureJob(m_job);
  connect(m_job.data(), &KJob::result,
          this, &IMDBFetcher::slotComplete);
}

void IMDBFetcher::stop() {
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

void IMDBFetcher::slotComplete(KJob*) {
  if(m_job->error()) {
    myDebug() << m_job->errorString();
    m_job->uiDelegate()->showErrorMessage();
    stop();
    return;
  }

  const auto data = m_job->data();
  if(data.isEmpty()) {
    myDebug() << "IMDB - no data";
    stop();
    return;
  }
  m_job = nullptr;

#if 0
  myWarning() << "Remove JSON debug from imdbfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/imdb-graphql-search.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << QString::fromUtf8(data.constData(), data.size());
  }
  f.close();
#endif

  // for Raw searches, the result should be a single title
  if(request().key() == Raw) {
    auto entry = parseResult(data);
    if(entry) {
      FetchResult* r = new FetchResult(this, entry);
      m_entries.insert(r->uid, entry);
      Q_EMIT signalResultFound(r);
    }
    stop();
    return;
  }

  QJsonParseError jsonError;
  QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
  if(doc.isNull()) {
    myDebug() << "null JSON document:" << jsonError.errorString();
    message(jsonError.errorString(), MessageHandler::Error);
  }

  const auto docObj = doc.object()
                         .value("data"_L1).toObject()
                         .value("mainSearch"_L1).toObject();
  const auto list = docObj["edges"_L1].toArray();
  for(const auto& edge: std::as_const(list)) {
    const auto obj = edge["node"_L1]["entity"_L1].toObject();
    const auto id = objValue(obj, "id");
    const auto title = objValue(obj, "titleText", "text");
    const auto year = objValue(obj, "releaseYear", "year");

    FetchResult* r = new FetchResult(this, title, year);
    m_matches.insert(r->uid, id);
    m_titleTypes.insert(r->uid, objValue(obj, "titleType", "text"));
    Q_EMIT signalResultFound(r);
  }

  stop();
}

Tellico::Data::EntryPtr IMDBFetcher::fetchEntryHook(uint uid_) {
  // if we already grabbed this one, then just pull it out of the dict
  Data::EntryPtr entry = m_entries[uid_];
  if(entry) {
    return entry;
  }
  if(!m_matches.contains(uid_)) {
    myDebug() << "no id match for" << uid_;
    return entry;
  }

  entry = readGraphQL(m_matches.value(uid_), m_titleTypes.value(uid_));
  if(entry) {
    m_entries.insert(uid_, entry); // keep for later
  }

  return entry;
}

Tellico::Fetch::FetchRequest IMDBFetcher::updateRequest(Data::EntryPtr entry_) {
  QUrl link = QUrl::fromUserInput(entry_->field(QStringLiteral("imdb")));

  if(!link.isEmpty() && link.isValid()) {
    return FetchRequest(Fetch::Raw, link.url());
  }

  // optimistically try searching for title and rely on Collection::sameEntry() to figure things out
  const QString t = entry_->field(QStringLiteral("title"));
  if(!t.isEmpty()) {
    return FetchRequest(Fetch::Title, t);
  }
  return FetchRequest();
}

void IMDBFetcher::configureJob(QPointer<KIO::StoredTransferJob> job_) {
  KJobWidgets::setWindow(job_, GUI::Proxy::widget());
  job_->addMetaData(QStringLiteral("content-type"), QStringLiteral("Content-Type: application/json"));
  job_->addMetaData(QStringLiteral("accept"), QStringLiteral("application/json"));
  job_->addMetaData(QStringLiteral("origin"), QLatin1String("https://www.imdb.com"));
  QStringList headers;
  headers += QStringLiteral("x-imdb-client-name: imdb-web-next-localized");

  QString localeName;
  if(m_useSystemLocale || m_customLocale.isEmpty()) {
    // use default locale instead of system in case it was changed
    localeName = QLocale().name();
    myLog() << "Using system locale:" << localeName;
  } else {
    localeName = m_customLocale;
    myLog() << "Using custom locale:" << localeName;
  }
  localeName.replace(QLatin1Char('_'), QLatin1Char('-'));

  job_->addMetaData(QStringLiteral("Languages"), localeName);
  headers += QStringLiteral("x-imdb-user-country: %1").arg(localeName.section(QLatin1Char('-'), 1, 1));

  job_->addMetaData(QStringLiteral("customHTTPHeader"), headers.join(QLatin1String("\r\n")));
}

Tellico::Data::EntryPtr IMDBFetcher::readGraphQL(const QString& imdbId_, const QString& titleType_) {
  const auto query = titleType_ == QLatin1String("TV Series") ? episodeQuery() : titleQuery();
  QJsonObject vars;
  vars.insert(QLatin1String("id"), imdbId_);

  QJsonObject payload;
  payload.insert(QLatin1String("operationName"), QLatin1String("TitleFull"));
  payload.insert(QLatin1String("query"), query);
  payload.insert(QLatin1String("variables"), vars);

  QPointer<KIO::StoredTransferJob> job = KIO::storedHttpPost(QJsonDocument(payload).toJson(),
                                                             QUrl(QLatin1String("https://api.graphql.imdb.com")),
                                                             KIO::HideProgressInfo);
  configureJob(job);

  if(!job->exec()) {
    myDebug() << "IMDB: graphql failure";
    myDebug() << job->errorString();
    return Data::EntryPtr();
  }

  const auto data = job->data();
#if 0
  myWarning() << "Remove JSON debug from imdbfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/imdb-graphql-title.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << QString::fromUtf8(data.constData(), data.size());
  }
  f.close();
#endif
  return parseResult(data);
}

Tellico::Data::EntryPtr IMDBFetcher::parseResult(const QByteArray& data_) {
  QJsonParseError jsonError;
  QJsonDocument doc = QJsonDocument::fromJson(data_, &jsonError);
  if(doc.isNull()) {
    myDebug() << "null JSON document:" << jsonError.errorString();
    message(jsonError.errorString(), MessageHandler::Error);
    return Data::EntryPtr();
  }
  Data::CollPtr coll(new Data::VideoCollection(true));
  Data::EntryPtr entry(new Data::Entry(coll));
  const auto docObj = doc.object();
  const auto obj = docObj["data"_L1]["title"_L1].toObject();
  entry->setField(QStringLiteral("title"), objValue(obj, "titleText", "text"));
  entry->setField(QStringLiteral("year"), objValue(obj, "releaseYear", "year"));
  entry->setField(QStringLiteral("language"), objValue(obj, "spokenLanguages", "spokenLanguages"));
  entry->setField(QStringLiteral("plot"), objValue(obj, "plot", "plotText", "plainText"));
  entry->setField(QStringLiteral("genre"), objValue(obj, "genres", "genres", "text"));
  entry->setField(QStringLiteral("nationality"), objValue(obj, "countriesOfOrigin", "countries", "text"));
  entry->setField(QStringLiteral("audio-track"), objValue(obj, "technicalSpecifications", "soundMixes", "items", "text"));
  entry->setField(QStringLiteral("aspect-ratio"), objValue(obj, "technicalSpecifications", "aspectRatios", "items", "aspectRatio"));
  entry->setField(QStringLiteral("color"), objValue(obj, "technicalSpecifications", "colorations", "items", "text"));
  const int runTime = obj["runtime"_L1]["seconds"_L1].toInt();
  if(runTime > 0) {
    entry->setField(QStringLiteral("running-time"), QString::number(runTime/60));
  }
  entry->setField(QStringLiteral("language"), objValue(obj, "spokenLanguages", "spokenLanguages", "text"));
  entry->setField(QStringLiteral("plot"), objValue(obj, "plot", "plotText", "plainText"));

  if(m_imageSize != NoImage) {
    QUrl imageUrl(objValue(obj, "primaryImage", "url"));
    // LargeImage just means use default available size
    if(m_imageSize != LargeImage) {
      // limit to 256 for small and 640 for medium
      const int maxDim = m_imageSize == SmallImage ? 256 : 640;
      const auto imageWidth = objValue(obj, "primaryImage", "width").toFloat();
      const auto imageHeight = objValue(obj, "primaryImage", "height").toFloat();
      const auto ratio = imageWidth/imageHeight;
      int newWidth, newHeight;
      if(ratio < 1) {
        newWidth = ratio*maxDim;
        newHeight = maxDim;
      } else {
        newWidth = maxDim;
        newHeight = ratio*maxDim;
      }
      auto param = QStringLiteral("QL75_SX%1_CR0,0,%1,%2_.jpg").arg(newWidth).arg(newHeight);
      imageUrl.setPath(imageUrl.path().replace(QLatin1String(".jpg"), param));
    }
    entry->setField(QStringLiteral("cover"), ImageFactory::addImage(imageUrl, true));
  }

  entry->setField(QStringLiteral("studio"),
                  objValue(obj, "companyCredits", "edges", "node", "company", "companyText", "text"));

  const QString certification(QStringLiteral("certification"));
  QString cert = objValue(obj, "certificate", "rating");
  if(!cert.isEmpty()) {
    // set default certification, assuming US for now
    if(cert == QLatin1String("Not Rated")) {
      cert = QLatin1Char('U');
    }
    const QString certCountry = objValue(obj, "certificate", "country", "text");
    if(certCountry == QLatin1String("United States")) {
      cert += QStringLiteral(" (USA)");
    } else if(!certCountry.isEmpty()) {
      cert += QStringLiteral(" (%1)").arg(certCountry);
    }
    const QStringList& certsAllowed = coll->fieldByName(certification)->allowed();
    if(certsAllowed.contains(cert)) {
      entry->setField(certification, cert);
    } else {
      myLog() << "Skipping certification as not allowed:" << cert;
    }
  }

  QStringList directors;
  auto list = obj["principalDirectors"_L1].toArray();
  for(const auto& director: std::as_const(list)) {
    directors += objValue(director.toObject(), "credits", "name", "nameText", "text");
  }
  // favor principalDirectors over all the directors, but episodes may be directors only
  if(list.isEmpty()) {
    list = obj["directors"_L1]["edges"_L1].toArray();
    for(const auto& edge: std::as_const(list)) {
      directors += objValue(edge.toObject(), "node", "name", "nameText", "text");
    }
  }
  entry->setField(QStringLiteral("director"), directors.join(FieldFormat::delimiterString()));

  entry->setField(QStringLiteral("producer"),
                  objValue(obj, "producers", "edges", "node", "name", "nameText", "text"));
  entry->setField(QStringLiteral("composer"),
                  objValue(obj, "composers", "edges", "node", "name", "nameText", "text"));
  entry->setField(QStringLiteral("writer"),
                  objValue(obj, "writers", "edges", "node", "name", "nameText", "text"));


  QStringList cast;
  list = obj["cast"_L1]["edges"_L1].toArray();
  for(const auto& edge: std::as_const(list)) {
    const auto o = edge["node"_L1].toObject();
    cast += objValue(o, "name", "nameText", "text")
          + FieldFormat::columnDelimiterString()
          + objValue(o, "characters", "name");
    if(cast.count() >= m_numCast) {
      break;
    }
  }
  entry->setField(QStringLiteral("cast"), cast.join(FieldFormat::rowDelimiterString()));

  const QString imdb(QStringLiteral("imdb"));
  if(!coll->hasField(imdb) && optionalFields().contains(imdb)) {
    coll->addField(Data::Field::createDefaultField(Data::Field::ImdbField));
  }
  if(coll->hasField(imdb) && coll->fieldByName(imdb)->type() == Data::Field::URL) {
    entry->setField(imdb, objValue(obj, "canonicalUrl"));
  }

  const QString imdbRating(QStringLiteral("imdb-rating"));
  if(optionalFields().contains(imdbRating)) {
    if(!coll->hasField(imdbRating)) {
      Data::FieldPtr f(new Data::Field(imdbRating, i18n("IMDb Rating"), Data::Field::Rating));
      f->setCategory(i18n("General"));
      f->setProperty(QStringLiteral("maximum"), QStringLiteral("10"));
      coll->addField(f);
    }
    entry->setField(imdbRating, objValue(obj, "ratingsSummary", "aggregateRating"));
  }

  const QString origtitle(QStringLiteral("origtitle"));
  if(optionalFields().contains(origtitle)) {
    Data::FieldPtr f(new Data::Field(origtitle, i18n("Original Title")));
    f->setFormatType(FieldFormat::FormatTitle);
    coll->addField(f);
    entry->setField(origtitle, objValue(obj, "originalTitleText", "text"));
  }

  const QString alttitle(QStringLiteral("alttitle"));
  if(optionalFields().contains(alttitle)) {
    Data::FieldPtr f(new Data::Field(alttitle, i18n("Alternative Titles"), Data::Field::Table));
    f->setFormatType(FieldFormat::FormatTitle);
    coll->addField(f);
    QStringList akas;
    list = obj["akas"_L1]["edges"_L1].toArray();
    for(const auto& edge: std::as_const(list)) {
      akas += objValue(edge.toObject(), "node", "text");
    }
    akas.removeDuplicates();
    entry->setField(alttitle, akas.join(FieldFormat::rowDelimiterString()));
  }

  const QString episode(QStringLiteral("episode"));
  if(objValue(obj, "titleType", "text") == QLatin1String("TV Series") &&
     optionalFields().contains(episode)) {
    coll->addField(Data::Field::createDefaultField(Data::Field::EpisodeField));
    QStringList episodes;
    list = obj["episodes"_L1]["episodes"_L1]["edges"_L1].toArray();
    for(const auto& edge: std::as_const(list)) {
      const auto nodeObj = edge["node"_L1].toObject();
      QString row = objValue(nodeObj, "titleText", "text");
      const auto seriesObj = nodeObj["series"_L1].toObject();
      // future episodes have a "Episode #" start
      if(!row.startsWith("Episode #"_L1) && seriesObj.contains("displayableEpisodeNumber"_L1)) {
        row += FieldFormat::columnDelimiterString() + objValue(seriesObj, "displayableEpisodeNumber", "displayableSeason", "text")
             + FieldFormat::columnDelimiterString() + objValue(seriesObj, "displayableEpisodeNumber", "episodeNumber", "text");
      }
      episodes += row;
    }
    entry->setField(episode, episodes.join(FieldFormat::rowDelimiterString()));
  }

  return entry;
}

QString IMDBFetcher::defaultName() {
  return i18n("Internet Movie Database");
}

QString IMDBFetcher::defaultIcon() {
  return favIcon(QUrl(QLatin1String("https://www.imdb.com")),
                 QUrl(QLatin1String("https://m.media-amazon.com/images/G/01/imdb/images-ANDW73HA/favicon_desktop_32x32._CB1582158068_.png")));
}

//static
Tellico::StringHash IMDBFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("imdb")]             = i18n("IMDb Link");
  hash[QStringLiteral("imdb-rating")]      = i18n("IMDb Rating");
  hash[QStringLiteral("alttitle")]         = i18n("Alternative Titles");
  hash[QStringLiteral("allcertification")] = i18n("Certifications");
  hash[QStringLiteral("origtitle")]        = i18n("Original Title");
  hash[QStringLiteral("episode")]          = i18n("Episodes");
  return hash;
}

Tellico::Fetch::ConfigWidget* IMDBFetcher::configWidget(QWidget* parent_) const {
  return new IMDBFetcher::ConfigWidget(parent_, this);
}

IMDBFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const IMDBFetcher* fetcher_/*=0*/)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;

  QLabel* label = new QLabel(i18n("&Maximum cast: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_numCast = new QSpinBox(optionsWidget());
  m_numCast->setMaximum(99);
  m_numCast->setMinimum(0);
  m_numCast->setValue(IMDB_DEFAULT_CAST_SIZE);
  void (QSpinBox::* textChanged)(const QString&) = &QSpinBox::textChanged;
  connect(m_numCast, textChanged, this, &ConfigWidget::slotSetModified);
  l->addWidget(m_numCast, row, 1);
  QString w = i18n("The list of cast members may include many people. Set the maximum number returned from the search.");
  label->setWhatsThis(w);
  m_numCast->setWhatsThis(w);
  label->setBuddy(m_numCast);

  label = new QLabel(i18n("&Image size: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_imageCombo = new GUI::ComboBox(optionsWidget());
  m_imageCombo->addItem(i18n("Small Image"), SmallImage);
  m_imageCombo->addItem(i18n("Medium Image"), MediumImage);
  m_imageCombo->addItem(i18n("Large Image"), LargeImage);
  m_imageCombo->addItem(i18n("No Image"), NoImage);
  void (GUI::ComboBox::* activatedInt)(int) = &GUI::ComboBox::activated;
  connect(m_imageCombo, activatedInt, this, &ConfigWidget::slotSetModified);
  l->addWidget(m_imageCombo, row, 1);
  w = i18n("The cover image may be downloaded as well. However, too many large images in the "
           "collection may degrade performance.");
  label->setWhatsThis(w);
  m_imageCombo->setWhatsThis(w);
  label->setBuddy(m_imageCombo);

  auto localeGroupBox = new QGroupBox(i18n("Locale"), optionsWidget());
  l->addWidget(localeGroupBox, ++row, 0, 1, -1);

  m_systemLocaleRadioButton = new QRadioButton(i18n("Use system locale"), localeGroupBox);
  m_customLocaleRadioButton = new QRadioButton(i18n("Use custom locale"), localeGroupBox);
  m_customLocaleEdit = new GUI::LineEdit(localeGroupBox);
  m_customLocaleEdit->setEnabled(false);

  auto localeGroupLayout = new QGridLayout(localeGroupBox);
  localeGroupLayout->addWidget(m_systemLocaleRadioButton, 0, 0);
  localeGroupLayout->addWidget(m_customLocaleRadioButton, 1, 0);
  localeGroupLayout->addWidget(m_customLocaleEdit, 1, 1);
  localeGroupBox->setLayout(localeGroupLayout);

  auto localeGroup = new QButtonGroup(localeGroupBox);
  localeGroup->addButton(m_systemLocaleRadioButton, 0 /* id */);
  localeGroup->addButton(m_customLocaleRadioButton, 1 /* id */);
  connect(localeGroup, &QButtonGroup::idClicked, this, &ConfigWidget::slotSetModified);
  connect(localeGroup, &QButtonGroup::idClicked, this, &ConfigWidget::slotLocaleChanged);
  connect(m_customLocaleEdit, &QLineEdit::textChanged, this, &ConfigWidget::slotSetModified);

  l->setRowStretch(++row, 10);

  // now add additional fields widget
  addFieldsWidget(IMDBFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
  KAcceleratorManager::manage(optionsWidget());

  if(fetcher_) {
    m_numCast->setValue(fetcher_->m_numCast);
    m_imageCombo->setCurrentData(fetcher_->m_imageSize);
    if(fetcher_->m_useSystemLocale) {
      m_systemLocaleRadioButton->setChecked(true);
      m_customLocaleEdit->setText(QLocale().name());
    } else {
      m_customLocaleRadioButton->setChecked(true);
      m_customLocaleEdit->setEnabled(true);
      m_customLocaleEdit->setText(fetcher_->m_customLocale);
    }
  } else { //defaults
    m_imageCombo->setCurrentData(MediumImage);
  }
}

void IMDBFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  config_.deleteEntry("Host"); // clear old host entry
  config_.writeEntry("Max Cast", m_numCast->value());
  config_.deleteEntry("Fetch Images"); // no longer used
  const int n = m_imageCombo->currentData().toInt();
  config_.writeEntry("Image Size", n);
  config_.deleteEntry("Lang"); // no longer used
  config_.writeEntry("System Locale", m_systemLocaleRadioButton->isChecked());
  config_.writeEntry("Custom Locale", m_customLocaleRadioButton->isChecked() ?
                                        m_customLocaleEdit->text().trimmed() :
                                        QString());
}

QString IMDBFetcher::ConfigWidget::preferredName() const {
  return i18n("Internet Movie Database");
}

void IMDBFetcher::ConfigWidget::slotLocaleChanged(int id_) {
  // id 0 is system locale, 1 is custom locale
  m_customLocaleEdit->setEnabled(id_ == 1);
}
