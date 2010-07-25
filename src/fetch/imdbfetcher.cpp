/***************************************************************************
    Copyright (C) 2004-2009 Robby Stephenson <robby@periapsis.org>
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

#include "imdbfetcher.h"
#include "../gui/guiproxy.h"
#include "../collections/videocollection.h"
#include "../entry.h"
#include "../field.h"
#include "../fieldformat.h"
#include "../core/filehandler.h"
#include "../images/imagefactory.h"
#include "../tellico_utils.h"
#include "../gui/listwidgetitem.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <kdialog.h>
#include <KConfigGroup>
#include <klineedit.h>
#include <knuminput.h>
#include <KVBox>
#include <KListWidget>
#include <kio/job.h>
#include <kio/jobuidelegate.h>

#include <QRegExp>
#include <QFile>
#include <QMap>
#include <QLabel>
#include <QCheckBox>
#include <QGroupBox>
#include <QGridLayout>

//#define IMDB_TEST

namespace {
  static const char* IMDB_SERVER = "akas.imdb.com";
  static const uint IMDB_MAX_RESULTS = 20;
}

using namespace Tellico;
using Tellico::Fetch::IMDBFetcher;

QRegExp* IMDBFetcher::s_tagRx = 0;
QRegExp* IMDBFetcher::s_anchorRx = 0;
QRegExp* IMDBFetcher::s_anchorTitleRx = 0;
QRegExp* IMDBFetcher::s_anchorNameRx = 0;
QRegExp* IMDBFetcher::s_titleRx = 0;

// static
void IMDBFetcher::initRegExps() {
  s_tagRx = new QRegExp(QLatin1String("<.*>"));
  s_tagRx->setMinimal(true);

  s_anchorRx = new QRegExp(QLatin1String("<a\\s+[^>]*href\\s*=\\s*\"([^\"]*)\"[^<]*>([^<]*)</a>"), Qt::CaseInsensitive);
  s_anchorRx->setMinimal(true);

  s_anchorTitleRx = new QRegExp(QLatin1String("<a\\s+[^>]*href\\s*=\\s*\"([^\"]*/title/[^\"]*)\"[^<]*>([^<]*)</a>"), Qt::CaseInsensitive);
  s_anchorTitleRx->setMinimal(true);

  s_anchorNameRx = new QRegExp(QLatin1String("<a\\s+[^>]*href\\s*=\\s*\"([^\"]*/name/[^\"]*)\"[^<]*>([^<]*)</a>"), Qt::CaseInsensitive);
  s_anchorNameRx->setMinimal(true);

  s_titleRx = new QRegExp(QLatin1String("<title>(.*)</title>"), Qt::CaseInsensitive);
  s_titleRx->setMinimal(true);
}

IMDBFetcher::IMDBFetcher(QObject* parent_) : Fetcher(parent_),
    m_job(0), m_started(false), m_fetchImages(true), m_host(QLatin1String(IMDB_SERVER)),
    m_numCast(10), m_limit(IMDB_MAX_RESULTS), m_countOffset(0) {
  if(!s_tagRx) {
    initRegExps();
  }
}

IMDBFetcher::~IMDBFetcher() {
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
  m_numCast = config_.readEntry("Max Cast", 10);
  m_fetchImages = config_.readEntry("Fetch Images", true);
}

// multiple values not supported
void IMDBFetcher::search() {
  m_started = true;
  m_redirected = false;

  m_matches.clear();
  m_popularTitles.clear();
  m_exactTitles.clear();
  m_partialTitles.clear();
  m_currentTitleBlock = Unknown;
  m_countOffset = 0;

#ifdef IMDB_TEST
  if(request().key == Title) {
    m_url = KUrl(QLatin1String("/home/robby/imdb-title.html"));
    m_redirected = false;
  } else {
    m_url = KUrl(QLatin1String("/home/robby/imdb-name.html"));
    m_redirected = true;
  }
#else
  m_url = KUrl();
  m_url.setProtocol(QLatin1String("http"));
  m_url.setHost(m_host.isEmpty() ? QLatin1String(IMDB_SERVER) : m_host);
  m_url.setPath(QLatin1String("/find"));

  switch(request().key) {
    case Title:
      m_url.addQueryItem(QLatin1String("s"), QLatin1String("tt"));
      break;

    case Person:
      m_url.addQueryItem(QLatin1String("s"), QLatin1String("nm"));
      break;

    default:
      myWarning() << "FetchKey not supported";
      stop();
      return;
  }

  // as far as I can tell, the url encoding should always be iso-8859-1?
  m_url.addQueryItem(QLatin1String("q"), request().value);

//  myDebug() << "url =" << m_url;
#endif

  m_job = KIO::storedGet(m_url, KIO::NoReload, KIO::HideProgressInfo);
  m_job->ui()->setWindow(GUI::Proxy::widget());
  connect(m_job, SIGNAL(result(KJob*)),
          SLOT(slotComplete(KJob*)));
  connect(m_job, SIGNAL(redirection(KIO::Job*, const KUrl&)),
          SLOT(slotRedirection(KIO::Job*, const KUrl&)));
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
    m_currentTitleBlock = m_countOffset == 0 ? Approx : Partial;
  }

  if(m_currentTitleBlock == Approx) {
    parseTitleBlock(m_approxTitles);
    m_currentTitleBlock = m_countOffset == 0 ? Unknown : Approx;
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
//  myLog();
  if(m_job) {
    m_job->kill();
    m_job = 0;
  }

  m_started = false;
  m_redirected = false;

  emit signalDone(this);
}

void IMDBFetcher::slotRedirection(KIO::Job*, const KUrl& toURL_) {
  m_url = toURL_;
  m_redirected = true;
}

