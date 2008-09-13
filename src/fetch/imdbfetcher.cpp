/***************************************************************************
    copyright            : (C) 2004-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "imdbfetcher.h"
#include "../tellico_kernel.h"
#include "../collections/videocollection.h"
#include "../entry.h"
#include "../field.h"
#include "../filehandler.h"
#include "../latin1literal.h"
#include "../imagefactory.h"
#include "../tellico_utils.h"
#include "../gui/listboxtext.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <kdialogbase.h>
#include <kconfig.h>
#include <klineedit.h>
#include <knuminput.h>

#include <qregexp.h>
#include <qfile.h>
#include <qmap.h>
#include <qvbox.h>
#include <qlabel.h>
#include <qlistbox.h>
#include <qwhatsthis.h>
#include <qlayout.h>
#include <qcheckbox.h>
#include <qvgroupbox.h>

//#define IMDB_TEST

namespace {
  static const char* IMDB_SERVER = "akas.imdb.com";
  static const uint IMDB_MAX_RESULTS = 20;
  static const QString sep = QString::fromLatin1("; ");
}

using Tellico::Fetch::IMDBFetcher;

QRegExp* IMDBFetcher::s_tagRx = 0;
QRegExp* IMDBFetcher::s_anchorRx = 0;
QRegExp* IMDBFetcher::s_anchorTitleRx = 0;
QRegExp* IMDBFetcher::s_anchorNameRx = 0;
QRegExp* IMDBFetcher::s_titleRx = 0;

// static
void IMDBFetcher::initRegExps() {
  s_tagRx = new QRegExp(QString::fromLatin1("<.*>"));
  s_tagRx->setMinimal(true);

  s_anchorRx = new QRegExp(QString::fromLatin1("<a\\s+[^>]*href\\s*=\\s*\"([^\"]*)\"[^<]*>([^<]*)</a>"), false);
  s_anchorRx->setMinimal(true);

  s_anchorTitleRx = new QRegExp(QString::fromLatin1("<a\\s+[^>]*href\\s*=\\s*\"([^\"]*/title/[^\"]*)\"[^<]*>([^<]*)</a>"), false);
  s_anchorTitleRx->setMinimal(true);

  s_anchorNameRx = new QRegExp(QString::fromLatin1("<a\\s+[^>]*href\\s*=\\s*\"([^\"]*/name/[^\"]*)\"[^<]*>([^<]*)</a>"), false);
  s_anchorNameRx->setMinimal(true);

  s_titleRx = new QRegExp(QString::fromLatin1("<title>(.*)</title>"), false);
  s_titleRx->setMinimal(true);
}

IMDBFetcher::IMDBFetcher(QObject* parent_, const char* name_) : Fetcher(parent_, name_),
    m_job(0), m_started(false), m_fetchImages(true), m_host(QString::fromLatin1(IMDB_SERVER)),
    m_limit(IMDB_MAX_RESULTS), m_countOffset(0) {
  if(!s_tagRx) {
    initRegExps();
  }
}

IMDBFetcher::~IMDBFetcher() {
}

QString IMDBFetcher::defaultName() {
  return i18n("Internet Movie Database");
}

QString IMDBFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool IMDBFetcher::canFetch(int type) const {
  return type == Data::Collection::Video;
}

void IMDBFetcher::readConfigHook(const KConfigGroup& config_) {
  QString h = config_.readEntry("Host");
  if(!h.isEmpty()) {
    m_host = h;
  }
  m_numCast = config_.readNumEntry("Max Cast", 10);
  m_fetchImages = config_.readBoolEntry("Fetch Images", true);
  m_fields = config_.readListEntry("Custom Fields");
}

// multiple values not supported
void IMDBFetcher::search(FetchKey key_, const QString& value_) {
  m_key = key_;
  m_value = value_;
  m_started = true;
  m_redirected = false;
  m_data.truncate(0);
  m_matches.clear();
  m_popularTitles.truncate(0);
  m_exactTitles.truncate(0);
  m_partialTitles.truncate(0);
  m_currentTitleBlock = Unknown;
  m_countOffset = 0;

// only search if current collection is a video collection
  if(Kernel::self()->collectionType() != Data::Collection::Video) {
    myDebug() << "IMDBFetcher::search() - collection type mismatch, stopping" << endl;
    stop();
    return;
  }

#ifdef IMDB_TEST
  if(m_key == Title) {
    m_url = KURL::fromPathOrURL(QString::fromLatin1("/home/robby/imdb-title.html"));
    m_redirected = false;
  } else {
    m_url = KURL::fromPathOrURL(QString::fromLatin1("/home/robby/imdb-name.html"));
    m_redirected = true;
  }
#else
  m_url = KURL();
  m_url.setProtocol(QString::fromLatin1("http"));
  m_url.setHost(m_host.isEmpty() ? QString::fromLatin1(IMDB_SERVER) : m_host);
  m_url.setPath(QString::fromLatin1("/find"));

  switch(key_) {
    case Title:
      m_url.addQueryItem(QString::fromLatin1("s"), QString::fromLatin1("tt"));
      break;

    case Person:
      m_url.addQueryItem(QString::fromLatin1("s"), QString::fromLatin1("nm"));
      break;

    default:
      kdWarning() << "IMDBFetcher::search() - FetchKey not supported" << endl;
      stop();
      return;
  }

  // as far as I can tell, the url encoding should always be iso-8859-1
  // not utf-8
  m_url.addQueryItem(QString::fromLatin1("q"), value_, 4 /* iso-8859-1 */);

//  myDebug() << "IMDBFetcher::search() url = " << m_url << endl;
#endif

  m_job = KIO::get(m_url, false, false);
  connect(m_job, SIGNAL(data(KIO::Job*, const QByteArray&)),
          SLOT(slotData(KIO::Job*, const QByteArray&)));
  connect(m_job, SIGNAL(result(KIO::Job*)),
          SLOT(slotComplete(KIO::Job*)));
  connect(m_job, SIGNAL(redirection(KIO::Job *, const KURL&)),
          SLOT(slotRedirection(KIO::Job*, const KURL&)));
}

