/***************************************************************************
    Copyright (C) 2019 Robby Stephenson <robby@periapsis.org>
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

#include "mobygamesfetcher.h"
#include "../collections/gamecollection.h"
#include "../images/imagefactory.h"
#include "../core/filehandler.h"
#include "../utils/guiproxy.h"
#include "../utils/string_utils.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>
#include <KJob>
#include <KJobUiDelegate>
#include <KJobWidgets/KJobWidgets>
#include <KIO/StoredTransferJob>

#include <QUrl>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QTextCodec>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>
#include <QThread>

namespace {
  static const int MOBYGAMES_MAX_RETURNS_TOTAL = 10;
  static const char* MOBYGAMES_API_URL = "https://api.mobygames.com/v1";
}

using namespace Tellico;
using Tellico::Fetch::MobyGamesFetcher;

MobyGamesFetcher::MobyGamesFetcher(QObject* parent_)
    : Fetcher(parent_), m_started(false) {
  //  setLimit(MOBYGAMES_MAX_RETURNS_TOTAL);
  if(m_esrbHash.isEmpty()) {
    populateHashes();
  }
}

MobyGamesFetcher::~MobyGamesFetcher() {
}

QString MobyGamesFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

QString MobyGamesFetcher::attribution() const {
  return i18n("This information was freely provided by <a href=\"http://mobygames.com\">MobyGames</a>.");
}

bool MobyGamesFetcher::canSearch(FetchKey k) const {
  return k == Title;
}

bool MobyGamesFetcher::canFetch(int type) const {
  return type == Data::Collection::Game;
}

void MobyGamesFetcher::readConfigHook(const KConfigGroup& config_) {
  QString k = config_.readEntry("API Key");
  if(!k.isEmpty()) {
    m_apiKey = k;
  }
}

void MobyGamesFetcher::search() {
  continueSearch();
}

void MobyGamesFetcher::continueSearch() {
  m_started = true;

  if(m_apiKey.isEmpty()) {
    myDebug() << "empty API key";
    message(i18n("An access key is required to use this data source.")
            + QLatin1Char(' ') +
            i18n("Those values must be entered in the data source settings."), MessageHandler::Error);
    stop();
    return;
  }

  QUrl u(QString::fromLatin1(MOBYGAMES_API_URL));
  u.setPath(u.path() + QLatin1String("/games"));

  QUrlQuery q;
  switch(request().key) {
    case Title:
      q.addQueryItem(QStringLiteral("title"), request().value);
      break;

    default:
      myWarning() << "key not recognized:" << request().key;
      stop();
      return;
  }
  q.addQueryItem(QStringLiteral("api_key"), m_apiKey);
  q.addQueryItem(QStringLiteral("limit"), QString::number(MOBYGAMES_MAX_RETURNS_TOTAL));
  q.addQueryItem(QStringLiteral("format"), QStringLiteral("normal"));
  u.setQuery(q);
//  u = QUrl::fromLocalFile(QStringLiteral("/home/robby/games.json"));
  myDebug() << u;

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job, SIGNAL(result(KJob*)), SLOT(slotComplete(KJob*)));
}

void MobyGamesFetcher::stop() {
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

Tellico::Data::EntryPtr MobyGamesFetcher::fetchEntryHook(uint uid_) {
  if(!m_entries.contains(uid_)) {
    myDebug() << "no entry ptr";
    return Data::EntryPtr();
  }

  Data::EntryPtr entry = m_entries.value(uid_);

  QUrl u(QString::fromLatin1(MOBYGAMES_API_URL));
  u.setPath(u.path() + QString::fromLatin1("/games/%1/platforms/%2")
                       .arg(entry->field(QStringLiteral("moby-id")),
                            entry->field(QStringLiteral("platform-id"))));
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("api_key"), m_apiKey);
  u.setQuery(q);
//  myDebug() << u;

  // need to wait a bit after previous query
  QThread::msleep(300);
  QPointer<KIO::StoredTransferJob> job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(job, GUI::Proxy::widget());
  if(!job->exec()) {
    myDebug() << job->errorString() << u;
    return entry;
  }
  QByteArray data = job->data();
  if(data.isEmpty()) {
    myDebug() << "no data for" << u;
    return entry;
  }
#if 0
  myWarning() << "Remove company debug from mobygamesfetcher.cpp";
  QFile file(QStringLiteral("/tmp/moby-platforms.json"));
  if(file.open(QIODevice::WriteOnly)) {
    QTextStream t(&file);
    t.setCodec("UTF-8");
    t << data;
  }
  file.close();
#endif

  QJsonDocument doc = QJsonDocument::fromJson(data);
  QVariantMap map = doc.object().toVariantMap();
  foreach(const QVariant& rating, map.value(QStringLiteral("ratings")).toList()) {
    const QVariantMap ratingMap = rating.toMap();
    const QString ratingSystem = ratingMap.value(QStringLiteral("rating_system_name")).toString();
    if(ratingSystem == QStringLiteral("PEGI Rating")) {
      QString rating = ratingMap.value(QStringLiteral("rating_name")).toString();
      if(!rating.startsWith(QStringLiteral("PEGI"))) {
        rating.prepend(QStringLiteral("PEGI "));
      }
      entry->setField(QStringLiteral("pegi"), rating);
    } else if(ratingSystem == QStringLiteral("ESRB Rating")) {
      const int esrb = ratingMap.value(QStringLiteral("rating_id")).toInt();
      if(m_esrbHash.contains(esrb)) {
        entry->setField(QStringLiteral("certification"), m_esrbHash.value(esrb));
      }
    }
  }
  // just use the first release
  const QVariantList releaseList = map.value(QStringLiteral("releases")).toList();
  if(!releaseList.isEmpty()) {
    const QVariantMap releaseMap = releaseList.at(0).toMap();
    QStringList pubs, devs;
    foreach(const QVariant& company, releaseMap.value(QStringLiteral("companies")).toList()) {
      const QVariantMap companyMap = company.toMap();
      if(companyMap.value(QStringLiteral("role")) == QStringLiteral("Developed by")) {
        devs += companyMap.value(QStringLiteral("company_name")).toString();
      } else if(companyMap.value(QStringLiteral("role")) == QStringLiteral("Published by")) {
        pubs += companyMap.value(QStringLiteral("company_name")).toString();
      }
    }
//    myDebug() << pubs << devs;
    entry->setField(QStringLiteral("publisher"), pubs.join(FieldFormat::delimiterString()));
    entry->setField(QStringLiteral("developer"), devs.join(FieldFormat::delimiterString()));
  }

  // check for empty cover
  const QString image_id = entry->field(QStringLiteral("cover"));
  if(!image_id.isEmpty()) {
    return entry;
  }

  u = QUrl(QString::fromLatin1(MOBYGAMES_API_URL));
  u.setPath(u.path() + QString::fromLatin1("/games/%1/platforms/%2/covers")
                       .arg(entry->field(QStringLiteral("moby-id")),
                            entry->field(QStringLiteral("platform-id"))));
  u.setQuery(q);
//  myDebug() << u;

  // need to wait a bit after previous query
  QThread::msleep(1000);
  job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(job, GUI::Proxy::widget());
  if(!job->exec()) {
    myDebug() << job->errorString() << u;
    return entry;
  }
  data = job->data();
  if(data.isEmpty()) {
    myDebug() << "no data for" << u;
    return entry;
  }
#if 0
  myWarning() << "Remove covers debug from mobygamesfetcher.cpp";
  QFile file2(QStringLiteral("/tmp/moby-covers.json"));
  if(file2.open(QIODevice::WriteOnly)) {
    QTextStream t(&file2);
    t.setCodec("UTF-8");
    t << data;
  }
  file2.close();
#endif

  QString coverUrl;
  doc = QJsonDocument::fromJson(data);
  map = doc.object().toVariantMap();
  QVariantList coverGroupList = map.value(QStringLiteral("cover_groups")).toList();
  foreach(const QVariant& coverGroup, coverGroupList) {
    // just take the first cover
    QVariantList coverList = coverGroup.toMap().value(QStringLiteral("covers")).toList();
    // take the cover that is a scan of "Media" unless there is none
    bool foundCover = false;
    foreach(const QVariant& coverVariant, coverList) {
      const QVariantMap coverMap = coverVariant.toMap();
      if(coverMap.value(QStringLiteral("scan_of")) == QStringLiteral("Media") ||
         coverMap.value(QStringLiteral("scan_of")) == QStringLiteral("Front Cover")) {
        coverUrl = coverMap.value(QStringLiteral("thumbnail_image")).toString();
        foundCover = true;
        break;
      }
    }
    if(foundCover) {
      // no need to continue iteration through cover groups
      break;
    }
  }

  if(!coverUrl.isEmpty()) {
//    myDebug() << coverUrl;
    const QString id = ImageFactory::addImage(QUrl::fromUserInput(coverUrl), true /* quiet */);
    if(id.isEmpty()) {
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    }
    // empty image ID is ok
    entry->setField(QStringLiteral("cover"), id);
  }

  // clear the placeholder fields
  entry->setField(QStringLiteral("moby-id"), QString());
  entry->setField(QStringLiteral("platform-id"), QString());
  return entry;
}

