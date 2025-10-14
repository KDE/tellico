/***************************************************************************
    Copyright (C) 2020 Robby Stephenson <robby@periapsis.org>
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

#include "numistafetcher.h"
#include "../collections/coincollection.h"
#include "../entry.h"
#include "../images/imagefactory.h"
#include "../gui/combobox.h"
#include "../utils/guiproxy.h"
#include "../utils/string_utils.h"
#include "../utils/objvalue.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KIO/StoredTransferJob>
#include <KJobUiDelegate>
#include <KJobWidgets>
#include <KConfigGroup>
#include <KCountryFlagEmojiIconEngine>
#include <KLanguageName>

#include <QLabel>
#include <QLineEdit>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

namespace {
  static const int NUMISTA_MAX_RETURNS_TOTAL = 20;
  static const char* NUMISTA_API_URL = "https://api.numista.com/api/v1";
  static const char* NUMISTA_MAGIC_TOKEN = "2e19b8f32c5e8fbd96aeb2c0590d70458ef81d5b0657b1f6741685e1f9cf7a0983d7d0e0a2c69bcca7cfb4c08fde1c5a562e083e2d44a492a5e4b9c3d2a42a7c536a99f8511bfdbca9fb6d29f587fbbf";
}

using namespace Tellico;
using Tellico::Fetch::NumistaFetcher;

NumistaFetcher::NumistaFetcher(QObject* parent_)
    : Fetcher(parent_)
    , m_limit(NUMISTA_MAX_RETURNS_TOTAL)
    , m_total(-1)
    , m_page(1)
    , m_job(nullptr)
    , m_locale(QStringLiteral("en"))
    , m_started(false) {
}

NumistaFetcher::~NumistaFetcher() {
}

QString NumistaFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool NumistaFetcher::canFetch(int type) const {
  return type == Data::Collection::Coin;
}

void NumistaFetcher::readConfigHook(const KConfigGroup& config_) {
  QString k = config_.readEntry("API Key");
  if(!k.isEmpty()) {
    m_apiKey = k;
  }
  k = config_.readEntry("Locale", "en");
  if(!k.isEmpty()) {
    m_locale = k.toLower();
  }
}

void NumistaFetcher::setLimit(int limit_) {
  m_limit = qBound(1, limit_, NUMISTA_MAX_RETURNS_TOTAL);
}

void NumistaFetcher::search() {
  m_started = true;
  m_total = -1;
  m_page = 1;
  m_year.clear();
  doSearch();
}

void NumistaFetcher::continueSearch() {
  m_started = true;
  m_page++;
  doSearch();
}

void NumistaFetcher::doSearch() {
  QUrl u(QString::fromLatin1(NUMISTA_API_URL));
  // all searches are for coins
  u.setPath(u.path() + QStringLiteral("/coins"));

  if(m_apiKey.isEmpty()) {
    m_apiKey = Tellico::reverseObfuscate(NUMISTA_MAGIC_TOKEN);
  }

  // pull out year, keep the regexp a little loose
  QRegularExpression yearRX(QStringLiteral("[0-9]{4}"));
  QRegularExpressionMatch match = yearRX.match(request().value());
  if(match.hasMatch()) {
    m_year = match.captured(0);
  }

  QString queryString;
  switch(request().key()) {
    case Keyword:
      queryString = request().value();
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      stop();
      return;
  }
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("q"), queryString);
  q.addQueryItem(QStringLiteral("count"), QString::number(m_limit));
  q.addQueryItem(QStringLiteral("page"), QString::number(m_page));
  q.addQueryItem(QStringLiteral("lang"), m_locale);
  u.setQuery(q);
//  myDebug() << "url: " << u.url();

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  m_job->addMetaData(QStringLiteral("customHTTPHeader"), QStringLiteral("Numista-API-Key: ") + m_apiKey);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result,
          this, &NumistaFetcher::slotComplete);
}

void NumistaFetcher::stop() {
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

void NumistaFetcher::slotComplete(KJob* ) {
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
  myWarning() << "Remove debug from numistafetcher.cpp";
  QFile f(QStringLiteral("/tmp/test.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
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
  QJsonObject obj = doc.object();

  // check for error
  if(obj.contains(QStringLiteral("error"))) {
    const QString msg = obj.value(QStringLiteral("error")).toString();
    message(msg, MessageHandler::Error);
    myDebug() << "NumistaFetcher -" << msg;
    stop();
    return;
  }

  m_total = obj.value(QLatin1String("count")).toInt();
  m_hasMoreResults = m_total > m_page*m_limit;

  int count = 0;
  QJsonArray results = obj.value(QLatin1String("coins")).toArray();
  for(QJsonArray::const_iterator i = results.constBegin(); i != results.constEnd(); ++i) {
    if(count >= m_limit) {
      break;
    }
    QJsonObject result = (*i).toObject();

    QString desc = result.value(QLatin1String("issuer")).toObject()
                         .value(QLatin1String("name")).toString();
    const QString minYear = result.value(QLatin1String("minYear")).toString();
    if(!minYear.isEmpty()) {
      desc += QLatin1Char('/') + minYear + QLatin1Char('-') + result.value(QLatin1String("maxYear")).toString();
    }
    QString title = result.value(QLatin1String("title")).toString();
    // some results include &quot;
    title.replace(QLatin1String("&quot;"), QLatin1String("\""));
    FetchResult* r = new FetchResult(this, title, desc);
    m_matches.insert(r->uid, result.value(QLatin1String("id")).toInt());
    Q_EMIT signalResultFound(r);
    ++count;
  }

  stop(); // required
}

Tellico::Data::EntryPtr NumistaFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(entry) {
    return entry;
  }

  if(!m_matches.contains(uid_)) {
    myWarning() << "no matching coin id";
    return Data::EntryPtr();
  }

  QUrl url(QString::fromLatin1(NUMISTA_API_URL));
  url.setPath(url.path() + QLatin1String("/coins/") + QString::number(m_matches[uid_]));
//  myDebug() << url.url();
  QPointer<KIO::StoredTransferJob> job = KIO::storedGet(url, KIO::NoReload, KIO::HideProgressInfo);
  job->addMetaData(QStringLiteral("customHTTPHeader"), QStringLiteral("Numista-API-Key: ") + m_apiKey);
  KJobWidgets::setWindow(job, GUI::Proxy::widget());
  if(!job->exec()) {
    myDebug() << job->errorString() << url;
    return Data::EntryPtr();
  }
  const QByteArray data = job->data();
  if(data.isEmpty()) {
    myDebug() << "no data for" << url;
    return Data::EntryPtr();
  }
#if 0
  myWarning() << "Remove debug2 from numistafetcher.cpp";
  QFile f(QStringLiteral("/tmp/test2-numista.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << data;
  }
  f.close();
#endif

  entry = parseEntry(data);
  if(!entry) {
    myDebug() << "No discernible entry data";
    return Data::EntryPtr();
  }

  QString image = entry->field(QStringLiteral("obverse"));
  if(!image.isEmpty() && optionalFields().contains(QStringLiteral("obverse"))) {
    const QString id = ImageFactory::addImage(QUrl::fromUserInput(image), true /* quiet */);
    if(id.isEmpty()) {
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    }
    entry->setField(QStringLiteral("obverse"), id);
  }
  image = entry->field(QStringLiteral("reverse"));
  if(!image.isEmpty() && optionalFields().contains(QStringLiteral("reverse"))) {
    const QString id = ImageFactory::addImage(QUrl::fromUserInput(image), true /* quiet */);
    if(id.isEmpty()) {
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    }
    entry->setField(QStringLiteral("reverse"), id);
  }

  return entry;
}