void IMDBFetcher::continueSearch() {
  m_started = true;
  m_limit += IMDB_MAX_RESULTS;

  if(m_currentTitleBlock == Popular) {
    parseTitleBlock(m_popularTitles);
    // if the offset is 0, then we need to be looking at the next block
    m_currentTitleBlock = m_countOffset == 0 ? Exact : Popular;
  }

  // current title block might have changed
  if(m_currentTitleBlock == Exact) {
    parseTitleBlock(m_exactTitles);
    m_currentTitleBlock = m_countOffset == 0 ? Partial : Exact;
  }

  if(m_currentTitleBlock == Partial) {
    parseTitleBlock(m_partialTitles);
    m_currentTitleBlock = m_countOffset == 0 ? Unknown : Partial;
  }

  if(m_currentTitleBlock == SinglePerson) {
    parseSingleNameResult();
  }

  stop();
}

void IMDBFetcher::stop() {
  if(!m_started) {
    return;
  }
//  myLog() << "IMDBFetcher::stop()" << endl;
  if(m_job) {
    m_job->kill();
    m_job = 0;
  }

  m_started = false;
  m_redirected = false;

  emit signalDone(this);
}

void IMDBFetcher::slotData(KIO::Job*, const QByteArray& data_) {
  QDataStream stream(m_data, IO_WriteOnly | IO_Append);
  stream.writeRawBytes(data_.data(), data_.size());
}

void IMDBFetcher::slotRedirection(KIO::Job*, const KURL& toURL_) {
  m_url = toURL_;
  m_redirected = true;
}

void IMDBFetcher::slotComplete(KIO::Job* job_) {
  // since the fetch is done, don't worry about holding the job pointer
  m_job = 0;

  if(job_->error()) {
    job_->showErrorDialog(Kernel::self()->widget());
    stop();
    return;
  }

  if(m_data.isEmpty()) {
    stop();
    return;
  }

  // a single result was found if we got redirected
  if(m_key == Title) {
    if(m_redirected) {
      parseSingleTitleResult();
    } else {
      parseMultipleTitleResults();
    }
  } else {
    if(m_redirected) {
      parseSingleNameResult();
    } else {
      parseMultipleNameResults();
    }
  }
}

void IMDBFetcher::parseSingleTitleResult() {
//  myDebug() << "IMDBFetcher::parseSingleTitleResult()" << endl;
  s_titleRx->search(Tellico::decodeHTML(QString(m_data)));
  // split title at parenthesis
  const QString cap1 = s_titleRx->cap(1);
  int pPos = cap1.find('(');
  // FIXME: maybe remove parentheses here?
  SearchResult* r = new SearchResult(this,
                                     pPos == -1 ? cap1 : cap1.left(pPos),
                                     pPos == -1 ? QString::null : cap1.mid(pPos),
                                     QString());
  m_matches.insert(r->uid, m_url);
  emit signalResultFound(r);

  m_hasMoreResults = false;
  stop();
}

void IMDBFetcher::parseMultipleTitleResults() {
//  myDebug() << "IMDBFetcher::parseMultipleTitleResults()" << endl;
  QString output = Tellico::decodeHTML(QString(m_data));

  // IMDb can return three title lists, popular, exact, and partial
  // the popular titles are in the first table, after the "Popular Results" text
  int pos_popular = output.find(QString::fromLatin1("Popular Titles"),  0,                    false);
  int pos_exact   = output.find(QString::fromLatin1("Exact Matches"),   QMAX(pos_popular, 0), false);
  int pos_partial = output.find(QString::fromLatin1("Partial Matches"), QMAX(pos_exact, 0),   false);
  int end_popular = pos_exact; // keep track of where to end
  if(end_popular == -1) {
    end_popular = pos_partial == -1 ? output.length() : pos_partial;
  }
  int end_exact = pos_partial; // keep track of where to end
  if(end_exact == -1) {
    end_exact = output.length();
  }

  // if found popular matches
  if(pos_popular > -1) {
    m_popularTitles = output.mid(pos_popular, end_popular-pos_popular);
  }
  // if found exact matches
  if(pos_exact > -1) {
    m_exactTitles = output.mid(pos_exact, end_exact-pos_exact);
  }
  if(pos_partial > -1) {
    m_partialTitles = output.mid(pos_partial);
  }

  parseTitleBlock(m_popularTitles);
  // if the offset is 0, then we need to be looking at the next block
  m_currentTitleBlock = m_countOffset == 0 ? Exact : Popular;

  if(m_matches.size() < m_limit) {
    parseTitleBlock(m_exactTitles);
    m_currentTitleBlock = m_countOffset == 0 ? Partial : Exact;
  }

  if(m_matches.size() < m_limit) {
    parseTitleBlock(m_partialTitles);
    m_currentTitleBlock = m_countOffset == 0 ? Unknown : Partial;
  }

#ifndef NDEBUG
  if(m_matches.size() == 0) {
    myDebug() << "IMDBFetcher::parseMultipleTitleResults() - no matches found." << endl;
  }
#endif

  stop();
}

