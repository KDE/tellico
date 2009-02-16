/***************************************************************************
    copyright            : (C) 2006-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "animenfofetcher.h"
#include "messagehandler.h"
#include "../tellico_kernel.h"
#include "../tellico_utils.h"
#include "../collections/videocollection.h"
#include "../entry.h"
#include "../filehandler.h"
#include "../imagefactory.h"
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

//#define ANIMENFO_TEST

namespace {
  static const char* ANIMENFO_BASE_URL = "http://www.animenfo.com/search.php";
}

using Tellico::Fetch::AnimeNfoFetcher;

AnimeNfoFetcher::AnimeNfoFetcher(QObject* parent_)
    : Fetcher(parent_), m_started(false) {
}

QString AnimeNfoFetcher::defaultName() {
  return QString::fromLatin1("AnimeNfo.com");
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

void AnimeNfoFetcher::search(Tellico::Fetch::FetchKey key_, const QString& value_) {
  m_started = true;
  m_matches.clear();

#ifdef ANIMENFO_TEST
  KUrl u = KUrl(QString::fromLatin1("/home/robby/animenfo.html"));
#else
  KUrl u(QString::fromLatin1(ANIMENFO_BASE_URL));
  u.addQueryItem(QString::fromLatin1("action"),   QString::fromLatin1("Go"));
  u.addQueryItem(QString::fromLatin1("option"),   QString::fromLatin1("keywords"));
  u.addQueryItem(QString::fromLatin1("queryin"),    QString::fromLatin1("anime_titles"));

  if(!canFetch(Kernel::self()->collectionType())) {
    message(i18n("%1 does not allow searching for this collection type.", source()), MessageHandler::Warning);
    stop();
    return;
  }

  switch(key_) {
    case Keyword:
      u.addQueryItem(QString::fromLatin1("query"), value_);
      break;

    default:
      kWarning() << "AnimeNfoFetcher::search() - key not recognized: " << key_;
      stop();
      return;
  }
#endif
//  myDebug() << "AnimeNfoFetcher::search() - url: " << u.url() << endl;

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  m_job->ui()->setWindow(Kernel::self()->widget());
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
//  myDebug() << "AnimeNfoFetcher::slotComplete()" << endl;

  if(m_job->error()) {
    m_job->ui()->showErrorMessage();
    stop();
    return;
  }

  QByteArray data = m_job->data();
  if(data.isEmpty()) {
    myDebug() << "AnimeNfoFetcher::slotComplete() - no data" << endl;
    stop();
    return;
  }

  // since the fetch is done, don't worry about holding the job pointer
  m_job = 0;

  QString s = Tellico::decodeHTML(QString(data));

  QRegExp infoRx(QString::fromLatin1("<td\\s+[^>]*class\\s*=\\s*[\"']anime_info[\"'][^>]*>(.*)</td>"), Qt::CaseInsensitive);
  infoRx.setMinimal(true);
  QRegExp anchorRx(QString::fromLatin1("<a\\s+[^>]*href\\s*=\\s*[\"'](.*)[\"'][^>]*>(.*)</a>"), Qt::CaseInsensitive);
  anchorRx.setMinimal(true);
  QRegExp yearRx(QString::fromLatin1("\\d{4}"));

  // search page comes in groups of threes
  int n = 0;
  QString u, t, y;

  for(int pos = infoRx.indexIn(s); m_started && pos > -1; pos = infoRx.indexIn(s, pos+1)) {
    if(n == 0 && !u.isEmpty()) {
      SearchResult* r = new SearchResult(Fetcher::Ptr(this), t, y, QString());
      emit signalResultFound(r);

#ifdef ANIMENFO_TEST
      KUrl url = KUrl(QString::fromLatin1("/home/robby/animetitle.html"));
#else
      KUrl url(QString::fromLatin1(ANIMENFO_BASE_URL), u);
      url.setQuery(QString::null);
#endif
      m_matches.insert(r->uid, url);

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
#ifndef ANIMENFO_TEST
  if(!u.isEmpty()) {
    SearchResult* r = new SearchResult(Fetcher::Ptr(this), t, y, QString());
    emit signalResultFound(r);
    KUrl url(QString::fromLatin1(ANIMENFO_BASE_URL), u);
    url.setQuery(QString::null);
    m_matches.insert(r->uid, url);
  }
#endif
  stop();
}

Tellico::Data::EntryPtr AnimeNfoFetcher::fetchEntry(uint uid_) {
  // if we already grabbed this one, then just pull it out of the dict
  Data::EntryPtr entry = m_entries[uid_];
  if(entry) {
    return entry;
  }

  KUrl url = m_matches[uid_];
  if(url.isEmpty()) {
    kWarning() << "AnimeNfoFetcher::fetchEntry() - no url in map";
    return Data::EntryPtr();
  }

  QString results = Tellico::decodeHTML(FileHandler::readTextFile(url, true));
  if(results.isEmpty()) {
    myDebug() << "AnimeNfoFetcher::fetchEntry() - no text results" << endl;
    return Data::EntryPtr();
  }

#if 0
  kWarning() << "Remove debug from animenfofetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setEncoding(QTextStream::UnicodeUTF8);
    t << results;
  }
  f.close();
#endif

  entry = parseEntry(results);
  if(!entry) {
    myDebug() << "AnimeNfoFetcher::fetchEntry() - error in processing entry" << endl;
    return Data::EntryPtr();
  }
  m_entries.insert(uid_, entry); // keep for later
  return entry;
}

Tellico::Data::EntryPtr AnimeNfoFetcher::parseEntry(const QString& str_) {
 // myDebug() << "AnimeNfoFetcher::parseEntry()" << endl;
 // class might be anime_info_top
  QRegExp infoRx(QString::fromLatin1("<td\\s+[^>]*class\\s*=\\s*[\"']anime_info[^>]*>(.*)</td>"), Qt::CaseInsensitive);
  infoRx.setMinimal(true);
  QRegExp tagRx(QString::fromLatin1("<.*>"));
  tagRx.setMinimal(true);
  QRegExp anchorRx(QString::fromLatin1("<a\\s+[^>]*href\\s*=\\s*[\"'](.*)[\"'][^>]*>(.*)</a>"), Qt::CaseInsensitive);
  anchorRx.setMinimal(true);
  QRegExp jsRx(QString::fromLatin1("<script.*</script>"), Qt::CaseInsensitive);
  jsRx.setMinimal(true);

  QString s = str_;
  s.remove(jsRx);

  Data::CollPtr coll(new Data::VideoCollection(true));

  // add new fields
  Data::FieldPtr f(new Data::Field(QString::fromLatin1("origtitle"), i18n("Original Title")));
  coll->addField(f);

  f = new Data::Field(QString::fromLatin1("alttitle"), i18n("Alternative Titles"), Data::Field::Table);
  f->setFormatFlag(Data::Field::FormatTitle);
  coll->addField(f);

  f = new Data::Field(QString::fromLatin1("distributor"), i18n("Distributor"));
  f->setCategory(i18n("Other People"));
  f->setFlags(Data::Field::AllowCompletion | Data::Field::AllowMultiple | Data::Field::AllowGrouped);
  f->setFormatFlag(Data::Field::FormatPlain);
  coll->addField(f);

  f = new Data::Field(QString::fromLatin1("episodes"), i18n("Episodes"), Data::Field::Number);
  f->setCategory(i18n("Features"));
  coll->addField(f);

 // map captions in HTML to field names
  QMap<QString, QString> fieldMap;
  fieldMap.insert(QString::fromLatin1("Title"), QString::fromLatin1("title"));
  fieldMap.insert(QString::fromLatin1("Japanese Title"), QString::fromLatin1("origtitle"));
  fieldMap.insert(QString::fromLatin1("Total Episodes"), QString::fromLatin1("episodes"));
  fieldMap.insert(QString::fromLatin1("Genres"), QString::fromLatin1("genre"));
  fieldMap.insert(QString::fromLatin1("Year Published"), QString::fromLatin1("year"));
  fieldMap.insert(QString::fromLatin1("Studio"), QString::fromLatin1("studio"));
  fieldMap.insert(QString::fromLatin1("US Distribution"), QString::fromLatin1("distributor"));

  Data::EntryPtr entry(new Data::Entry(coll));

  int n = 0;
  QString key, value;
  int oldpos = -1;
  for(int pos = infoRx.indexIn(s); pos > -1; pos = infoRx.indexIn(s, pos+1)) {
    if(n == 0 && !key.isEmpty()) {
      if(fieldMap.contains(key)) {
        value = value.simplified();
        if(value.length() > 2) { // might be "-"
          if(key == QLatin1String("Genres")) {
            entry->setField(fieldMap[key], value.split(QRegExp(QString::fromLatin1("\\s*,\\s*"))).join(QString::fromLatin1("; ")));
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
    oldpos = pos;
  }

  // image
  QRegExp imgRx(QString::fromLatin1("<img\\s+[^>]*src\\s*=\\s*[\"']([^>]*)[\"']\\s+[^>]*alt\\s*=\\s*[\"']%1[\"']")
                                    .arg(entry->field(QString::fromLatin1("title"))), Qt::CaseInsensitive);
  imgRx.setMinimal(true);
  int pos = imgRx.indexIn(s);
  if(pos > -1) {
    KUrl imgURL(QString::fromLatin1(ANIMENFO_BASE_URL), imgRx.cap(1));
    QString id = ImageFactory::addImage(imgURL, true);
    if(!id.isEmpty()) {
      entry->setField(QString::fromLatin1("cover"), id);
    }
  }

  // now look for alternative titles and plot
  const QString a = QString::fromLatin1("Alternative titles");
  pos = s.indexOf(a, oldpos+1, Qt::CaseInsensitive);
  if(pos > -1) {
    pos += a.length();
  }
  int pos2 = -1;
  if(pos > -1) {
    pos2 = s.indexOf(QString::fromLatin1("Description"), pos+1, Qt::CaseInsensitive);
    if(pos2 > -1) {
      value = s.mid(pos, pos2-pos).remove(tagRx).simplified();
      entry->setField(QString::fromLatin1("alttitle"), value);
    }
  }
  QRegExp descRx(QString::fromLatin1("class\\s*=\\s*[\"']description[\"'][^>]*>(.*)<"), Qt::CaseInsensitive);
  descRx.setMinimal(true);
  pos = descRx.indexIn(s, qMax(pos, pos2));
  if(pos > -1) {
    entry->setField(QString::fromLatin1("plot"), descRx.cap(1).simplified());
  }

  return entry;
}

void AnimeNfoFetcher::updateEntry(Tellico::Data::EntryPtr entry_) {
  QString t = entry_->field(QString::fromLatin1("title"));
  if(!t.isEmpty()) {
    search(Fetch::Keyword, t);
    return;
  }
  emit signalDone(this); // always need to emit this if not continuing with the search
}

Tellico::Fetch::ConfigWidget* AnimeNfoFetcher::configWidget(QWidget* parent_) const {
  return new AnimeNfoFetcher::ConfigWidget(parent_);
}

AnimeNfoFetcher::ConfigWidget::ConfigWidget(QWidget* parent_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();
}

QString AnimeNfoFetcher::ConfigWidget::preferredName() const {
  return AnimeNfoFetcher::defaultName();
}

#include "animenfofetcher.moc"
