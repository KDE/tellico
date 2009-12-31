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
#include "../gui/guiproxy.h"
#include "../tellico_utils.h"
#include "../collections/bookcollection.h"
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

//#define IBS_TEST

namespace {
  static const char* IBS_BASE_URL = "http://www.internetbookshop.it/ser/serpge.asp";
}

using namespace Tellico;
using Tellico::Fetch::IBSFetcher;

IBSFetcher::IBSFetcher(QObject* parent_)
    : Fetcher(parent_), m_started(false) {
}

IBSFetcher::~IBSFetcher() {
}

QString IBSFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool IBSFetcher::canFetch(int type) const {
  return type == Data::Collection::Book || type == Data::Collection::Bibtex;
}

void IBSFetcher::readConfigHook(const KConfigGroup& config_) {
  Q_UNUSED(config_);
}

void IBSFetcher::search() {
  m_started = true;
  m_matches.clear();

#ifdef IBS_TEST
  KUrl u = KUrl(QLatin1String("/home/robby/ibs.html"));
#else
  KUrl u(IBS_BASE_URL);

  switch(request().key) {
    case Title:
      u.addQueryItem(QLatin1String("Type"), QLatin1String("keyword"));
      u.addQueryItem(QLatin1String("T"), request().value);
      break;

    case Person:
      u.addQueryItem(QLatin1String("Type"), QLatin1String("keyword"));
      u.addQueryItem(QLatin1String("A"), request().value);
      break;

    case ISBN:
      {
        QString s = request().value;
        s.remove(QLatin1Char('-'));
        // limit to first isbn
        s = s.section(QLatin1Char(';'), 0, 0);
        u.setFileName(QLatin1String("serdsp.asp"));
        u.addQueryItem(QLatin1String("isbn"), s);
      }
      break;

    case Keyword:
      u.addQueryItem(QLatin1String("Type"), QLatin1String("keyword"));
      u.addQueryItem(QLatin1String("S"), request().value);
      break;

    default:
      myWarning() << "key not recognized: " << request().key;
      stop();
      return;
  }
#endif
//  myDebug() << "url: " << u.url();

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  m_job->ui()->setWindow(GUI::Proxy::widget());
  if(request().key == ISBN) {
    connect(m_job, SIGNAL(result(KJob*)), SLOT(slotCompleteISBN(KJob*)));
  } else {
    connect(m_job, SIGNAL(result(KJob*)), SLOT(slotComplete(KJob*)));
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

  // since the fetch is done, don't worry about holding the job pointer
  m_job = 0;

  QString s = Tellico::decodeHTML(data);
  // really specific regexp
  //QString pat = QLatin1String("http://www.internetbookshop.it/code/");
  QString pat = QLatin1String("http://www.ibs.it/code/");
  QRegExp anchorRx(QLatin1String("<a\\s+[^>]*href\\s*=\\s*[\"'](") +
                   QRegExp::escape(pat) +
                   QLatin1String("[^\"]*)\"[^>]*><b>([^<]+)<"), Qt::CaseInsensitive);
  anchorRx.setMinimal(true);
  QRegExp tagRx(QLatin1String("<.*>"));
  tagRx.setMinimal(true);

  QString u, t, d;
  int pos2;
  for(int pos = anchorRx.indexIn(s); m_started && pos > -1; pos = anchorRx.indexIn(s, pos+anchorRx.matchedLength())) {
    if(!u.isEmpty()) {
      FetchResult* r = new FetchResult(Fetcher::Ptr(this), t, d);
      emit signalResultFound(r);

#ifdef IBS_TEST
      KUrl url = KUrl(QLatin1String("/home/robby/ibs2.html"));
#else
      // the url probable contains &amp; so be careful
      KUrl url = u.replace(QLatin1String("&amp;"), QLatin1String("&"));
#endif
      m_matches.insert(r->uid, url);

      u.clear();
      t.clear();
      d.clear();
    }
    u = anchorRx.cap(1);
    t = anchorRx.cap(2);
    pos2 = s.indexOf(QLatin1String("<br>"), pos, Qt::CaseInsensitive);
    if(pos2 > -1) {
      int pos3 = s.indexOf(QLatin1String("<br>"), pos2+1, Qt::CaseInsensitive);
      if(pos3 > -1) {
        d = s.mid(pos2, pos3-pos2).remove(tagRx).simplified();
      }
    }
  }
#ifndef IBS_TEST
  if(!u.isEmpty()) {
    FetchResult* r = new FetchResult(Fetcher::Ptr(this), t, d);
    emit signalResultFound(r);
    m_matches.insert(r->uid, u.replace(QLatin1String("&amp;"), QLatin1String("&")));
  }
#endif

  stop();
}

void IBSFetcher::slotCompleteISBN(KJob* job_) {
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

  // since the fetch is done, don't worry about holding the job pointer
  m_job = 0;

  QString str = Tellico::decodeHTML(data);
  if(str.indexOf(QLatin1String("Libro non presente"), 0, Qt::CaseInsensitive) > -1) {
    stop();
    return;
  }
  Data::EntryPtr entry = parseEntry(str);
  if(entry) {
    QString desc = entry->field(QLatin1String("author"))
                 + QLatin1Char('/') + entry->field(QLatin1String("publisher"));
    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry->title(), desc, entry->field(QLatin1String("isbn")));
    emit signalResultFound(r);
    m_matches.insert(r->uid, static_cast<KIO::TransferJob*>(job_)->url().url());
  }

  stop();
}