void IMDBFetcher::parseTitleBlock(const QString& str_) {
  if(str_.isEmpty()) {
    m_countOffset = 0;
    return;
  }
//  myDebug() << "IMDBFetcher::parseTitleBlock() - " << m_currentTitleBlock << endl;

  QRegExp akaRx(QString::fromLatin1("aka (.*)(</li>|<br)"), false);
  akaRx.setMinimal(true);

  m_hasMoreResults = false;

  int count = 0;
  int start = s_anchorTitleRx->search(str_);
  while(m_started && start > -1) {
    // split title at parenthesis
    const QString cap1 = s_anchorTitleRx->cap(1); // the anchor url
    const QString cap2 = s_anchorTitleRx->cap(2).stripWhiteSpace(); // the anchor text
    start += s_anchorTitleRx->matchedLength();
    int pPos = cap2.find('('); // if it has parentheses, use that for description
    QString desc;
    if(pPos > -1) {
      int pPos2 = cap2.find(')', pPos+1);
      if(pPos2 > -1) {
        desc = cap2.mid(pPos+1, pPos2-pPos-1);
      }
    } else {
      // parenthesis might be outside anchor tag
      int end = s_anchorTitleRx->search(str_, start);
      if(end == -1) {
        end = str_.length();
      }
      QString text = str_.mid(start, end-start);
      pPos = text.find('(');
      if(pPos > -1) {
        int pNewLine = text.find(QString::fromLatin1("<br"));
        if(pNewLine == -1 || pPos < pNewLine) {
          int pPos2 = text.find(')', pPos);
          desc = text.mid(pPos+1, pPos2-pPos-1);
        }
        pPos = -1;
      }
    }
    // multiple matches might have 'aka' info
    int end = s_anchorTitleRx->search(str_, start+1);
    if(end == -1) {
      end = str_.length();
    }
    int akaPos = akaRx.search(str_, start+1);
    if(akaPos > -1 && akaPos < end) {
      // limit to 50 chars
      desc += QChar(' ') + akaRx.cap(1).stripWhiteSpace().remove(*s_tagRx);
      if(desc.length() > 50) {
        desc = desc.left(50) + QString::fromLatin1("...");
      }
    }

    start = s_anchorTitleRx->search(str_, start);

    if(count < m_countOffset) {
      ++count;
      continue;
    }

    // if we got this far, then there is a valid result
    if(m_matches.size() >= m_limit) {
      m_hasMoreResults = true;
      break;
    }

    SearchResult* r = new SearchResult(this, pPos == -1 ? cap2 : cap2.left(pPos), desc, QString());
    KURL u(m_url, cap1);
    u.setQuery(QString::null);
    m_matches.insert(r->uid, u);
    emit signalResultFound(r);
    ++count;
  }
  if(!m_hasMoreResults && m_currentTitleBlock != Partial) {
    m_hasMoreResults = true;
  }
  m_countOffset = m_matches.size() < m_limit ? 0 : count;
}

void IMDBFetcher::parseSingleNameResult() {
//  myDebug() << "IMDBFetcher::parseSingleNameResult()" << endl;

  m_currentTitleBlock = SinglePerson;

  QString output = Tellico::decodeHTML(QString(m_data));

  int pos = s_anchorTitleRx->search(output);
  if(pos == -1) {
    stop();
    return;
  }

  QRegExp tvRegExp(QString::fromLatin1("TV\\sEpisode"), false);

  int len = 0;
  int count = 0;
  QString desc;
  for( ; m_started && pos > -1; pos = s_anchorTitleRx->search(output, pos+len)) {
    desc.truncate(0);
    bool isEpisode = false;
    len = s_anchorTitleRx->cap(0).length();
    // split title at parenthesis
    const QString cap2 = s_anchorTitleRx->cap(2).stripWhiteSpace();
    int pPos = cap2.find('(');
    if(pPos > -1) {
      desc = cap2.mid(pPos);
    } else {
      // look until the next <a
      int aPos = output.find(QString::fromLatin1("<a"), pos+len, false);
      if(aPos == -1) {
        aPos = output.length();
      }
      QString tmp = output.mid(pos+len, aPos-pos-len);
      if(tmp.find(tvRegExp) > -1) {
        isEpisode = true;
      }
      pPos = tmp.find('(');
      if(pPos > -1) {
        int pNewLine = tmp.find(QString::fromLatin1("<br"));
        if(pNewLine == -1 || pPos < pNewLine) {
          int pEnd = tmp.find(')', pPos+1);
          desc = tmp.mid(pPos+1, pEnd-pPos-1).remove(*s_tagRx);
        }
        // but need to indicate it wasn't found initially
        pPos = -1;
      }
    }

    ;

    if(count < m_countOffset) {
      ++count;
      continue;
    }

    ++count;
    if(isEpisode) {
      continue;
    }

    // if we got this far, then there is a valid result
    if(m_matches.size() >= m_limit) {
      m_hasMoreResults = true;
      break;
    }

    // FIXME: maybe remove parentheses here?
    SearchResult* r = new SearchResult(this, pPos == -1 ? cap2 : cap2.left(pPos), desc, QString());
    KURL u(m_url, s_anchorTitleRx->cap(1)); // relative URL constructor
    u.setQuery(QString::null);
    m_matches.insert(r->uid, u);
//    myDebug() << u.prettyURL() << endl;
//    myDebug() << cap2 << endl;
    emit signalResultFound(r);
  }
  if(pos == -1) {
    m_hasMoreResults = false;
  }
  m_countOffset = count - 1;

  stop();
}