Tellico::Fetch::FetchRequest MobyGamesFetcher::updateRequest(Data::EntryPtr entry_) {
  QString title = entry_->field(QStringLiteral("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }
  return FetchRequest();
}

void MobyGamesFetcher::slotComplete(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);

  if(job->error()) {
    job->uiDelegate()->showErrorMessage();
    stop();
    return;
  }

  const QByteArray data = job->data();
  if(data.isEmpty()) {
    myDebug() << "no data";
    stop();
    return;
  }
  // see bug 319662. If fetcher is cancelled, job is killed
  // if the pointer is retained, it gets double-deleted
  m_job = nullptr;

#if 0
  myWarning() << "Remove debug from mobygamesfetcher.cpp";
  QFile file(QStringLiteral("/tmp/test.json"));
  if(file.open(QIODevice::WriteOnly)) {
    QTextStream t(&file);
    t.setCodec("UTF-8");
    t << data;
  }
  file.close();
#endif

  Data::CollPtr coll(new Data::GameCollection(true));
  if(optionalFields().contains(QStringLiteral("pegi"))) {
    QStringList pegi = QStringLiteral("PEGI 3, PEGI 7, PEGI 12, PEGI 16, PEGI 18")
                                    .split(QRegExp(QStringLiteral("\\s*,\\s*")), QString::SkipEmptyParts);
    Data::FieldPtr field(new Data::Field(QStringLiteral("pegi"), i18n("PEGI Rating"), pegi));
    field->setFlags(Data::Field::AllowGrouped);
    field->setCategory(i18n("General"));
    coll->addField(field);
  }
  if(optionalFields().contains(QStringLiteral("mobygames"))) {
    Data::FieldPtr field(new Data::Field(QStringLiteral("mobygames"), i18n("MobyGames Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }
  // placeholder for mobygames id, to be removed later
  Data::FieldPtr f1(new Data::Field(QStringLiteral("moby-id"), QString(), Data::Field::Number));
  coll->addField(f1);
  Data::FieldPtr f2(new Data::Field(QStringLiteral("platform-id"), QString(), Data::Field::Number));
  coll->addField(f2);

  QJsonDocument doc = QJsonDocument::fromJson(data);
  QVariantMap map = doc.object().toVariantMap();
  foreach(const QVariant& result, map.value(QStringLiteral("games")).toList()) {
    QVariantMap resultMap = result.toMap();
    Data::EntryList entries = createEntries(coll, resultMap);
    foreach(const Data::EntryPtr& entry, entries) {
      FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
      m_entries.insert(r->uid, entry);
      emit signalResultFound(r);
    }
  }

  stop();
}

Tellico::Data::EntryList MobyGamesFetcher::createEntries(Data::CollPtr coll_, const QVariantMap& resultMap_) {
  Data::EntryPtr entry(new Data::Entry(coll_));
  entry->setField(QStringLiteral("title"), mapValue(resultMap_, "title"));
  entry->setField(QStringLiteral("description"), mapValue(resultMap_, "description"));
  entry->setField(QStringLiteral("moby-id"), mapValue(resultMap_, "game_id"));

  QStringList genres;
  foreach(const QVariant& genreMap, resultMap_.value(QStringLiteral("genres")).toList()) {
    QString g = genreMap.toMap().value(QStringLiteral("genre_name")).toString();
    if(!g.isEmpty()) {
      genres << g;
    }
  }
  entry->setField(QStringLiteral("genre"), genres.join(FieldFormat::delimiterString()));

  if(optionalFields().contains(QLatin1String("mobygames"))) {
    entry->setField(QStringLiteral("mobygames"), mapValue(resultMap_, "moby_url"));
  }

  Data::EntryList entries;
  // return a new entry for every platform
  foreach(const QVariant& platformMapV, resultMap_.value(QStringLiteral("platforms")).toList()) {
    Data::EntryPtr newEntry(new Data::Entry(*entry));

    const QVariantMap platformMap = platformMapV.toMap();
    const QString platform = platformMap.value(QStringLiteral("platform_name")).toString();
    if(platform == QStringLiteral("Wii")) {
      newEntry->setField(QStringLiteral("platform"), i18n("Nintendo Wii"));
    } else if(platform == QStringLiteral("Wii U")) {
      // TODO:: add WII U to defaults?
      newEntry->setField(QStringLiteral("platform"), i18n("Nintendo Wii"));
    } else if(platform == QStringLiteral("Nintendo GameCube")) {
      newEntry->setField(QStringLiteral("platform"), i18n("GameCube"));
    } else {
      // also make the assumption that if the platform name isn't already in the allowed list, it should be added
      Data::FieldPtr f = coll_->fieldByName(QStringLiteral("platform"));
      if(f && !f->allowed().contains(platform)) {
        f->setAllowed(QStringList(f->allowed()) << platform);
      }
      newEntry->setField(QStringLiteral("platform"), platform);
    }

    newEntry->setField(QStringLiteral("platform-id"),
                       platformMap.value(QStringLiteral("platform_id")).toString());
    newEntry->setField(QStringLiteral("year"),
                       platformMap.value(QStringLiteral("first_release_date")).toString().left(4));
    entries << newEntry;
  }
  return entries;
}

void MobyGamesFetcher::populateHashes() {
  // cheat by grabbing i18n values from default collection
  Data::CollPtr c(new Data::GameCollection(true));
  QStringList esrb = c->fieldByName(QStringLiteral("certification"))->allowed();
  Q_ASSERT(esrb.size() == 8);
  while(esrb.size() < 8) {
    esrb << QString();
  }
  // 89 == EC
  // 90 == Everyone
  // etc.
  // 95 == Pending
  // https://www.mobygames.com/attribute/sheet/attributeId,89
  m_esrbHash.insert(89, esrb.at(6));
  m_esrbHash.insert(90, esrb.at(5));
  m_esrbHash.insert(91, esrb.at(4));
  m_esrbHash.insert(92, esrb.at(3));
  m_esrbHash.insert(93, esrb.at(2));
  m_esrbHash.insert(94, esrb.at(1));
  m_esrbHash.insert(95, esrb.at(7));
}

Tellico::Fetch::ConfigWidget* MobyGamesFetcher::configWidget(QWidget* parent_) const {
  return new MobyGamesFetcher::ConfigWidget(parent_, this);
}

QString MobyGamesFetcher::defaultName() {
  return QStringLiteral("MobyGames");
}

QString MobyGamesFetcher::defaultIcon() {
  return favIcon("http://www.mobygames.com");
}

Tellico::StringHash MobyGamesFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("pegi")] = i18n("PEGI Rating");
  hash[QStringLiteral("mobygames")] = i18n("MobyGames Link");
  return hash;
}

MobyGamesFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const MobyGamesFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;

  QLabel* al = new QLabel(i18n("Registration is required for accessing the %1 data source. "
                               "If you agree to the terms and conditions, <a href='%2'>sign "
                               "up for an account</a>, and enter your information below.",
                                MobyGamesFetcher::defaultName(),
                                QLatin1String("https://www.mobygames.com/info/api")),
                          optionsWidget());
  al->setOpenExternalLinks(true);
  al->setWordWrap(true);
  ++row;
  l->addWidget(al, row, 0, 1, 2);
  // richtext gets weird with size
  al->setMinimumWidth(al->sizeHint().width());

  QLabel* label = new QLabel(i18n("Access key: "), optionsWidget());
  l->addWidget(label, ++row, 0);

  m_apiKeyEdit = new QLineEdit(optionsWidget());
  connect(m_apiKeyEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_apiKeyEdit, row, 1);
  label->setBuddy(m_apiKeyEdit);

  l->setRowStretch(++row, 10);

  // now add additional fields widget
  addFieldsWidget(MobyGamesFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());

  if(fetcher_) {
    m_apiKeyEdit->setText(fetcher_->m_apiKey);
  }
}

void MobyGamesFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  const QString apiKey = m_apiKeyEdit->text().trimmed();
  if(!apiKey.isEmpty()) {
    config_.writeEntry("API Key", apiKey);
  }
}

QString MobyGamesFetcher::ConfigWidget::preferredName() const {
  return MobyGamesFetcher::defaultName();
}
