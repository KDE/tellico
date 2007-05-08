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

#include "ibsfetcher.h"
#include "messagehandler.h"
#include "../tellico_kernel.h"
#include "../tellico_utils.h"
#include "../collections/bookcollection.h"
#include "../entry.h"
#include "../filehandler.h"
#include "../latin1literal.h"
#include "../imagefactory.h"

#include <klocale.h>
#include <kconfig.h>
#include <kio/job.h>

#include <qregexp.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qfile.h>

//#define IBS_TEST

namespace {
  static const char* IBS_BASE_URL = "http://www.internetbookshop.it/ser/serpge.asp";
}

using Tellico::Fetch::IBSFetcher;

IBSFetcher::IBSFetcher(QObject* parent_, const char* name_ /*=0*/)
    : Fetcher(parent_, name_), m_started(false) {
}

QString IBSFetcher::defaultName() {
  return i18n("Internet Bookshop (ibs.it)");
}

QString IBSFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool IBSFetcher::canFetch(int type) const {
  return type == Data::Collection::Book || type == Data::Collection::Bibtex;
}

void IBSFetcher::readConfigHook(KConfig* config_, const QString& group_) {
  Q_UNUSED(config_);
  Q_UNUSED(group_);
}

void IBSFetcher::search(FetchKey key_, const QString& value_) {
  m_started = true;
  m_matches.clear();

#ifdef IBS_TEST
  KURL u = KURL::fromPathOrURL(QString::fromLatin1("/home/robby/ibs.html"));
#else
  KURL u(QString::fromLatin1(IBS_BASE_URL));

  if(!canFetch(Kernel::self()->collectionType())) {
    message(i18n("%1 does not allow searching for this collection type.").arg(source()), MessageHandler::Warning);
    stop();
    return;
  }

  switch(key_) {
    case Title:
      u.addQueryItem(QString::fromLatin1("Type"), QString::fromLatin1("keyword"));
      u.addQueryItem(QString::fromLatin1("T"), value_);
      break;

    case Person:
      u.addQueryItem(QString::fromLatin1("Type"), QString::fromLatin1("keyword"));
      u.addQueryItem(QString::fromLatin1("A"), value_);
      break;

    case ISBN:
      {
        QString s = value_;
        s.remove('-');
        // limit to first isbn
        s = s.section(';', 0, 0);
        u.setFileName(QString::fromLatin1("serdsp.asp"));
        u.addQueryItem(QString::fromLatin1("isbn"), s);
      }
      break;

    case Keyword:
      u.addQueryItem(QString::fromLatin1("Type"), QString::fromLatin1("keyword"));
      u.addQueryItem(QString::fromLatin1("S"), value_);
      break;

    default:
      kdWarning() << "IBSFetcher::search() - key not recognized: " << key_ << endl;
      stop();
      return;
  }
#endif
//  myDebug() << "IBSFetcher::search() - url: " << u.url() << endl;

  m_job = KIO::get(u, false, false);
  connect(m_job, SIGNAL(data(KIO::Job*, const QByteArray&)),
          SLOT(slotData(KIO::Job*, const QByteArray&)));
  if(key_ == ISBN) {
    connect(m_job, SIGNAL(result(KIO::Job*)), SLOT(slotCompleteISBN(KIO::Job*)));
  } else {
    connect(m_job, SIGNAL(result(KIO::Job*)), SLOT(slotComplete(KIO::Job*)));
  }
}