void IMDBFetcher::parseMultipleNameResults() {
//  myDebug() << "IMDBFetcher::parseMultipleNameResults()" << endl;

  // the exact results are in the first table after the "exact results" text
  QString output = Tellico::decodeHTML(QString(m_data));
  int pos = output.find(QString::fromLatin1("Popular Results"), 0, false);
  if(pos == -1) {
    pos = output.find(QString::fromLatin1("Exact Matches"), 0, false);
  }

  // find beginning of partial matches
  int end = output.find(QString::fromLatin1("Other Results"), QMAX(pos, 0), false);
  if(end == -1) {
    end = output.find(QString::fromLatin1("Partial Matches"), QMAX(pos, 0), false);
    if(end == -1) {
      end = output.find(QString::fromLatin1("Approx Matches"), QMAX(pos, 0), false);
      if(end == -1) {
        end = output.length();
      }
    }
  }

  QMap<QString, KURL> map;
  QMap<QString, int> nameMap;

  QString s;
  // if found exact matches
  if(pos > -1) {
    pos = s_anchorNameRx->search(output, pos+13);
    while(pos > -1 && pos < end && m_matches.size() < m_limit) {
      KURL u(m_url, s_anchorNameRx->cap(1));
      s = s_anchorNameRx->cap(2).stripWhiteSpace() + ' ';
      // if more than one exact, add parentheses
      if(nameMap.contains(s) && nameMap[s] > 0) {
        // fix the first one that didn't have a number
        if(nameMap[s] == 1) {
          KURL u2 = map[s];
          map.remove(s);
          map.insert(s + "(1) ", u2);
        }
        nameMap.insert(s, nameMap[s] + 1);
        // check for duplicate names
        s += QString::fromLatin1("(%1) ").arg(nameMap[s]);
      } else {
        nameMap.insert(s, 1);
      }
      map.insert(s, u);
      pos = s_anchorNameRx->search(output, pos+s_anchorNameRx->cap(0).length());
    }
  }

  // go ahead and search for partial matches
  pos = s_anchorNameRx->search(output, end);
  while(pos > -1 && m_matches.size() < m_limit) {
    KURL u(m_url, s_anchorNameRx->cap(1)); // relative URL
    s = s_anchorNameRx->cap(2).stripWhiteSpace();
    if(nameMap.contains(s) && nameMap[s] > 0) {
    // fix the first one that didn't have a number
      if(nameMap[s] == 1) {
        KURL u2 = map[s];
        map.remove(s);
        map.insert(s + " (1)", u2);
      }
      nameMap.insert(s, nameMap[s] + 1);
      // check for duplicate names
      s += QString::fromLatin1(" (%1)").arg(nameMap[s]);
    } else {
      nameMap.insert(s, 1);
    }
    map.insert(s, u);
    pos = s_anchorNameRx->search(output, pos+s_anchorNameRx->cap(0).length());
  }

  if(map.count() == 0) {
    stop();
    return;
  }

  KDialogBase* dlg = new KDialogBase(Kernel::self()->widget(), "imdb dialog",
                                     true, i18n("Select IMDB Result"), KDialogBase::Ok|KDialogBase::Cancel);
  QVBox* box = new QVBox(dlg);
  box->setSpacing(10);
  (void) new QLabel(i18n("<qt>Your search returned multiple matches. Please select one below.</qt>"), box);

  QListBox* listBox = new QListBox(box);
  listBox->setMinimumWidth(400);
  listBox->setColumnMode(QListBox::FitToWidth);
  const QStringList values = map.keys();
  for(QStringList::ConstIterator it = values.begin(); it != values.end(); ++it) {
    if((*it).endsWith(QChar(' '))) {
      GUI::ListBoxText* box = new GUI::ListBoxText(listBox, *it, 0);
      box->setColored(true);
    } else {
      (void) new GUI::ListBoxText(listBox, *it);
    }
  }
  listBox->setSelected(0, true);
  QWhatsThis::add(listBox, i18n("<qt>Select a search result.</qt>"));

  dlg->setMainWidget(box);
  if(dlg->exec() != QDialog::Accepted || listBox->currentText().isEmpty()) {
    dlg->delayedDestruct();
    stop();
    return;
  }

  m_url = map[listBox->currentText()];
  dlg->delayedDestruct();

  // redirected is true since that's how I tell if an exact match has been found
  m_redirected = true;
  m_data.truncate(0);
  m_job = KIO::get(m_url, false, false);
  connect(m_job, SIGNAL(data(KIO::Job*, const QByteArray&)),
          SLOT(slotData(KIO::Job*, const QByteArray&)));
  connect(m_job, SIGNAL(result(KIO::Job*)),
          SLOT(slotComplete(KIO::Job*)));
  connect(m_job, SIGNAL(redirection(KIO::Job *, const KURL&)),
          SLOT(slotRedirection(KIO::Job*, const KURL&)));

  // do not stop() here
}

Tellico::Data::EntryPtr IMDBFetcher::fetchEntry(uint uid_) {
  // if we already grabbed this one, then just pull it out of the dict
  Data::EntryPtr entry = m_entries[uid_];
  if(entry) {
    return entry;
  }

  KURL url = m_matches[uid_];
  if(url.isEmpty()) {
    myDebug() << "IMDBFetcher::fetchEntry() - no url found" << endl;
    return 0;
  }

  KURL origURL = m_url; // keep to switch back
  QString results;
  // if the url matches the current one, no need to redownload it
  if(url == m_url) {
//    myDebug() << "IMDBFetcher::fetchEntry() - matches previous URL, no downloading needed." << endl;
    results = Tellico::decodeHTML(QString(m_data));
  } else {
    // now it's sychronous
#ifdef IMDB_TEST
    KURL u = KURL::fromPathOrURL(QString::fromLatin1("/home/robby/imdb-title-result.html"));
    results = Tellico::decodeHTML(FileHandler::readTextFile(u));
#else
    // be quiet about failure
    results = Tellico::decodeHTML(FileHandler::readTextFile(url, true));
    m_url = url; // needed for processing
#endif
  }
  if(results.isEmpty()) {
    myDebug() << "IMDBFetcher::fetchEntry() - no text results" << endl;
    m_url = origURL;
    return 0;
  }

  entry = parseEntry(results);
  m_url = origURL;
  if(!entry) {
    myDebug() << "IMDBFetcher::fetchEntry() - error in processing entry" << endl;
    return 0;
  }
  m_entries.insert(uid_, entry); // keep for later
  return entry;
}

