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
  static const char* IMDB_SERVER = "www.imdb.com";
  static const uint IMDB_MAX_RESULTS = 25;
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
    m_job(0), m_started(false), m_fetchImages(true), m_host(QString::fromLatin1(IMDB_SERVER)), m_limit(IMDB_MAX_RESULTS) {
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

void IMDBFetcher::readConfig(KConfig* config_, const QString& group_) {
  KConfigGroupSaver groupSaver(config_, group_);
  QString s = config_->readEntry("Name", defaultName());
  if(!s.isEmpty()) {
    m_name = s;
  }
  QString h = config_->readEntry("Host");
  if(!h.isEmpty()) {
    m_host = h;
  }
  m_numCast = config_->readNumEntry("Max Cast", 10);
  m_fetchImages = config_->readBoolEntry("Fetch Images", true);
  m_fields = config_->readListEntry("Custom Fields");
}

// multiple values not supported
void IMDBFetcher::search(FetchKey key_, const QString& value_) {
  m_key = key_;
  m_value = value_;
  m_started = true;
  m_redirected = false;
  m_data.truncate(0);
  m_matches.clear();

// only search if current collection is a video collection
  if(Kernel::self()->collectionType() != Data::Collection::Video) {
    myDebug() << "IMDBFetcher::search() - collection type mismatch, stopping" << endl;
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
//  myDebug() << m_url.url() << endl;
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
//  myDebug() << "IMDBFetcher::stop()" << endl;
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
                                     pPos == -1 ? QString::null : cap1.mid(pPos));
  m_matches.insert(r->uid, m_url);
  emit signalResultFound(r);

  stop();
}

void IMDBFetcher::parseMultipleTitleResults() {
//  myDebug() << "IMDBFetcher::parseMultipleTitleResults()" << endl;
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
  if(m_matches.size() >= m_limit) {
    stop();
    return;
  }

  // go ahead and search for partial matches
  if(pos_partial > -1) {
    parseTitleBlock(output.mid(pos_partial));
  }

#ifndef NDEBUG
  if(m_matches.size() == 0) {
    myDebug() << "IMDBFetcher::parseMultipleTitleResults() - no matches found." << endl;
  }
#endif

  stop();
}

void IMDBFetcher::parseTitleBlock(const QString& str_) {
//  myDebug() << "IMDBFetcher::parseTitleBlock()" << endl;

  QRegExp akaRx(QString::fromLatin1("aka (.*)</li>"), false);
  akaRx.setMinimal(true);

  int start = s_anchorTitleRx->search(str_);
  while(m_started && start > -1 && m_matches.size() < m_limit) {
    // split title at parenthesis
    const QString cap1 = s_anchorTitleRx->cap(1); // the anchor url
    const QString cap2 = s_anchorTitleRx->cap(2); // the anchor text
    int pPos = cap2.find('('); // if it has parentheses, use that for description
    QString desc;
    if(pPos > -1) {
      int pPos2 = cap2.find(')', pPos+1);
      if(pPos2 > -1) {
        desc = cap2.mid(pPos+1, pPos2-pPos-1);
      }
    } else {
      // parenthesis might be outside anchor tag
      int end = s_anchorTitleRx->search(str_, start+1);
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
    int end = s_anchorTitleRx->search(str_, start+1);
    int akaPos = akaRx.search(str_, start+1);
    if(akaPos > -1 && akaPos < end) {
      desc += QChar(' ') + akaRx.cap(1).stripWhiteSpace().remove(*s_tagRx);
    }

    SearchResult* r = new SearchResult(this, pPos == -1 ? cap2 : cap2.left(pPos), desc);
    KURL u(m_url, cap1);
    u.setQuery(QString::null);
    m_matches.insert(r->uid, u);
    emit signalResultFound(r);
    start = s_anchorTitleRx->search(str_, start+cap2.length());
  }
}

void IMDBFetcher::parseSingleNameResult() {
//  myDebug() << "IMDBFetcher::parseSingleNameResult()" << endl;

  QString output = Tellico::decodeHTML(QString(m_data));

  int pos = s_anchorTitleRx->search(output);
  if(pos == -1) {
    stop();
    return;
  }

  QString desc;
  while(m_started && pos > -1 && m_matches.size() < m_limit) {
    int len = s_anchorTitleRx->cap(0).length();
    // split title at parenthesis
    const QString cap2 = s_anchorTitleRx->cap(2);
    int pPos = cap2.find('(');
    if(pPos > -1) {
      desc = cap2.mid(pPos);
    } else {
      // look until the next <a
      int aPos = output.find(QString::fromLatin1("<a"), pos+len, false);
      if(aPos == -1) {
        aPos = output.length();
      }
      QString tmp = output.mid(pos+len, aPos-pos-len).remove(*s_tagRx);;
      pPos = tmp.find('(');
      if(pPos > -1) {
        int pEnd = tmp.find(')', pPos+1);
        desc = tmp.mid(pPos+1, pEnd-pPos-1);
        // but need to indicate it wasn't found initially
        pPos = -1;
      }
    }
    // FIXME: maybe remove parentheses here?
    SearchResult* r = new SearchResult(this, pPos == -1 ? cap2 : cap2.left(pPos), desc);
    KURL u(m_url, s_anchorTitleRx->cap(1)); // relative URL constructor
    u.setQuery(QString::null);
    m_matches.insert(r->uid, u);
//    myDebug() << u.prettyURL() << endl;
//    myDebug() << cap2 << endl;
    emit signalResultFound(r);
    pos = s_anchorTitleRx->search(output, pos+len);
  }

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
  int end = output.find(QString::fromLatin1("Other Results"), KMAX(pos, 0), false);
  if(end == -1) {
    end = output.find(QString::fromLatin1("Partial Matches"), KMAX(pos, 0), false);
    if(end == -1) {
      end = output.find(QString::fromLatin1("Approx Matches"), KMAX(pos, 0), false);
      if(end == -1) {
        end = output.length();
      }
    }
  }

  QMap<QString, KURL> map;

  // if found exact matches
  if(pos > -1) {
    int i = 1;
    pos = s_anchorNameRx->search(output, pos+13);
    while(pos > -1 && pos < end && m_matches.size() < m_limit) {
      KURL u(m_url, s_anchorNameRx->cap(1));
      // if more than one exact, add parentheses
      if(i > 1) {
        // check for duplicate names
        QString s = s_anchorNameRx->cap(2) + ' ';
        if(map.contains(s)) {
          s = s_anchorNameRx->cap(2) + " (" + QString::number(i) + ") ";
        }
        map.insert(s, u);
      } else {
        // add a space so we'll recognize an exact match later
        // QMap sorts alphabetically, so these will come first, too
        map.insert(s_anchorNameRx->cap(2) + ' ', u);
      }
      pos = s_anchorNameRx->search(output, pos+s_anchorNameRx->cap(0).length());
      ++i;
    }
  }

  // go ahead and search for partial matches
  pos = s_anchorNameRx->search(output, end);
  while(pos > -1 && m_matches.size() < m_limit) {
    KURL u(m_url, s_anchorNameRx->cap(1)); // relative URL
    map.insert(s_anchorNameRx->cap(2), u);
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
    return 0;
  }

  entry = parseEntry(results);
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
  doAlsoKnownAs(str_, entry);
  doPlot(str_, entry, m_url);
  doLists(str_, entry);
  doPerson(str_, entry, QString::fromLatin1("Directed"), QString::fromLatin1("director"));
  doPerson(str_, entry, QString::fromLatin1("Writing"), QString::fromLatin1("writer"));
  doRating(str_, entry);
  doCast(str_, entry, m_url);
  if(m_fetchImages) {
    // needs base URL
    doCover(str_, entry, m_url);
  }

  const QString imdb = QString::fromLatin1("imdb");
  if(!coll->hasField(imdb) && m_fields.contains(imdb)) {
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

void IMDBFetcher::doRunningTime(const QString& str_, Data::EntryPtr entry_) {
  // running time
  // this minimal thing makes regexps hard
  // "runtime:.*(\\d+)" matches to the end of the document if not minimal
  // and only grabs one digit if minimal
  QRegExp runtimeRx(QString::fromLatin1("runtime:[^\\d]*(\\d+)"));
  runtimeRx.setCaseSensitive(false);

  if(runtimeRx.search(str_) > -1) {
//    myDebug() << "running-time = " << runtimeRx.cap(1) << endl;
    entry_->setField(QString::fromLatin1("running-time"), runtimeRx.cap(1));
  }
}

void IMDBFetcher::doAlsoKnownAs(const QString& str_, Data::EntryPtr entry_) {
  if(!m_fields.contains(QString::fromLatin1("alttitle"))) {
    return;
  }

  // don't add a colon, since there's a <br> at the end
  // match until next b tag
  QRegExp akaRx(QString::fromLatin1("also known as(.*)<b(?:\\s.*)?>"));
  akaRx.setMinimal(true);
  akaRx.setCaseSensitive(false);

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

void IMDBFetcher::doPlot(const QString& str_, Data::EntryPtr entry_, const KURL& baseURL_) {
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

void IMDBFetcher::doPerson(const QString& str_, Data::EntryPtr entry_,
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

void IMDBFetcher::doCast(const QString& str_, Data::EntryPtr entry_, const KURL& baseURL_) {
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
  while(pos > -1 && pos < endPos && static_cast<int>(cast.count()) < m_numCast) {
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

void IMDBFetcher::doRating(const QString& str_, Data::EntryPtr entry_) {
  if(!m_fields.contains(QString::fromLatin1("imdb-rating"))) {
    return;
  }

  // don't add a colon, since there's a <br> at the end
  // match until next b tag
  QRegExp rx(QString::fromLatin1("(\\d+.?\\d*)/10"));
  rx.setMinimal(true);
  rx.setCaseSensitive(false);

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
      entry_->setField(QString::fromLatin1("imdb-rating"), QString::number((int)value));
    }
  }
}

void IMDBFetcher::doCover(const QString& str_, Data::EntryPtr entry_, const KURL& baseURL_) {
  // cover is the img with the "cover" alt text
  QRegExp imgRx(QString::fromLatin1("<img\\s+[^>]*src\\s*=\\s*\"([^\"]*)\"[^>]*>"), false);
  imgRx.setMinimal(true);

  QRegExp posterRx(QString::fromLatin1("<a\\s+[^>]*name\\s*=\\s*\"poster\"[^>]*>(.*)</a>"), false);
  posterRx.setMinimal(true);

  int pos = posterRx.search(str_);
  while(pos > -1) {
    if(imgRx.search(posterRx.cap(1)) > -1) {
      KURL u(baseURL_, imgRx.cap(1));
      const Data::Image& img = ImageFactory::addImage(u, true);
      if(!img.isNull()) {
        entry_->setField(QString::fromLatin1("cover"), img.id());
      }
      return;
    }
    pos = posterRx.search(str_, pos+1);
  }

  // didn't find the cover, IMDb also used to put "cover" inside the url
  const QString cover = QString::fromLatin1("cover");
  pos = imgRx.search(str_);
  while(pos > -1) {
    if(imgRx.cap(0).find(cover, 0, false) > -1) {
      KURL u(baseURL_, imgRx.cap(1));
      const Data::Image& img = ImageFactory::addImage(u, true);
      if(!img.isNull()) {
        entry_->setField(QString::fromLatin1("cover"), img.id());
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
    const QString allc = QString::fromLatin1("allcertification");
    if(m_fields.contains(allc)) {
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
//  myDebug() << "IMDBFetcher::updateEntry()" << endl;
  // only take first 5
  m_limit = 5;
  // optimistically try searching for title and rely on Collection::sameEntry() to figure things out
  QString t = entry_->field(QString::fromLatin1("title"));
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

  int row = 0;
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
  m_numCast = new KIntSpinBox(0, 25, 1, 10, 10, optionsWidget());
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
    m_fetchImageCheck->setChecked(true);
  }
}

void IMDBFetcher::ConfigWidget::saveConfig(KConfig* config_) {
  QString host = m_hostEdit->text().stripWhiteSpace();
  if(!host.isEmpty()) {
    config_->writeEntry("Host", host);
  }
  config_->writeEntry("Max Cast", m_numCast->value());
  config_->writeEntry("Fetch Images", m_fetchImageCheck->isChecked());

  saveFieldsConfig(config_);
  slotSetModified(false);
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
