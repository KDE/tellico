/***************************************************************************
    copyright            : (C) 2004 by Robby Stephenson
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
#include "../filehandler.h"
#include "../latin1literal.h"
#include "../imagefactory.h"
#include "../tellico_utils.h"

#include <klocale.h>
#include <kdebug.h>
#include <kdialogbase.h>
#include <kconfig.h>
#include <klineedit.h>

#include <qregexp.h>
#include <qfile.h>
#include <qmap.h>
#include <qvbox.h>
#include <qlabel.h>
#include <qlistbox.h>
#include <qwhatsthis.h>
#include <qlayout.h>
#include <qcheckbox.h>

//#define IMDB_TEST

namespace {
  static const char* IMDB_SERVER = "www.imdb.com";
  static const uint IMDB_MAX_RESULTS = 25;
  static const QString sep = QString::fromLatin1("; ");
}

using Tellico::Fetch::IMDBFetcher;

QRegExp* IMDBFetcher::s_tagRx = 0;
QRegExp* IMDBFetcher::s_anchorRx = 0;
QRegExp* IMDBFetcher::s_anchorRx2 = 0;
QRegExp* IMDBFetcher::s_titleRx = 0;

// static
void IMDBFetcher::initRegExps() {
  s_tagRx = new QRegExp(QString::fromLatin1("<.*>"));
  s_tagRx->setMinimal(true);

  s_anchorRx = new QRegExp(QString::fromLatin1("<a\\s+[^<]*href\\s*=\\s*\"([^\"]*)\">([^<]*)</a>"), false);
  s_anchorRx->setMinimal(true);

  s_anchorRx2 = new QRegExp(QString::fromLatin1("<a\\s+[^<]*href\\s*=\\s*\"([^\"]*/title/[^\"]*)\">([^<]*)</a>"), false);
  s_anchorRx2->setMinimal(true);

  s_titleRx = new QRegExp(QString::fromLatin1("<title>(.*)</title>"), false);
  s_titleRx->setMinimal(true);
}

IMDBFetcher::IMDBFetcher(QObject* parent_, const char* name_) : Fetcher(parent_, name_),
    m_job(0), m_started(false), m_fetchImages(false), m_host(QString::fromLatin1(IMDB_SERVER)) {
  if(!s_tagRx) {
    initRegExps();
  }
}

IMDBFetcher::~IMDBFetcher() {
  cleanUp();
}

void IMDBFetcher::cleanUp() {
  // need to delete collection pointers
  QPtrList<Data::Collection> collList;
  for(QIntDictIterator<Data::Entry> it(m_entries); it.current(); ++it) {
    if(collList.findRef(it.current()->collection()) == -1) {
      collList.append(it.current()->collection());
    }
  }
  collList.setAutoDelete(true); // will automatically delete all entries
}

QString IMDBFetcher::source() const {
  return i18n("Internet Movie Database");
}

void IMDBFetcher::readConfig(KConfig* config_, const QString& group_) {
  KConfigGroupSaver groupSaver(config_, group_);
  QString h = config_->readEntry("Host");
  if(!h.isEmpty()) {
    m_host = h;
  }
  bool b = config_->readBoolEntry("Fetch Images", true);
  m_fetchImages = b;
}