Tellico::Data::EntryPtr IMDBFetcher::parseEntry(const QString& str_) {
  Data::CollPtr coll = new Data::VideoCollection(true);
  Data::EntryPtr entry = new Data::Entry(coll);

  doTitle(str_, entry);
  doRunningTime(str_, entry);
  doAspectRatio(str_, entry);
  doAlsoKnownAs(str_, entry);
  doPlot(str_, entry, m_url);
  doLists(str_, entry);
  doPerson(str_, entry, QString::fromLatin1("Director"), QString::fromLatin1("director"));
  doPerson(str_, entry, QString::fromLatin1("Writer"), QString::fromLatin1("writer"));
  doRating(str_, entry);
  doCast(str_, entry, m_url);
  if(m_fetchImages) {
    // needs base URL
    doCover(str_, entry, m_url);
  }

  const QString imdb = QString::fromLatin1("imdb");
  if(!coll->hasField(imdb) && m_fields.findIndex(imdb) > -1) {
    Data::FieldPtr field = new Data::Field(imdb, i18n("IMDB Link"), Data::Field::URL);
    field->setCategory(i18n("General"));
    coll->addField(field);
  }
  if(coll->hasField(imdb) && coll->fieldByName(imdb)->type() == Data::Field::URL) {
    m_url.setQuery(QString::null);
    entry->setField(imdb, m_url.url());
  }
  return entry;
}

void IMDBFetcher::doTitle(const QString& str_, Data::EntryPtr entry_) {
  if(s_titleRx->search(str_) > -1) {
    const QString cap1 = s_titleRx->cap(1);
    // titles always have parentheses
    int pPos = cap1.find('(');
    QString title = cap1.left(pPos).stripWhiteSpace();
    // remove first and last quotes is there
    if(title.startsWith(QChar('"')) && title.endsWith(QChar('"'))) {
      title = title.mid(1, title.length()-2);
    }
    entry_->setField(QString::fromLatin1("title"), title);
    // remove parenthesis
    uint pPos2 = pPos+1;
    while(pPos2 < cap1.length() && cap1[pPos2].isDigit()) {
      ++pPos2;
    }
    QString year = cap1.mid(pPos+1, pPos2-pPos-1);
    if(!year.isEmpty()) {
      entry_->setField(QString::fromLatin1("year"), year);
    }
  }
}

void IMDBFetcher::doRunningTime(const QString& str_, Data::EntryPtr entry_) {
  // running time
  QRegExp runtimeRx(QString::fromLatin1("runtime:.*(\\d+)\\s+min"), false);
  runtimeRx.setMinimal(true);

  if(runtimeRx.search(str_) > -1) {
//    myDebug() << "running-time = " << runtimeRx.cap(1) << endl;
    entry_->setField(QString::fromLatin1("running-time"), runtimeRx.cap(1));
  }
}

void IMDBFetcher::doAspectRatio(const QString& str_, Data::EntryPtr entry_) {
  QRegExp rx(QString::fromLatin1("aspect ratio:.*([\\d\\.]+\\s*:\\s*[\\d\\.]+)"), false);
  rx.setMinimal(true);

  if(rx.search(str_) > -1) {
//    myDebug() << "aspect ratio = " << rx.cap(1) << endl;
    entry_->setField(QString::fromLatin1("aspect-ratio"), rx.cap(1).stripWhiteSpace());
  }
}

void IMDBFetcher::doAlsoKnownAs(const QString& str_, Data::EntryPtr entry_) {
  if(m_fields.findIndex(QString::fromLatin1("alttitle")) == -1) {
    return;
  }

  // match until next b tag
//  QRegExp akaRx(QString::fromLatin1("also known as(.*)<b(?:\\s.*)?>"));
  QRegExp akaRx(QString::fromLatin1("also known as(.*)<(b[>\\s/]|div)"), false);
  akaRx.setMinimal(true);

  if(akaRx.search(str_) > -1 && !akaRx.cap(1).isEmpty()) {
    Data::FieldPtr f = entry_->collection()->fieldByName(QString::fromLatin1("alttitle"));
    if(!f) {
      f = new Data::Field(QString::fromLatin1("alttitle"), i18n("Alternative Titles"), Data::Field::Table);
      f->setFormatFlag(Data::Field::FormatTitle);
      entry_->collection()->addField(f);
    }

    // split by <br>, remembering it could become valid xhtml!
    QRegExp brRx(QString::fromLatin1("<br[\\s/]*>"), false);
    brRx.setMinimal(true);
    QStringList list = QStringList::split(brRx, akaRx.cap(1));
    // lang could be included with [fr]
//    const QRegExp parRx(QString::fromLatin1("\\(.+\\)"));
    const QRegExp brackRx(QString::fromLatin1("\\[\\w+\\]"));
    QStringList values;
    for(QStringList::Iterator it = list.begin(); it != list.end(); ++it) {
      QString s = *it;
      // sometimes, the word "more" gets linked to the releaseinfo page, check that
      if(s.find(QString::fromLatin1("releaseinfo")) > -1) {
        continue;
      }
      s.remove(*s_tagRx);
      s.remove(brackRx);
      s = s.stripWhiteSpace();
      // the first value ends up being or starting with the colon after "Also know as"
      // I'm too lazy to figure out a better regexp
      if(s.startsWith(QChar(':'))) {
        s = s.mid(1);
      }
      if(!s.isEmpty()) {
        values += s;
      }
    }
    if(!values.isEmpty()) {
      entry_->setField(QString::fromLatin1("alttitle"), values.join(sep));
    }
  }
}

