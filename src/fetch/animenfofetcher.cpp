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

#include "animenfofetcher.h"
#include "../utils/guiproxy.h"
#include "../utils/string_utils.h"
#include "../collections/bookcollection.h"
#include "../collections/videocollection.h"
#include "../entry.h"
#include "../fieldformat.h"
#include "../core/filehandler.h"
#include "../images/imagefactory.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfig>
#include <KIO/Job>
#include <KIO/JobUiDelegate>
#include <KJobWidgets/KJobWidgets>

#include <QRegExp>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QUrlQuery>

namespace {
  static const char* ANIMENFO_BASE_URL = "http://www.animenfo.com/search.php";
}

using Tellico::Fetch::AnimeNfoFetcher;

AnimeNfoFetcher::AnimeNfoFetcher(QObject* parent_)
    : Fetcher(parent_), m_started(false) {
}

AnimeNfoFetcher::~AnimeNfoFetcher() {
}

QString AnimeNfoFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool AnimeNfoFetcher::canFetch(int type) const {
  return type == Data::Collection::Book ||
         type == Data::Collection::Bibtex ||
         type == Data::Collection::Video;
}

void AnimeNfoFetcher::readConfigHook(const KConfigGroup& config_) {
  Q_UNUSED(config_);
}

void AnimeNfoFetcher::search() {
  m_started = true;
  m_matches.clear();

  QUrl u(QString::fromLatin1(ANIMENFO_BASE_URL));
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("action"),   QStringLiteral("Go"));
  q.addQueryItem(QStringLiteral("option"),   QStringLiteral("keywords"));

  switch(request().collectionType()) {
    case Data::Collection::Book:
      q.addQueryItem(QStringLiteral("queryin"),  QStringLiteral("manga_titles"));
      break;

    case Data::Collection::Video:
      q.addQueryItem(QStringLiteral("queryin"),  QStringLiteral("anime_titles"));
      break;

    default:
      myWarning() << "collection type not valid:" << request().collectionType();
      stop();
      return;
  }

  switch(request().key()) {
    case Keyword:
      q.addQueryItem(QStringLiteral("query"), request().value());
      break;

    default:
      myWarning() << "key not recognized: " << request().key();
      stop();
      return;
  }
  u.setQuery(q);
//  myDebug() << "url:" << u;

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result,
          this, &AnimeNfoFetcher::slotComplete);
}