Tellico::Data::EntryPtr NumistaFetcher::parseEntry(const QByteArray& data_) {
  QJsonParseError parseError;
  QJsonDocument doc = QJsonDocument::fromJson(data_, &parseError);
  if(doc.isNull()) {
    myDebug() << "Bad json data:" << parseError.errorString();
    return Data::EntryPtr();
  }

  Data::CollPtr coll(new Data::CoinCollection(true));
  Data::EntryPtr entry(new Data::Entry(coll));
  coll->addEntries(entry);

  const auto obj = doc.object();
  // for type, try to tease out from title
  // use ruler name as a possible fallback
  QRegularExpression titleQuote(QStringLiteral("\"(.+)\""));
  QRegularExpressionMatch quoteMatch = titleQuote.match(objValue(obj, "title"));
  if(quoteMatch.hasMatch()) {
    entry->setField(QStringLiteral("type"), quoteMatch.captured(1));
  } else {
    entry->setField(QStringLiteral("type"), objValue(obj, "ruler", "name"));
  }

  entry->setField(QStringLiteral("denomination"), objValue(obj, "value", "text"));
  entry->setField(QStringLiteral("currency"), objValue(obj, "value", "currency", "name"));
  entry->setField(QStringLiteral("country"), objValue(obj, "issuer", "name"));
  entry->setField(QStringLiteral("mintmark"), objValue(obj, "mintLetter"));

  // if minyear = maxyear, then set the year of the coin
  const auto year = obj[QLatin1StringView("minYear")];
  if(year == obj[QLatin1StringView("maxYear")]) {
    entry->setField(QStringLiteral("year"), QString::number(year.toDouble()));
  } else if(!m_year.isEmpty()) {
    entry->setField(QStringLiteral("year"), m_year);
  }

  entry->setField(QStringLiteral("obverse"), objValue(obj, "obverse", "picture"));
  entry->setField(QStringLiteral("reverse"), objValue(obj, "reverse", "picture"));

  const QString numista(QStringLiteral("numista"));
  if(optionalFields().contains(numista)) {
    Data::FieldPtr field(new Data::Field(numista, i18n("Numista Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    coll->addField(field);
    entry->setField(numista, objValue(obj, "url"));
  }

  const QString desc(QStringLiteral("description"));
  if(!coll->hasField(desc) && optionalFields().contains(desc)) {
    Data::FieldPtr field(new Data::Field(desc, i18n("Description"), Data::Field::Para));
    coll->addField(field);
    entry->setField(QStringLiteral("description"), objValue(obj, "comments"));
  }

  const QString krause(QStringLiteral("km"));
  if(!coll->hasField(krause) && optionalFields().contains(krause)) {
    Data::FieldPtr field(new Data::Field(krause, allOptionalFields().value(krause)));
    field->setCategory(i18n("General"));
    coll->addField(field);
    const auto refArray = obj[QLatin1StringView("references")].toArray();
    for(const auto& ref : refArray) {
      const auto refObj = ref.toObject();
      if(objValue(refObj, "catalogue", "code") == QLatin1String("KM")) {
        entry->setField(krause, objValue(refObj, "number"));
        // don't break out, there could be multiple KM values and we want the last one
      }
    }
  }

  return entry;
}

Tellico::Fetch::FetchRequest NumistaFetcher::updateRequest(Data::EntryPtr entry_) {
  const QString t = entry_->field(QStringLiteral("type"));
  if(!t.isEmpty()) {
    const QString c = entry_->field(QStringLiteral("country"));
    return FetchRequest(Fetch::Keyword, t + QLatin1Char(' ') + c);
  }

  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* NumistaFetcher::configWidget(QWidget* parent_) const {
  return new NumistaFetcher::ConfigWidget(parent_, this);
}

QString NumistaFetcher::defaultName() {
  return QStringLiteral("Numista"); // no translation
}

QString NumistaFetcher::defaultIcon() {
  return favIcon("https://en.numista.com");
}

Tellico::StringHash NumistaFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("numista")] = i18n("Numista Link");
  hash[QStringLiteral("description")] = i18n("Description");
  // treat images as optional since Numista doesn't break out different images for each year
  hash[QStringLiteral("obverse")] = i18n("Obverse");
  hash[QStringLiteral("reverse")] = i18n("Reverse");
  hash[QStringLiteral("km")] = i18nc("Standard catalog of world coins number", "Krause-Mishler");
  return hash;
}

NumistaFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const NumistaFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;

  QLabel* label = new QLabel(i18n("Access key: "), optionsWidget());
  l->addWidget(label, ++row, 0);

  m_apiKeyEdit = new QLineEdit(optionsWidget());
  connect(m_apiKeyEdit, &QLineEdit::textChanged, this, &ConfigWidget::slotSetModified);
  l->addWidget(m_apiKeyEdit, row, 1);
  label->setBuddy(m_apiKeyEdit);

  label = new QLabel(i18n("Language: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_langCombo = new GUI::ComboBox(optionsWidget());
  m_langCombo->addItem(QIcon(new KCountryFlagEmojiIconEngine(QLatin1String("us"))),
                       KLanguageName::nameForCode(QLatin1String("en")),
                       QLatin1String("en"));
  m_langCombo->addItem(QIcon(new KCountryFlagEmojiIconEngine(QLatin1String("fr"))),
                       KLanguageName::nameForCode(QLatin1String("fr")),
                       QLatin1String("fr"));
  void (GUI::ComboBox::* activatedInt)(int) = &GUI::ComboBox::activated;
  connect(m_langCombo, activatedInt, this, &ConfigWidget::slotSetModified);
  connect(m_langCombo, activatedInt, this, &ConfigWidget::slotLangChanged);
  l->addWidget(m_langCombo, row, 1);
  label->setBuddy(m_langCombo);

  l->setRowStretch(++row, 10);

  // now add additional fields widget
  addFieldsWidget(NumistaFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());

  // don't show the default API key
  if(fetcher_) {
    if(fetcher_->m_apiKey != Tellico::reverseObfuscate(NUMISTA_MAGIC_TOKEN)) {
      m_apiKeyEdit->setText(fetcher_->m_apiKey);
    }
    m_langCombo->setCurrentData(fetcher_->m_locale);
  }
}

void NumistaFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  const QString apiKey = m_apiKeyEdit->text().trimmed();
  if(!apiKey.isEmpty()) {
    config_.writeEntry("API Key", apiKey);
  }
  const QString lang = m_langCombo->currentData().toString();
  config_.writeEntry("Locale", lang);
}

QString NumistaFetcher::ConfigWidget::preferredName() const {
  return i18n("Numista (%1)", m_langCombo->currentText());
}

void NumistaFetcher::ConfigWidget::slotLangChanged() {
  Q_EMIT signalName(preferredName());
}