void IMDBFetcher::doPlot(const QString& str_, Data::EntryPtr entry_, const KURL& baseURL_) {
  // plot summaries provided by users are on a separate page
  // should those be preferred?

  bool useUserSummary = false;

  QString thisPlot;
  // match until next opening tag
  QRegExp plotRx(QString::fromLatin1("plot\\s*(?:outline|summary)?:(.*)<[^/].*</"), false);
  plotRx.setMinimal(true);
  QRegExp plotURLRx(QString::fromLatin1("<a\\s+.*href\\s*=\\s*\".*/title/.*/plotsummary\""), false);
  plotURLRx.setMinimal(true);
  if(plotRx.search(str_) > -1) {
    thisPlot = plotRx.cap(1);
    thisPlot.remove(*s_tagRx); // remove HTML tags
    entry_->setField(QString::fromLatin1("plot"), thisPlot);
    // if thisPlot ends with (more) or contains
    // a url that ends with plotsummary, then we'll grab it, otherwise not
    if(plotRx.cap(0).endsWith(QString::fromLatin1("(more)</")) || plotURLRx.search(plotRx.cap(0)) > -1) {
      useUserSummary = true;
    }
  }

  if(useUserSummary) {
    QRegExp idRx(QString::fromLatin1("title/(tt\\d+)"));
    idRx.search(baseURL_.path());
    KURL plotURL = baseURL_;
    plotURL.setPath(QString::fromLatin1("/title/") + idRx.cap(1) + QString::fromLatin1("/plotsummary"));
    // be quiet about failure
    QString plotPage = FileHandler::readTextFile(plotURL, true);

    if(!plotPage.isEmpty()) {
      QRegExp plotRx(QString::fromLatin1("<p\\s+class\\s*=\\s*\"plotpar\">(.*)</p"));
      plotRx.setMinimal(true);
      if(plotRx.search(plotPage) > -1) {
        QString userPlot = plotRx.cap(1);
        userPlot.remove(*s_tagRx); // remove HTML tags
        // remove last little "written by", if there
        userPlot.remove(QRegExp(QString::fromLatin1("\\s*written by.*$"), false));
        entry_->setField(QString::fromLatin1("plot"), Tellico::decodeHTML(userPlot));
      }
    }
  }
}

void IMDBFetcher::doPerson(const QString& str_, Data::EntryPtr entry_,
                           const QString& imdbHeader_, const QString& fieldName_) {
  QRegExp br2Rx(QString::fromLatin1("<br[\\s/]*>\\s*<br[\\s/]*>"), false);
  br2Rx.setMinimal(true);
  QRegExp divRx(QString::fromLatin1("<[/]*div"), false);
  divRx.setMinimal(true);
  QString name = QString::fromLatin1("/name/");

  StringSet people;
  for(int pos = str_.find(imdbHeader_); pos > 0; pos = str_.find(imdbHeader_, pos)) {
    // loop until repeated <br> tags or </div> tag
    const int endPos1 = str_.find(br2Rx, pos);
    const int endPos2 = str_.find(divRx, pos);
    const int endPos = QMIN(endPos1, endPos2); // ok to be -1
    pos = s_anchorRx->search(str_, pos+1);
    while(pos > -1 && pos < endPos) {
      if(s_anchorRx->cap(1).find(name) > -1) {
        people.add(s_anchorRx->cap(2).stripWhiteSpace());
      }
      pos = s_anchorRx->search(str_, pos+1);
    }
  }
  if(!people.isEmpty()) {
    entry_->setField(fieldName_, people.toList().join(sep));
  }
}

void IMDBFetcher::doCast(const QString& str_, Data::EntryPtr entry_, const KURL& baseURL_) {
  // the extended cast list is on a separate page
  // that's usually a lot of people
  // but since it can be in billing order, the main actors might not
  // be in the short list
  QRegExp idRx(QString::fromLatin1("title/(tt\\d+)"));
  idRx.search(baseURL_.path());
#ifdef IMDB_TEST
  KURL castURL = KURL::fromPathOrURL(QString::fromLatin1("/home/robby/imdb-title-fullcredits.html"));
#else
  KURL castURL = baseURL_;
  castURL.setPath(QString::fromLatin1("/title/") + idRx.cap(1) + QString::fromLatin1("/fullcredits"));
#endif
  // be quiet about failure and be sure to translate entities
  QString castPage = Tellico::decodeHTML(FileHandler::readTextFile(castURL, true));

  int pos = -1;
  // the text to search, depends on which page is being read
  QString castText = castPage;
  if(castText.isEmpty()) {
    // fall back to short list
    castText = str_;
    pos = castText.find(QString::fromLatin1("cast overview"), 0, false);
    if(pos == -1) {
      pos = castText.find(QString::fromLatin1("credited cast"), 0, false);
    }
  } else {
    // first look for anchor
    QRegExp castAnchorRx(QString::fromLatin1("<a\\s+name\\s*=\\s*\"cast\""), false);
    pos = castText.find(castAnchorRx);
    if(pos < 0) {
      QRegExp tableClassRx(QString::fromLatin1("<table\\s+class\\s*=\\s*\"cast\""), false);
      pos = castText.find(tableClassRx);
      if(pos < 0) {
        // fragile, the word "cast" appears in the title, but need to find
        // the one right above the actual cast table
        // for TV shows, there's a link on the sidebar for "episodes case"
        // so need to not match that one
        pos = castText.find(QString::fromLatin1("cast</"), 0, false);
        if(pos > 9) {
          // back up 9 places
          if(castText.mid(pos-9, 9).startsWith(QString::fromLatin1("episodes"))) {
            // find next cast list
            pos = castText.find(QString::fromLatin1("cast</"), pos+6, false);
          }
        }
      }
    }
  }
  if(pos == -1) { // no cast list found
    myDebug() << "IMDBFetcher::doCast() - no cast list found" << endl;
    return;
  }

  const QString name = QString::fromLatin1("/name/");
  QRegExp tdRx(QString::fromLatin1("<td[^>]*>(.*)</td>"), false);
  tdRx.setMinimal(true);

  QStringList cast;
  // loop until closing table tag
  const int endPos = castText.find(QString::fromLatin1("</table"), pos, false);
  pos = s_anchorRx->search(castText, pos+1);
  while(pos > -1 && pos < endPos && static_cast<int>(cast.count()) < m_numCast) {
    if(s_anchorRx->cap(1).find(name) > -1) {
      // now search for <td> item with character name
      // there's a column with ellipses then the character
      const int pos2 = tdRx.search(castText, pos);
      if(pos2 > -1 && tdRx.search(castText, pos2+1) > -1) {
        cast += s_anchorRx->cap(2).stripWhiteSpace()
              + QString::fromLatin1("::") + tdRx.cap(1).simplifyWhiteSpace().remove(*s_tagRx);
      } else {
        cast += s_anchorRx->cap(2).stripWhiteSpace();
      }
    }
    pos = s_anchorRx->search(castText, pos+1);
  }

  if(!cast.isEmpty()) {
    entry_->setField(QString::fromLatin1("cast"), cast.join(sep));
  }
}