void IMDBFetcher::slotComplete(KJob*) {
  if(m_job->error()) {
    m_job->ui()->showErrorMessage();
    stop();
    return;
  }

  m_text = Tellico::fromHtmlData(m_job->data());
  if(m_text.isEmpty()) {
    myDebug() << "No data returned";
    stop();
    return;
  }

#if 0
  myWarning() << "Remove debug from imdbfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/testimdbresults.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << m_text;
  }
  f.close();
#endif

  // since the fetch is done, don't worry about holding the job pointer
  m_job = 0;

  // a single result was found if we got redirected
  if(request().key == Title) {
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
//  DEBUG_LINE
  s_titleRx->indexIn(Tellico::decodeHTML(m_text));
  // split title at parenthesis
  const QString cap1 = s_titleRx->cap(1);
  int pPos = cap1.indexOf(QLatin1Char('('));
  // FIXME: maybe remove parentheses here?
  FetchResult* r = new FetchResult(Fetcher::Ptr(this),
                                   pPos == -1 ? cap1 : cap1.left(pPos),
                                   pPos == -1 ? QString() : cap1.mid(pPos));
  m_matches.insert(r->uid, m_url);
  emit signalResultFound(r);

  m_hasMoreResults = false;
  stop();
}

void IMDBFetcher::parseMultipleTitleResults() {
//  DEBUG_LINE;
  QString output = Tellico::decodeHTML(m_text);

  // IMDb can return three title lists, popular, exact, and partial
  // the popular titles are in the first table
  int pos_popular = output.indexOf(QLatin1String("Popular Titles"),  0,                    Qt::CaseInsensitive);
  int pos_exact   = output.indexOf(QLatin1String("Exact Matches"),   qMax(pos_popular, 0), Qt::CaseInsensitive);
  int pos_partial = output.indexOf(QLatin1String("Partial Matches"), qMax(pos_exact, 0),   Qt::CaseInsensitive);
  int pos_approx  = output.indexOf(QLatin1String("Approx Matches"),  qMax(pos_partial, 0), Qt::CaseInsensitive);

  int end_popular = pos_exact; // keep track of where to end
  if(end_popular == -1) {
    end_popular = pos_partial == -1 ? (pos_approx == -1 ? output.length() : pos_approx) : pos_partial;
  }
  int end_exact = pos_partial; // keep track of where to end
  if(end_exact == -1) {
    end_exact = pos_approx == -1 ? output.length() : pos_approx;
  }
  int end_partial = pos_approx; // keep track of where to end
  if(end_partial == -1) {
    end_partial = output.length();
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
    m_partialTitles = output.mid(pos_partial-end_partial);
  }
  if(pos_approx > -1) {
    m_approxTitles = output.mid(pos_approx);
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
    m_currentTitleBlock = m_countOffset == 0 ? Approx : Partial;
  }

  if(m_matches.size() < m_limit) {
    parseTitleBlock(m_approxTitles);
    m_currentTitleBlock = m_countOffset == 0 ? Unknown : Approx;
  }

  if(m_matches.size() == 0) {
    myDebug() << "no matches found.";
  }

  stop();
}