void IBSFetcher::stop() {
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

void IBSFetcher::slotData(KIO::Job*, const QByteArray& data_) {
  QDataStream stream(m_data, IO_WriteOnly | IO_Append);
  stream.writeRawBytes(data_.data(), data_.size());
}

void IBSFetcher::slotComplete(KIO::Job* job_) {
  // since the fetch is done, don't worry about holding the job pointer
  m_job = 0;

  if(job_->error()) {
    job_->showErrorDialog(Kernel::self()->widget());
    stop();
    return;
  }

  if(m_data.isEmpty()) {
    myDebug() << "IBSFetcher::slotComplete() - no data" << endl;
    stop();
    return;
  }

  QString s = Tellico::decodeHTML(QString(m_data));
  // really specific regexp
  QString pat = QString::fromLatin1("http://www.internetbookshop.it/code/");
  QRegExp anchorRx(QString::fromLatin1("<a\\s+[^>]*href\\s*=\\s*[\"'](") +
                   QRegExp::escape(pat) +
                   QString::fromLatin1("[^\"]*)\"[^>]*><b>([^<]+)<"), false);
  anchorRx.setMinimal(true);
  QRegExp tagRx(QString::fromLatin1("<.*>"));
  tagRx.setMinimal(true);

  QString u, t, d;
  int pos2;
  for(int pos = anchorRx.search(s); m_started && pos > -1; pos = anchorRx.search(s, pos+anchorRx.matchedLength())) {
    if(!u.isEmpty()) {
      SearchResult* r = new SearchResult(this, t, d, QString());
      emit signalResultFound(r);

#ifdef IBS_TEST
      KURL url = KURL::fromPathOrURL(QString::fromLatin1("/home/robby/ibs2.html"));
#else
      // the url probable contains &amp; so be careful
      KURL url = u.replace(QString::fromLatin1("&amp;"), QChar('&'));
#endif
      m_matches.insert(r->uid, url);

      u.truncate(0);
      t.truncate(0);
      d.truncate(0);
    }
    u = anchorRx.cap(1);
    t = anchorRx.cap(2);
    pos2 = s.find(QString::fromLatin1("<br>"), pos, false);
    if(pos2 > -1) {
      int pos3 = s.find(QString::fromLatin1("<br>"), pos2+1, false);
      if(pos3 > -1) {
        d = s.mid(pos2, pos3-pos2).remove(tagRx).simplifyWhiteSpace();
      }
    }
  }
#ifndef IBS_TEST
  if(!u.isEmpty()) {
    SearchResult* r = new SearchResult(this, t, d, QString());
    emit signalResultFound(r);
    m_matches.insert(r->uid, u.replace(QString::fromLatin1("&amp;"), QChar('&')));
  }
#endif

  stop();
}

void IBSFetcher::slotCompleteISBN(KIO::Job* job_) {
  // since the fetch is done, don't worry about holding the job pointer
  m_job = 0;

  if(job_->error()) {
    job_->showErrorDialog(Kernel::self()->widget());
    stop();
    return;
  }

  if(m_data.isEmpty()) {
    myDebug() << "IBSFetcher::slotCompleteISBN() - no data" << endl;
    stop();
    return;
  }

  QString str = Tellico::decodeHTML(QString(m_data));
  if(str.find(QString::fromLatin1("Libro non presente"), 0, false /* cas-sensitive */) > -1) {
    stop();
    return;
  }
  Data::EntryPtr entry = parseEntry(str);
  if(entry) {
    QString desc = entry->field(QString::fromLatin1("author"))
                 + '/' + entry->field(QString::fromLatin1("publisher"));
    SearchResult* r = new SearchResult(this, entry->title(), desc, entry->field(QString::fromLatin1("isbn")));
    emit signalResultFound(r);
    m_matches.insert(r->uid, static_cast<KIO::TransferJob*>(job_)->url().url());
  }

  stop();
}

Tellico::Data::EntryPtr IBSFetcher::fetchEntry(uint uid_) {
  // if we already grabbed this one, then just pull it out of the dict
  Data::EntryPtr entry = m_entries[uid_];
  if(entry) {
    return entry;
  }

  KURL url = m_matches[uid_];
  if(url.isEmpty()) {
    kdWarning() << "IBSFetcher::fetchEntry() - no url in map" << endl;
    return 0;
  }

  QString results = Tellico::decodeHTML(FileHandler::readTextFile(url, true));
  if(results.isEmpty()) {
    myDebug() << "IBSFetcher::fetchEntry() - no text results" << endl;
    return 0;
  }

//  myDebug() << url.url() << endl;
#if 0
  kdWarning() << "Remove debug from ibsfetcher.cpp" << endl;
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
    myDebug() << "IBSFetcher::fetchEntry() - error in processing entry" << endl;
    return 0;
  }
  m_entries.insert(uid_, entry); // keep for later
  return entry;
}