void IMDBFetcher::doRating(const QString& str_, Data::EntryPtr entry_) {
  if(m_fields.findIndex(QString::fromLatin1("imdb-rating")) == -1) {
    return;
  }

  // don't add a colon, since there's a <br> at the end
  // some of the imdb images use /10.gif in their path, so check for space or bracket
  QRegExp rx(QString::fromLatin1("[>\\s](\\d+.?\\d*)/10[<//s]"), false);
  rx.setMinimal(true);

  if(rx.search(str_) > -1 && !rx.cap(1).isEmpty()) {
    Data::FieldPtr f = entry_->collection()->fieldByName(QString::fromLatin1("imdb-rating"));
    if(!f) {
      f = new Data::Field(QString::fromLatin1("imdb-rating"), i18n("IMDB Rating"), Data::Field::Rating);
      f->setCategory(i18n("General"));
      f->setProperty(QString::fromLatin1("maximum"), QString::fromLatin1("10"));
      entry_->collection()->addField(f);
    }

    bool ok;
    float value = rx.cap(1).toFloat(&ok);
    if(ok) {
      entry_->setField(QString::fromLatin1("imdb-rating"), QString::number(value));
    }
  }
}

void IMDBFetcher::doCover(const QString& str_, Data::EntryPtr entry_, const KURL& baseURL_) {
  // cover is the img with the "cover" alt text
  QRegExp imgRx(QString::fromLatin1("<img\\s+[^>]*src\\s*=\\s*\"([^\"]*)\"[^>]*>"), false);
  imgRx.setMinimal(true);

  QRegExp posterRx(QString::fromLatin1("<a\\s+[^>]*name\\s*=\\s*\"poster\"[^>]*>(.*)</a>"), false);
  posterRx.setMinimal(true);

  const QString cover = QString::fromLatin1("cover");

  int pos = posterRx.search(str_);
  while(pos > -1) {
    if(imgRx.search(posterRx.cap(1)) > -1) {
      KURL u(baseURL_, imgRx.cap(1));
      QString id = ImageFactory::addImage(u, true);
      if(!id.isEmpty()) {
        entry_->setField(cover, id);
      }
      return;
    }
    pos = posterRx.search(str_, pos+1);
  }

  // didn't find the cover, IMDb also used to put "cover" inside the url
  pos = imgRx.search(str_);
  while(pos > -1) {
    if(imgRx.cap(0).find(cover, 0, false) > -1) {
      KURL u(baseURL_, imgRx.cap(1));
      QString id = ImageFactory::addImage(u, true);
      if(!id.isEmpty()) {
        entry_->setField(cover, id);
      }
      return;
    }
    pos = imgRx.search(str_, pos+1);
  }
}

// end up reparsing whole string, but it's not really that slow
// loook at every anchor tag in the string
void IMDBFetcher::doLists(const QString& str_, Data::EntryPtr entry_) {
  const QString genre = QString::fromLatin1("/Genres/");
  const QString country = QString::fromLatin1("/Countries/");
  const QString lang = QString::fromLatin1("/Languages/");
  const QString colorInfo = QString::fromLatin1("color-info");
  const QString cert = QString::fromLatin1("certificates=");
  const QString soundMix = QString::fromLatin1("sound-mix=");
  const QString year = QString::fromLatin1("/Years/");
  const QString company = QString::fromLatin1("/company/");

  // IIMdb also has links with the word "sections" in them, remove that
  // for genres and nationalities

  QStringList genres, countries, langs, certs, tracks, studios;
  for(int pos = s_anchorRx->search(str_); pos > -1; pos = s_anchorRx->search(str_, pos+1)) {
    const QString cap1 = s_anchorRx->cap(1);
    if(cap1.find(genre) > -1) {
      if(s_anchorRx->cap(2).find(QString::fromLatin1(" section"), 0, false) == -1) {
        genres += s_anchorRx->cap(2).stripWhiteSpace();
      }
    } else if(cap1.find(country) > -1) {
      if(s_anchorRx->cap(2).find(QString::fromLatin1(" section"), 0, false) == -1) {
        countries += s_anchorRx->cap(2).stripWhiteSpace();
      }
    } else if(cap1.find(lang) > -1) {
      langs += s_anchorRx->cap(2).stripWhiteSpace();
    } else if(cap1.find(colorInfo) > -1) {
      // change "black and white" to "black & white"
      entry_->setField(QString::fromLatin1("color"),
                       s_anchorRx->cap(2).replace(QString::fromLatin1("and"), QChar('&')).stripWhiteSpace());
    } else if(cap1.find(cert) > -1) {
      certs += s_anchorRx->cap(2).stripWhiteSpace();
    } else if(cap1.find(soundMix) > -1) {
      tracks += s_anchorRx->cap(2).stripWhiteSpace();
    } else if(cap1.find(company) > -1) {
      studios += s_anchorRx->cap(2).stripWhiteSpace();
      // if year field wasn't set before, do it now
    } else if(entry_->field(QString::fromLatin1("year")).isEmpty() && cap1.find(year) > -1) {
      entry_->setField(QString::fromLatin1("year"), s_anchorRx->cap(2).stripWhiteSpace());
    }
  }

  entry_->setField(QString::fromLatin1("genre"), genres.join(sep));
  entry_->setField(QString::fromLatin1("nationality"), countries.join(sep));
  entry_->setField(QString::fromLatin1("language"), langs.join(sep));
  entry_->setField(QString::fromLatin1("audio-track"), tracks.join(sep));
  entry_->setField(QString::fromLatin1("studio"), studios.join(sep));
  if(!certs.isEmpty()) {
    // first try to set default certification
    const QStringList& certsAllowed = entry_->collection()->fieldByName(QString::fromLatin1("certification"))->allowed();
    for(QStringList::ConstIterator it = certs.begin(); it != certs.end(); ++it) {
      QString country = (*it).section(':', 0, 0);
      QString cert = (*it).section(':', 1, 1);
      if(cert == Latin1Literal("Unrated")) {
        cert = QChar('U');
      }
      cert += QString::fromLatin1(" (") + country + ')';
      if(certsAllowed.findIndex(cert) > -1) {
        entry_->setField(QString::fromLatin1("certification"), cert);
        break;
      }
    }

    // now add new field for all certifications
    const QString allc = QString::fromLatin1("allcertification");
    if(m_fields.findIndex(allc) > -1) {
      Data::FieldPtr f = entry_->collection()->fieldByName(allc);
      if(!f) {
        f = new Data::Field(allc, i18n("Certifications"), Data::Field::Table);
        f->setFlags(Data::Field::AllowGrouped);
        entry_->collection()->addField(f);
      }
      entry_->setField(QString::fromLatin1("allcertification"), certs.join(sep));
    }
  }
}

