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
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <kio/job.h>
#include <KJobUiDelegate>
#include <KJobWidgets/KJobWidgets>

#include <QRegExp>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>

namespace {
  static const char* IBS_BASE_URL = "http://www.internetbookshop.it/ser/serpge.asp";
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
        u = u.adjusted(QUrl::RemoveFilename);
        u.setPath(u.path() + QLatin1String("serdsp.asp"));

        QString s = request().value;
        s.remove(QLatin1Char('-'));
        // limit to first isbn
        s = s.section(QLatin1Char(';'), 0, 0);
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
//  myDebug() << "url: " << u.url();

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
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
      // the url probable contains &amp; so be careful
      QUrl url(u.replace(QLatin1String("&amp;"), QLatin1String("&")));
      FetchResult* r = new FetchResult(Fetcher::Ptr(this), t, d);
      m_matches.insert(r->uid, url);
      emit signalResultFound(r);

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

  if(!u.isEmpty()) {
    FetchResult* r = new FetchResult(Fetcher::Ptr(this), t, d);
    m_matches.insert(r->uid, QUrl(u.replace(QLatin1String("&amp;"), QLatin1String("&"))));
    emit signalResultFound(r);
  }

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
    m_matches.insert(r->uid, static_cast<KIO::TransferJob*>(job_)->url());
    emit signalResultFound(r);
  }

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
//  QString pat = QLatin1String("%1(?:<[^>]+>)+([^<>\\s][^<>]+)");
  QString pat = QLatin1String("%1(?:<[^>]+>)+(.+)</td.*>");

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
  fieldMap.insert(QLatin1String("Traduttore"), QLatin1String("translator"));
  fieldMap.insert(QLatin1String("Curatore"), QLatin1String("editor"));

  QRegExp tagRx(QLatin1String("<.*>"));
  tagRx.setMinimal(true);

  QRegExp pagesRx(QLatin1String("\\s*(\\d+) p\\.(\\s*,\\s*)?"));
  QRegExp yearRx(QLatin1String("^\\d{4}"));

  Data::EntryPtr entry(new Data::Entry(coll));

  for(QHash<QString, QString>::Iterator it = fieldMap.begin(); it != fieldMap.end(); ++it) {
    QRegExp infoRx(pat.arg(it.key()));
    infoRx.setMinimal(true);
    int pos = infoRx.indexIn(str_);
    if(pos > -1) {
      if(it.value() == QLatin1String("edition")) {
        QString data = infoRx.cap(1).remove(tagRx).trimmed();
        int pos2 = pagesRx.indexIn(data);
        if(pos2 > -1) {
          entry->setField(QLatin1String("pages"), pagesRx.cap(1));
          data = data.remove(pagesRx);
        }
        // assume that if the value starts with 4 digits, then it's a year
        pos2 = yearRx.indexIn(data);
        if(pos2 > -1) {
          entry->setField(QLatin1String("pub_year"), yearRx.cap(0));
          data = data.remove(yearRx).trimmed();
          // might start with a comma now
          if(data.startsWith(QLatin1String(","))) {
            data = data.mid(1).trimmed();
          }
        }
        // now it might be the binding
        if(data == QLatin1String("brossura")) {
          entry->setField(QLatin1String("binding"), i18n("Paperback"));
        } else if(data == QLatin1String("rilegato")) {
          entry->setField(QLatin1String("binding"), i18n("Hardback"));
        } else {
          entry->setField(it.value(), data);
        }
      } else {
        entry->setField(it.value(), infoRx.cap(1).remove(tagRx).trimmed());
      }
    }
  }

  QRegExp isbnRx(QLatin1String("isbn=([\\dxX]{13})"), Qt::CaseInsensitive);
  QString isbn;
  int pos = isbnRx.indexIn(str_);
  if(pos > -1) {
    isbn = isbnRx.cap(1);
  }
  // image
  if(!isbn.isEmpty()) {
    entry->setField(QLatin1String("isbn"), isbn);
#if 1
    QUrl imgURL(QString::fromLatin1("http://giotto.ibs.it/cop/copt13.asp?f=%1").arg(isbn));
//    myLog() << "cover = " << imgURL;
    QString id = ImageFactory::addImage(imgURL, true, QUrl(QString::fromLatin1("http://internetbookshop.it")));
    if(!id.isEmpty()) {
      entry->setField(QLatin1String("cover"), id);
    }
#else
    QRegExp imgRx(QString::fromLatin1("<img\\s+[^>]*\\s*src\\s*=\\s*\"(http://[^/]*\\.ibs\\.it/[^\"]+e=%1)").arg(isbn));
    imgRx.setMinimal(true);
    pos = imgRx.indexIn(str_);
    if(pos > -1) {
//      myLog() << "cover = " << imgRx.cap(1);
      QString id = ImageFactory::addImage(imgRx.cap(1), true, QUrl("http://internetbookshop.it"));
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

  // IBS switches the surname and family name of the author and translator
  const QStringList peopleFields = QStringList() << QLatin1String("author")
                                                 << QLatin1String("editor")
                                                 << QLatin1String("translator");
  foreach(const QString& fieldName, peopleFields) {
    QStringList names = FieldFormat::splitValue(entry->field(fieldName));
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
      entry->setField(fieldName, names.join(FieldFormat::delimiterString()));
    }
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

