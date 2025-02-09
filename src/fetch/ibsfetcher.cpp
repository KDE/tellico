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
#include "../collections/bookcollection.h"
#include "../entry.h"
#include "../fieldformat.h"
#include "../core/filehandler.h"
#include "../images/imagefactory.h"
#include "../utils/isbnvalidator.h"
#include "../utils/string_utils.h"
#include "../utils/objvalue.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KIO/StoredTransferJob>
#include <KJobUiDelegate>
#include <KJobWidgets>

#include <QRegularExpression>
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
bool IBSFetcher::canSearch(Fetch::FetchKey k) const {
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
  q.addQueryItem(QStringLiteral("ts"), QStringLiteral("as"));
  q.addQueryItem(QStringLiteral("filterProduct_type"), QStringLiteral("ITBOOK"));

  switch(request().key()) {
    case Title:
      {
        // can't have ampersands
        QString s = request().value();
        s.remove(QLatin1Char('&'));
        q.addQueryItem(QStringLiteral("query"), s.simplified());
      }
      break;

    case ISBN:
      {
        QString s = request().value();
        // limit to first isbn
        s = s.section(QLatin1Char(';'), 0, 0);
        // isbn13 search doesn't work?
        s = ISBNValidator::isbn13(s);
        // dashes don't work
        s.remove(QLatin1Char('-'));
        q.addQueryItem(QStringLiteral("query"), s);
      }
      break;

    case Keyword:
      q.addQueryItem(QStringLiteral("query"), request().value());
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
  connect(m_job.data(), &KJob::result, this, &IBSFetcher::slotComplete);
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
  Q_EMIT signalDone(this);
}

void IBSFetcher::slotComplete(KJob*) {
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

#if 0
  myWarning() << "Remove debug from ibsfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test-ibs.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << data;
  }
  f.close();
#endif

  QString s = Tellico::decodeHTML(data);
  static const QRegularExpression itemRx(QLatin1String("<div class=\"cc-product-list-item.*?>(.+?)<!--"),
                                         QRegularExpression::DotMatchesEverythingOption);
  static const QRegularExpression titleRx(QStringLiteral("<a [^>]*href=\"(.+?)\"[^>]*?class=\"cc-title\">(.+?)</a"),
                                          QRegularExpression::DotMatchesEverythingOption);
  static const QRegularExpression yearRx(QLatin1String("<span class=\"cc-publisher\">.*?([12]\\d{3}).*?</"),
                                         QRegularExpression::DotMatchesEverythingOption);
  static const QRegularExpression tagRx(QLatin1String("<.*?>"));

  QString url, title, year;
  auto matchIterator = itemRx.globalMatch(s);
  while(matchIterator.hasNext() && m_started) {
    auto itemMatch = matchIterator.next();
    const QString s = itemMatch.captured(1);
    auto titleMatch = titleRx.match(s);
    if(titleMatch.hasMatch()) {
      url = titleMatch.captured(1);
      title = titleMatch.captured(2).remove(tagRx).simplified();
    }
    auto yearMatch = yearRx.match(s);
    if(yearMatch.hasMatch()) {
      year = yearMatch.captured(1).remove(tagRx).simplified();
    }
    if(!url.isEmpty() && !title.isEmpty()) {
      // the url probable contains &amp; so be careful
      QUrl u = m_job->url();
      u = u.resolved(QUrl(url.replace(QLatin1String("&amp;"), QLatin1String("&"))));
//      myDebug() << u << title << year;
      FetchResult* r = new FetchResult(this, title, year);
      m_matches.insert(r->uid, u);
      Q_EMIT signalResultFound(r);
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

#if 0
  myDebug() << url.url();
  myWarning() << "Remove debug2 from ibsfetcher.cpp";
  QFile f(QLatin1String("/tmp/test-ibs2.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
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
  static const QRegularExpression jsonRx(QLatin1String("<script type=\"application/ld\\+json\">(.*?)</script"),
                                         QRegularExpression::DotMatchesEverythingOption);

  const auto jsonMatch = jsonRx.match(str_);
  if(!jsonMatch.hasMatch()) {
    myDebug() << "No JSON block";
    return Data::EntryPtr();
  }

#if 0
  myWarning() << "Remove json debug from ibsfetcher.cpp";
  QFile f(QLatin1String("/tmp/test.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << jsonRx.cap(1);
  }
  f.close();
#endif
  QJsonDocument doc = QJsonDocument::fromJson(jsonMatch.captured(1).toUtf8());
  const auto docObj = doc.object();
  const auto mainObj = docObj[QLatin1StringView("mainEntity")].toObject();
  if(mainObj.isEmpty()) {
    myDebug() << "no JSON object";
    return Data::EntryPtr();
  }

  Data::CollPtr coll(new Data::BookCollection(true));
  Data::EntryPtr entry(new Data::Entry(coll));

  // as genre, take the last breadcrumb
  QString genre = objValue(docObj, "breadcrumb");
  genre = genre.section(QStringLiteral(">"), -1);
  entry->setField(QStringLiteral("genre"), genre);

  // the title in the embedded loses it's identifier? "La..."
  entry->setField(QStringLiteral("title"), objValue(mainObj, "name"));
  entry->setField(QStringLiteral("author"), objValue(mainObj, "author"));

  const QString bookFormat = objValue(mainObj, "bookFormat");
  if(bookFormat == QLatin1String("https://schema.org/Paperback")) {
    entry->setField(QStringLiteral("binding"), i18n("Paperback"));
  } else if(bookFormat == QLatin1String("https://schema.org/Hardcover")) {
    entry->setField(QStringLiteral("binding"), i18n("Hardback"));
  } else if(bookFormat == QLatin1String("https://schema.org/EBook")) {
    entry->setField(QStringLiteral("binding"), i18n("E-Book"));
  }

  entry->setField(QStringLiteral("pub_year"), objValue(mainObj, "datePublished"));
  entry->setField(QStringLiteral("isbn"), objValue(mainObj, "isbn"));

  const QString id = ImageFactory::addImage(QUrl::fromUserInput(objValue(mainObj, "image")),
                                            true /* quiet */);
  if(id.isEmpty()) {
    message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
  }
  // empty image ID is ok
  entry->setField(QStringLiteral("cover"), id);

  // inLanguage is upper-case language code
  const QString lang = objValue(mainObj, "inLanguage");
  entry->setField(QStringLiteral("language"), QLocale(lang.toLower()).nativeLanguageName());

  entry->setField(QStringLiteral("plot"), objValue(mainObj, "description"));
  entry->setField(QStringLiteral("pages"), objValue(mainObj, "numberOfPages"));
  entry->setField(QStringLiteral("publisher"), objValue(mainObj, "publisher"));

  // multiple authors do not show up in the embedded JSON
  static const QRegularExpression titleDivRx(QLatin1String("<div id=\"title\">(.*?)</div>"),
                                             QRegularExpression::DotMatchesEverythingOption);
  const auto titleDivMatch = titleDivRx.match(str_);
  if(titleDivMatch.hasMatch()) {
    const QString titleDiv = titleDivMatch.captured(1);
    static const QRegularExpression authorRx(QLatin1String("<a href=\"/libri/autori/[^>]+?>(.*?)</a>"),
                                             QRegularExpression::DotMatchesEverythingOption);
    QStringList authors;
    auto i = authorRx.globalMatch(titleDiv);
    while(i.hasNext()) {
      const auto match = i.next();
      authors << match.captured(1).simplified();
    }
    if(!authors.isEmpty()) {
      entry->setField(QStringLiteral("author"), authors.join(FieldFormat::delimiterString()));
    }
    // the title in the embedded loses its identifier? "La..."
    static const QRegularExpression labelRx(QLatin1String("<label>(.*?)</label>"),
                                            QRegularExpression::DotMatchesEverythingOption);
    const auto labelMatch = labelRx.match(titleDiv);
    if(labelMatch.hasMatch()) {
      entry->setField(QStringLiteral("title"), labelMatch.captured(1).simplified());
    }
  }

  static const QRegularExpression tagRx(QLatin1String("<.*?>"));

  // editor is not in embedded json
  static const QRegularExpression editorRx(QLatin1String("Curatore:.*?>(.*?)</a"),
                                           QRegularExpression::DotMatchesEverythingOption);
  auto match = editorRx.match(str_);
  if(match.hasMatch()) {
    entry->setField(QStringLiteral("editor"), match.captured(1).remove(tagRx).simplified());
  }

  // translator is not in embedded json
  static const QRegularExpression translatorRx(QLatin1String("Traduttore:.*?>(.*?)</a"),
                                               QRegularExpression::DotMatchesEverythingOption);
  match = translatorRx.match(str_);
  if(match.hasMatch()) {
    entry->setField(QStringLiteral("translator"), match.captured(1).remove(tagRx).simplified());
  }

  // edition is not in embedded json
  static const QRegularExpression editionRx(QLatin1String("Editore:.*?>(.*?)</a"),
                                            QRegularExpression::DotMatchesEverythingOption);
  match = editionRx.match(str_);
  if(match.hasMatch()) {
    entry->setField(QStringLiteral("edition"), match.captured(1).remove(tagRx).simplified());
  }

  // series is not in embedded json
  static const QRegularExpression seriesRx(QLatin1String("Collana:.*?>(.*?)</a"),
                                           QRegularExpression::DotMatchesEverythingOption);
  match = seriesRx.match(str_);
  if(match.hasMatch()) {
    entry->setField(QStringLiteral("series"), match.captured(1).remove(tagRx).simplified());
  }

  return entry;
}

Tellico::Fetch::FetchRequest IBSFetcher::updateRequest(Data::EntryPtr entry_) {
  QString isbn = entry_->field(QStringLiteral("isbn"));
  if(!isbn.isEmpty()) {
    return FetchRequest(Fetch::ISBN, isbn);
  }
  QString t = entry_->field(QStringLiteral("title"));
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
  return favIcon("http://www.ibs.it");
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