void IMDBFetcher::parseTitleBlock(const QString& str_) {
  if(str_.isEmpty()) {
    m_countOffset = 0;
    return;
  }

  QRegExp akaRx(QLatin1String("aka (.*)(</li>|</td>|<br)"), Qt::CaseInsensitive);
  akaRx.setMinimal(true);

  m_hasMoreResults = false;

  int count = 0;
  int start = s_anchorTitleRx->indexIn(str_);
  while(m_started && start > -1) {
    // split title at parenthesis
    const QString cap1 = s_anchorTitleRx->cap(1); // the anchor url
    const QString cap2 = s_anchorTitleRx->cap(2).trimmed(); // the anchor text
    start += s_anchorTitleRx->matchedLength();
    int pPos = cap2.indexOf(QLatin1Char('(')); // if it has parentheses, use that for description
    QString desc;
    if(pPos > -1) {
      int pPos2 = cap2.indexOf(QLatin1Char(')'), pPos+1);
      if(pPos2 > -1) {
        desc = cap2.mid(pPos+1, pPos2-pPos-1);
      }
    } else {
      // parenthesis might be outside anchor tag
      int end = s_anchorTitleRx->indexIn(str_, start);
      if(end == -1) {
        end = str_.length();
      }
      QString text = str_.mid(start, end-start);
      pPos = text.indexOf(QLatin1Char('('));
      if(pPos > -1) {
        int pNewLine = text.indexOf(QLatin1String("<br"));
        if(pNewLine == -1 || pPos < pNewLine) {
          int pPos2 = text.indexOf(QLatin1Char(')'), pPos);
          desc = text.mid(pPos+1, pPos2-pPos-1);
        }
        pPos = -1;
      }
    }
    // multiple matches might have 'aka' info
    int end = s_anchorTitleRx->indexIn(str_, start+1);
    if(end == -1) {
      end = str_.length();
    }
    int akaPos = akaRx.indexIn(str_, start+1);
    if(akaPos > -1 && akaPos < end) {
      // limit to 50 chars
      desc += QLatin1Char(' ') + akaRx.cap(1).trimmed().remove(*s_tagRx);
      if(desc.length() > 50) {
        desc = desc.left(50) + QLatin1String("...");
      }
    }

    start = s_anchorTitleRx->indexIn(str_, start);

    if(count < m_countOffset) {
      ++count;
      continue;
    }

    // if we got this far, then there is a valid result
    if(m_matches.size() >= m_limit) {
      m_hasMoreResults = true;
      break;
    }

    FetchResult* r = new FetchResult(Fetcher::Ptr(this), pPos == -1 ? cap2 : cap2.left(pPos), desc);
    KUrl u(m_url, cap1);
    u.setQuery(QString());
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
//  DEBUG_LINE

  m_currentTitleBlock = SinglePerson;

  QString output = Tellico::decodeHTML(m_text);

  int pos = s_anchorTitleRx->indexIn(output);
  if(pos == -1) {
    stop();
    return;
  }

  QRegExp tvRegExp(QLatin1String("TV\\sEpisode"), Qt::CaseInsensitive);

  int len = 0;
  int count = 0;
  QString desc;
  for( ; m_started && pos > -1; pos = s_anchorTitleRx->indexIn(output, pos+len)) {
    desc.clear();
    bool isEpisode = false;
    len = s_anchorTitleRx->cap(0).length();
    // split title at parenthesis
    const QString cap2 = s_anchorTitleRx->cap(2).trimmed();
    int pPos = cap2.indexOf(QLatin1Char('('));
    if(pPos > -1) {
      desc = cap2.mid(pPos);
    } else {
      // look until the next <a
      int aPos = output.indexOf(QLatin1String("<a"), pos+len, Qt::CaseInsensitive);
      if(aPos == -1) {
        aPos = output.length();
      }
      QString tmp = output.mid(pos+len, aPos-pos-len);
      if(tvRegExp.indexIn(tmp) > -1) {
        isEpisode = true;
      }
      pPos = tmp.indexOf(QLatin1Char('('));
      if(pPos > -1) {
        int pNewLine = tmp.indexOf(QLatin1String("<br"));
        if(pNewLine == -1 || pPos < pNewLine) {
          int pEnd = tmp.indexOf(QLatin1Char(')'), pPos+1);
          desc = tmp.mid(pPos+1, pEnd-pPos-1).remove(*s_tagRx);
        }
        // but need to indicate it wasn't found initially
        pPos = -1;
      }
    }

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
    FetchResult* r = new FetchResult(Fetcher::Ptr(this), pPos == -1 ? cap2 : cap2.left(pPos), desc, QString());
    KUrl u(m_url, s_anchorTitleRx->cap(1)); // relative URL constructor
    u.setQuery(QString());
    m_matches.insert(r->uid, u);
//    myDebug() << u;
//    myDebug() << cap2;
    emit signalResultFound(r);
  }
  if(pos == -1) {
    m_hasMoreResults = false;
  }
  m_countOffset = count - 1;

  stop();
}

void IMDBFetcher::parseMultipleNameResults() {
//  DEBUG_LINE

  // the exact results are in the first table after the "exact results" text
  QString output = Tellico::decodeHTML(m_text);
  int pos = output.indexOf(QLatin1String("Popular Results"), 0, Qt::CaseInsensitive);
  if(pos == -1) {
    pos = output.indexOf(QLatin1String("Exact Matches"), 0, Qt::CaseInsensitive);
  }

  // find beginning of partial matches
  int end = output.indexOf(QLatin1String("Other Results"), qMax(pos, 0), Qt::CaseInsensitive);
  if(end == -1) {
    end = output.indexOf(QLatin1String("Partial Matches"), qMax(pos, 0), Qt::CaseInsensitive);
    if(end == -1) {
      end = output.indexOf(QLatin1String("Approx Matches"), qMax(pos, 0), Qt::CaseInsensitive);
      if(end == -1) {
        end = output.length();
      }
    }
  }

  QHash<QString, KUrl> map;
  QHash<QString, int> nameMap;

  QString s;
  // if found exact matches
  if(pos > -1) {
    pos = s_anchorNameRx->indexIn(output, pos+13);
    while(pos > -1 && pos < end && m_matches.size() < m_limit) {
      KUrl u(m_url, s_anchorNameRx->cap(1));
      s = s_anchorNameRx->cap(2).trimmed() + QLatin1Char(' ');
      // if more than one exact, add parentheses
      if(nameMap.contains(s) && nameMap[s] > 0) {
        // fix the first one that didn't have a number
        if(nameMap[s] == 1) {
          KUrl u2 = map[s];
          map.remove(s);
          map.insert(s + QLatin1String("(1) "), u2);
        }
        nameMap.insert(s, nameMap[s] + 1);
        // check for duplicate names
        s += QString::fromLatin1("(%1) ").arg(nameMap[s]);
      } else {
        nameMap.insert(s, 1);
      }
      map.insert(s, u);
      pos = s_anchorNameRx->indexIn(output, pos+s_anchorNameRx->cap(0).length());
    }
  }

  // go ahead and search for partial matches
  pos = s_anchorNameRx->indexIn(output, end);
  while(pos > -1 && m_matches.size() < m_limit) {
    KUrl u(m_url, s_anchorNameRx->cap(1)); // relative URL
    s = s_anchorNameRx->cap(2).trimmed();
    if(nameMap.contains(s) && nameMap[s] > 0) {
    // fix the first one that didn't have a number
      if(nameMap[s] == 1) {
        KUrl u2 = map[s];
        map.remove(s);
        map.insert(s + QLatin1String(" (1)"), u2);
      }
      nameMap.insert(s, nameMap[s] + 1);
      // check for duplicate names
      s += QString::fromLatin1(" (%1)").arg(nameMap[s]);
    } else {
      nameMap.insert(s, 1);
    }
    map.insert(s, u);
    pos = s_anchorNameRx->indexIn(output, pos+s_anchorNameRx->cap(0).length());
  }

  if(map.count() == 0) {
    stop();
    return;
  }

  KDialog dlg(GUI::Proxy::widget());
  dlg.setCaption(i18n("Select IMDb Result"));
  dlg.setModal(false);
  dlg.setButtons(KDialog::Ok|KDialog::Cancel);

  KVBox* box = new KVBox(&dlg);
  box->setSpacing(10);
  (void) new QLabel(i18n("<qt>Your search returned multiple matches. Please select one below.</qt>"), box);

  KListWidget* listWidget = new KListWidget(box);
  listWidget->setMinimumWidth(400);
  listWidget->setWrapping(true);
  const QStringList values = map.keys();
  foreach(const QString& value, values) {
    if(value.endsWith(QLatin1Char(' '))) {
      GUI::ListWidgetItem* box = new GUI::ListWidgetItem(value, listWidget);
      box->setColored(true);
      listWidget->insertItem(0, box);
    } else {
      GUI::ListWidgetItem* box = new GUI::ListWidgetItem(value, listWidget);
      listWidget->addItem(box);
    }
  }
  listWidget->item(0)->setSelected(true);
  listWidget->setWhatsThis(i18n("<qt>Select a search result.</qt>"));

  dlg.setMainWidget(box);
  if(dlg.exec() != QDialog::Accepted) {
    stop();
    return;
  }

  QListWidgetItem* cItem = listWidget->currentItem();
  QString cText;
  if(cItem) {
    cText = cItem->text();
  }
  if(cText.isEmpty()) {
    stop();
    return;
  }

  m_url = map[cText];

  // redirected is true since that's how I tell if an exact match has been found
  m_redirected = true;
  m_text.clear();
  m_job = KIO::storedGet(m_url, KIO::NoReload, KIO::HideProgressInfo);
  m_job->ui()->setWindow(GUI::Proxy::widget());
  connect(m_job, SIGNAL(result(KJob*)),
          SLOT(slotComplete(KJob*)));
  connect(m_job, SIGNAL(redirection(KIO::Job *, const KUrl&)),
          SLOT(slotRedirection(KIO::Job*, const KUrl&)));

  // do not stop() here
}

Tellico::Data::EntryPtr IMDBFetcher::fetchEntryHook(uint uid_) {
  // if we already grabbed this one, then just pull it out of the dict
  Data::EntryPtr entry = m_entries[uid_];
  if(entry) {
    return entry;
  }

  KUrl url = m_matches[uid_];
  if(url.isEmpty()) {
    myDebug() << "no url found";
    return Data::EntryPtr();
  }

  KUrl origURL = m_url; // keep to switch back
  QString results;
  // if the url matches the current one, no need to redownload it
  if(url == m_url) {
//    myDebug() << "matches previous URL, no downloading needed.";
    results = Tellico::decodeHTML(m_text);
  } else {
    // now it's sychronous
#ifdef IMDB_TEST
    KUrl u(QLatin1String("/home/robby/imdb-title-result.html"));
    results = Tellico::decodeHTML(FileHandler::readTextFile(u));
#else
    // be quiet about failure
    results = Tellico::decodeHTML(FileHandler::readTextFile(url, true));
    m_url = url; // needed for processing
#if 0
  myWarning() << "Remove debug from imdbfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/testimdbresult.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << results;
  }
  f.close();
#endif

#endif
  }
  if(results.isEmpty()) {
    myDebug() << "no text results";
    m_url = origURL;
    return Data::EntryPtr();
  }

  entry = parseEntry(results);
  m_url = origURL;
  if(!entry) {
    myDebug() << "error in processing entry";
    return Data::EntryPtr();
  }
  m_entries.insert(uid_, entry); // keep for later
  return entry;
}