// multiple values not supported
void IMDBFetcher::search(FetchKey key_, const QString& value_, bool) {
  m_key = key_;
  m_value = value_;
  m_started = true;
  m_redirected = false;
  m_data.truncate(0);
  m_matches.clear();

// only search if current collection is a video collection
  if(Kernel::self()->collection()->type() != Data::Collection::Video) {
    kdDebug() << "IMDBFetcher::search() - collection type mismatch, stopping" << endl;
    stop();
    return;
  }

#ifdef IMDB_TEST
  if(m_key == Title) {
    m_url = KURL::fromPathOrURL(QString::fromLatin1("/home/robby/imdb-title.html"));
  } else {
    m_url = KURL::fromPathOrURL(QString::fromLatin1("/home/robby/imdb-name.html"));
  }
  m_redirected = false;
#else
  m_url.setProtocol(QString::fromLatin1("http"));
  m_url.setHost(m_host.isEmpty() ? QString::fromLatin1(IMDB_SERVER) : m_host);
  m_url.setPath(QString::fromLatin1("/find"));
  switch(key_) {
    case Title:
      m_url.setQuery(QString::fromLatin1("tt=on;q=" + value_));
      break;

    case Person:
      m_url.setQuery(QString::fromLatin1("nm=on;q=" + value_));
      break;

    default:
      kdWarning() << "IMDBFetcher::search() - FetchKey not supported" << endl;
      stop();
      return;
  }
//  kdDebug() << m_url.prettyURL() << endl;
#endif

  m_job = KIO::get(m_url, false, false);
  connect(m_job, SIGNAL(data(KIO::Job*, const QByteArray&)),
          SLOT(slotData(KIO::Job*, const QByteArray&)));
  connect(m_job, SIGNAL(result(KIO::Job*)),
          SLOT(slotComplete(KIO::Job*)));
  connect(m_job, SIGNAL(redirection(KIO::Job *, const KURL&)),
          SLOT(slotRedirection(KIO::Job*, const KURL&)));
}

void IMDBFetcher::stop() {
  if(!m_started) {
    return;
  }
  if(m_job) {
    m_job->kill();
    m_job = 0;
  }

  emit signalDone(this);

  m_started = false;
  m_redirected = false;
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
//  kdDebug() << "IMDBFetcher::slotComplete()" << endl;

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
//  kdDebug() << "IMDBFetcher::parseSingleTitleResult()" << endl;
  s_titleRx->search(Tellico::decodeHTML(QString(m_data)));
  // split title at parenthesis
  const QString cap1 = s_titleRx->cap(1);
  int pPos = cap1.find('(');
  // FIXME: maybe remove parentheses here?
  SearchResult* r = new SearchResult(this,
                                     pPos == -1 ? cap1 : cap1.left(pPos),
                                     pPos == -1 ? QString::null : cap1.mid(pPos));
  m_results.insert(r->uid, r);
  m_matches.insert(r->uid, m_url);
  emit signalResultFound(*r);

  stop();
}

void IMDBFetcher::parseMultipleTitleResults() {
//  kdDebug() << "IMDBFetcher::parseMultipleTitleResults()" << endl;
  QString output = Tellico::decodeHTML(QString(m_data));

  // IMDb can return three title lists, popular, exact, and partial
  // the popular titles are in the first table, after the "Popular Results" text
  int pos_popular = output.find(QString::fromLatin1("Popular Titles"),  0,                    false);
  int pos_exact   = output.find(QString::fromLatin1("Exact Matches"),   KMAX(pos_popular, 0), false);
  int pos_partial = output.find(QString::fromLatin1("Partial Matches"), KMAX(pos_exact, 0),   false);
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
    parseTitleBlock(output.mid(pos_popular, end_popular-pos_popular));
  }

  // if found exact matches
  if(pos_exact > -1) {
    parseTitleBlock(output.mid(pos_exact, end_exact-pos_exact));
  }

  // if there are enough exact matches, stop now
  if(m_matches.size() >= IMDB_MAX_RESULTS) {
    stop();
    return;
  }

  // go ahead and search for partial matches
  if(pos_partial > -1) {
    parseTitleBlock(output.mid(pos_partial));
  }

#ifndef NDEBUG
  if(m_matches.size() == 0) {
    kdDebug() << "IMDBFetcher::parseMultipleTitleResults() - no matches found." << endl;
  }
#endif

  stop();
}

