/***************************************************************************
    Copyright (C) 2012-2013 Robby Stephenson <robby@periapsis.org>
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

#include "allocinefetcher.h"
#include "../collections/videocollection.h"
#include "../images/imagefactory.h"
#include "../entry.h"
#include "../utils/guiproxy.h"
#include "../utils/string_utils.h"
#include "../core/filehandler.h"
#include "../tellico_debug.h"

#include <KIO/Job>
#include <KIO/JobUiDelegate>
#include <KLocalizedString>
#include <KJobWidgets/KJobWidgets>
#include <KConfigGroup>

#include <QSpinBox>
#include <QUrl>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QTextCodec>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>

namespace {
  static const char* ALLOCINE_API_KEY = "100043982026";
  static const char* ALLOCINE_API_URL = "http://api.allocine.fr/rest/v3/";
  static const char* ALLOCINE_PARTNER_KEY = "29d185d98c984a359e6e6f26a0474269";
}

using namespace Tellico;
using Tellico::Fetch::AbstractAllocineFetcher;
using Tellico::Fetch::AllocineFetcher;

AbstractAllocineFetcher::AbstractAllocineFetcher(QObject* parent_, const QString& baseUrl_)
    : Fetcher(parent_)
    , m_started(false)
    , m_apiKey(QLatin1String(ALLOCINE_API_KEY))
    , m_baseUrl(baseUrl_)
    , m_numCast(10) {
  Q_ASSERT(!m_baseUrl.isEmpty());
}

AbstractAllocineFetcher::~AbstractAllocineFetcher() {
}

bool AbstractAllocineFetcher::canSearch(FetchKey k) const {
  return k == Keyword;
}

bool AbstractAllocineFetcher::canFetch(int type) const {
  return type == Data::Collection::Video;
}

void AbstractAllocineFetcher::readConfigHook(const KConfigGroup& config_) {
  QString k = config_.readEntry("API Key", ALLOCINE_API_KEY);
  if(!k.isEmpty()) {
    m_apiKey = k;
  }
  m_numCast = config_.readEntry("Max Cast", 10);
}

void AbstractAllocineFetcher::search() {
  m_started = true;

  QUrl u(m_baseUrl);
  u = u.adjusted(QUrl::StripTrailingSlash);
  u.setPath(u.path() + QLatin1Char('/') + QLatin1String("search"));
//  myDebug() << u;

  // the order of the parameters appears to matter
  QList<QPair<QString, QString> > params;
  params.append(qMakePair(QStringLiteral("partner"), m_apiKey));

  // I can't figure out how to encode accent marks, but they don't
  // seem to be necessary
  QString q = removeAccents(request().value);
  // should I just remove all non alphabetical characters?
  // see https://bugs.kde.org/show_bug.cgi?id=337432
  q.remove(QRegExp(QLatin1String("[,:!?;\\(\\)]")));
  q.replace(QLatin1Char('\''), QLatin1Char('+'));
  q.replace(QLatin1Char(' '), QLatin1Char('+'));

  switch(request().key) {
    case Keyword:
      params.append(qMakePair(QStringLiteral("q"), q));
      break;

    default:
      myWarning() << "key not recognized: " << request().key;
      return;
  }

  params.append(qMakePair(QStringLiteral("format"), QStringLiteral("json")));
  params.append(qMakePair(QStringLiteral("filter"), QStringLiteral("movie")));

  const QString sed = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd"));
  params.append(qMakePair(QStringLiteral("sed"), sed));

  const QByteArray sig = calculateSignature(params);

  QUrlQuery query;
  query.setQueryItems(params);
  query.addQueryItem(QStringLiteral("sig"), QLatin1String(sig));
  u.setQuery(query);
//  myDebug() << u;

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  // 10/8/17: UserAgent appears necessary to receive data
  m_job->addMetaData(QStringLiteral("UserAgent"), QStringLiteral("Tellico/%1")
                                                                .arg(QLatin1String(TELLICO_VERSION)));
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job, SIGNAL(result(KJob*)), SLOT(slotComplete(KJob*)));
}

void AbstractAllocineFetcher::stop() {
  if(!m_started) {
    return;
  }
  if(m_job) {
    m_job->kill();
  }
  m_started = false;
  emit signalDone(this);
}

Tellico::Data::EntryPtr AbstractAllocineFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }

  QString code = entry->field(QStringLiteral("allocine-code"));
  if(code.isEmpty()) {
    // could mean we already updated the entry
    myDebug() << "no allocine release found";
    return entry;
  }

  QUrl u(m_baseUrl);
  u = u.adjusted(QUrl::StripTrailingSlash);
  u.setPath(u.path() + QLatin1Char('/') + QLatin1String("movie"));

  // the order of the parameters appears to matter
  QList<QPair<QString, QString> > params;
  params.append(qMakePair(QStringLiteral("partner"), m_apiKey));
  params.append(qMakePair(QStringLiteral("code"), code));
  params.append(qMakePair(QStringLiteral("profile"), QStringLiteral("large")));
  params.append(qMakePair(QStringLiteral("filter"), QStringLiteral("movie")));
  params.append(qMakePair(QStringLiteral("format"), QStringLiteral("json")));

  const QString sed = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd"));
  params.append(qMakePair(QStringLiteral("sed"), sed));

  const QByteArray sig = calculateSignature(params);

  QUrlQuery query;
  query.setQueryItems(params);
  query.addQueryItem(QStringLiteral("sig"), QLatin1String(sig));
  u.setQuery(query);
//  myDebug() << "url: " << u;
  // 10/8/17: UserAgent appears necessary to receive data
//  QByteArray data = FileHandler::readDataFile(u, true);
  KIO::StoredTransferJob* dataJob = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  dataJob->addMetaData(QStringLiteral("UserAgent"), QStringLiteral("Tellico/%1")
                                                                  .arg(QLatin1String(TELLICO_VERSION)));
  if(!dataJob->exec()) {
    myDebug() << "Failed to load" << u;
    return entry;
  }
  const QByteArray data = dataJob->data();

#if 0
  myWarning() << "Remove debug2 from allocinefetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test2.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << data;
  }
  f.close();
#endif

  QJsonParseError error;
  QJsonDocument doc = QJsonDocument::fromJson(data, &error);
  QVariantMap result = doc.object().toVariantMap().value(QStringLiteral("movie")).toMap();
  if(error.error != QJsonParseError::NoError) {
    myDebug() << "Bad JSON results";
#if 0
    myWarning() << "Remove debug3 from allocinefetcher.cpp";
    QFile f2(QString::fromLatin1("/tmp/test3.json"));
    if(f2.open(QIODevice::WriteOnly)) {
      QTextStream t(&f2);
      t.setCodec("UTF-8");
      t << data;
    }
    f2.close();
#endif
    return entry;
  }
  populateEntry(entry, result);

  // image might still be a URL
  const QString image_id = entry->field(QStringLiteral("cover"));
  if(image_id.contains(QLatin1Char('/'))) {
    const QString id = ImageFactory::addImage(QUrl::fromUserInput(image_id), true /* quiet */);
    if(id.isEmpty()) {
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    }
    // empty image ID is ok
    entry->setField(QStringLiteral("cover"), id);
  }

  // don't want to include id
  entry->collection()->removeField(QStringLiteral("allocine-code"));
  QStringList castRows = FieldFormat::splitTable(entry->field(QStringLiteral("cast")));
  while(castRows.count() > m_numCast) {
    castRows.removeLast();
  }
  entry->setField(QStringLiteral("cast"), castRows.join(FieldFormat::rowDelimiterString()));
  return entry;
}