void AnimeNfoFetcher::stop() {
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

void AnimeNfoFetcher::slotComplete(KJob*) {
//  myDebug();

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

  QString s = Tellico::decodeHTML(data);
#if 0
  myWarning() << "Remove debug from animenfofetcher.cpp";
  QFile f(QLatin1String("/tmp/test.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << s;
  }
  f.close();
#endif

  QRegExp infoRx(QLatin1String("<td\\s+[^>]*class\\s*=\\s*[\"']anime_info[\"'][^>]*>(.*)</td>"), Qt::CaseInsensitive);
  infoRx.setMinimal(true);
  QRegExp anchorRx(QLatin1String("<a\\s+[^>]*href\\s*=\\s*[\"'](.*)[\"'][^>]*>(.*)</a>"), Qt::CaseInsensitive);
  anchorRx.setMinimal(true);
  QRegExp yearRx(QLatin1String("\\d{4}"));

  // search page comes in groups of threes
  int n = 0;
  QString u, t, y;

  for(int pos = infoRx.indexIn(s); m_started && pos > -1; pos = infoRx.indexIn(s, pos+1)) {
    if(n == 0 && !u.isEmpty()) {
      FetchResult* r = new FetchResult(this, t, y);
      QUrl url = QUrl(QString::fromLatin1(ANIMENFO_BASE_URL)).resolved(QUrl(u));
      url.setQuery(QString());
      m_matches.insert(r->uid, url);
      // don't emit signal until after putting url in matches hash
      emit signalResultFound(r);

      u.clear();
      t.clear();
      y.clear();
    }
    switch(n) {
      case 0: // title and url
        {
          int pos2 = anchorRx.indexIn(infoRx.cap(1));
          if(pos2 > -1) {
            u = anchorRx.cap(1);
            t = anchorRx.cap(2);
          }
        }
        break;
      case 1: // don't case
        break;
      case 2:
        if(yearRx.exactMatch(infoRx.cap(1))) {
          y = infoRx.cap(1);
        }
        break;
    }

    n = (n+1)%3;
  }

  // grab last response
  if(!u.isEmpty()) {
    FetchResult* r = new FetchResult(this, t, y, QString());
    QUrl url = QUrl(QString::fromLatin1(ANIMENFO_BASE_URL)).resolved(QUrl(u));
    url.setQuery(QString());
    m_matches.insert(r->uid, url);
    // don't emit signal until after putting url in matches hash
    emit signalResultFound(r);
  }

  stop();
}

Tellico::Data::EntryPtr AnimeNfoFetcher::fetchEntryHook(uint uid_) {
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

  QString results = Tellico::decodeHTML(FileHandler::readTextFile(url, true, true));
  if(results.isEmpty()) {
    myDebug() << "no text results";
    return Data::EntryPtr();
  }

#if 0
  myWarning() << "Remove debug from animenfofetcher.cpp";
  QFile f(QLatin1String("/tmp/test.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << results;
  }
  f.close();
#endif

  entry = parseEntry(results, url);
  if(!entry) {
    myDebug() << "error in processing entry";
    return Data::EntryPtr();
  }
  m_entries.insert(uid_, entry); // keep for later
  return entry;
}

Tellico::Data::EntryPtr AnimeNfoFetcher::parseEntry(const QString& str_, const QUrl& url_) {
 // myDebug();
 // class might be anime_info_top
  QRegExp infoRx(QLatin1String("<td\\s+[^>]*class\\s*=\\s*[\"']anime_info[^>]*>(.*)</td>"), Qt::CaseInsensitive);
  infoRx.setMinimal(true);
  QRegExp tagRx(QLatin1String("<.*>"));
  tagRx.setMinimal(true);
  QRegExp anchorRx(QLatin1String("<a\\s+[^>]*href\\s*=\\s*[\"'](.*)[\"'][^>]*>(.*)</a>"), Qt::CaseInsensitive);
  anchorRx.setMinimal(true);
  QRegExp jsRx(QLatin1String("<script.*</script>"), Qt::CaseInsensitive);
  jsRx.setMinimal(true);

  QString s = str_;
  s.remove(jsRx);

  Data::CollPtr coll;
  switch(request().collectionType()) {
    case Data::Collection::Book:
    case Data::Collection::Bibtex:
      coll = Data::CollPtr(new Data::BookCollection(true));
      break;

    case Data::Collection::Video:
      coll = Data::CollPtr(new Data::VideoCollection(true));
      break;

    default:
      return Data::EntryPtr();
  }

  // add new fields
  Data::FieldPtr f(new Data::Field(QStringLiteral("origtitle"), i18n("Original Title")));
  coll->addField(f);

  f = new Data::Field(QStringLiteral("alttitle"), i18n("Alternative Titles"), Data::Field::Table);
  f->setFormatType(FieldFormat::FormatTitle);
  coll->addField(f);

  f = new Data::Field(QStringLiteral("distributor"), i18n("Distributor"));
  f->setCategory(i18n("Other People"));
  f->setFlags(Data::Field::AllowCompletion | Data::Field::AllowMultiple | Data::Field::AllowGrouped);
  f->setFormatType(FieldFormat::FormatPlain);
  coll->addField(f);

  f = new Data::Field(QStringLiteral("episodes"), i18n("Episodes"), Data::Field::Number);
  f->setCategory(i18n("Features"));
  coll->addField(f);

  f = new Data::Field(QStringLiteral("animenfo"), i18n("AnimeNfo Link"), Data::Field::URL);
  f->setCategory(i18n("General"));
  coll->addField(f);

  f = new Data::Field(QStringLiteral("animenfo-rating"), i18n("AnimeNfo Rating"), Data::Field::Rating);
  f->setCategory(i18n("General"));
  f->setProperty(QStringLiteral("maximum"), QStringLiteral("10"));
  coll->addField(f);

 // map captions in HTML to field names
  QHash<QString, QString> fieldMap;
  fieldMap.insert(QStringLiteral("Title"), QStringLiteral("title"));
  fieldMap.insert(QStringLiteral("Japanese Title"), QStringLiteral("origtitle"));
  fieldMap.insert(QStringLiteral("Total Episodes"), QStringLiteral("episodes"));
  fieldMap.insert(QStringLiteral("Category"), QStringLiteral("keyword"));
  fieldMap.insert(QStringLiteral("Genres"), QStringLiteral("genre"));
  fieldMap.insert(QStringLiteral("Genre"), QStringLiteral("genre"));
  fieldMap.insert(QStringLiteral("Studio"), QStringLiteral("studio"));
  fieldMap.insert(QStringLiteral("US Distribution"), QStringLiteral("distributor"));
  fieldMap.insert(QStringLiteral("Author"), QStringLiteral("author"));
  fieldMap.insert(QStringLiteral("Publisher"), QStringLiteral("publisher"));
  fieldMap.insert(QStringLiteral("Director"), QStringLiteral("director"));
  fieldMap.insert(QStringLiteral("Script"), QStringLiteral("writer"));
  fieldMap.insert(QStringLiteral("Music"), QStringLiteral("composer"));
  fieldMap.insert(QStringLiteral("User Rating"), QStringLiteral("animenfo-rating"));

  switch(request().collectionType()) {
    case Data::Collection::Book:
    case Data::Collection::Bibtex:
      fieldMap.insert(QStringLiteral("Year Published"), QStringLiteral("pub_year"));
      break;
    case Data::Collection::Video:
      fieldMap.insert(QStringLiteral("Year Published"), QStringLiteral("year"));
      break;
    default:
      break;
  }

  Data::EntryPtr entry(new Data::Entry(coll));

  QString fullTitle;

  int n = 0;
  QString key, value;
  for(int pos = infoRx.indexIn(s); pos > -1; pos = infoRx.indexIn(s, pos+1)) {
    if(n == 0 && !key.isEmpty()) {
      if(fieldMap.contains(key)) {
        value = value.simplified();
        if(value.endsWith(QLatin1Char(';'))) {
          value.chop(1);
        }
        if(!value.isEmpty() && value != QLatin1String("-")) {
          const QString fieldName = fieldMap.value(key);
          if(key == QLatin1String("Title")) {
            // strip possible trailing year, etc.
            fullTitle = value;
            value.remove(QRegExp(QLatin1String("\\s*\\([^)]*\\)$")));
            entry->setField(fieldName, value);
          } else if(key == QLatin1String("Total Episodes")) {
            // strip possible trailing text
            value.remove(QRegExp(QLatin1String("[\\D].*$")));
            entry->setField(fieldName, value);
          } else if(key == QLatin1String("User Rating")) {
            QRegExp rating(QLatin1String("^(.*)/10"));
            if(rating.indexIn(value) > -1) {
              const double d = rating.cap(1).toDouble();
              entry->setField(fieldName, QString::number(static_cast<int>(d+0.5)));
            }
          } else if(key == QLatin1String("Year Published")) {
            // strip possible trailing text
            value.remove(QRegExp(QLatin1String("[\\D;].*$")));
            entry->setField(fieldName, value);
          } else {
            entry->setField(fieldName, value);
          }
          if(fieldName == QLatin1String("studio") ||
             fieldName == QLatin1String("genre") ||
             fieldName == QLatin1String("script") ||
             fieldName == QLatin1String("distributor") ||
             fieldName == QLatin1String("director") ||
             fieldName == QLatin1String("writer") ||
             fieldName == QLatin1String("author") ||
             fieldName == QLatin1String("publisher") ||
             fieldName == QLatin1String("composer")) {
            QStringList values = entry->field(fieldName).split(QRegExp(QLatin1String("\\s*,\\s*")));
            entry->setField(fieldName, values.join(FieldFormat::delimiterString()));
          }
        }
      }
      key.clear();
      value.clear();
    }
    switch(n) {
      case 0:
        key = infoRx.cap(1).remove(tagRx);
        break;
      case 1:
        value = infoRx.cap(1).replace(QLatin1String("<br />"), QLatin1String("; ")).remove(tagRx);
        break;
    }
    n = (n+1)%2;
  }
  entry->setField(QStringLiteral("animenfo"), url_.url());

  // image
  QRegExp imgRx(QStringLiteral("<img\\s+[^>]*src\\s*=\\s*[\"']([^>]*)[\"']\\s+[^>]*alt\\s*=\\s*[\"']%1[\"']")
                                    .arg(QRegExp::escape(fullTitle)), Qt::CaseInsensitive);
  imgRx.setMinimal(true);
  int pos = imgRx.indexIn(s);
  if(pos > -1) {
    QUrl imgURL = QUrl(QLatin1String(ANIMENFO_BASE_URL)).resolved(QUrl(imgRx.cap(1)));
    QString id = ImageFactory::addImage(imgURL, true);
    if(!id.isEmpty()) {
      entry->setField(QStringLiteral("cover"), id);
    } else {
      myDebug() << "bad cover" << imgURL.url();
    }
  }

  // now look for alternative titles and plot
  const QString a = QStringLiteral("Alternative titles");
  pos = s.indexOf(a, 0, Qt::CaseInsensitive);
  if(pos > -1) {
    pos += a.length();
    int pos2 = s.indexOf(QLatin1String("<td class=\"anime_cat_left"), pos+1);
    if(pos2 > -1) {
      value = s.mid(pos, pos2-pos).simplified();
      value.replace(QLatin1String("<br />"), FieldFormat::rowDelimiterString());
      value = value.remove(tagRx).trimmed();
      entry->setField(QStringLiteral("alttitle"), value);
    }
  }

  pos = s.indexOf(QLatin1String("Description"), pos > -1 ? pos : 0);
  if(pos > -1) {
    QRegExp descRx(QLatin1String("<td\\s[^>]*class\\s*=\\s*[\"']description[\"'].*>(.*)</td"), Qt::CaseInsensitive);
    descRx.setMinimal(true);
    pos = descRx.indexIn(s, pos+1);
    if(pos > -1) {
      entry->setField(QStringLiteral("plot"), descRx.cap(1).remove(tagRx).simplified());
    }
  }

  pos = s.indexOf(QLatin1String("Voice Talent"));
  if(pos > -1) {
    QRegExp charRx(QLatin1String("<a href=['\"]/anime/character/display.php.*>(.*)</a>"), Qt::CaseInsensitive);
    charRx.setMinimal(true);
    QRegExp voiceRx(QLatin1String("<a href=['\"]animeseiyuu.*>(.*)</a>"), Qt::CaseInsensitive);
    voiceRx.setMinimal(true);
    QStringList castLines;
    for(pos = s.indexOf(charRx, pos); pos > -1; pos = s.indexOf(charRx, pos+1)) {
      if(voiceRx.indexIn(s, pos) > -1) {
        castLines << voiceRx.cap(1) + FieldFormat::columnDelimiterString() + charRx.cap(1);
      }
    }
    entry->setField(QStringLiteral("cast"), castLines.join(FieldFormat::rowDelimiterString()));
  }

  return entry;
}

Tellico::Fetch::FetchRequest AnimeNfoFetcher::updateRequest(Data::EntryPtr entry_) {
  QString t = entry_->field(QStringLiteral("title"));
  if(!t.isEmpty()) {
    return FetchRequest(Fetch::Keyword, t);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* AnimeNfoFetcher::configWidget(QWidget* parent_) const {
  return new AnimeNfoFetcher::ConfigWidget(parent_, this);
}

QString AnimeNfoFetcher::defaultName() {
  return QStringLiteral("AnimeNfo.com");
}

QString AnimeNfoFetcher::defaultIcon() {
  return favIcon("http://animenfo.com");
}

//static
Tellico::StringHash AnimeNfoFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("distributor")]     = i18n("Distributor");
  hash[QStringLiteral("episodes")]        = i18n("Episodes");
  hash[QStringLiteral("origtitle")]       = i18n("Original Title");
  hash[QStringLiteral("alttitle")]        = i18n("Alternative Titles");
  hash[QStringLiteral("animenfo-rating")] = i18n("AnimeNfo Rating");
  hash[QStringLiteral("animenfo")]        = i18n("AnimeNfo Link");
  return hash;
}

AnimeNfoFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const AnimeNfoFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  // now add additional fields widget
  addFieldsWidget(AnimeNfoFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

QString AnimeNfoFetcher::ConfigWidget::preferredName() const {
  return AnimeNfoFetcher::defaultName();
}
