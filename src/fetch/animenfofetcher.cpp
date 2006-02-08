/***************************************************************************
    copyright            : (C) 2006 by Robby Stephenson
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
#include "../latin1literal.h"
#include "../imagefactory.h"

#include <klocale.h>
#include <kconfig.h>

#include <qregexp.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qfile.h>

//#define ANIMENFO_TEST

namespace {
  static const char* ANIMENFO_BASE_URL = "http://www.animenfo.com/search.php";
}

using Tellico::Fetch::AnimeNfoFetcher;

AnimeNfoFetcher::AnimeNfoFetcher(QObject* parent_, const char* name_ /*=0*/)
    : Fetcher(parent_, name_), m_name(defaultName()) {
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

void AnimeNfoFetcher::readConfig(KConfig* config_, const QString& group_) {
  KConfigGroupSaver groupSaver(config_, group_);
  QString s = config_->readEntry("Name");
  if(!s.isEmpty()) {
    m_name = s;
  }
//  m_fields = config_->readListEntry("Custom Fields", QString::fromLatin1("keyword"));
}

void AnimeNfoFetcher::search(FetchKey key_, const QString& value_) {
  m_started = true;
  m_matches.clear();

#ifdef ANIMENFO_TEST
  KURL u = KURL::fromPathOrURL(QString::fromLatin1("/home/robby/animenfo.html"));
#else
  KURL u(QString::fromLatin1(ANIMENFO_BASE_URL));
  u.addQueryItem(QString::fromLatin1("action"),   QString::fromLatin1("Go"));
  u.addQueryItem(QString::fromLatin1("option"),   QString::fromLatin1("keywords"));
  u.addQueryItem(QString::fromLatin1("queryin"),    QString::fromLatin1("anime_titles"));

  if(!canFetch(Kernel::self()->collectionType())) {
    message(i18n("%1 does not allow searching for this collection type.").arg(source()), MessageHandler::Warning);
    stop();
    return;
  }

  switch(key_) {
    case Keyword:
      u.addQueryItem(QString::fromLatin1("query"), value_);
      break;

    default:
      kdWarning() << "AnimeNfoFetcher::search() - key not recognized: " << key_ << endl;
      stop();
      return;
  }
#endif
//  myDebug() << "AnimeNfoFetcher::search() - url: " << u.url() << endl;

  m_job = KIO::get(u, false, false);
  connect(m_job, SIGNAL(data(KIO::Job*, const QByteArray&)),
          SLOT(slotData(KIO::Job*, const QByteArray&)));
  connect(m_job, SIGNAL(result(KIO::Job*)),
          SLOT(slotComplete(KIO::Job*)));
}

void AnimeNfoFetcher::stop() {
  if(!m_started) {
    return;
  }

  if(m_job) {
    m_job->kill();
    m_job = 0;
  }
  m_data.truncate(0);
  m_started = false;
  emit signalDone(this);
}

void AnimeNfoFetcher::slotData(KIO::Job*, const QByteArray& data_) {
  QDataStream stream(m_data, IO_WriteOnly | IO_Append);
  stream.writeRawBytes(data_.data(), data_.size());
}

void AnimeNfoFetcher::slotComplete(KIO::Job* job_) {
//  myDebug() << "AnimeNfoFetcher::slotComplete()" << endl;
  // since the fetch is done, don't worry about holding the job pointer
  m_job = 0;

  if(job_->error()) {
    job_->showErrorDialog(Kernel::self()->widget());
    stop();
    return;
  }

  if(m_data.isEmpty()) {
    myDebug() << "AnimeNfoFetcher::slotComplete() - no data" << endl;
    stop();
    return;
  }

  QString s = Tellico::decodeHTML(QString(m_data));

  QRegExp infoRx(QString::fromLatin1("<td\\s+[^>]*class\\s*=\\s*[\"']anime_info[\"'][^>]*>(.*)</td>"), false);
  infoRx.setMinimal(true);
  QRegExp anchorRx(QString::fromLatin1("<a\\s+[^>]*href\\s*=\\s*[\"'](.*)[\"'][^>]*>(.*)</a>"), false);
  anchorRx.setMinimal(true);
  QRegExp yearRx(QString::fromLatin1("\\d{4}"), false);

  // search page comes in groups of threes
  int n = 0;
  QString u, t, y;

  for(int pos = infoRx.search(s); m_started && pos > -1; pos = infoRx.search(s, pos+1)) {
    if(n == 0 && !u.isEmpty()) {
      SearchResult* r = new SearchResult(this, t, y);
      emit signalResultFound(r);

#ifdef ANIMENFO_TEST
      KURL url = KURL::fromPathOrURL(QString::fromLatin1("/home/robby/animetitle.html"));
#else
      KURL url(QString::fromLatin1(ANIMENFO_BASE_URL), u);
      url.setQuery(QString::null);
#endif
      m_matches.insert(r->uid, url);

      u.truncate(0);
      t.truncate(0);
      y.truncate(0);
    }
    switch(n) {
      case 0: // title and url
        {
          int pos2 = anchorRx.search(infoRx.cap(1));
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
    SearchResult* r = new SearchResult(this, t, y);
    emit signalResultFound(r);
    KURL url(QString::fromLatin1(ANIMENFO_BASE_URL), u);
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

  KURL url = m_matches[uid_];
  if(url.isEmpty()) {
    kdWarning() << "AnimeNfoFetcher::fetchEntry() - no url in map" << endl;
    return 0;
  }

  QString results = Tellico::decodeHTML(FileHandler::readTextFile(url, true));
  if(results.isEmpty()) {
    myDebug() << "AnimeNfoFetcher::fetchEntry() - no text results" << endl;
    return 0;
  }

#if 0
  kdWarning() << "Remove debug from animenfofetcher.cpp" << endl;
  QFile f(QString::fromLatin1("/tmp/test.html"));
  if(f.open(IO_WriteOnly)) {
    QTextStream t(&f);
    t.setEncoding(QTextStream::UnicodeUTF8);
    t << results;
  }
  f.close();
#endif

  entry = parseEntry(results);
  if(!entry) {
    myDebug() << "AnimeNfoFetcher::fetchEntry() - error in processing entry" << endl;
    return 0;
  }
  m_entries.insert(uid_, entry); // keep for later
  return entry;
}

Tellico::Data::EntryPtr AnimeNfoFetcher::parseEntry(const QString& str_) {
 // myDebug() << "AnimeNfoFetcher::parseEntry()" << endl;
 // class might be anime_info_top
  QRegExp infoRx(QString::fromLatin1("<td\\s+[^>]*class\\s*=\\s*[\"']anime_info[^>]*>(.*)</td>"), false);
  infoRx.setMinimal(true);
  QRegExp tagRx(QString::fromLatin1("<.*>"));
  tagRx.setMinimal(true);
  QRegExp anchorRx(QString::fromLatin1("<a\\s+[^>]*href\\s*=\\s*[\"'](.*)[\"'][^>]*>(.*)</a>"), false);
  anchorRx.setMinimal(true);
  QRegExp jsRx(QString::fromLatin1("<script.*</script>"), false);
  jsRx.setMinimal(true);

  QString s = str_;
  s.remove(jsRx);

  Data::CollPtr coll = new Data::VideoCollection(true);

  // add new fields
  Data::FieldPtr f = new Data::Field(QString::fromLatin1("origtitle"), i18n("Original Title"));
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

  Data::EntryPtr entry = new Data::Entry(coll);

  int n = 0;
  QString key, value;
  int oldpos = -1;
  for(int pos = infoRx.search(s); pos > -1; pos = infoRx.search(s, pos+1)) {
    if(n == 0 && !key.isEmpty()) {
      if(fieldMap.contains(key)) {
        value = value.simplifyWhiteSpace();
        if(value.length() > 2) { // might be "-"
          if(key == Latin1Literal("Genres")) {
            entry->setField(fieldMap[key], QStringList::split(QRegExp(QString::fromLatin1("\\s*,\\s*")),
                                                              value).join(QString::fromLatin1("; ")));
          } else {
            entry->setField(fieldMap[key], value);
          }
        }
      }
      key.truncate(0);
      value.truncate(0);
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
                                    .arg(entry->field(QString::fromLatin1("title"))), false);
  imgRx.setMinimal(true);
  int pos = imgRx.search(s);
  if(pos > -1) {
    KURL imgURL(QString::fromLatin1(ANIMENFO_BASE_URL), imgRx.cap(1));
    const Data::Image& img = ImageFactory::addImage(imgURL, true);
    if(!img.isNull()) {
      entry->setField(QString::fromLatin1("cover"), img.id());
    }
  }

  // now look for alternative titles and plot
  const QString a = QString::fromLatin1("Alternative titles");
  pos = s.find(a, oldpos+1, false);
  if(pos > -1) {
    pos += a.length();
  }
  int pos2 = -1;
  if(pos > -1) {
    pos2 = s.find(QString::fromLatin1("Description"), pos+1, true);
    if(pos2 > -1) {
      value = s.mid(pos, pos2-pos).remove(tagRx).simplifyWhiteSpace();
      entry->setField(QString::fromLatin1("alttitle"), value);
    }
  }
  QRegExp descRx(QString::fromLatin1("class\\s*=\\s*[\"']description[\"'][^>]*>(.*)<"), false);
  descRx.setMinimal(true);
  pos = descRx.search(s, QMAX(pos, pos2));
  if(pos > -1) {
    entry->setField(QString::fromLatin1("plot"), descRx.cap(1).simplifyWhiteSpace());
  }

  return entry;
}

void AnimeNfoFetcher::updateEntry(Data::EntryPtr entry_) {
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

#include "animenfofetcher.moc"