void IMDBFetcher::parseTitleBlock(const QString& str_) {
  QRegExp akaRx;
  akaRx.setPattern(QString::fromLatin1("aka (.*)</li>"));
  akaRx.setMinimal(true);
  akaRx.setCaseSensitive(false);

  int start = s_anchorRx2->search(str_);
  while(start > -1 && m_matches.size() < IMDB_MAX_RESULTS) {
    // split title at parenthesis
    const QString cap1 = s_anchorRx2->cap(1); // the anchor url
    const QString cap2 = s_anchorRx2->cap(2); // the anchor text
    int pPos = cap2.find('('); // if it has parentheses, use that for description
    QString desc;
    if(pPos > -1) {
      int pPos2 = cap2.find(')', pPos+1);
      if(pPos2 > -1) {
        desc = cap2.mid(pPos+1, pPos2-pPos-1);
      }
    } else {
      // parenthesis might be outside anchor tag
      int end = s_anchorRx2->search(str_, start+1);
      if(end == -1) {
        end = str_.length();
      }
      // remove tags
      QString text = str_.mid(start, end-start).remove(*s_tagRx);
      pPos = text.find('(');
      if(pPos > -1) {
        int pPos2 = text.find(')', pPos);
        if(pPos2 > -1) {
          desc = text.mid(pPos+1, pPos2-pPos-1);
        }
      }
    }
    // multiple matches might have 'aka' info
    int end = s_anchorRx2->search(str_, start+1);
    int akaPos = akaRx.search(str_, start+1);
    if(akaPos > -1 && akaPos < end) {
      desc += QChar(' ') + akaRx.cap(1).stripWhiteSpace().remove(*s_tagRx);
    }

    SearchResult* r = new SearchResult(this, pPos == -1 ? cap2 : cap2.left(pPos), desc);
    m_results.insert(r->uid, r);
    KURL u;
    if(KURL::isRelativeURL(cap1)) {
      u.setProtocol(m_url.protocol());
      u.setHost(m_url.host());
      u.setPath(cap1);
    } else {
      u = KURL(cap1);
    }
    m_matches.insert(r->uid, u);
    emit signalResultFound(*r);
    start = s_anchorRx2->search(str_, start+cap2.length());
  }
}

void IMDBFetcher::parseSingleNameResult() {
//  kdDebug() << "IMDBFetcher::parseSingleNameResult()" << endl;

  QString output = Tellico::decodeHTML(QString(m_data));

  int pos = s_anchorRx2->search(output);
  if(pos == -1) {
    stop();
    return;
  }

  while(pos > -1 && m_matches.size() < IMDB_MAX_RESULTS) {
    // split title at parenthesis
    const QString cap2 = s_anchorRx2->cap(2);
    int pPos = cap2.find('(');
    // FIXME: maybe remove parentheses here?
    SearchResult* r = new SearchResult(this,
                                       pPos == -1 ? cap2 : cap2.left(pPos),
                                       pPos == -1 ? QString::null : cap2.mid(pPos));
    m_results.insert(r->uid, r);
    KURL u(m_url, s_anchorRx2->cap(1)); // relative URL constructor
    m_matches.insert(r->uid, u);
//    kdDebug() << u.prettyURL() << endl;
//    kdDebug() << cap2 << endl;
    emit signalResultFound(*r);
    pos = s_anchorRx2->search(output, pos+s_anchorRx2->cap(0).length());
  }

  stop();
}

void IMDBFetcher::parseMultipleNameResults() {
//  kdDebug() << "IMDBFetcher::parseMultipleNameResults()" << endl;

  // the exact results are in the first table after the "exact results" text
  QString output = Tellico::decodeHTML(QString(m_data));
  int pos = output.find(QString::fromLatin1("Exact Matches"), 0, false);

  // find beginning of partial matches
  int end = output.find(QString::fromLatin1("Partial Matches"), KMAX(pos, 0), false);
  if(end == -1) {
    end = output.length();
  }

  QMap<QString, KURL> map;

  // if found exact matches
  if(pos > -1) {
    int i = 1;
    pos = s_anchorRx2->search(output, pos+13);
    while(pos > -1 && pos < end && m_matches.size() < IMDB_MAX_RESULTS) {
      KURL u(m_url, s_anchorRx2->cap(1));
      map.insert(s_anchorRx2->cap(2) + ' ' + QString::number(i), u);
      pos = s_anchorRx2->search(output, pos+s_anchorRx2->cap(0).length());
      ++i;
    }
  }

  // go ahead and search for partial matches
  pos = s_anchorRx2->search(output, end);
  while(pos > -1 && m_matches.size() < IMDB_MAX_RESULTS) {
    KURL u(m_url, s_anchorRx2->cap(1)); // relative URL
    map.insert(s_anchorRx2->cap(2), u);
    pos = s_anchorRx2->search(output, pos+s_anchorRx2->cap(0).length());
  }

  if(map.count() == 0) {
    stop();
    return;
  }

  KDialogBase* dlg = new KDialogBase(Kernel::self()->widget(), "imdb dialog",
                                     true, i18n("Select IMDB Result"), KDialogBase::Ok);
  QVBox* box = new QVBox(dlg);
  box->setSpacing(10);
  (void) new QLabel(i18n("<qt>Your search returned multiple matches. Please select one below.</qt>"), box);

  QListBox* listBox = new QListBox(box);
  listBox->setMinimumWidth(400);
  listBox->setColumnMode(QListBox::FitToWidth);
  listBox->insertStringList(map.keys());
  listBox->setSelected(0, true);
  QWhatsThis::add(listBox, i18n("<qt>Select a search result.</qt>"));

  dlg->setMainWidget(box);
  dlg->exec();

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

  // do not stop() here
}

