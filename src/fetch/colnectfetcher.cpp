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

#include "colnectfetcher.h"
#include "../collections/coincollection.h"
#include "../images/imagefactory.h"
#include "../gui/combobox.h"
#include "../utils/guiproxy.h"
#include "../utils/string_utils.h"
#include "../entry.h"
#include "../fieldformat.h"
#include "../core/filehandler.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>
#include <KJob>
#include <KJobUiDelegate>
#include <KJobWidgets/KJobWidgets>
#include <KIO/StoredTransferJob>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QTextCodec>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QRegularExpression>
#include <QStandardPaths>

namespace {
  static const char* COLNECT_API_URL = "https://api.tellico-project.org/colnect";
//  static const char* COLNECT_API_URL = "https://api.colnect.net";
  static const char* COLNECT_IMAGE_URL = "https://i.colnect.net";
}

using namespace Tellico;
using Tellico::Fetch::ColnectFetcher;

ColnectFetcher::ColnectFetcher(QObject* parent_)
    : Fetcher(parent_)
    , m_started(false)
    , m_locale(QStringLiteral("en")) {
}

ColnectFetcher::~ColnectFetcher() {
}

QString ColnectFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

QString ColnectFetcher::attribution() const {
  return QStringLiteral("Catalog information courtesy of Colnect, an online collectors community.");
}

bool ColnectFetcher::canSearch(FetchKey k) const {
  return k == Keyword;
}

bool ColnectFetcher::canFetch(int type) const {
  return type == Data::Collection::Coin;
}

void ColnectFetcher::readConfigHook(const KConfigGroup& config_) {
  QString k = config_.readEntry("Locale", "en");
  if(!k.isEmpty()) {
    m_locale = k.toLower();
  }
  Q_ASSERT_X(m_locale.length() == 2, "ColnectFetcher::readConfigHook", "lang should be 2 char short iso");
}

void ColnectFetcher::search() {
  m_started = true;
  m_year.clear();

  QUrl u(QString::fromLatin1(COLNECT_API_URL));
  // Colnect API calls are encoded as a path
  QString query(QLatin1Char('/') + m_locale);

  QString value = request().value;
  switch(request().key) {
    case Keyword:
      {
        query += QStringLiteral("/list/cat/coins");
        // pull out mint year, keep the regexp a little loose
        QRegularExpression yearRX(QStringLiteral("[0-9]{4}"));
        QRegularExpressionMatch match = yearRX.match(value);
        if(match.hasMatch()) {
          m_year = match.captured(0);
          query += QStringLiteral("/mint_year/") + m_year;
          value = value.remove(yearRX);
        }
      }
      // everything left is for the item description
      query += QStringLiteral("/description/") + value.simplified();
      break;

    case Raw:
      query += QStringLiteral("/item/cat/coins/id/") + value;
      break;

    default:
      myWarning() << "key not recognized:" << request().key;
      stop();
      return;
  }

  u.setPath(u.path() + query);
//  myDebug() << "url:" << u;

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result, this, &ColnectFetcher::slotComplete);
}

void ColnectFetcher::stop() {
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

Tellico::Data::EntryPtr ColnectFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }

  // if there's a colnect-id in the entry, need to fetch all the data
  const QString id = entry->field(QStringLiteral("colnect-id"));
  if(!id.isEmpty()) {
    QUrl u(QString::fromLatin1(COLNECT_API_URL));
    QString query(QLatin1Char('/') + m_locale + QStringLiteral("/item/cat/coins/id/") + id);
    u.setPath(u.path() + query);
//    myDebug() << "Reading item data from url:" << u;

    QPointer<KIO::StoredTransferJob> job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
    KJobWidgets::setWindow(job, GUI::Proxy::widget());
    if(!job->exec()) {
      myDebug() << "Colnect item data:" << job->errorString() << u;
      return entry;
    }
    const QByteArray data = job->data();
    if(data.isEmpty()) {
      myDebug() << "no colnect item data for" << u;
      return entry;
    }
#if 0
    myWarning() << "Remove item debug from colnectfetcher.cpp";
    QFile file(QStringLiteral("/tmp/colnectitemtest.json"));
    if(file.open(QIODevice::WriteOnly)) {
      QTextStream t(&file);
      t.setCodec("UTF-8");
      t << data;
    }
    file.close();
#endif
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
    Q_ASSERT_X(!doc.isNull(), "colnect", jsonError.errorString().toUtf8().constData());
    const QVariantList resultList = doc.array().toVariantList();
    Q_ASSERT_X(!resultList.isEmpty(), "colnect", "no item results");
    Q_ASSERT_X(static_cast<QMetaType::Type>(resultList.at(0).type()) == QMetaType::QString, "colnect",
               "Weird single item result, first value is not a string");
    populateEntry(entry, resultList);
  }

  // image might still be a URL only
  QString image = entry->field(QStringLiteral("obverse"));
  if(image.contains(QLatin1Char('/'))) {
    const QString id = ImageFactory::addImage(QUrl::fromUserInput(image), true /* quiet */);
    if(id.isEmpty()) {
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    }
    // empty image ID is ok
    entry->setField(QStringLiteral("obverse"), id);
  }
  // now the reverse image
  image = entry->field(QStringLiteral("reverse"));
  if(image.contains(QLatin1Char('/'))) {
    const QString id = ImageFactory::addImage(QUrl::fromUserInput(image), true /* quiet */);
    if(id.isEmpty()) {
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    }
    entry->setField(QStringLiteral("reverse"), id);
  }

  // don't want to include id
  entry->setField(QStringLiteral("colnect-id"), QString());
  return entry;
}

