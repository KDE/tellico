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
#include "../gui/guiproxy.h"
#include "../tellico_utils.h"
#include "../collections/videocollection.h"
#include "../entry.h"
#include "../fieldformat.h"
#include "../core/filehandler.h"
#include "../images/imagefactory.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <kconfig.h>
#include <kio/job.h>
#include <kio/jobuidelegate.h>

#include <QRegExp>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>

namespace {
  static const char* ANIMENFO_BASE_URL = "http://www.animenfo.com/search.php";
}

using namespace Tellico;
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
  return type == Data::Collection::Video;
}

void AnimeNfoFetcher::readConfigHook(const KConfigGroup& config_) {
  Q_UNUSED(config_);
}

void AnimeNfoFetcher::search() {
  m_started = true;
  m_matches.clear();

  KUrl u(ANIMENFO_BASE_URL);
  u.addQueryItem(QLatin1String("action"),   QLatin1String("Go"));
  u.addQueryItem(QLatin1String("option"),   QLatin1String("keywords"));
  u.addQueryItem(QLatin1String("queryin"),  QLatin1String("anime_titles"));

  switch(request().key) {
    case Keyword:
      u.addQueryItem(QLatin1String("query"), request().value);
      break;

    default:
      myWarning() << "key not recognized: " << request().key;
      stop();
      return;
  }
//  myDebug() << "url:" << u.url();

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  m_job->ui()->setWindow(GUI::Proxy::widget());
  connect(m_job, SIGNAL(result(KJob*)),
          SLOT(slotComplete(KJob*)));
}

