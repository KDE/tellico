/***************************************************************************
    Copyright (C) 2022 Robby Stephenson <robby@periapsis.org>
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

#include "gaminghistoryfetcher.h"
#include "../utils/guiproxy.h"
#include "../utils/string_utils.h"
#include "../collections/gamecollection.h"
#include "../entry.h"
#include "../core/filehandler.h"
#include "../images/imagefactory.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfig>
#include <KIO/StoredTransferJob>
#include <KIO/JobUiDelegate>
#include <KJobWidgets>

#include <QRegularExpression>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QUrlQuery>

namespace {
  static const char* GAMINGHISTORY_BASE_URL = "https://www.arcade-history.com/index.php";
}

using namespace Tellico;
using Tellico::Fetch::GamingHistoryFetcher;

GamingHistoryFetcher::GamingHistoryFetcher(QObject* parent_)
    : Fetcher(parent_), m_started(false) {
  populateYearIndex();
}

GamingHistoryFetcher::~GamingHistoryFetcher() {
}

QString GamingHistoryFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool GamingHistoryFetcher::canFetch(int type) const {
  return type == Data::Collection::Game;
}

void GamingHistoryFetcher::readConfigHook(const KConfigGroup& config_) {
  Q_UNUSED(config_);
}

void GamingHistoryFetcher::search() {
  m_started = true;
  m_matches.clear();

  QUrl u(QString::fromLatin1(GAMINGHISTORY_BASE_URL));
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("page"), QStringLiteral("database"));

  switch(request().key()) {
    case Keyword:
      {
        QString value = request().value();
        // pull out year, keep the regexp a little loose
        QRegularExpression yearRX(QStringLiteral("\\s*[0-9]{4}\\s*"));
        QRegularExpressionMatch match = yearRX.match(value);
        if(match.hasMatch()) {
          // fragile, but the form uses a year index
          QString year = match.captured(0).trimmed();
          if(m_yearIndex.contains(year)) {
            q.addQueryItem(QStringLiteral("annee"), QString::number(m_yearIndex.value(year)));
            value = value.remove(yearRX);
          }
        }
        q.addQueryItem(QStringLiteral("lemot"), value);
      }
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      stop();
      return;
  }
  u.setQuery(q);
//  myDebug() << "url:" << u;

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result,
          this, &GamingHistoryFetcher::slotComplete);
}

void GamingHistoryFetcher::stop() {
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

void GamingHistoryFetcher::slotComplete(KJob*) {
  if(m_job->error()) {
    m_job->uiDelegate()->showErrorMessage();
    stop();
    return;
  }

  const QByteArray data = m_job->data();
  if(data.isEmpty()) {
    myDebug() << "no data";
    stop();
    return;
  }

  // since the fetch is done, don't worry about holding the job pointer
  m_job = nullptr;

  const QString s = Tellico::decodeHTML(data);
#if 0
  myWarning() << "Remove debug from gaminghistoryfetcher.cpp";
  QFile f(QStringLiteral("/tmp/test.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << s;
  }
  f.close();
#endif

  static const QRegularExpression rowRx(QStringLiteral("<tr class='big-box'>(.+?)</tr"));
  static const QRegularExpression dataRx(QStringLiteral("<td data-title='(.+?)'>(.+?)</td"));
  static const QRegularExpression tagRx(QLatin1String("<.*?>"));
  static const QRegularExpression emRx(QLatin1String("<em.*?>[^<]+?</em>"));
  static const QRegularExpression anchorRx(QStringLiteral("<a[^>]+?href='(.+?)'"));

  QRegularExpressionMatchIterator i = rowRx.globalMatch(s);
  while(i.hasNext()) {
    Data::CollPtr coll(new Data::GameCollection(true));
    Data::EntryPtr entry(new Data::Entry(coll));
    coll->addEntries(entry);
    QString u;
    QRegularExpressionMatch rowMatch = i.next();
    QRegularExpressionMatchIterator i2 = dataRx.globalMatch(rowMatch.captured(1));
    while(i2.hasNext()) {
      QRegularExpressionMatch dataMatch = i2.next();
      const auto dataType = dataMatch.captured(1);
      QString dataValue = dataMatch.captured(2);
      if(dataType == QLatin1String("Name")) {
        auto anchorMatch = anchorRx.match(dataValue);
        if(anchorMatch.hasMatch()) {
          u = anchorMatch.captured(1);
        }
        dataValue = dataValue.remove(emRx).remove(tagRx).simplified();
        entry->setField(QStringLiteral("title"), dataValue);
      } else if(dataType == QLatin1String("Year")) {
        entry->setField(QStringLiteral("year"), dataValue);
      } else if(dataType == QLatin1String("Publisher")) {
        dataValue = dataValue.remove(emRx).remove(tagRx).simplified();
        entry->setField(QStringLiteral("publisher"), dataValue);
      } else if(dataType == QLatin1String("Type")) {
        populatePlatform(entry, dataValue);
      }
    }

    FetchResult* r = new FetchResult(this, entry);
    m_entries.insert(r->uid, entry);
    QUrl url = QUrl(QString::fromLatin1(GAMINGHISTORY_BASE_URL)).resolved(QUrl(u));
    if(optionalFields().contains(QStringLiteral("gaming-history"))) {
      Data::FieldPtr field(new Data::Field(QStringLiteral("gaming-history"), i18n("Gaming History Link"), Data::Field::URL));
      field->setCategory(i18n("General"));
      coll->addField(field);
      entry->setField(QStringLiteral("gaming-history"), url.url());
    }
    m_matches.insert(r->uid, url);
    // don't emit signal until after putting url in matches hash
    emit signalResultFound(r);
  }

  if(m_matches.isEmpty()) {
    // an exact match is handled by returning a page with <script> at the top
    if(s.startsWith(QLatin1String("<script>"))) {
      static const QRegularExpression locationRx(QLatin1String("'([^']+?)'</script>"));
      auto locationMatch = locationRx.match(s);
      if(locationMatch.hasMatch()) {
        Data::CollPtr coll(new Data::GameCollection(true));
        Data::EntryPtr entry(new Data::Entry(coll));
        coll->addEntries(entry);

        QUrl u(locationMatch.captured(1));
        parseSingleResult(entry, u);

        FetchResult* r = new FetchResult(this, entry);
        m_entries.insert(r->uid, entry);
        emit signalResultFound(r);
      }
    } else {
      myDebug() << "no results";
    }
  }

  stop();
}

Tellico::Data::EntryPtr GamingHistoryFetcher::fetchEntryHook(uint uid_) {
  if(!m_entries.contains(uid_)) {
    myWarning() << "no entry in hash";
    return Data::EntryPtr();
  }

  Data::EntryPtr entry = m_entries[uid_];
  // if the url is not in the hash, the entry has already been fully populated
  if(!m_matches.contains(uid_)) {
    return entry;
  }

  QString results = Tellico::decodeHTML(FileHandler::readTextFile(m_matches[uid_], true, true));
  if(results.isEmpty()) {
    myDebug() << "no text results from" << m_matches[uid_];
    return entry;
  }

#if 0
  myWarning() << "Remove debug2 from gaminghistoryfetcher.cpp";
  QFile f(QStringLiteral("/tmp/test2.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << results;
  }
  f.close();
#endif

  parseEntry(entry, results);
  // remove url to signal the entry is fully populated
  m_matches.remove(uid_);
  return entry;
}

void GamingHistoryFetcher::parseEntry(Data::EntryPtr entry, const QString& str_) {
  static const QRegularExpression tagRx(QLatin1String("<.*?>"));
  static const QRegularExpression divRx(QLatin1String("<div class='ContainerTableau100'><div class='CelluleTexte100'>(.+?)</div"),
                                        QRegularExpression::DotMatchesEverythingOption);
  auto divMatch = divRx.match(str_);
  if(divMatch.hasMatch()) {
    QString desc = divMatch.captured(1);
    desc.replace(QLatin1String("<br />"), QLatin1String("\n"));
    // if the title is empty, need to parse it
    if(entry->title().isEmpty()) {
      const QString info = desc.section(QLatin1Char('\n'), 0, 0).remove(tagRx).simplified();
      QRegularExpression infoRx(QString::fromUtf8("^(.+?) \u00A9 (\\d{4}) (.+?)$"));
      auto infoMatch = infoRx.match(info);
      if(infoMatch.hasMatch()) {
        entry->setField(QStringLiteral("title"), infoMatch.captured(1).trimmed());
        entry->setField(QStringLiteral("year"), infoMatch.captured(2).trimmed());
        entry->setField(QStringLiteral("publisher"), infoMatch.captured(3).trimmed());
      }
    }
    // take the description as everything after the first line break
    desc = desc.section(QLatin1Char('\n'), 1).remove(tagRx).simplified();
    entry->setField(QStringLiteral("description"), desc);
  }

  // if the platform is empty, grab it from the html title
  if(entry->field(QStringLiteral("platform")).isEmpty()) {
    static const QRegularExpression titleRx(QLatin1String("<title>.+?, (.+?) by .+?</title>"));
    auto titleMatch = titleRx.match(str_);
    if(titleMatch.hasMatch()) {
      populatePlatform(entry, titleMatch.captured(1));
    }
  }

  static const QRegularExpression coverRx(QLatin1String("<img [^>]*?id='kukulcan'[^>]*?src='([^>]+?)'"));
  auto coverMatch = coverRx.match(str_);
  if(coverMatch.hasMatch()) {
    QString u = coverMatch.captured(1);
    QUrl coverUrl = QUrl(QString::fromLatin1(GAMINGHISTORY_BASE_URL)).resolved(QUrl(u));

    const QString id = ImageFactory::addImage(coverUrl, true /* quiet */);
    if(id.isEmpty()) {
      myDebug() << "Could not load" << coverUrl;
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    }
    // empty image ID is ok
    entry->setField(QStringLiteral("cover"), id);
  }
}