Tellico::Fetch::FetchRequest ColnectFetcher::updateRequest(Data::EntryPtr entry_) {
  const QString title = entry_->field(QStringLiteral("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Keyword, title);
  }
  return FetchRequest();
}

void ColnectFetcher::slotComplete(KJob* job_) {
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
  myWarning() << "Remove debug from colnectfetcher.cpp";
  QFile f(QStringLiteral("/tmp/colnecttest.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << data;
  }
  f.close();
#endif

  QJsonParseError jsonError;
  QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
  if(doc.isNull()) {
    myDebug() << "null JSON document:" << jsonError.errorString();
    message(jsonError.errorString(), MessageHandler::Error);
    stop();
    return;
  }
  QVariantList resultList = doc.array().toVariantList();
  if(resultList.isEmpty()) {
    myDebug() << "no results";
    stop();
    return;
  }

  m_hasMoreResults = false; // for now, no continued searches

  Data::CollPtr coll(new Data::CoinCollection(true));
  // placeholder for colnect id, to be removed later
  Data::FieldPtr f1(new Data::Field(QStringLiteral("colnect-id"), QString()));
  coll->addField(f1);

  const QString series(QStringLiteral("series"));
  if(!coll->hasField(series) && optionalFields().contains(series)) {
    Data::FieldPtr field(new Data::Field(series, i18n("Series")));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }

  const QString desc(QStringLiteral("description"));
  if(!coll->hasField(desc) && optionalFields().contains(desc)) {
    Data::FieldPtr field(new Data::Field(desc, i18n("Description"), Data::Field::Para));
    coll->addField(field);
  }

  const QString mintage(QStringLiteral("mintage"));
  if(!coll->hasField(mintage) && optionalFields().contains(mintage)) {
    Data::FieldPtr field(new Data::Field(mintage, i18n("Mintage"), Data::Field::Number));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }

  // if the first item in the array is a string, probably a single item result, possibly from a Raw query
  if(!resultList.isEmpty() &&
     static_cast<QMetaType::Type>(resultList.at(0).type()) == QMetaType::QString) {
    Data::EntryPtr entry(new Data::Entry(coll));
    populateEntry(entry, resultList);

    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);

    stop();
    return;
  }

  // here, we have multiple results to loop through
//  myDebug() << "Reading" << resultList.size() << "results";
  foreach(const QVariant& result, resultList) {
    // be sure to check that the fetcher has not been stopped
    // crashes can occur if not
    if(!m_started) {
      break;
    }

    Data::EntryPtr entry(new Data::Entry(coll));
    //list action - returns array of [item_id,series_id,producer_id,front_picture_id, back_picture_id,item_description,catalog_codes,item_name]
    const QVariantList values = result.toJsonArray().toVariantList();
    entry->setField(QStringLiteral("colnect-id"), values.first().toString());
    entry->setField(QStringLiteral("description"), values.last().toString());
    entry->setField(QStringLiteral("year"), m_year);

    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
  }

  stop();
}

void ColnectFetcher::populateEntry(Data::EntryPtr entry_, const QVariantList& resultList_) {
  if(m_colnectFields.isEmpty()) {
    readDataList();
    // set minimum size of list here
    if(m_colnectFields.count() < 26) {
      return;
    }
  }
  if(resultList_.count() != m_colnectFields.count()) {
    myDebug() << "field count mismatch! Got" << resultList_.count() << ", expected" << m_colnectFields.count();
    return;
  }

  // lookup the field name for the list index
  int idx = m_colnectFields.value(QStringLiteral("Issued on"), -1);

  // the year may have already been set in the query term
  if(m_year.isEmpty() && idx > -1) {
    entry_->setField(QStringLiteral("year"), resultList_.at(idx).toString());
  }

  idx = m_colnectFields.value(QStringLiteral("Country"), -1);
  if(idx > -1) {
    entry_->setField(QStringLiteral("country"), resultList_.at(idx).toString());
  }

  idx = m_colnectFields.value(QStringLiteral("Currency"), -1);
  if(idx > -1) {
    entry_->setField(QStringLiteral("currency"), resultList_.at(idx).toString());
    idx = m_colnectFields.value(QStringLiteral("FaceValue"), -1);
    if(idx > -1) {
      // bad assumption, but go with it. First char is currency symbol
      QString currency = entry_->field(QStringLiteral("currency"));
      if(!currency.isEmpty()) currency.truncate(1);
      const double value = resultList_.at(idx).toDouble();
      entry_->setField(QStringLiteral("denomination"),
                       QLocale::system().toCurrencyString(value, currency));
    }
  }

  idx = m_colnectFields.value(QStringLiteral("Series"), -1);
  static const QString series(QStringLiteral("series"));
  if(idx > -1 && optionalFields().contains(series)) {
    entry_->setField(series, resultList_.at(idx).toString());
  }

  idx = m_colnectFields.value(QStringLiteral("Known mintage"), -1);
  static const QString mintage(QStringLiteral("mintage"));
  if(idx > -1 && optionalFields().contains(mintage)) {
    entry_->setField(mintage, resultList_.at(idx).toString());
  }

  idx = m_colnectFields.value(QStringLiteral("Description"), -1);
  static const QString desc(QStringLiteral("description"));
  if(idx > -1 && optionalFields().contains(desc)) {
    entry_->setField(desc, resultList_.at(idx).toString());
  }

  idx = m_colnectFields.value(QStringLiteral("FrontPicture"), -1);
  if(idx  > -1 && optionalFields().contains(QStringLiteral("obverse"))) {
    entry_->setField(QStringLiteral("obverse"),
                     imageUrl(resultList_.at(0).toString(),
                              resultList_.at(idx).toString()));
  }

  idx = m_colnectFields.value(QStringLiteral("BackPicture"), -1);
  if(idx  > -1 && optionalFields().contains(QStringLiteral("reverse"))) {
    entry_->setField(QStringLiteral("reverse"),
                     imageUrl(resultList_.at(0).toString(),
                              resultList_.at(idx).toString()));
  }
}

Tellico::Fetch::ConfigWidget* ColnectFetcher::configWidget(QWidget* parent_) const {
  return new ColnectFetcher::ConfigWidget(parent_, this);
}

QString ColnectFetcher::defaultName() {
  return QStringLiteral("Colnect"); // no translation
}

QString ColnectFetcher::defaultIcon() {
  return favIcon("https://colnect.com");
}

Tellico::StringHash ColnectFetcher::allOptionalFields() {
  StringHash hash;
  // treat images as optional since Colnect doesn't break out different images for each year
  hash[QStringLiteral("obverse")] = i18n("Obverse");
  hash[QStringLiteral("reverse")] = i18n("Reverse");
  hash[QStringLiteral("series")] = i18n("Series");
  /* TRANSLATORS: Mintage refers to the number of coins minted */
  hash[QStringLiteral("mintage")] = i18n("Mintage");
  hash[QStringLiteral("description")] = i18n("Description");
  return hash;
}

// Colnect specific method of turning name text into a slug
//  $str = html_entity_decode($str, ENT_QUOTES, 'UTF-8');
//  $str = preg_replace('/&[^;]+;/', '_', $str); # change HTML elements to underscore
//  $str = str_replace(array('.', '"', '>', '<', '\\', ':', '/', '?', '#', '[', ']', '@', '!', '$', '&', '\'', '(', ')', '*', '+', ',', ';', '='), '', $str);
//  $str = preg_replace('/[\s_]+/', '_', $str); # any space sequence becomes a single underscore
//  $str = trim($str, '_'); # trim underscores
QString ColnectFetcher::URLize(const QString& name_) {
  QString slug = name_;
  static const QString underscore(QStringLiteral("_"));
  static const QRegularExpression htmlElements(QStringLiteral("&[^;]+;"));
  static const QRegularExpression toRemove(QStringLiteral("[.\"><\\:/?#\\[\\]@!$&'()*+,;=]"));
  static const QRegularExpression spaces(QStringLiteral("\\s"));
  slug.replace(htmlElements, underscore);
  slug.remove(toRemove);
  slug.replace(spaces, underscore);
  while(slug.startsWith(underscore)) slug = slug.mid(1);
  while(slug.endsWith(underscore)) slug.chop(1);
  return slug;
}

QString ColnectFetcher::imageUrl(const QString& name_, const QString& id_) {
  const QString nameSlug = URLize(name_);
  const int id = id_.toInt();
  QUrl u(QString::fromLatin1(COLNECT_IMAGE_URL));
  // uses 't' for thumbnail, use 'f' for full-size
  u.setPath(QString::fromLatin1("/t/%1/%2/%3.jpg")
                           .arg(id / 1000)
                           .arg(id % 1000, 3, 10, QLatin1Char('0'))
                           .arg(nameSlug));
//  myDebug() << "Image url:" << u;
  return u.toString();
}

void ColnectFetcher::readDataList() {
//  myDebug() << "Reading Colnect fields";
  QUrl u(QString::fromLatin1(COLNECT_API_URL));
  // Colnect API calls are encoded as a path
  QString query(QLatin1Char('/') + m_locale + QStringLiteral("/fields/cat/coins/"));
  u.setPath(u.path() + query);

//  myDebug() << "Reading" << u;
  const QByteArray data = FileHandler::readDataFile(u, true);
  QJsonDocument doc = QJsonDocument::fromJson(data);
  if(doc.isNull()) {
    myDebug() << "null JSON document in colnect fields";
    return;
  }
  QVariantList resultList = doc.array().toVariantList();
  if(resultList.isEmpty()) {
    myDebug() << "no colnect field results";
    return;
  }
  m_colnectFields.clear();
  for(int i = 0; i < resultList.size(); ++i) {
    m_colnectFields.insert(resultList.at(i).toString(), i);
//    if(i == 5) myDebug() << m_colnectFields;
  }
//  myDebug() << "Number of Colnect fields:" << m_colnectFields.count();
}

ColnectFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const ColnectFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;

  QLabel* label = new QLabel(i18n("Language: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_langCombo = new GUI::ComboBox(optionsWidget());

#define LANG_ITEM(NAME, CY, ISO) \
  m_langCombo->addItem(QIcon(QStandardPaths::locate(QStandardPaths::GenericDataLocation,                       \
                                                    QStringLiteral("kf5/locale/countries/" CY "/flag.png"))), \
                       i18nc("Language", NAME),                                                                \
                       QLatin1String(ISO));
  LANG_ITEM("English", "us", "en");
  LANG_ITEM("French",  "fr", "fr");
  LANG_ITEM("German",  "de", "de");
  LANG_ITEM("Spanish", "es", "es");
#undef LANG_ITEM

  void (GUI::ComboBox::* activatedInt)(int) = &GUI::ComboBox::activated;
  connect(m_langCombo, activatedInt, this, &ConfigWidget::slotSetModified);
  connect(m_langCombo, activatedInt, this, &ConfigWidget::slotLangChanged);
  l->addWidget(m_langCombo, row, 1);
  label->setBuddy(m_langCombo);

  l->setRowStretch(++row, 10);

  // now add additional fields widget
  addFieldsWidget(ColnectFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());

  if(fetcher_) {
    m_langCombo->setCurrentData(fetcher_->m_locale);
  }
}

void ColnectFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  const QString lang = m_langCombo->currentData().toString();
  config_.writeEntry("Locale", lang);
}

QString ColnectFetcher::ConfigWidget::preferredName() const {
  return QString::fromLatin1("Colnect (%1)").arg(m_langCombo->currentText());
}

void ColnectFetcher::ConfigWidget::slotLangChanged() {
  emit signalName(preferredName());
}