void IMDBFetcher::updateEntry(Data::EntryPtr entry_) {
//  myLog() << "IMDBFetcher::updateEntry() - " << entry_->title() << endl;
  // only take first 5
  m_limit = 5;
  QString t = entry_->field(QString::fromLatin1("title"));
  KURL link = entry_->field(QString::fromLatin1("imdb"));
  if(!link.isEmpty() && link.isValid()) {
    // check if we want a different host
    if(link.host() != m_host) {
//      myLog() << "IMDBFetcher::updateEntry() - switching hosts to " << m_host << endl;
      link.setHost(m_host);
    }
    m_key = Fetch::Title;
    m_value = t;
    m_started = true;
    m_data.truncate(0);
    m_matches.clear();
    m_url = link;
    m_redirected = true; // m_redirected is used as a flag later to tell if we get a single result
    m_job = KIO::get(m_url, false, false);
    connect(m_job, SIGNAL(data(KIO::Job*, const QByteArray&)),
            SLOT(slotData(KIO::Job*, const QByteArray&)));
    connect(m_job, SIGNAL(result(KIO::Job*)),
            SLOT(slotComplete(KIO::Job*)));
    connect(m_job, SIGNAL(redirection(KIO::Job *, const KURL&)),
            SLOT(slotRedirection(KIO::Job*, const KURL&)));
    return;
  }
  // optimistically try searching for title and rely on Collection::sameEntry() to figure things out
  if(!t.isEmpty()) {
    search(Fetch::Title, t);
    return;
  }
  emit signalDone(this); // always need to emit this if not continuing with the search
}

Tellico::Fetch::ConfigWidget* IMDBFetcher::configWidget(QWidget* parent_) const {
  return new IMDBFetcher::ConfigWidget(parent_, this);
}

IMDBFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const IMDBFetcher* fetcher_/*=0*/)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget(), 4, 2);
  l->setSpacing(4);
  l->setColStretch(1, 10);

  int row = -1;
  QLabel* label = new QLabel(i18n("Hos&t: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_hostEdit = new KLineEdit(optionsWidget());
  connect(m_hostEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_hostEdit, row, 1);
  QString w = i18n("The Internet Movie Database uses several different servers. Choose the one "
                   "you wish to use.");
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_hostEdit, w);
  label->setBuddy(m_hostEdit);

  label = new QLabel(i18n("&Maximum cast: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_numCast = new KIntSpinBox(0, 99, 1, 10, 10, optionsWidget());
  connect(m_numCast, SIGNAL(valueChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_numCast, row, 1);
  w = i18n("The list of cast members may include many people. Set the maximum number returned from the search.");
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_numCast, w);
  label->setBuddy(m_numCast);

  m_fetchImageCheck = new QCheckBox(i18n("Download cover &image"), optionsWidget());
  connect(m_fetchImageCheck, SIGNAL(clicked()), SLOT(slotSetModified()));
  ++row;
  l->addMultiCellWidget(m_fetchImageCheck, row, row, 0, 1);
  w = i18n("The cover image may be downloaded as well. However, too many large images in the "
           "collection may degrade performance.");
  QWhatsThis::add(m_fetchImageCheck, w);

  l->setRowStretch(++row, 10);

  // now add additional fields widget
  addFieldsWidget(IMDBFetcher::customFields(), fetcher_ ? fetcher_->m_fields : QStringList());

  if(fetcher_) {
    m_hostEdit->setText(fetcher_->m_host);
    m_numCast->setValue(fetcher_->m_numCast);
    m_fetchImageCheck->setChecked(fetcher_->m_fetchImages);
  } else { //defaults
    m_hostEdit->setText(QString::fromLatin1(IMDB_SERVER));
    m_numCast->setValue(10);
    m_fetchImageCheck->setChecked(true);
  }
}

void IMDBFetcher::ConfigWidget::saveConfig(KConfigGroup& config_) {
  QString host = m_hostEdit->text().stripWhiteSpace();
  if(!host.isEmpty()) {
    config_.writeEntry("Host", host);
  }
  config_.writeEntry("Max Cast", m_numCast->value());
  config_.writeEntry("Fetch Images", m_fetchImageCheck->isChecked());

  saveFieldsConfig(config_);
  slotSetModified(false);
}

QString IMDBFetcher::ConfigWidget::preferredName() const {
  return IMDBFetcher::defaultName();
}

//static
Tellico::StringMap IMDBFetcher::customFields() {
  StringMap map;
  map[QString::fromLatin1("imdb")]             = i18n("IMDB Link");
  map[QString::fromLatin1("imdb-rating")]      = i18n("IMDB Rating");
  map[QString::fromLatin1("alttitle")]         = i18n("Alternative Titles");
  map[QString::fromLatin1("allcertification")] = i18n("Certifications");
  return map;
}

#include "imdbfetcher.moc"