Tellico::Data::EntryPtr IBSFetcher::parseEntry(const QString& str_) {
 // myDebug() << "IBSFetcher::parseEntry()" << endl;
 // class might be anime_info_top
  QString pat = QString::fromLatin1("%1(?:<[^>]+>)+([^<>\\s][^<>]+)");

  QRegExp isbnRx(QString::fromLatin1("isbn=([\\dxX]{13})"), false);
  QString isbn;
  int pos = isbnRx.search(str_);
  if(pos > -1) {
    isbn = isbnRx.cap(1);
  }

  Data::CollPtr coll = new Data::BookCollection(true);

 // map captions in HTML to field names
  QMap<QString, QString> fieldMap;
  fieldMap.insert(QString::fromLatin1("Titolo"), QString::fromLatin1("title"));
  fieldMap.insert(QString::fromLatin1("Autore"), QString::fromLatin1("author"));
  fieldMap.insert(QString::fromLatin1("Anno"), QString::fromLatin1("pub_year"));
  fieldMap.insert(QString::fromLatin1("Categoria"), QString::fromLatin1("genre"));
  fieldMap.insert(QString::fromLatin1("Rilegatura"), QString::fromLatin1("binding"));
  fieldMap.insert(QString::fromLatin1("Editore"), QString::fromLatin1("publisher"));
  fieldMap.insert(QString::fromLatin1("Dati"), QString::fromLatin1("edition"));

  QRegExp pagesRx(QString::fromLatin1("(\\d+) p\\.(\\s*,\\s*)?"));
  Data::EntryPtr entry = new Data::Entry(coll);

  for(QMap<QString, QString>::Iterator it = fieldMap.begin(); it != fieldMap.end(); ++it) {
    QRegExp infoRx(pat.arg(it.key()));
    pos = infoRx.search(str_);
    if(pos > -1) {
      if(it.data() == Latin1Literal("edition")) {
        int pos2 = pagesRx.search(infoRx.cap(1));
        if(pos2 > -1) {
          entry->setField(QString::fromLatin1("pages"), pagesRx.cap(1));
          entry->setField(it.data(), infoRx.cap(1).remove(pagesRx));
        } else {
          entry->setField(it.data(), infoRx.cap(1));
        }
      } else {
        entry->setField(it.data(), infoRx.cap(1));
      }
    }
  }

  // image
  if(!isbn.isEmpty()) {
    entry->setField(QString::fromLatin1("isbn"), isbn);
#if 1
    QString imgURL = QString::fromLatin1("http://giotto.ibs.it/cop/copt13.asp?f=%1").arg(isbn);
    myLog() << "IBSFetcher() - cover = " << imgURL << endl;
    QString id = ImageFactory::addImage(imgURL, true, QString::fromLatin1("http://internetbookshop.it"));
    if(!id.isEmpty()) {
      entry->setField(QString::fromLatin1("cover"), id);
    }
#else
    QRegExp imgRx(QString::fromLatin1("<img\\s+[^>]*\\s*src\\s*=\\s*\"(http://[^/]*\\.ibs\\.it/[^\"]+e=%1)").arg(isbn));
    imgRx.setMinimal(true);
    pos = imgRx.search(str_);
    if(pos > -1) {
      myLog() << "IBSFetcher() - cover = " << imgRx.cap(1) << endl;
      QString id = ImageFactory::addImage(imgRx.cap(1), true, QString::fromLatin1("http://internetbookshop.it"));
      if(!id.isEmpty()) {
        entry->setField(QString::fromLatin1("cover"), id);
      }
    }
#endif
  }

  // now look for description
  QRegExp descRx(QString::fromLatin1("Descrizione(?:<[^>]+>)+([^<>\\s].+)</span>"), false);
  descRx.setMinimal(true);
  pos = descRx.search(str_);
  if(pos == -1) {
    descRx.setPattern(QString::fromLatin1("In sintesi(?:<[^>]+>)+([^<>\\s].+)</span>"));
    pos = descRx.search(str_);
  }
  if(pos > -1) {
    Data::FieldPtr f = new Data::Field(QString::fromLatin1("plot"), i18n("Plot Summary"), Data::Field::Para);
    coll->addField(f);
    entry->setField(f, descRx.cap(1).simplifyWhiteSpace());
  }

  // IBS switches the surname and family name of the author
  QStringList names = entry->fields(QString::fromLatin1("author"), false);
  if(!names.isEmpty() && !names[0].isEmpty()) {
    for(QStringList::Iterator it = names.begin(); it != names.end(); ++it) {
      if((*it).find(',') > -1) {
        continue; // skip if it has a comma
      }
      QStringList words = QStringList::split(' ', *it);
      if(words.isEmpty()) {
        continue;
      }
      // put first word in back
      words.append(words[0]);
      words.pop_front();
      *it = words.join(QChar(' '));
    }
    entry->setField(QString::fromLatin1("author"), names.join(QString::fromLatin1("; ")));
  }
  return entry;
}

void IBSFetcher::updateEntry(Data::EntryPtr entry_) {
  QString isbn = entry_->field(QString::fromLatin1("isbn"));
  if(!isbn.isEmpty()) {
    search(Fetch::ISBN, isbn);
    return;
  }
  QString t = entry_->field(QString::fromLatin1("title"));
  if(!t.isEmpty()) {
    search(Fetch::Title, t);
    return;
  }

  myDebug() << "IBSFetcher::updateEntry() - insufficient info to search" << endl;
  emit signalDone(this); // always need to emit this if not continuing with the search
}

Tellico::Fetch::ConfigWidget* IBSFetcher::configWidget(QWidget* parent_) const {
  return new IBSFetcher::ConfigWidget(parent_);
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

#include "ibsfetcher.moc"
