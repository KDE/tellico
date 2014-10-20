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

#include <config.h>
#include "allocinefetcher.h"
#include "../collections/videocollection.h"
#include "../entry.h"
#include "../gui/guiproxy.h"
#include "../tellico_utils.h"
#include "../core/filehandler.h"
#include "../tellico_debug.h"

#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <KLocale>
#include <KConfigGroup>
#include <KLineEdit>
#include <KIntSpinBox>
#include <KCodecs>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QTextCodec>
#include <QCryptographicHash>

#ifdef HAVE_QJSON
#include <qjson/parser.h>
#endif

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
#ifdef HAVE_QJSON
  return k == Keyword;
#else
  return false;
#endif
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

#ifdef HAVE_QJSON
  KUrl u(m_baseUrl);
  u.addPath(QLatin1String("search"));

  // the order of the parameters appears to matter
  QList<QPair<QString, QString> > params;
  params.append(qMakePair(QString::fromLatin1("partner"), m_apiKey));

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
      params.append(qMakePair(QString::fromLatin1("q"), q));
      break;

    default:
      myWarning() << "key not recognized: " << request().key;
      return;
  }

  params.append(qMakePair(QString::fromLatin1("format"), QString::fromLatin1("json")));
  params.append(qMakePair(QString::fromLatin1("filter"), QString::fromLatin1("movie")));

  const QString sed = QDateTime::currentDateTime().toUTC().toString(QLatin1String("yyyyMMdd"));
  params.append(qMakePair(QString::fromLatin1("sed"), sed));

  const QByteArray sig = calculateSignature(params);

  u.setQueryItems(params);
  u.addQueryItem(QLatin1String("sig"), QLatin1String(sig));
//  myDebug() << "url:" << u;

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  m_job->ui()->setWindow(GUI::Proxy::widget());
  connect(m_job, SIGNAL(result(KJob*)), SLOT(slotComplete(KJob*)));
#else
  stop();
#endif
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

  QString code = entry->field(QLatin1String("allocine-code"));
  if(code.isEmpty()) {
    myWarning() << "no allocine release found";
    return entry;
  }

  KUrl u(m_baseUrl);
  u.addPath(QLatin1String("movie"));

  // the order of the parameters appears to matter
  QList<QPair<QString, QString> > params;
  params.append(qMakePair(QString::fromLatin1("partner"), m_apiKey));
  params.append(qMakePair(QString::fromLatin1("code"), code));
  params.append(qMakePair(QString::fromLatin1("profile"), QString::fromLatin1("large")));
  params.append(qMakePair(QString::fromLatin1("filter"), QString::fromLatin1("movie")));
  params.append(qMakePair(QString::fromLatin1("format"), QString::fromLatin1("json")));

  const QString sed = QDateTime::currentDateTime().toUTC().toString(QLatin1String("yyyyMMdd"));
  params.append(qMakePair(QString::fromLatin1("sed"), sed));

  const QByteArray sig = calculateSignature(params);

  u.setQueryItems(params);
  u.addQueryItem(QLatin1String("sig"), QLatin1String(sig));
//  myDebug() << "url: " << u;
  // quiet
  QByteArray data = FileHandler::readDataFile(u, true);

#if 0
  myWarning() << "Remove debug from allocinefetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test2.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << data;
  }
  f.close();
#endif

#ifdef HAVE_QJSON
  QJson::Parser parser;
  bool ok;
  QVariantMap result = parser.parse(data, &ok).toMap().value(QLatin1String("movie")).toMap();
  if(!ok) {
    myDebug() << "Bad JSON results";
#if 0
    myWarning() << "Remove debug from allocinefetcher.cpp";
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
#endif

  // don't want to include id
  entry->collection()->removeField(QLatin1String("allocine-code"));
  QStringList castRows = FieldFormat::splitTable(entry->field(QLatin1String("cast")));
  while(castRows.count() > m_numCast) {
    castRows.removeLast();
  }
  entry->setField(QLatin1String("cast"), castRows.join(FieldFormat::rowDelimiterString()));
  return entry;
}

void AbstractAllocineFetcher::slotComplete(KJob*) {
#ifdef HAVE_QJSON
//  myDebug();

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
  myWarning() << "Remove debug from allocinefetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << data;
  }
  f.close();