void AnimeNfoFetcher::stop() {
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

void AnimeNfoFetcher::slotComplete(KJob*) {
//  myDebug();

  if(m_job->error()) {
    m_job->ui()->showErrorMessage();
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
  m_job = 0;

  QString s = Tellico::decodeHTML(data);

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
      FetchResult* r = new FetchResult(Fetcher::Ptr(this), t, y);
      KUrl url(KUrl(ANIMENFO_BASE_URL), u);
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
    FetchResult* r = new FetchResult(Fetcher::Ptr(this), t, y, QString());
    KUrl url(KUrl(ANIMENFO_BASE_URL), u);
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

  KUrl url = m_matches[uid_];
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
    t.setEncoding(QTextStream::UnicodeUTF8);
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

Tellico::Data::EntryPtr AnimeNfoFetcher::parseEntry(const QString& str_) {
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

  Data::CollPtr coll(new Data::VideoCollection(true));

  // add new fields
  Data::FieldPtr f(new Data::Field(QLatin1String("origtitle"), i18n("Original Title")));
  coll->addField(f);

  f = new Data::Field(QLatin1String("alttitle"), i18n("Alternative Titles"), Data::Field::Table);
  f->setFormatType(FieldFormat::FormatTitle);
  coll->addField(f);

  f = new Data::Field(QLatin1String("distributor"), i18n("Distributor"));
  f->setCategory(i18n("Other People"));
  f->setFlags(Data::Field::AllowCompletion | Data::Field::AllowMultiple | Data::Field::AllowGrouped);
  f->setFormatType(FieldFormat::FormatPlain);
  coll->addField(f);

  f = new Data::Field(QLatin1String("episodes"), i18n("Episodes"), Data::Field::Number);
  f->setCategory(i18n("Features"));
  coll->addField(f);

 // map captions in HTML to field names
  QHash<QString, QString> fieldMap;
  fieldMap.insert(QLatin1String("Title"), QLatin1String("title"));
  fieldMap.insert(QLatin1String("Japanese Title"), QLatin1String("origtitle"));
  fieldMap.insert(QLatin1String("Total Episodes"), QLatin1String("episodes"));
  fieldMap.insert(QLatin1String("Category"), QLatin1String("keyword"));
  fieldMap.insert(QLatin1String("Genres"), QLatin1String("genre"));
  fieldMap.insert(QLatin1String("Year Published"), QLatin1String("year"));
  fieldMap.insert(QLatin1String("Studio"), QLatin1String("studio"));
  fieldMap.insert(QLatin1String("US Distribution"), QLatin1String("distributor"));

  Data::EntryPtr entry(new Data::Entry(coll));

  QString fullTitle;
  
  int n = 0;
  QString key, value;
  for(int pos = infoRx.indexIn(s); pos > -1; pos = infoRx.indexIn(s, pos+1)) {
    if(n == 0 && !key.isEmpty()) {
      if(fieldMap.contains(key)) {
        value = value.simplified();
        if(value != QLatin1String("-")) {
          if(key == QLatin1String("Genres")) {
            entry->setField(fieldMap[key], value.split(QRegExp(QLatin1String("\\s*,\\s*"))).join(FieldFormat::delimiterString()));
          } else if(key == QLatin1String("Title")) {
            // strip possible trailing year, etc.
            fullTitle = value;
            value.remove(QRegExp(QLatin1String("\\s*\\([^)]*\\)$")));
            entry->setField(fieldMap[key], value);
          } else if(key == QLatin1String("Total Episodes")) {
            // strip possible trailing text
            value.remove(QRegExp(QLatin1String("[\\D].*$")));
            entry->setField(fieldMap[key], value);
          } else {
            entry->setField(fieldMap[key], value);
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
        value = infoRx.cap(1).remove(tagRx);
        break;
    }
    n = (n+1)%2;
  }

  // image
  QRegExp imgRx(QString::fromLatin1("<img\\s+[^>]*src\\s*=\\s*[\"']([^>]*)[\"']\\s+[^>]*alt\\s*=\\s*[\"']%1[\"']")
                                    .arg(QRegExp::escape(fullTitle)), Qt::CaseInsensitive);
  imgRx.setMinimal(true);
  int pos = imgRx.indexIn(s);
  if(pos > -1) {
    KUrl imgURL(KUrl(ANIMENFO_BASE_URL), imgRx.cap(1));
    QString id = ImageFactory::addImage(imgURL, true);
    if(!id.isEmpty()) {
      entry->setField(QLatin1String("cover"), id);
    } else {
      myDebug() << "bad cover" << imgURL.url();
    }
  }

  // now look for alternative titles and plot
  const QString a = QLatin1String("Alternative titles");
  pos = s.indexOf(a, 0, Qt::CaseInsensitive);
  if(pos > -1) {
    pos += a.length();
    int pos2 = s.indexOf(QLatin1String("<td class=\"anime_cat_left"), pos+1);
    if(pos2 > -1) {
      value = s.mid(pos, pos2-pos);
      value.replace(QLatin1String("<br />"), FieldFormat::rowDelimiterString());
      value = value.remove(tagRx).simplified();
      entry->setField(QLatin1String("alttitle"), value);
    }
  }
  QRegExp descRx(QLatin1String("<td\\s[^>]*class\\s*=\\s*[\"']description[\"'].*>(.*)</td"), Qt::CaseInsensitive);
  descRx.setMinimal(true);
  pos = descRx.indexIn(s);
  if(pos > -1) {
    entry->setField(QLatin1String("plot"), descRx.cap(1).simplified());
  }

  return entry;
}

Tellico::Fetch::FetchRequest AnimeNfoFetcher::updateRequest(Data::EntryPtr entry_) {
  QString t = entry_->field(QLatin1String("title"));
  if(!t.isEmpty()) {
    return FetchRequest(Fetch::Keyword, t);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* AnimeNfoFetcher::configWidget(QWidget* parent_) const {
  return new AnimeNfoFetcher::ConfigWidget(parent_, this);
}

QString AnimeNfoFetcher::defaultName() {
  return QLatin1String("AnimeNfo.com");
}

QString AnimeNfoFetcher::defaultIcon() {
  return favIcon("http://animenfo.com");
}

//static
Tellico::StringHash AnimeNfoFetcher::allOptionalFields() {
  StringHash hash;
  hash[QLatin1String("distributor")] = i18n("Distributor");
  hash[QLatin1String("episodes")]    = i18n("Episodes");
  hash[QLatin1String("origtitle")]   = i18n("Original Title");
  hash[QLatin1String("alttitle")]    = i18n("Alternative Titles");
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

#include "animenfofetcher.moc"