void AbstractAllocineFetcher::slotComplete(KJob*) {
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
  myWarning() << "Remove debug from allocinefetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << data;
  }
  f.close();
#endif

  QJsonDocument doc = QJsonDocument::fromJson(data);
  QVariantMap result = doc.object().toVariantMap().value(QStringLiteral("feed")).toMap();
//  myDebug() << "total:" << result.value(QLatin1String("totalResults"));

  QVariantList resultList = result.value(QStringLiteral("movie")).toList();
  if(resultList.isEmpty()) {
    myDebug() << "no results";
    stop();
    return;
  }

  foreach(const QVariant& result, resultList) {
  //  myDebug() << "found result:" << result;

    //create a new collection for every result since we end up removing the allocine code field
    // when fetchEntryHook is called. See bug 338389
    Data::EntryPtr entry(new Data::Entry(createCollection()));
    populateEntry(entry, result.toMap());

    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
  }

  m_hasMoreResults = false;
  stop();
}

Tellico::Data::CollPtr AbstractAllocineFetcher::createCollection() const {
  Data::CollPtr coll(new Data::VideoCollection(true));
  // always add the allocine release code for fetchEntryHook
  Data::FieldPtr field(new Data::Field(QStringLiteral("allocine-code"), QStringLiteral("Allocine Code"), Data::Field::Number));
  field->setCategory(i18n("General"));
  coll->addField(field);

  // add new fields
  if(optionalFields().contains(QLatin1String("allocine"))) {
    Data::FieldPtr field(new Data::Field(QStringLiteral("allocine"), i18n("Allocine Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }
  if(optionalFields().contains(QLatin1String("origtitle"))) {
    Data::FieldPtr f(new Data::Field(QStringLiteral("origtitle"), i18n("Original Title")));
    f->setFormatType(FieldFormat::FormatTitle);
    coll->addField(f);
  }

  return coll;
}

void AbstractAllocineFetcher::populateEntry(Data::EntryPtr entry, const QVariantMap& resultMap) {
  if(entry->collection()->hasField(QStringLiteral("allocine-code"))) {
    entry->setField(QStringLiteral("allocine-code"), mapValue(resultMap, "code"));
  }

  entry->setField(QStringLiteral("title"), mapValue(resultMap, "title"));
  if(optionalFields().contains(QLatin1String("origtitle"))) {
    entry->setField(QStringLiteral("origtitle"), mapValue(resultMap, "originalTitle"));
  }
  if(entry->title().isEmpty()) {
    entry->setField(QStringLiteral("title"), mapValue(resultMap,  "originalTitle"));
  }
  entry->setField(QStringLiteral("year"), mapValue(resultMap, "productionYear"));
  entry->setField(QStringLiteral("plot"), mapValue(resultMap, "synopsis"));

  const int runTime = mapValue(resultMap, "runtime").toInt();
  entry->setField(QStringLiteral("running-time"), QString::number(runTime/60));

  const QVariantList castList = resultMap.value(QStringLiteral("castMember")).toList();
  QStringList actors, directors, producers, composers;
  foreach(const QVariant& castVariant, castList) {
    const QVariantMap castMap = castVariant.toMap();
    const int code = mapValue(castMap, "activity", "code").toInt();
    switch(code) {
      case 8001:
        actors << (mapValue(castMap, "person", "name") + FieldFormat::columnDelimiterString() + mapValue(castMap, "role"));
        break;
      case 8002:
        directors << mapValue(castMap, "person", "name");
        break;
      case 8029:
        producers << mapValue(castMap, "person", "name");
        break;
      case 8003:
        composers << mapValue(castMap, "person", "name");
        break;
    }
  }
  entry->setField(QStringLiteral("cast"), actors.join(FieldFormat::rowDelimiterString()));
  entry->setField(QStringLiteral("director"), directors.join(FieldFormat::delimiterString()));
  entry->setField(QStringLiteral("producer"), producers.join(FieldFormat::delimiterString()));
  entry->setField(QStringLiteral("composer"), composers.join(FieldFormat::delimiterString()));

  const QVariantMap releaseMap = resultMap.value(QStringLiteral("release")).toMap();
  entry->setField(QStringLiteral("studio"), mapValue(releaseMap, "distributor", "name"));

  QStringList genres;
  foreach(const QVariant& variant, resultMap.value(QLatin1String("genre")).toList()) {
    genres << i18n(mapValue(variant.toMap(), "$").toUtf8().constData());
  }
  entry->setField(QStringLiteral("genre"), genres.join(FieldFormat::delimiterString()));

  QStringList nats;
  foreach(const QVariant& variant, resultMap.value(QLatin1String("nationality")).toList()) {
    nats << mapValue(variant.toMap(), "$");
  }
  entry->setField(QStringLiteral("nationality"), nats.join(FieldFormat::delimiterString()));

  QStringList langs;
  foreach(const QVariant& variant, resultMap.value(QLatin1String("language")).toList()) {
    langs << mapValue(variant.toMap(), "$");
  }
  entry->setField(QStringLiteral("language"), langs.join(FieldFormat::delimiterString()));

  const QVariantMap colorMap = resultMap.value(QStringLiteral("color")).toMap();
  if(colorMap.value(QStringLiteral("code")) == QLatin1String("12001")) {
    entry->setField(QStringLiteral("color"), i18n("Color"));
  }

  entry->setField(QStringLiteral("cover"), mapValue(resultMap, "poster", "href"));

  if(optionalFields().contains(QStringLiteral("allocine"))) {
    entry->setField(QStringLiteral("allocine"), mapValue(resultMap, "link", "href"));
  }
}

Tellico::Fetch::FetchRequest AbstractAllocineFetcher::updateRequest(Data::EntryPtr entry_) {
  QString title = entry_->field(QStringLiteral("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Keyword, title);
  }
  return FetchRequest();
}

AbstractAllocineFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const AbstractAllocineFetcher* fetcher_)
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
  m_numCast->setValue(10);
  connect(m_numCast, SIGNAL(valueChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_numCast, row, 1);
  QString w = i18n("The list of cast members may include many people. Set the maximum number returned from the search.");
  label->setWhatsThis(w);
  m_numCast->setWhatsThis(w);
  label->setBuddy(m_numCast);

  l->setRowStretch(++row, 10);

  m_numCast->setValue(fetcher_ ? fetcher_->m_numCast : 10);
}

void AbstractAllocineFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  config_.writeEntry("Max Cast", m_numCast->value());
}

QByteArray AbstractAllocineFetcher::calculateSignature(const QList<QPair<QString, QString> >& params_) {
  typedef QPair<QString, QString> StringPair;
  QByteArray queryString;
  foreach(const StringPair& pair, params_) {
    queryString.append(pair.first.toUtf8().toPercentEncoding("+"));
    queryString.append('=');
    queryString.append(pair.second.toUtf8().toPercentEncoding("+"));
    queryString.append('&');
  }
  // remove final '&'
  queryString.chop(1);

  const QByteArray toSign = ALLOCINE_PARTNER_KEY + queryString;
  const QByteArray hash = QCryptographicHash::hash(toSign, QCryptographicHash::Sha1);
  QByteArray sig = hash.toBase64();
  return sig;
}

/**********************************************************************************************/

AllocineFetcher::AllocineFetcher(QObject* parent_)
    : AbstractAllocineFetcher(parent_, QLatin1String(ALLOCINE_API_URL)) {
}

QString AllocineFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

Tellico::Fetch::ConfigWidget* AllocineFetcher::configWidget(QWidget* parent_) const {
  return new AllocineFetcher::ConfigWidget(parent_, this);
}

QString AllocineFetcher::defaultName() {
  return QStringLiteral("AlloCiné.fr");
}

QString AllocineFetcher::defaultIcon() {
  return favIcon("http://www.allocine.fr");
}

Tellico::StringHash AllocineFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("origtitle")] = i18n("Original Title");
  hash[QStringLiteral("allocine")]  = i18n("Allocine Link");
  return hash;
}

AllocineFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const AbstractAllocineFetcher* fetcher_)
    : AbstractAllocineFetcher::ConfigWidget(parent_, fetcher_) {
  // now add additional fields widget
  addFieldsWidget(AllocineFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

QString AllocineFetcher::ConfigWidget::preferredName() const {
  return AllocineFetcher::defaultName();
}