Tellico::Data::Entry* IMDBFetcher::fetchEntry(uint uid_) {
  // if we already grabbed this one, then just pull it out of the dict
  Data::Entry* entry = m_entries[uid_];
  if(entry) {
    return new Data::Entry(*entry, Kernel::self()->collection());
  }

  KURL url = m_matches[uid_];
  if(url.isEmpty()) {
    return 0;
  }

  QString results;
  // if the url matches the current one, no need to redownload it
  if(url == m_url) {
//    kdDebug() << "IMDBFetcher::fetchEntry() - matches previous URL, no downloading needed." << endl;
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
    return 0;
  }

  entry = parseEntry(results);
  if(!entry) {
    return 0;
  }
  m_entries.insert(uid_, entry); // keep for later
  return new Data::Entry(*entry, Kernel::self()->collection()); // clone
}

Tellico::Data::Entry* IMDBFetcher::parseEntry(const QString& str_) {
  Data::Collection* coll = new Data::VideoCollection(true);
  Data::Entry* entry = new Data::Entry(coll);

  doTitle(str_, entry);
  doRunningTime(str_, entry);
  doAlsoKnownAs(str_, entry);
  doPlot(str_, entry, m_url);
  doLists(str_, entry);
  doPerson(str_, entry, QString::fromLatin1("Directed"), QString::fromLatin1("director"));
  doPerson(str_, entry, QString::fromLatin1("Writing"), QString::fromLatin1("writer"));
  doCast(str_, entry, m_url);
  if(m_fetchImages) {
    // needs base URL
    doCover(str_, entry, m_url);
  }

  return entry;
}