void GamingHistoryFetcher::parseSingleResult(Data::EntryPtr entry, const QUrl& url_) {
  QString results = Tellico::decodeHTML(FileHandler::readTextFile(url_, true, true));
  parseEntry(entry, results);
  if(optionalFields().contains(QStringLiteral("gaming-history"))) {
    Data::FieldPtr field(new Data::Field(QStringLiteral("gaming-history"), i18n("Gaming History Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    entry->collection()->addField(field);
    entry->setField(QStringLiteral("gaming-history"), url_.url());
  }
}

void GamingHistoryFetcher::populatePlatform(Data::EntryPtr entry, const QString& platform_) {
  static const QString platformString(QStringLiteral("platform"));

  QString platform = platform_;
  if(platform.endsWith(QLatin1String(" game")) ||
     platform.endsWith(QLatin1String(" disc"))) {
    platform.chop(5);
  } else if(platform.endsWith(QLatin1String(" disk.")) ||
            platform.endsWith(QLatin1String(" cass.")) ||
            platform.endsWith(QLatin1String(" cart."))) {
    platform.chop(6);
  } else if(platform.endsWith(QLatin1String(" CD"))) {
    platform.chop(3);
  }

  Data::FieldPtr platformField = entry->collection()->fieldByName(platformString);
  if(platformField && !platformField->allowed().contains(platform)) {
    QStringList allowed = platformField->allowed();
    allowed.append(platform);
    platformField->setAllowed(allowed);
  }

   entry->setField(platformString, platform);
}

Tellico::Fetch::FetchRequest GamingHistoryFetcher::updateRequest(Data::EntryPtr entry_) {
  QString t = entry_->field(QStringLiteral("title"));
  if(!t.isEmpty()) {
    return FetchRequest(Fetch::Keyword, t);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* GamingHistoryFetcher::configWidget(QWidget* parent_) const {
  return new GamingHistoryFetcher::ConfigWidget(parent_, this);
}

QString GamingHistoryFetcher::defaultName() {
  return QStringLiteral("Gaming History");
}

QString GamingHistoryFetcher::defaultIcon() {
  return favIcon("https://www.arcade-history.com");
}

//static
Tellico::StringHash GamingHistoryFetcher::allOptionalFields() {
  StringHash hash;
  hash.insert(QStringLiteral("gaming-history"), i18n("Gaming History Link"));
  return hash;
}

void GamingHistoryFetcher::populateYearIndex() {
  m_yearIndex.clear();
  m_yearIndex.insert(QStringLiteral("1971"), 1);
  m_yearIndex.insert(QStringLiteral("1972"), 2);
  m_yearIndex.insert(QStringLiteral("1973"), 3);
  m_yearIndex.insert(QStringLiteral("1974"), 4);
  m_yearIndex.insert(QStringLiteral("1975"), 5);
  m_yearIndex.insert(QStringLiteral("1976"), 6);
  m_yearIndex.insert(QStringLiteral("1977"), 7);
  m_yearIndex.insert(QStringLiteral("1978"), 8);
  m_yearIndex.insert(QStringLiteral("1979"), 9);
  m_yearIndex.insert(QStringLiteral("1980"), 11);
  m_yearIndex.insert(QStringLiteral("1981"), 12);
  m_yearIndex.insert(QStringLiteral("1982"), 13);
  m_yearIndex.insert(QStringLiteral("1983"), 14);
  m_yearIndex.insert(QStringLiteral("1984"), 15);
  m_yearIndex.insert(QStringLiteral("1985"), 16);
  m_yearIndex.insert(QStringLiteral("1986"), 17);
  m_yearIndex.insert(QStringLiteral("1987"), 18);
  m_yearIndex.insert(QStringLiteral("1988"), 19);
  m_yearIndex.insert(QStringLiteral("1989"), 20);
  m_yearIndex.insert(QStringLiteral("1990"), 22);
  m_yearIndex.insert(QStringLiteral("1991"), 23);
  m_yearIndex.insert(QStringLiteral("1992"), 24);
  m_yearIndex.insert(QStringLiteral("1993"), 25);
  m_yearIndex.insert(QStringLiteral("1994"), 26);
  m_yearIndex.insert(QStringLiteral("1995"), 27);
  m_yearIndex.insert(QStringLiteral("1996"), 28);
  m_yearIndex.insert(QStringLiteral("1997"), 29);
  m_yearIndex.insert(QStringLiteral("1998"), 30);
  m_yearIndex.insert(QStringLiteral("1999"), 31);
  m_yearIndex.insert(QStringLiteral("2000"), 34);
  m_yearIndex.insert(QStringLiteral("2001"), 35);
  m_yearIndex.insert(QStringLiteral("2002"), 36);
  m_yearIndex.insert(QStringLiteral("2003"), 37);
  m_yearIndex.insert(QStringLiteral("2004"), 38);
  m_yearIndex.insert(QStringLiteral("2005"), 39);
  m_yearIndex.insert(QStringLiteral("2006"), 44);
  m_yearIndex.insert(QStringLiteral("2007"), 107);
  m_yearIndex.insert(QStringLiteral("2008"), 150);
  m_yearIndex.insert(QStringLiteral("2009"), 151);
  m_yearIndex.insert(QStringLiteral("2010"), 163);
  m_yearIndex.insert(QStringLiteral("2011"), 165);
  m_yearIndex.insert(QStringLiteral("2012"), 168);
  m_yearIndex.insert(QStringLiteral("2013"), 170);
  m_yearIndex.insert(QStringLiteral("2014"), 171);
  m_yearIndex.insert(QStringLiteral("2015"), 172);
  m_yearIndex.insert(QStringLiteral("2016"), 173);
  m_yearIndex.insert(QStringLiteral("2017"), 174);
  m_yearIndex.insert(QStringLiteral("2018"), 175);
  m_yearIndex.insert(QStringLiteral("2019"), 176);
  m_yearIndex.insert(QStringLiteral("2020"), 178);
  m_yearIndex.insert(QStringLiteral("2021"), 179);
  m_yearIndex.insert(QStringLiteral("2022"), 180);
  m_yearIndex.insert(QStringLiteral("2023"), 181);
}

GamingHistoryFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const GamingHistoryFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  // now add additional fields widget
  addFieldsWidget(GamingHistoryFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

QString GamingHistoryFetcher::ConfigWidget::preferredName() const {
  return GamingHistoryFetcher::defaultName();
}