Tellico::Data::EntryPtr IMDBFetcher::parseEntry(const QString& str_) {
  Data::CollPtr coll(new Data::VideoCollection(true));
  Data::EntryPtr entry(new Data::Entry(coll));

  doTitle(str_, entry);
  doRunningTime(str_, entry);
  doAspectRatio(str_, entry);
  doAlsoKnownAs(str_, entry);
  doPlot(str_, entry, m_url);
  doLists(str_, entry);
  doPerson(str_, entry, QLatin1String("Director"), QLatin1String("director"));
  doPerson(str_, entry, QLatin1String("Writer"), QLatin1String("writer"));
  doRating(str_, entry);
  doCast(str_, entry, m_url);
  if(m_fetchImages) {
    // needs base URL
    doCover(str_, entry, m_url);
  }

  const QString imdb = QLatin1String("imdb");
  if(!coll->hasField(imdb) && optionalFields().contains(imdb)) {
    Data::FieldPtr field(new Data::Field(imdb, i18n("IMDb Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }
  if(coll->hasField(imdb) && coll->fieldByName(imdb)->type() == Data::Field::URL) {
    m_url.setQuery(QString());
    entry->setField(imdb, m_url.url());
  }
  return entry;
}

void IMDBFetcher::doTitle(const QString& str_, Tellico::Data::EntryPtr entry_) {
  if(s_titleRx->indexIn(str_) > -1) {
    const QString cap1 = s_titleRx->cap(1);
    // titles always have parentheses
    int pPos = cap1.indexOf(QLatin1Char('('));
    QString title = cap1.left(pPos).trimmed();
    // remove first and last quotes is there
    if(title.startsWith(QLatin1Char('"')) && title.endsWith(QLatin1Char('"'))) {
      title = title.mid(1, title.length()-2);
    }
    entry_->setField(QLatin1String("title"), title);
    // remove parenthesis
    int pPos2 = pPos+1;
    while(pPos2 < cap1.length() && cap1[pPos2].isDigit()) {
      ++pPos2;
    }
    QString year = cap1.mid(pPos+1, pPos2-pPos-1);
    if(!year.isEmpty()) {
      entry_->setField(QLatin1String("year"), year);
    }
  }
}

void IMDBFetcher::doRunningTime(const QString& str_, Tellico::Data::EntryPtr entry_) {
  // running time
  QRegExp runtimeRx(QLatin1String("runtime:.*(\\d+)\\s+min"), Qt::CaseInsensitive);
  runtimeRx.setMinimal(true);

  if(runtimeRx.indexIn(str_) > -1) {
    entry_->setField(QLatin1String("running-time"), runtimeRx.cap(1));
  }
}

void IMDBFetcher::doAspectRatio(const QString& str_, Tellico::Data::EntryPtr entry_) {
  QRegExp rx(QLatin1String("aspect ratio:.*([\\d\\.]+\\s*:\\s*[\\d\\.]+)"), Qt::CaseInsensitive);
  rx.setMinimal(true);

  if(rx.indexIn(str_) > -1) {
    entry_->setField(QLatin1String("aspect-ratio"), rx.cap(1).trimmed());
  }
}

void IMDBFetcher::doAlsoKnownAs(const QString& str_, Tellico::Data::EntryPtr entry_) {
  if(!optionalFields().contains(QLatin1String("alttitle"))) {
    return;
  }

  // match until next b tag
//  QRegExp akaRx(QLatin1String("also known as(.*)<b(?:\\s.*)?>"));
  QRegExp akaRx(QLatin1String("also known as(.*)<(b[>\\s/]|div)"), Qt::CaseInsensitive);
  akaRx.setMinimal(true);

  if(akaRx.indexIn(str_) > -1 && !akaRx.cap(1).isEmpty()) {
    Data::FieldPtr f = entry_->collection()->fieldByName(QLatin1String("alttitle"));
    if(!f) {
      f = new Data::Field(QLatin1String("alttitle"), i18n("Alternative Titles"), Data::Field::Table);
      f->setFormatType(FieldFormat::FormatTitle);
      entry_->collection()->addField(f);
    }

    // split by <br>, remembering it could become valid xhtml!
    QRegExp brRx(QLatin1String("<br[\\s/]*>"), Qt::CaseInsensitive);
    brRx.setMinimal(true);
    QStringList list = akaRx.cap(1).split(brRx);
    // lang could be included with [fr]
//    const QRegExp parRx(QLatin1String("\\(.+\\)"));
    const QRegExp brackRx(QLatin1String("\\[\\w+\\]"));
    QStringList values;
    for(QStringList::Iterator it = list.begin(); it != list.end(); ++it) {
      QString s = *it;
      // sometimes, the word "more" gets linked to the releaseinfo page, check that
      if(s.indexOf(QLatin1String("releaseinfo")) > -1) {
        continue;
      }
      s.remove(*s_tagRx);
      s.remove(brackRx);
      s = s.trimmed();
      // the first value ends up being or starting with the colon after "Also know as"
      // I'm too lazy to figure out a better regexp
      if(s.startsWith(QLatin1Char(':'))) {
        s = s.mid(1);
      }
      if(!s.isEmpty()) {
        values += s;
      }
    }
    if(!values.isEmpty()) {
      entry_->setField(QLatin1String("alttitle"), values.join(FieldFormat::rowDelimiterString()));
    }
  }
}

void IMDBFetcher::doPlot(const QString& str_, Tellico::Data::EntryPtr entry_, const KUrl& baseURL_) {
  // plot summaries provided by users are on a separate page
  // should those be preferred?

  bool useUserSummary = false;

  QString thisPlot;
  // match until next opening tag
  QRegExp plotRx(QLatin1String("plot\\s*(?:outline|summary)?:(.*)<[^/].*</"), Qt::CaseInsensitive);
  plotRx.setMinimal(true);
  QRegExp plotURLRx(QLatin1String("<a\\s+.*href\\s*=\\s*\".*/title/.*/plotsummary\""), Qt::CaseInsensitive);
  plotURLRx.setMinimal(true);
  if(plotRx.indexIn(str_) > -1) {
    thisPlot = plotRx.cap(1);
    thisPlot.remove(*s_tagRx); // remove HTML tags
    entry_->setField(QLatin1String("plot"), thisPlot);
    // if thisPlot ends with (more) or contains
    // a url that ends with plotsummary, then we'll grab it, otherwise not
    if(plotRx.cap(0).endsWith(QLatin1String("(more)</")) || plotURLRx.indexIn(plotRx.cap(0)) > -1) {
      useUserSummary = true;
    }
  }

  if(useUserSummary) {
    QRegExp idRx(QLatin1String("title/(tt\\d+)"));
    idRx.indexIn(baseURL_.path());
    KUrl plotURL = baseURL_;
    plotURL.setPath(QLatin1String("/title/") + idRx.cap(1) + QLatin1String("/plotsummary"));
    // be quiet about failure
    QString plotPage = FileHandler::readTextFile(plotURL, true);

    if(!plotPage.isEmpty()) {
      QRegExp plotRx(QLatin1String("<p\\s+class\\s*=\\s*\"plotpar\">(.*)</p"));
      plotRx.setMinimal(true);
      if(plotRx.indexIn(plotPage) > -1) {
        QString userPlot = plotRx.cap(1);
        userPlot.remove(*s_tagRx); // remove HTML tags
        // remove last little "written by", if there
        userPlot.remove(QRegExp(QLatin1String("\\s*written by.*$"), Qt::CaseInsensitive));
        entry_->setField(QLatin1String("plot"), Tellico::decodeHTML(userPlot));
      }
    }
  }
}

void IMDBFetcher::doPerson(const QString& str_, Tellico::Data::EntryPtr entry_,
                           const QString& imdbHeader_, const QString& fieldName_) {
  QRegExp br2Rx(QLatin1String("<br[\\s/]*>\\s*<br[\\s/]*>"), Qt::CaseInsensitive);
  br2Rx.setMinimal(true);
  QRegExp divRx(QLatin1String("</div"), Qt::CaseInsensitive);
  divRx.setMinimal(true);
  QString name = QLatin1String("/name/");

  StringSet people;
  for(int pos = str_.indexOf(imdbHeader_); pos > 0; pos = str_.indexOf(imdbHeader_, pos+1)) {
    // loop until repeated <br> tags or </div> tag
    const int endPos1 = br2Rx.indexIn(str_, pos);
    const int endPos2 = divRx.indexIn(str_, pos);
    const int endPos = endPos1 == -1 ? endPos2
                                     : (endPos2 == -1 ? -1
                                                      : qMin(endPos1, endPos2)
                                       ); // ok to be -1
    pos = s_anchorRx->indexIn(str_, pos+1);
    while(pos > -1 && pos < endPos) {
      if(s_anchorRx->cap(1).indexOf(name) > -1) {
        people.add(s_anchorRx->cap(2).trimmed());
      }
      pos = s_anchorRx->indexIn(str_, pos+1);
    }
  }
  if(!people.isEmpty()) {
    entry_->setField(fieldName_, people.toList().join(FieldFormat::delimiterString()));
  }
}

void IMDBFetcher::doCast(const QString& str_, Tellico::Data::EntryPtr entry_, const KUrl& baseURL_) {
  // the extended cast list is on a separate page
  // that's usually a lot of people
  // but since it can be in billing order, the main actors might not
  // be in the short list
  QRegExp idRx(QLatin1String("title/(tt\\d+)"));
  idRx.indexIn(baseURL_.path());
#ifdef IMDB_TEST
  KUrl castURL(QLatin1String("/home/robby/imdb-title-fullcredits.html"));
#else
  KUrl castURL = baseURL_;
  castURL.setPath(QLatin1String("/title/") + idRx.cap(1) + QLatin1String("/fullcredits"));
#endif
  // be quiet about failure and be sure to translate entities
  QString castPage = Tellico::decodeHTML(FileHandler::readTextFile(castURL, true));
#if 0
  myWarning() << "Remove debug from imdbfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/testimdbcast.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << castPage;
  }
  f.close();
#endif


  int pos = -1;
  // the text to search, depends on which page is being read
  QString castText = castPage;
  if(castText.isEmpty()) {
    // fall back to short list
    castText = str_;
    pos = castText.indexOf(QLatin1String("cast overview"), 0, Qt::CaseInsensitive);
    if(pos == -1) {
      pos = castText.indexOf(QLatin1String("credited cast"), 0, Qt::CaseInsensitive);
    }
  } else {
    // first look for anchor
    QRegExp castAnchorRx(QLatin1String("<a\\s+name\\s*=\\s*\"cast\""), Qt::CaseInsensitive);
    pos = castAnchorRx.indexIn(castText);
    if(pos < 0) {
      QRegExp tableClassRx(QLatin1String("<table\\s+class\\s*=\\s*\"cast\""), Qt::CaseInsensitive);
      pos = tableClassRx.indexIn(castText);
      if(pos < 0) {
        // fragile, the word "cast" appears in the title, but need to find
        // the one right above the actual cast table
        // for TV shows, there's a link on the sidebar for "episodes case"
        // so need to not match that one
        pos = castText.indexOf(QLatin1String("cast</"), 0, Qt::CaseInsensitive);
        if(pos > 9) {
          // back up 9 places
          if(castText.mid(pos-9, 9).startsWith(QLatin1String("episodes"))) {
            // find next cast list
            pos = castText.indexOf(QLatin1String("cast</"), pos+6, Qt::CaseInsensitive);
          }
        }
      }
    }
  }
  if(pos == -1) { // no cast list found
    myDebug() << "no cast list found";
    return;
  }

  const QString name = QLatin1String("/name/");
  QRegExp tdRx(QLatin1String("<td[^>]*>(.*)</td>"), Qt::CaseInsensitive);
  tdRx.setMinimal(true);

  QStringList cast;
  // loop until closing table tag
  const int endPos = castText.indexOf(QLatin1String("</table"), pos, Qt::CaseInsensitive);
  pos = s_anchorRx->indexIn(castText, pos+1);
  while(pos > -1 && pos < endPos && cast.count() < m_numCast) {
    if(s_anchorRx->cap(1).indexOf(name) > -1) {
      // now search for <td> item with character name
      // there's a column with ellipses then the character
      const int pos2 = tdRx.indexIn(castText, pos);
      if(pos2 > -1 && tdRx.indexIn(castText, pos2+1) > -1) {
        cast += s_anchorRx->cap(2).trimmed()
              + FieldFormat::columnDelimiterString()
              + tdRx.cap(1).simplified().remove(*s_tagRx);
      } else {
        cast += s_anchorRx->cap(2).trimmed();
      }
    }
    pos = s_anchorRx->indexIn(castText, pos+1);
  }

  if(!cast.isEmpty()) {
    entry_->setField(QLatin1String("cast"), cast.join(FieldFormat::rowDelimiterString()));
  }
}

void IMDBFetcher::doRating(const QString& str_, Tellico::Data::EntryPtr entry_) {
  if(!optionalFields().contains(QLatin1String("imdb-rating"))) {
    return;
  }

  // don't add a colon, since there's a <br> at the end
  // some of the imdb images use /10.gif in their path, so check for space or bracket
  QRegExp rx(QLatin1String("[>\\s](\\d+.?\\d*)/10[<//s]"), Qt::CaseInsensitive);
  rx.setMinimal(true);

  if(rx.indexIn(str_) > -1 && !rx.cap(1).isEmpty()) {
    Data::FieldPtr f = entry_->collection()->fieldByName(QLatin1String("imdb-rating"));
    if(!f) {
      f = new Data::Field(QLatin1String("imdb-rating"), i18n("IMDb Rating"), Data::Field::Rating);
      f->setCategory(i18n("General"));
      f->setProperty(QLatin1String("maximum"), QLatin1String("10"));
      entry_->collection()->addField(f);
    }

    bool ok;
    float value = rx.cap(1).toFloat(&ok);
    if(ok) {
      entry_->setField(QLatin1String("imdb-rating"), QString::number(value));
    }
  }
}

void IMDBFetcher::doCover(const QString& str_, Tellico::Data::EntryPtr entry_, const KUrl& baseURL_) {
  // cover is the img with the "cover" alt text
  QRegExp imgRx(QLatin1String("<img\\s+[^>]*src\\s*=\\s*\"([^\"]*)\"[^>]*>"), Qt::CaseInsensitive);
  imgRx.setMinimal(true);

  QRegExp posterRx(QLatin1String("<a\\s+[^>]*name\\s*=\\s*\"poster\"[^>]*>(.*)</a>"), Qt::CaseInsensitive);
  posterRx.setMinimal(true);

  const QString cover = QLatin1String("cover");

  int pos = posterRx.indexIn(str_);
  while(pos > -1) {
    if(imgRx.indexIn(posterRx.cap(1)) > -1) {
      KUrl u(baseURL_, imgRx.cap(1));
      QString id = ImageFactory::addImage(u, true);
      if(!id.isEmpty()) {
        entry_->setField(cover, id);
      }
      return;
    }
    pos = posterRx.indexIn(str_, pos+1);
  }

  // didn't find the cover, IMDb also used to put "cover" inside the url
  pos = imgRx.indexIn(str_);
  while(pos > -1) {
    if(imgRx.cap(0).indexOf(cover, 0, Qt::CaseInsensitive) > -1) {
      KUrl u(baseURL_, imgRx.cap(1));
      QString id = ImageFactory::addImage(u, true);
      if(!id.isEmpty()) {
        entry_->setField(cover, id);
      }
      return;
    }
    pos = imgRx.indexIn(str_, pos+1);
  }
}

// end up reparsing whole string, but it's not really that slow
// look at every anchor tag in the string
void IMDBFetcher::doLists(const QString& str_, Tellico::Data::EntryPtr entry_) {
  const QString genre = QLatin1String("/Genres/");
  const QString country = QLatin1String("/Countries/");
  const QString lang = QLatin1String("/Languages/");
  const QString colorInfo = QLatin1String("colors=");
  const QString cert = QLatin1String("certificates=");
  const QString soundMix = QLatin1String("sound_mixes=");
  const QString year = QLatin1String("/Years/");
  const QString company = QLatin1String("/company/");

  // IIMdb also has links with the word "sections" in them, remove that
  // for genres and nationalities

  QStringList genres, countries, langs, certs, tracks, studios;
  for(int pos = s_anchorRx->indexIn(str_); pos > -1; pos = s_anchorRx->indexIn(str_, pos+1)) {
    const QString cap1 = s_anchorRx->cap(1);
    if(cap1.indexOf(genre) > -1) {
      if(s_anchorRx->cap(2).indexOf(QLatin1String(" section"), 0, Qt::CaseInsensitive) == -1) {
        genres += s_anchorRx->cap(2).trimmed();
      }
    } else if(cap1.indexOf(country) > -1) {
      if(s_anchorRx->cap(2).indexOf(QLatin1String(" section"), 0, Qt::CaseInsensitive) == -1) {
        countries += s_anchorRx->cap(2).trimmed();
      }
    } else if(cap1.indexOf(lang) > -1) {
      langs += s_anchorRx->cap(2).trimmed();
    } else if(cap1.indexOf(colorInfo) > -1) {
      // change "black and white" to "black & white"
      entry_->setField(QLatin1String("color"),
                       s_anchorRx->cap(2).replace(QLatin1String("and"), QLatin1String("&")).trimmed());
    } else if(cap1.indexOf(cert) > -1) {
      certs += s_anchorRx->cap(2).trimmed();
    } else if(cap1.indexOf(soundMix) > -1) {
      tracks += s_anchorRx->cap(2).trimmed();
    } else if(cap1.indexOf(company) > -1) {
      studios += s_anchorRx->cap(2).trimmed();
      // if year field wasn't set before, do it now
    } else if(entry_->field(QLatin1String("year")).isEmpty() && cap1.indexOf(year) > -1) {
      entry_->setField(QLatin1String("year"), s_anchorRx->cap(2).trimmed());
    }
  }

  entry_->setField(QLatin1String("genre"), genres.join(FieldFormat::delimiterString()));
  entry_->setField(QLatin1String("nationality"), countries.join(FieldFormat::delimiterString()));
  entry_->setField(QLatin1String("language"), langs.join(FieldFormat::delimiterString()));
  entry_->setField(QLatin1String("audio-track"), tracks.join(FieldFormat::delimiterString()));
  entry_->setField(QLatin1String("studio"), studios.join(FieldFormat::delimiterString()));
  if(!certs.isEmpty()) {
    // first try to set default certification
    const QStringList& certsAllowed = entry_->collection()->fieldByName(QLatin1String("certification"))->allowed();
    foreach(const QString& cert, certs) {
      QString country = cert.section(QLatin1Char(':'), 0, 0);
      QString lcert = cert.section(QLatin1Char(':'), 1, 1);
      if(lcert == QLatin1String("Unrated")) {
        lcert = QLatin1Char('U');
      }
      lcert += QLatin1String(" (") + country + QLatin1Char(')');
      if(certsAllowed.indexOf(cert) > -1) {
        entry_->setField(QLatin1String("certification"), lcert);
        break;
      }
    }

    // now add new field for all certifications
    const QString allc = QLatin1String("allcertification");
    if(optionalFields().contains(allc)) {
      Data::FieldPtr f = entry_->collection()->fieldByName(allc);
      if(!f) {
        f = new Data::Field(allc, i18n("Certifications"), Data::Field::Table);
        f->setFlags(Data::Field::AllowGrouped);
        entry_->collection()->addField(f);
      }
      entry_->setField(QLatin1String("allcertification"), certs.join(FieldFormat::rowDelimiterString()));
    }
  }
}

Tellico::Fetch::FetchRequest IMDBFetcher::updateRequest(Data::EntryPtr entry_) {
  const QString t = entry_->field(QLatin1String("title"));
  KUrl link = entry_->field(QLatin1String("imdb"));

  if(!link.isEmpty() && link.isValid()) {
    myWarning() << "IMDb link searching not implemented";
    return FetchRequest(Title, t);

    // TODO: fix this
    // check if we want a different host
    if(link.host() != m_host) {
//      myLog() << "switching hosts to " << m_host;
      link.setHost(m_host);
    }
//    request().key = Fetch::Title;
//    request().value = t;
    m_started = true;
    m_text.clear();
    m_matches.clear();
    m_url = link;
    m_redirected = true; // m_redirected is used as a flag later to tell if we get a single result
    m_job = KIO::storedGet(m_url, KIO::NoReload, KIO::HideProgressInfo);
    m_job->ui()->setWindow(GUI::Proxy::widget());
    connect(m_job, SIGNAL(result(KJob*)),
            SLOT(slotComplete(KJob*)));
    connect(m_job, SIGNAL(redirection(KIO::Job*, const KUrl&)),
            SLOT(slotRedirection(KIO::Job*, const KUrl&)));
  }

  // optimistically try searching for title and rely on Collection::sameEntry() to figure things out
  if(!t.isEmpty()) {
    return FetchRequest(Fetch::Title, t);
  }
  return FetchRequest();
}

QString IMDBFetcher::defaultName() {
  return i18n("Internet Movie Database");
}

QString IMDBFetcher::defaultIcon() {
  return favIcon("http://imdb.com");
}
//static
Tellico::StringHash IMDBFetcher::allOptionalFields() {
  StringHash hash;
  hash[QLatin1String("imdb")]             = i18n("IMDb Link");
  hash[QLatin1String("imdb-rating")]      = i18n("IMDb Rating");
  hash[QLatin1String("alttitle")]         = i18n("Alternative Titles");
  hash[QLatin1String("allcertification")] = i18n("Certifications");
  return hash;
}

Tellico::Fetch::ConfigWidget* IMDBFetcher::configWidget(QWidget* parent_) const {
  return new IMDBFetcher::ConfigWidget(parent_, this);
}

IMDBFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const IMDBFetcher* fetcher_/*=0*/)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;
  QLabel* label = new QLabel(i18n("Hos&t: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_hostEdit = new KLineEdit(optionsWidget());
  connect(m_hostEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_hostEdit, row, 1);
  QString w = i18n("The Internet Movie Database uses several different servers. Choose the one "
                   "you wish to use.");
  label->setWhatsThis(w);
  m_hostEdit->setWhatsThis(w);
  label->setBuddy(m_hostEdit);

  label = new QLabel(i18n("&Maximum cast: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_numCast = new KIntSpinBox(0, 99, 1, 10, optionsWidget());
  connect(m_numCast, SIGNAL(valueChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_numCast, row, 1);
  w = i18n("The list of cast members may include many people. Set the maximum number returned from the search.");
  label->setWhatsThis(w);
  m_numCast->setWhatsThis(w);
  label->setBuddy(m_numCast);

  m_fetchImageCheck = new QCheckBox(i18n("Download cover &image"), optionsWidget());
  connect(m_fetchImageCheck, SIGNAL(clicked()), SLOT(slotSetModified()));
  ++row;
  l->addWidget(m_fetchImageCheck, row, 0, 1, 2);
  w = i18n("The cover image may be downloaded as well. However, too many large images in the "
           "collection may degrade performance.");
  m_fetchImageCheck->setWhatsThis(w);

  l->setRowStretch(++row, 10);

  // now add additional fields widget
  addFieldsWidget(IMDBFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());

  if(fetcher_) {
    m_hostEdit->setText(fetcher_->m_host);
    m_numCast->setValue(fetcher_->m_numCast);
    m_fetchImageCheck->setChecked(fetcher_->m_fetchImages);
  } else { //defaults
    m_hostEdit->setText(QLatin1String(IMDB_SERVER));
    m_numCast->setValue(10);
    m_fetchImageCheck->setChecked(true);
  }
}

void IMDBFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  QString host = m_hostEdit->text().trimmed();
  if(!host.isEmpty()) {
    config_.writeEntry("Host", host);
  }
  config_.writeEntry("Max Cast", m_numCast->value());
  config_.writeEntry("Fetch Images", m_fetchImageCheck->isChecked());
}

QString IMDBFetcher::ConfigWidget::preferredName() const {
  return IMDBFetcher::defaultName();
}

#include "imdbfetcher.moc"