Tellico::Data::EntryPtr IBSFetcher::fetchEntryHook(uint uid_) {
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

  QString results = Tellico::decodeHTML(FileHandler::readTextFile(url, true));
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

Tellico::Data::EntryPtr IBSFetcher::parseEntry(const QString& str_) {
 // myDebug();
 // class might be anime_info_top
  QString pat = QLatin1String("%1(?:<[^>]+>)+([^<>\\s][^<>]+)");

  QRegExp isbnRx(QLatin1String("isbn=([\\dxX]{13})"), Qt::CaseInsensitive);
  QString isbn;
  int pos = isbnRx.indexIn(str_);
  if(pos > -1) {
    isbn = isbnRx.cap(1);
  }

  Data::CollPtr coll(new Data::BookCollection(true));

 // map captions in HTML to field names
  QHash<QString, QString> fieldMap;
  fieldMap.insert(QLatin1String("Titolo"), QLatin1String("title"));
  fieldMap.insert(QLatin1String("Autore"), QLatin1String("author"));
  fieldMap.insert(QLatin1String("Anno"), QLatin1String("pub_year"));
  fieldMap.insert(QLatin1String("Categoria"), QLatin1String("genre"));
  fieldMap.insert(QLatin1String("Rilegatura"), QLatin1String("binding"));
  fieldMap.insert(QLatin1String("Editore"), QLatin1String("publisher"));
  fieldMap.insert(QLatin1String("Dati"), QLatin1String("edition"));

  QRegExp pagesRx(QLatin1String("(\\d+) p\\.(\\s*,\\s*)?"));
  Data::EntryPtr entry(new Data::Entry(coll));

  for(QHash<QString, QString>::Iterator it = fieldMap.begin(); it != fieldMap.end(); ++it) {
    QRegExp infoRx(pat.arg(it.key()));
    pos = infoRx.indexIn(str_);
    if(pos > -1) {
      if(it.value() == QLatin1String("edition")) {
        int pos2 = pagesRx.indexIn(infoRx.cap(1));
        if(pos2 > -1) {
          entry->setField(QLatin1String("pages"), pagesRx.cap(1));
          entry->setField(it.value(), infoRx.cap(1).remove(pagesRx));
        } else {
          entry->setField(it.value(), infoRx.cap(1));
        }
      } else {
        entry->setField(it.value(), infoRx.cap(1));
      }
    }
  }

  // image
  if(!isbn.isEmpty()) {
    entry->setField(QLatin1String("isbn"), isbn);
#if 1
    KUrl imgURL = QString::fromLatin1("http://giotto.ibs.it/cop/copt13.asp?f=%1").arg(isbn);
    myLog() << "cover = " << imgURL;
    QString id = ImageFactory::addImage(imgURL, true, KUrl("http://internetbookshop.it"));
    if(!id.isEmpty()) {
      entry->setField(QLatin1String("cover"), id);
    }
#else
    QRegExp imgRx(QString::fromLatin1("<img\\s+[^>]*\\s*src\\s*=\\s*\"(http://[^/]*\\.ibs\\.it/[^\"]+e=%1)").arg(isbn));
    imgRx.setMinimal(true);
    pos = imgRx.indexIn(str_);
    if(pos > -1) {
      myLog() << "cover = " << imgRx.cap(1);
      QString id = ImageFactory::addImage(imgRx.cap(1), true, KUrl("http://internetbookshop.it"));
      if(!id.isEmpty()) {
        entry->setField(QLatin1String("cover"), id);
      }
    }
#endif
  }

  // now look for description
  QRegExp descRx(QLatin1String("Descrizione(?:<[^>]+>)+([^<>\\s].+)</span>"), Qt::CaseInsensitive);
  descRx.setMinimal(true);
  pos = descRx.indexIn(str_);
  if(pos == -1) {
    descRx.setPattern(QLatin1String("In sintesi(?:<[^>]+>)+([^<>\\s].+)</span>"));
    pos = descRx.indexIn(str_);
  }
  if(pos > -1) {
    Data::FieldPtr f(new Data::Field(QLatin1String("plot"), i18n("Plot Summary"), Data::Field::Para));
    coll->addField(f);
    entry->setField(f, descRx.cap(1).simplified());
  }

  // IBS switches the surname and family name of the author
  QStringList names = FieldFormat::splitValue(entry->field(QLatin1String("author")));
  if(!names.isEmpty() && !names[0].isEmpty()) {
    for(QStringList::Iterator it = names.begin(); it != names.end(); ++it) {
      if((*it).indexOf(QLatin1Char(',')) > -1) {
        continue; // skip if it has a comma
      }
      QStringList words = (*it).split(QLatin1Char(' '));
      if(words.isEmpty()) {
        continue;
      }
      // put first word in back
      words.append(words[0]);
      words.pop_front();
      *it = words.join(QLatin1String(" "));
    }
    entry->setField(QLatin1String("author"), names.join(FieldFormat::delimiterString()));
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
  return favIcon("http://internetbookshop.it");
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