void IMDBFetcher::doTitle(const QString& str_, Data::Entry* entry_) {
  if(s_titleRx->search(str_) > -1) {
    const QString cap1 = s_titleRx->cap(1);
    // titles always have parentheses
    int pPos = cap1.find('(');
    entry_->setField(QString::fromLatin1("title"), cap1.left(pPos).stripWhiteSpace());
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

void IMDBFetcher::doRunningTime(const QString& str_, Data::Entry* entry_) {
  // running time
  // this minimal thing makes regexps hard
  // "runtime:.*(\\d+)" matches to the end of the document if not minimal
  // and only grabs one digit if minimal
  QRegExp runtimeRx(QString::fromLatin1("runtime:[^\\d]*(\\d+)"));
  runtimeRx.setCaseSensitive(false);

  if(runtimeRx.search(str_) > -1) {
//    kdDebug() << "running-time = " << runtimeRx.cap(1) << endl;
    entry_->setField(QString::fromLatin1("running-time"), runtimeRx.cap(1));
  }
}

void IMDBFetcher::doAlsoKnownAs(const QString& str_, Data::Entry* entry_) {
  // don't add a colon, since there's a <br> at the end
  // match until next b tag
  QRegExp akaRx(QString::fromLatin1("also known as(.*)<b(?:\\s.*)?>"));
  akaRx.setMinimal(true);
  akaRx.setCaseSensitive(false);

  if(akaRx.search(str_) > -1 && !akaRx.cap(1).isEmpty()) {
    Data::Field* f = new Data::Field(QString::fromLatin1("alttitle"), i18n("Alternative Titles"),
                                     Data::Field::Table);
    f->setFormatFlag(Data::Field::FormatTitle);
    entry_->collection()->addField(f);

    // split by <br>, remembering it could become valid xhtml!
    QRegExp brRx(QString::fromLatin1("<br[\\s/]*>"), false);
    brRx.setMinimal(true);
    QStringList list = QStringList::split(brRx, akaRx.cap(1));
    // only valid title if it contains parentheses with a year
    const QRegExp parYearRx(QString::fromLatin1("\\(\\d+\\)"));
    QStringList values;
    for(QStringList::Iterator it = list.begin(); it != list.end(); ++it) {
      if((*it).find(parYearRx) > -1) {
        (*it).remove(*s_tagRx);
        values += (*it).stripWhiteSpace();
      }
    }
    entry_->setField(QString::fromLatin1("alttitle"), values.join(sep));
  }
}

void IMDBFetcher::doPlot(const QString& str_, Data::Entry* entry_, const KURL& baseURL_) {
  // plot summries provided by users are on a separate page
  // should those be preferred?

  bool useUserSummary = false;

  if(useUserSummary) {
    QRegExp idRx(QString::fromLatin1("title/(tt\\d+)"));
    idRx.search(baseURL_.path());
    KURL plotURL = baseURL_;
    plotURL.setPath(QString::fromLatin1("/title/") + idRx.cap(1) + QString::fromLatin1("/plotsummary"));
    // be quiet about failure
    QString plotPage = FileHandler::readTextFile(plotURL, true);

    if(plotPage.isEmpty()) {
      useUserSummary = false;
    } else {
      QRegExp plotRx(QString::fromLatin1("<p\\s+class\\s*=\\s*\"plotpar\">(.*)</p"));
      plotRx.setMinimal(true);
      if(plotRx.search(plotPage) > -1) {
        QString plot = plotRx.cap(1);
        plot.remove(*s_tagRx); // remove HTML tags
        entry_->setField(QString::fromLatin1("plot"), plot);
      }
    }
  }

   // this isn't an else tag since the other call may fail and set useUserSummary to false
  if(!useUserSummary) {
    // match until next opening tag
    QRegExp plotRx(QString::fromLatin1("plot outline:(.*)<[^/]"), false);
    plotRx.setMinimal(true);
    if(plotRx.search(str_) > -1) {
      QString plot = plotRx.cap(1);
      plot.remove(*s_tagRx); // remove HTML tags
      entry_->setField(QString::fromLatin1("plot"), plot);
    }
  }
}

void IMDBFetcher::doPerson(const QString& str_, Data::Entry* entry_,
                           const QString& imdbHeader_, const QString& fieldName_) {
  int pos = str_.find(imdbHeader_);
  if(pos > -1) {
    QStringList people;
    // loop until repeated <br> tags
    QRegExp br2Rx(QString::fromLatin1("<br[\\s/]*>\\s*<br[\\s/]*>"), false);
    br2Rx.setMinimal(true);

    const QString name = QString::fromLatin1("/name/");
    const int endPos = str_.find(br2Rx, pos);
    pos = s_anchorRx->search(str_, pos+1);
    while(pos > -1 && pos < endPos) {
      if(s_anchorRx->cap(1).find(name) > -1) {
        people += s_anchorRx->cap(2);
      }
      pos = s_anchorRx->search(str_, pos+1);
    }
    if(!people.isEmpty()) {
      entry_->setField(fieldName_, people.join(sep));
    }
  }
}

void IMDBFetcher::doCast(const QString& str_, Data::Entry* entry_, const KURL& baseURL_) {
  // the extended cast list is on a separate page
  // that's usually a lot of people
  // but since it can be in billing order, the main actors might not
  // be in the short list
  QRegExp idRx(QString::fromLatin1("title/(tt\\d+)"));
  idRx.search(baseURL_.path());
  KURL castURL = baseURL_;
  castURL.setPath(QString::fromLatin1("/title/") + idRx.cap(1) + QString::fromLatin1("/fullcredits"));
  // be quiet about failure and be sure to translate entities
  QString castPage = Tellico::decodeHTML(FileHandler::readTextFile(castURL, true));

  QStringList cast;
  int pos;
  // the text to search, depends on which page is being read
  QString& castText = castPage;
  if(!castText.isEmpty()) {
    // fragile, the word "cast" appears in the title, but need to find
    // the one right above the actual cast table
    pos = castText.find(QString::fromLatin1("cast</"), 0, false);
  } else { // fall back to short list
    pos = str_.find(QString::fromLatin1("cast overview"), 0, false);
    if(pos == -1) {
      pos = str_.find(QString::fromLatin1("credited cast"), 0, false);
    }
    castText = str_;
  }
  if(pos == -1) { // no cast list found
    return;
  }

  const QString name = QString::fromLatin1("/name/");
  QRegExp tdRx(QString::fromLatin1("<td[^>]*>(.*)</td>"), false);
  tdRx.setMinimal(true);

  // loop until closing table tag
  const int endPos = castText.find(QString::fromLatin1("</table"), pos, false);
  pos = s_anchorRx->search(castText, pos+1);
  while(pos > -1 && pos < endPos) {
    if(s_anchorRx->cap(1).find(name) > -1) {
      // now search for <td> item with character name
      // there's a column with ellipses then the character
      const int pos2 = tdRx.search(castText, pos);
      if(pos2 > -1 && tdRx.search(castText, pos2+1) > -1) {
        cast += s_anchorRx->cap(2) + QString::fromLatin1("::") + tdRx.cap(1).simplifyWhiteSpace().remove(*s_tagRx);
      } else {
        cast += s_anchorRx->cap(2);
      }
    }
    pos = s_anchorRx->search(castText, pos+1);
  }

  if(!cast.isEmpty()) {
    entry_->setField(QString::fromLatin1("cast"), cast.join(sep));
  }
}

void IMDBFetcher::doCover(const QString& str_, Data::Entry* entry_, const KURL& baseURL_) {
  // cover is the img with the "cover" alt text
  QRegExp imgRx(QString::fromLatin1("<img\\s+[^>]*src\\s*=\\s*\"([^\"]*)\"[^>]*>"), false);
  imgRx.setMinimal(true);

  const QString cover = QString::fromLatin1("cover");
  int pos = imgRx.search(str_);
  while(pos > -1) {
    if(imgRx.cap(0).find(cover, 0, false) > -1) {
      KURL u(baseURL_, imgRx.cap(1));
      const Data::Image& img = ImageFactory::addImage(u, true);
      if(!img.isNull()) {
        entry_->setField(QString::fromLatin1("cover"), img.id());
      }
      break;
    }
    pos = imgRx.search(str_, pos+1);
  }
}

// end up reparsing whole string, but it's not really that slow
// loook at every anchor tag in the string
void IMDBFetcher::doLists(const QString& str_, Data::Entry* entry_) {
  const QString genre = QString::fromLatin1("/Genres/");
  const QString country = QString::fromLatin1("/Countries/");
  const QString lang = QString::fromLatin1("/Languages/");
  const QString colorInfo = QString::fromLatin1("color-info");
  const QString cert = QString::fromLatin1("certificates=");
  const QString soundMix = QString::fromLatin1("sound-mix=");
  const QString year = QString::fromLatin1("/Years/");

  QStringList genres, countries, langs, certs, tracks;
  for(int pos = s_anchorRx->search(str_); pos > -1; pos = s_anchorRx->search(str_, pos+1)) {
    if(s_anchorRx->cap(1).find(genre) > -1) {
      genres += s_anchorRx->cap(2);
    } else if(s_anchorRx->cap(1).find(country) > -1) {
      countries += s_anchorRx->cap(2);
    } else if(s_anchorRx->cap(1).find(lang) > -1) {
      langs += s_anchorRx->cap(2);
    } else if(s_anchorRx->cap(1).find(colorInfo) > -1) {
      // change "black and white" to "black & white"
      entry_->setField(QString::fromLatin1("color"),
                       s_anchorRx->cap(2).replace(QString::fromLatin1("and"), QChar('&')));
    } else if(s_anchorRx->cap(1).find(cert) > -1) {
      certs += s_anchorRx->cap(2);
    } else if(s_anchorRx->cap(1).find(soundMix) > -1) {
      tracks += s_anchorRx->cap(2);
      // if year field wasn't set before, do it now
    } else if(entry_->field(QString::fromLatin1("year")).isEmpty()
              && s_anchorRx->cap(1).find(year) > -1) {
      entry_->setField(QString::fromLatin1("year"), s_anchorRx->cap(2));
    }
  }

  entry_->setField(QString::fromLatin1("genre"), genres.join(sep));
  entry_->setField(QString::fromLatin1("nationality"), countries.join(sep));
  entry_->setField(QString::fromLatin1("language"), langs.join(sep));
  entry_->setField(QString::fromLatin1("audio-track"), tracks.join(sep));
  if(!certs.isEmpty()) {
    // first try to set default certification
    const QStringList& certsAllowed = entry_->collection()->fieldByName(QString::fromLatin1("certification"))->allowed();
    for(QStringList::ConstIterator it = certs.begin(); it != certs.end(); ++it) {
      QString country = (*it).section(':', 0, 0);
      QString cert = (*it).section(':', 1, 1);
      if(cert == Latin1Literal("Unrated")) {
        cert = 'U';
      }
      cert += QString::fromLatin1(" (") + country + ')';
      if(certsAllowed.findIndex(cert) > -1) {
        entry_->setField(QString::fromLatin1("certification"), cert);
        break;
      }
    }

    // now add new field for all certifications
    Data::Field* f = new Data::Field(QString::fromLatin1("allcertification"), i18n("Certifications"),
                                     Data::Field::Table);
    f->setFlags(Data::Field::AllowGrouped);
    entry_->collection()->addField(f);
    entry_->setField(QString::fromLatin1("allcertification"), certs.join(sep));
  }
}

Tellico::Fetch::ConfigWidget* IMDBFetcher::configWidget(QWidget* parent_) {
  return new IMDBFetcher::ConfigWidget(parent_, this);
}

IMDBFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, IMDBFetcher* fetcher_/*=0*/)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(this, 3, 2);
  l->setSpacing(4);
//  l->setAutoAdd(true);
  QLabel* label = new QLabel(i18n("&Host: "), this);
  l->addWidget(label, 0, 0);
  m_hostEdit = new KLineEdit(this);
  connect(m_hostEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_hostEdit, 0, 1);
  QString w = i18n("The Internet Movie Database uses several different servers. Choose the one "
                   "you wish to use.");
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_hostEdit, w);
  label->setBuddy(m_hostEdit);

  m_fetchImageCheck = new QCheckBox(i18n("Download cover image"), this);
  connect(m_fetchImageCheck, SIGNAL(clicked()), SLOT(slotSetModified()));
  l->addMultiCellWidget(m_fetchImageCheck, 1, 1, 0, 1);
  w = i18n("The cover image may be downloaded as well. However, too many large images in the "
           "collection may degrade performance.");
  QWhatsThis::add(m_fetchImageCheck, w);

  l->setRowStretch(3, 1);

  if(fetcher_) {
    m_hostEdit->setText(fetcher_->m_host);
    m_fetchImageCheck->setChecked(fetcher_->m_fetchImages);
  } else { //defaults
    m_hostEdit->setText(QString::fromLatin1(IMDB_SERVER));
  }
}

void IMDBFetcher::ConfigWidget::saveConfig(KConfig* config_) {
  QString host = m_hostEdit->text();
  if(!host.isEmpty()) {
    config_->writeEntry("Host", host);
  }
  config_->writeEntry("Fetch Images", m_fetchImageCheck->isChecked());
  slotSetModified(false);
}

#include "imdbfetcher.moc"
