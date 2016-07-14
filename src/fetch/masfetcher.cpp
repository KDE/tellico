/***************************************************************************
    Copyright (C) 2012 Robby Stephenson <robby@periapsis.org>
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

#include "masfetcher.h"
#include "../collections/bibtexcollection.h"
#include "../images/imagefactory.h"
#include "../utils/guiproxy.h"
#include "../utils/string_utils.h"
#include "../entry.h"
#include "../core/filehandler.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KIO/Job>
#include <KJobUiDelegate>
#include <KJobWidgets/KJobWidgets>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QTextCodec>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>

namespace {
  static const int MAS_MAX_RETURNS = 20;
  static const char* MAS_API_URL = "http://academic.research.microsoft.com/json.svc/search";
  static const char* MAS_API_ID = "77aed835-28ad-45c3-abd2-bcd4ae3dbc2f";
}

using namespace Tellico;
using Tellico::Fetch::MASFetcher;

MASFetcher::MASFetcher(QObject* parent_)
    : Fetcher(parent_), m_started(false), m_start(0), m_total(-1) {
}

MASFetcher::~MASFetcher() {
}

QString MASFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

QString MASFetcher::attribution() const {
  return i18n("This data is licensed under <a href=""%1"">specific terms</a>.",
              QLatin1String("http://academic.research.microsoft.com/About/TermsofUseSpecEN.html"));
}

bool MASFetcher::canSearch(FetchKey k) const {
  return k == Title || k == Person || k == Keyword;
}

bool MASFetcher::canFetch(int type) const {
  return type == Data::Collection::Bibtex;
}

void MASFetcher::readConfigHook(const KConfigGroup&) {
}

void MASFetcher::search() {
  m_started = true;
  m_start = 0;
  m_total = -1;
  doSearch();
}

void MASFetcher::continueSearch() {
  m_started = true;
  doSearch();
}

void MASFetcher::doSearch() {
  QUrl u(QString::fromLatin1(MAS_API_URL));
  QUrlQuery q;
  q.addQueryItem(QLatin1String("Version"), QLatin1String("1.2"));
  q.addQueryItem(QLatin1String("AppId"), QLatin1String(MAS_API_ID));
  q.addQueryItem(QLatin1String("StartIdx"), QString::number(m_start+1));
  q.addQueryItem(QLatin1String("EndIdx"), QString::number(m_start+MAS_MAX_RETURNS));
  q.addQueryItem(QLatin1String("ResultObjects"), QLatin1String("Publication"));
  q.addQueryItem(QLatin1String("PublicationContent"), QLatin1String("AllInfo"));

  switch(request().key) {
    case Title:
      q.addQueryItem(QLatin1String("TitleQuery"), request().value);
      break;

    case Person:
      q.addQueryItem(QLatin1String("AuthorQuery"), request().value);
      break;

    case Keyword:
      q.addQueryItem(QLatin1String("FulltextQuery"), request().value);
      break;

    default:
      break;
  }
  u.setQuery(q);

//  myDebug() << "url:" << u;

  QPointer<KIO::StoredTransferJob> job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(job, GUI::Proxy::widget());
  connect(job, SIGNAL(result(KJob*)), SLOT(slotComplete(KJob*)));
}

void MASFetcher::stop() {
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

Tellico::Data::EntryPtr MASFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }
  return entry;
}

Tellico::Fetch::FetchRequest MASFetcher::updateRequest(Data::EntryPtr entry_) {
  const QString title = entry_->field(QLatin1String("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }
  return FetchRequest();
}

void MASFetcher::slotComplete(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);
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

#if 0
  myWarning() << "Remove debug from MASFetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/testmas.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << data;
  }
  f.close();
#endif

  QJsonDocument doc = QJsonDocument::fromJson(data);
  const QVariantMap resultsMap = doc.object().toVariantMap();

  if(m_total == -1) {
    m_total = resultsMap.value(QLatin1String("d")).toMap()
                               .value(QLatin1String("Publication")).toMap()
                               .value(QLatin1String("TotalItem")).toInt();
  }

  const QVariantList resultList = resultsMap.value(QLatin1String("d")).toMap()
                                 .value(QLatin1String("Publication")).toMap()
                                 .value(QLatin1String("Result")).toList();

  if(resultList.isEmpty()) {
    myDebug() << "no results";
    stop();
    return;
  }

  Data::CollPtr coll(new Data::BibtexCollection(true));
  foreach(const QVariant& result, resultList) {
  //  myDebug() << "found result:" << result;
    Data::EntryPtr entry(new Data::Entry(coll));
    populateEntry(entry, result.toMap());

    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
  }

  m_start = m_entries.count();
  m_hasMoreResults = m_start < m_total;
  stop();
}

void MASFetcher::populateEntry(Data::EntryPtr entry_, const QVariantMap& result_) {
  entry_->setField(QLatin1String("title"), value(result_, "Title"));
  entry_->setField(QLatin1String("year"), value(result_, "Year"));
  entry_->setField(QLatin1String("doi"), value(result_, "DOI"));
  entry_->setField(QLatin1String("abstract"), value(result_, "Abstract"));
  entry_->setField(QLatin1String("url"), value(result_, "FullVersionURL", 0, true)); // first URL only
  entry_->setField(QLatin1String("journal"), value(result_, "Journal", "FullName"));
//  entry_->setField(QLatin1String("issn"), value(result_, "Journal", "ISSN"));

  QStringList authors;
  foreach(const QVariant& authorVariant, result_.value(QLatin1String("Author")).toList()) {
    const QVariantMap authorMap = authorVariant.toMap();
    const QString author = value(authorMap, "FirstName") + QLatin1Char(' ')
                         + value(authorMap, "MiddleName")
                         + (value(authorMap, "MiddleName").isEmpty() ? QString() : QLatin1String(" "))
                         + value(authorMap, "LastName");
    if(!author.isEmpty()) {
      authors << author;
    }
  }
  if(!authors.isEmpty()) {
    entry_->setField(QLatin1String("author"), authors.join(FieldFormat::delimiterString()));
  }

  QStringList keywords;
  foreach(const QVariant& keywordVariant, result_.value(QLatin1String("Keyword")).toList()) {
    // if the keyword name is included, use it
    const QString keywordName = keywordVariant.toMap().value(QLatin1String("Name")).toString();
    if(!keywordName.isEmpty()) {
      keywords << keywordName;
      continue;
    }

    // otherwise, see if we've seen it before and grab it from hash
    const QString keywordId = keywordVariant.toMap().value(QLatin1String("ID")).toString();
    if(m_keywordHash.contains(keywordId)) {
      keywords << m_keywordHash.value(keywordId);
      continue;
    }

    // finally, look it up with a new query
    QUrl ku(QString::fromLatin1(MAS_API_URL));
    QUrlQuery q;
    q.addQueryItem(QLatin1String("Version"), QLatin1String("1.2"));
    q.addQueryItem(QLatin1String("AppId"), QLatin1String(MAS_API_ID));
    q.addQueryItem(QLatin1String("StartIdx"), QLatin1String("1"));
    q.addQueryItem(QLatin1String("EndIdx"), QLatin1String("1"));
    q.addQueryItem(QLatin1String("ResultObjects"), QLatin1String("Keyword"));
    q.addQueryItem(QLatin1String("PublicationContent"), QLatin1String("AllInfo"));
    q.addQueryItem(QLatin1String("KeywordID"), keywordId);
    ku.setQuery(q);
//    myDebug() << ku;

    QByteArray data = FileHandler::readDataFile(ku, true /*quiet*/);
    QJsonDocument doc = QJsonDocument::fromJson(data);
    const QVariantList keywordList = doc.object().toVariantMap()
                                    .value(QLatin1String("d")).toMap()
                                    .value(QLatin1String("Keyword")).toMap()
                                    .value(QLatin1String("Result")).toList();
    if(!keywordList.isEmpty()) {
      keywords << keywordList.at(0).toMap().value(QLatin1String("Name")).toString();
      m_keywordHash.insert(keywordId, keywords.last());
    }
  }
  if(!keywords.isEmpty()) {
    entry_->setField(QLatin1String("keyword"), keywords.join(FieldFormat::delimiterString()));
  }
}

