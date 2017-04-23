/***************************************************************************
    Copyright (C) 2006-2009 Robby Stephenson <robby@periapsis.org>
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

#include "ibsfetcher.h"
#include "../utils/guiproxy.h"
#include "../utils/string_utils.h"
#include "../collections/bookcollection.h"
#include "../entry.h"
#include "../fieldformat.h"
#include "../core/filehandler.h"
#include "../images/imagefactory.h"
#include "../utils/isbnvalidator.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KIO/Job>
#include <KJobUiDelegate>
#include <KJobWidgets/KJobWidgets>

#include <QRegExp>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
  static const char* IBS_BASE_URL = "https://www.ibs.it/search/";
}

using namespace Tellico;
using Tellico::Fetch::IBSFetcher;

IBSFetcher::IBSFetcher(QObject* parent_)
    : Fetcher(parent_), m_total(0), m_started(false) {
}

IBSFetcher::~IBSFetcher() {
}

QString IBSFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool IBSFetcher::canFetch(int type) const {
  return type == Data::Collection::Book || type == Data::Collection::Bibtex;
}

// No UPC or Raw for now.
bool IBSFetcher::canSearch(FetchKey k) const {
  return k == Title || k == Person || k == ISBN;
}

void IBSFetcher::readConfigHook(const KConfigGroup& config_) {
  Q_UNUSED(config_);
}

void IBSFetcher::search() {
  m_started = true;
  m_matches.clear();

  QUrl u(QString::fromLatin1(IBS_BASE_URL));
  QUrlQuery q;
  q.addQueryItem(QLatin1String("ts"), QLatin1String("as"));

  switch(request().key) {
    case Title:
      {
        // can't have ampersands
        QString s = request().value;
        s.remove(QLatin1Char('&'));
        q.addQueryItem(QLatin1String("query"), s);
      }
      break;

    case ISBN:
      {
        QString s = request().value;
        // limit to first isbn
        s = s.section(QLatin1Char(';'), 0, 0);
        // isbn10 search doesn't work?
        s = ISBNValidator::isbn13(s);
        // dashes don't work
        s.remove(QLatin1Char('-'));
        q.addQueryItem(QLatin1String("query"), s);
      }
      break;

    case Keyword:
      q.addQueryItem(QLatin1String("query"), request().value);
      break;

    default:
      myWarning() << "key not recognized: " << request().key;
      stop();
      return;
  }
  u.setQuery(q);
//  myDebug() << "url: " << u.url();

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job, SIGNAL(result(KJob*)), SLOT(slotComplete(KJob*)));
}

void IBSFetcher::stop() {
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

void IBSFetcher::slotComplete(KJob*) {
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

  QString s = Tellico::decodeHTML(data);
  // really specific regexp
  QRegExp itemRx(QLatin1String("class=\"item \">(.*)class=\"price"));
  itemRx.setMinimal(true);
  QRegExp titleRx(QLatin1String("<div class=\"title\">\\s*<a href=\"(.*)\">(.*)</div>"));
  titleRx.setMinimal(true);
  QRegExp yearRx(QLatin1String("<label>Anno</label>(.*)</"));
  yearRx.setMinimal(true);
  QRegExp tagRx(QLatin1String("<.*>"));
  tagRx.setMinimal(true);

  QString url, title, year;
  for(int pos = itemRx.indexIn(s); m_started && pos > -1; pos = itemRx.indexIn(s, pos+itemRx.matchedLength())) {
    QString s = itemRx.cap(1);
    if(s.contains(titleRx)) {
      url = titleRx.cap(1);
      title = titleRx.cap(2).remove(tagRx).simplified();
    }
    if(s.contains(yearRx)) {
      year = yearRx.cap(1).remove(tagRx).simplified();
    }
    if(!url.isEmpty() && !title.isEmpty()) {
      // the url probable contains &amp; so be careful
      QUrl u = m_job->url();
      u = u.resolved(QUrl(url.replace(QLatin1String("&amp;"), QLatin1String("&"))));
      FetchResult* r = new FetchResult(Fetcher::Ptr(this), title, year);
      m_matches.insert(r->uid, u);
      emit signalResultFound(r);
    }
  }

  // since the fetch is done, don't worry about holding the job pointer
  m_job = nullptr;
  stop();
}

Tellico::Data::EntryPtr IBSFetcher::fetchEntryHook(uint uid_) {
  // if we already grabbed this one, then just pull it out of the dict
  Data::EntryPtr entry = m_entries[uid_];
  if(entry) {
    return entry;
  }

  QUrl url = m_matches[uid_];
  if(url.isEmpty()) {
    myWarning() << "no url in map";
    return Data::EntryPtr();
  }

  QString results = Tellico::decodeHTML(FileHandler::readDataFile(url, true));
  if(results.isEmpty()) {
    myDebug() << "no text results";
    return Data::EntryPtr();
  }

//  myDebug() << url.url();
#if 0
  myWarning() << "Remove debug from ibsfetcher.cpp";
  QFile f(QLatin1String("/tmp/test.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << results;
  }
  f.close();
#endif

  entry = parseEntry(results);
  if(!entry) {
    myDebug() << "error in processing entry";
    return Data::EntryPtr();
  }
  m_entries.insert(uid_, entry); // keep for later
  return entry;
}

Tellico::Data::EntryPtr IBSFetcher::parseEntry(const QString& str_) {
  QRegExp jsonRx(QLatin1String("<script type=\"application/ld\\+json\">(.*)</"));
  jsonRx.setMinimal(true);

  if(!str_.contains(jsonRx)) {
    myDebug() << "No JSON block";
    return Data::EntryPtr();
  }

  QJsonDocument doc = QJsonDocument::fromJson(jsonRx.cap(1).toUtf8());
  QVariantMap objectMap = doc.object().toVariantMap();
  QVariantMap resultMap = objectMap.value(QLatin1String("mainEntity")).toMap();
  if(resultMap.isEmpty()) {
    myDebug() << "no JSON object";
    return Data::EntryPtr();
  }

  Data::CollPtr coll(new Data::BookCollection(true));
  Data::EntryPtr entry(new Data::Entry(coll));

  // as genre, take the last breadcrumb
  QString genre = value(objectMap, "breadcrumb");
  genre = genre.section(QLatin1String(">"), -1);
  entry->setField(QLatin1String("genre"), genre);

  // the title in the embedded loses it's identifier? "La..."
  entry->setField(QLatin1String("title"), value(resultMap, "name"));
  entry->setField(QLatin1String("author"), value(resultMap, "author"));

  const QString bookFormat = value(resultMap, "bookFormat");
  if(bookFormat == QLatin1String("http://schema.org/Paperback")) {
    entry->setField(QLatin1String("binding"), i18n("Paperback"));
  } else if(bookFormat == QLatin1String("http://schema.org/Hardcover")) {
    entry->setField(QLatin1String("binding"), i18n("Hardback"));
  } else if(bookFormat == QLatin1String("http://schema.org/EBook")) {
    entry->setField(QLatin1String("binding"), i18n("E-Book"));
  }

  entry->setField(QLatin1String("pub_year"), value(resultMap, "datePublished"));
  entry->setField(QLatin1String("cover"), value(resultMap, "image"));
  entry->setField(QLatin1String("isbn"), value(resultMap, "isbn"));

  // inLanguage is upper-case language code
  const QString lang = value(resultMap, "inLanguage");
  entry->setField(QLatin1String("language"), QLocale(lang.toLower()).nativeLanguageName());

  Data::FieldPtr f(new Data::Field(QLatin1String("plot"), i18n("Plot Summary"), Data::Field::Para));
  coll->addField(f);
  entry->setField(f, value(resultMap, "description"));

  entry->setField(QLatin1String("pages"), value(resultMap, "numberOfPages"));
  entry->setField(QLatin1String("publisher"), value(resultMap, "publisher"));

  // multiple authors do not show up in the embedded JSON
  QRegExp titleDivRx(QLatin1String("<div id=\"title\">(.*)</div>"));
  titleDivRx.setMinimal(true);
  if(str_.contains(titleDivRx)) {
    const QString titleDiv = titleDivRx.cap(1);
    QRegExp authorRx(QLatin1String("<a href=\"/libri/autori/[^>]+>(.*)</a>"));
    authorRx.setMinimal(true);
    QStringList authors;
    for(int pos = authorRx.indexIn(titleDiv); pos > -1; pos = authorRx.indexIn(titleDiv, pos+authorRx.matchedLength())) {
      authors << authorRx.cap(1).simplified();
    }
    if(!authors.isEmpty()) {
      entry->setField(QLatin1String("author"), authors.join(FieldFormat::delimiterString()));
    }
    // the title in the embedded loses it's identifier? "La..."
    QRegExp labelRx(QLatin1String("<label>(.*)</label>"));
    if(titleDiv.contains(labelRx)) {
      entry->setField(QLatin1String("title"), labelRx.cap(1).simplified());
    }
  }

  QRegExp tagRx(QLatin1String("<.*>"));
  tagRx.setMinimal(true);

  // editor is not in embedded json
  QRegExp editorRx(QLatin1String("<strong>Curatore:</strong>(.*)</div"));
  editorRx.setMinimal(true);
  if(str_.contains(editorRx)) {
    entry->setField(QLatin1String("editor"), editorRx.cap(1).remove(tagRx).simplified());
  }

  // editor is not in embedded json
  QRegExp translatorRx(QLatin1String("<strong>Traduttore:</strong>(.*)</div"));
  translatorRx.setMinimal(true);
  if(str_.contains(translatorRx)) {
    entry->setField(QLatin1String("translator"), translatorRx.cap(1).remove(tagRx).simplified());
  }

  return entry;
}

Tellico::Fetch::FetchRequest IBSFetcher::updateRequest(Data::EntryPtr entry_) {
  QString isbn = entry_->field(QLatin1String("isbn"));
  if(!isbn.isEmpty()) {
    return FetchRequest(Fetch::ISBN, isbn);
  }
  QString t = entry_->field(QLatin1String("title"));
  if(!t.isEmpty()) {
    return FetchRequest(Fetch::Title, t);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* IBSFetcher::configWidget(QWidget* parent_) const {
  return new IBSFetcher::ConfigWidget(parent_);
}

QString IBSFetcher::defaultName() {
  return i18n("Internet Bookshop (ibs.it)");
}

QString IBSFetcher::defaultIcon() {
  return favIcon("http://ibs.it");
}

IBSFetcher::ConfigWidget::ConfigWidget(QWidget* parent_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();
}

QString IBSFetcher::ConfigWidget::preferredName() const {
  return IBSFetcher::defaultName();
}

// static
QString IBSFetcher::value(const QVariantMap& map, const char* name) {
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