#endif

  Data::CollPtr coll(new Data::VideoCollection(true));
  // always add the gbs-link for fetchEntryHook
  Data::FieldPtr field(new Data::Field(QLatin1String("allocine-code"), QLatin1String("Allocine Code"), Data::Field::URL));
  field->setCategory(i18n("General"));
  coll->addField(field);

  // add new fields
  if(optionalFields().contains(QLatin1String("allocine"))) {
    Data::FieldPtr field(new Data::Field(QLatin1String("allocine"), i18n("Allocine Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }
  if(optionalFields().contains(QLatin1String("origtitle"))) {
    Data::FieldPtr f(new Data::Field(QLatin1String("origtitle"), i18n("Original Title")));
    f->setFormatType(FieldFormat::FormatTitle);
    coll->addField(f);
  }

  QJson::Parser parser;
  bool ok;
  QVariantMap result = parser.parse(data, &ok).toMap().value(QLatin1String("feed")).toMap();
  if(!ok) {
    myDebug() << "Bad JSON results";
  }
//  myDebug() << "total:" << result.value(QLatin1String("totalResults"));

  QVariantList resultList = result.value(QLatin1String("movie")).toList();
  if(resultList.isEmpty()) {
    myDebug() << "no results";
    stop();
    return;
  }

  foreach(const QVariant& result, resultList) {
  //  myDebug() << "found result:" << result;

    Data::EntryPtr entry(new Data::Entry(coll));
    populateEntry(entry, result.toMap());

    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
  }

  m_hasMoreResults = false;
#endif
  stop();
}

void AbstractAllocineFetcher::populateEntry(Data::EntryPtr entry, const QVariantMap& resultMap) {
  if(entry->collection()->hasField(QLatin1String("allocine-code"))) {
    entry->setField(QLatin1String("allocine-code"), value(resultMap, "code"));
  }

  entry->setField(QLatin1String("title"), value(resultMap, "title"));
  if(optionalFields().contains(QLatin1String("origtitle"))) {
    entry->setField(QLatin1String("origtitle"), value(resultMap, "originalTitle"));
  }
  if(entry->title().isEmpty()) {
    entry->setField(QLatin1String("title"), value(resultMap,  "originalTitle"));
  }
  entry->setField(QLatin1String("year"), value(resultMap, "productionYear"));
  entry->setField(QLatin1String("plot"), value(resultMap, "synopsis"));

  const int runTime = value(resultMap, "runtime").toInt();
  entry->setField(QLatin1String("running-time"), QString::number(runTime/60));

  const QVariantList castList = resultMap.value(QLatin1String("castMember")).toList();
  QStringList actors, directors, producers, composers;
  foreach(const QVariant& castVariant, castList) {
    const QVariantMap castMap = castVariant.toMap();
    const int code = value(castMap, "activity", "code").toInt();
    switch(code) {
      case 8001:
        actors << (value(castMap, "person", "name") + FieldFormat::columnDelimiterString() + value(castMap, "role"));
        break;
      case 8002:
        directors << value(castMap, "person", "name");
        break;
      case 8029:
        producers << value(castMap, "person", "name");
        break;
      case 8003:
        composers << value(castMap, "person", "name");
        break;
    }
  }
  entry->setField(QLatin1String("cast"), actors.join(FieldFormat::rowDelimiterString()));
  entry->setField(QLatin1String("director"), directors.join(FieldFormat::delimiterString()));
  entry->setField(QLatin1String("producer"), producers.join(FieldFormat::delimiterString()));
  entry->setField(QLatin1String("composer"), composers.join(FieldFormat::delimiterString()));

  const QVariantMap releaseMap = resultMap.value(QLatin1String("release")).toMap();
  entry->setField(QLatin1String("studio"), value(releaseMap, "distributor", "name"));

  QStringList genres;
  foreach(const QVariant& variant, resultMap.value(QLatin1String("genre")).toList()) {
    genres << i18n(value(variant.toMap(), "$").toUtf8());
  }
  entry->setField(QLatin1String("genre"), genres.join(FieldFormat::delimiterString()));

  QStringList nats;
  foreach(const QVariant& variant, resultMap.value(QLatin1String("nationality")).toList()) {
    nats << value(variant.toMap(), "$");
  }
  entry->setField(QLatin1String("nationality"), nats.join(FieldFormat::delimiterString()));

  QStringList langs;
  foreach(const QVariant& variant, resultMap.value(QLatin1String("language")).toList()) {
    langs << value(variant.toMap(), "$");
  }
  entry->setField(QLatin1String("language"), langs.join(FieldFormat::delimiterString()));

  const QVariantMap colorMap = resultMap.value(QLatin1String("color")).toMap();
  if(colorMap.value(QLatin1String("code")) == QLatin1String("12001")) {
    entry->setField(QLatin1String("color"), i18n("Color"));
  }

  entry->setField(QLatin1String("cover"), value(resultMap, "poster", "href"));

  if(optionalFields().contains(QLatin1String("allocine"))) {
    entry->setField(QLatin1String("allocine"), value(resultMap, "link", "href"));
  }
}

Tellico::Fetch::FetchRequest AbstractAllocineFetcher::updateRequest(Data::EntryPtr entry_) {
  QString title = entry_->field(QLatin1String("title"));
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
  m_numCast = new KIntSpinBox(0, 99, 1, 10, optionsWidget());
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

// static
QString AbstractAllocineFetcher::value(const QVariantMap& map, const char* name) {
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

QString AbstractAllocineFetcher::value(const QVariantMap& map, const char* object, const char* name) {
  const QVariant v = map.value(QLatin1String(object));
  if(v.isNull())  {
    return QString();
  } else if(v.canConvert(QVariant::Map)) {
    return value(v.toMap(), name);
  } else if(v.canConvert(QVariant::List)) {
    QVariantList list = v.toList();
    return list.isEmpty() ? QString() : value(list.at(0).toMap(), name);
  } else {
    return QString();
  }
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
  const QByteArray sig = KCodecs::base64Encode(hash);
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
  return QString::fromUtf8("AlloCiné.fr");
}

QString AllocineFetcher::defaultIcon() {
  return favIcon("http://www.allocine.fr");
}

Tellico::StringHash AllocineFetcher::allOptionalFields() {
  StringHash hash;
  hash[QLatin1String("origtitle")] = i18n("Original Title");
  hash[QLatin1String("allocine")]  = i18n("Allocine Link");
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

#include "allocinefetcher.moc"