Tellico::Fetch::ConfigWidget* MASFetcher::configWidget(QWidget* parent_) const {
  return new MASFetcher::ConfigWidget(parent_, this);
}

QString MASFetcher::defaultName() {
  return QLatin1String("Microsoft Academic Search"); // no translation
}

QString MASFetcher::defaultIcon() {
  return favIcon("http://academic.research.microsoft.com");
}

Tellico::StringHash MASFetcher::allOptionalFields() {
  StringHash hash;
  return hash;
}

MASFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const MASFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  // now add additional fields widget
  addFieldsWidget(MASFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

void MASFetcher::ConfigWidget::saveConfigHook(KConfigGroup&) {
}

QString MASFetcher::ConfigWidget::preferredName() const {
  return MASFetcher::defaultName();
}

QString MASFetcher::value(const QVariantMap& map, const char* name, const char* name2, bool onlyFirst) {
  const QVariant v = map.value(QLatin1String(name));
  if(v.isNull())  {
    return QString();
  } else if(v.canConvert(QVariant::String)) {
    return v.toString();
  } else if(v.canConvert(QVariant::StringList)) {
    if(onlyFirst) {
      const QStringList list = v.toStringList();
      return list.isEmpty() ? QString() : list.at(0);
    } else {
      return v.toStringList().join(Tellico::FieldFormat::delimiterString());
    }
  } else if(v.canConvert(QVariant::Map)) {
    return v.toMap().value(QLatin1String(name2 ? name2 : "value")).toString();
  } else {
    return QString();
  }
}
