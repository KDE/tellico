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
#include "../utils/guiproxy.h"
#include "../collections/videocollection.h"
#include "../entry.h"
#include "../field.h"
#include "../fieldformat.h"
#include "../core/filehandler.h"
#include "../images/imagefactory.h"
#include "../utils/string_utils.h"
#include "../gui/listwidgetitem.h"
#include "../gui/combobox.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KDialog>
#include <KConfigGroup>
#include <KLineEdit>
#include <KIntSpinBox>
#include <kio/job.h>
#include <KJobUiDelegate>
#include <KAcceleratorManager>
#include <KJobWidgets/KJobWidgets>

#include <QRegExp>
#include <QFile>
#include <QMap>
#include <QLabel>
#include <QCheckBox>
#include <QGroupBox>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QListWidget>

namespace {
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

  s_anchorRx = new QRegExp(QLatin1String("<a\\s+[^>]*href\\s*=\\s*\"([^\"]+)\"[^<]*>([^<]+)</a>"), Qt::CaseInsensitive);
  s_anchorRx->setMinimal(true);

  s_anchorTitleRx = new QRegExp(QLatin1String("<a\\s+[^>]*href\\s*=\\s*\"([^\"]*/title/[^\"]*)\"[^<]*>([^<]*)</a>"), Qt::CaseInsensitive);
  s_anchorTitleRx->setMinimal(true);

  s_anchorNameRx = new QRegExp(QLatin1String("<a\\s+[^>]*href\\s*=\\s*\"([^\"]*/name/[^\"]*)\"[^<]*>(.+)</a>"), Qt::CaseInsensitive);
  s_anchorNameRx->setMinimal(true);

  s_titleRx = new QRegExp(QLatin1String("<title>(.*)</title>"), Qt::CaseInsensitive);
  s_titleRx->setMinimal(true);
}

// static
const IMDBFetcher::LangData& IMDBFetcher::langData(int lang_) {
  Q_ASSERT(lang_ >= 0);
  Q_ASSERT(lang_ <  6);
  static LangData dataVector[6] = {
    {
      i18n("Internet Movie Database"),
      QLatin1String("akas.imdb.com"),
      QLatin1String("findSectionHeader"),
      QLatin1String("Exact Matches"),
      QLatin1String("Partial Matches"),
      QLatin1String("Approx Matches"),
      QLatin1String("findSectionHeader"),
      QLatin1String("Other Results"),
      QLatin1String("aka"),
      QLatin1String("Director"),
      QLatin1String("Writer"),
      QLatin1String("Produced by"),
      QLatin1String("runtime:.*(\\d+)\\s+min"),
      QLatin1String("aspect ratio:"),
      QLatin1String("also known as"),
      QLatin1String("Production Co"),
      QLatin1String("cast"),
      QLatin1String("cast overview"),
      QLatin1String("credited cast"),
      QLatin1String("episodes"),
      QLatin1String("Genre"),
      QLatin1String("Sound"),
      QLatin1String("Color"),
      QLatin1String("Language"),
      QLatin1String("Certification"),
      QLatin1String("Country"),
      QLatin1String("plot\\s*(?:outline|summary)?")
    }, {
      i18n("Internet Movie Database (French)"),
      QLatin1String("www.imdb.fr"),
      QLatin1String("findSectionHeader"),
      QString::fromUtf8("Résultats Exacts"),
      QString::fromUtf8("Résultats Partiels"),
      QString::fromUtf8("Résultats Approximatif"),
      QString::fromUtf8("findSectionHeader"),
      QString::fromUtf8("Résultats Autres"),
      QLatin1String("autre titre"),
      QString::fromUtf8("Réalisateur"),
      QString::fromUtf8("Scénarist"),
      QString(),
      QString::fromUtf8("Durée:.*(\\d+)\\s+min"),
      QLatin1String("Format :"),
      QLatin1String("Alias"),
      QString::fromUtf8("Sociétés de Production"),
      QLatin1String("Ensemble"),
      QLatin1String("cast overview"), // couldn't get phrase
      QLatin1String("credited cast"), // couldn't get phrase
      QLatin1String("episodes"),
      QLatin1String("Genre"),
      QLatin1String("Son"),
      QLatin1String("Couleur"),
      QLatin1String("Langue"),
      QLatin1String("Classification"),
      QLatin1String("Pays"),
      QLatin1String("Intrigue\\s*")
    }, {
      i18n("Internet Movie Database (Spanish)"),
      QLatin1String("www.imdb.es"),
      QString::fromUtf8("findSectionHeader"),
      QString::fromUtf8("Resultados Exactos"),
      QString::fromUtf8("Resultados Parciales"),
      QString::fromUtf8("Resultados Aproximados"),
      QString::fromUtf8("findSectionHeader"),
      QString::fromUtf8("Resultados Otros"),
      QString::fromUtf8("otro título"),
      QLatin1String("Director"),
      QLatin1String("Escritores"),
      QString(),
      QString::fromUtf8("Duración:.*(\\d+)\\s+min"),
      QString::fromUtf8("Relación de Aspecto:"),
      QLatin1String("Conocido como"),
      QString::fromUtf8("Compañías Productores"),
      QLatin1String("Reparto"),
      QLatin1String("cast overview"), // couldn't get phrase
      QLatin1String("credited cast"), // couldn't get phrase
      QLatin1String("episodes"),
      QString::fromUtf8("Género"),
      QLatin1String("Sonido"),
      QLatin1String("Color"),
      QLatin1String("Idioma"),
      QString::fromUtf8("Clasificación"),
      QString::fromUtf8("País"),
      QLatin1String("Trama\\s*")
    }, {
      i18n("Internet Movie Database (German)"),
      QLatin1String("www.imdb.de"),
      QString::fromUtf8("findSectionHeader"),
      QString::fromUtf8("genaue Übereinstimmung"),
      QString::fromUtf8("teilweise Übereinstimmung"),
      QString::fromUtf8("näherungsweise Übereinstimmung"),
      QString::fromUtf8("findSectionHeader"),
      QString::fromUtf8("andere Übereinstimmung"),
      QString::fromUtf8("andere titel"),
      QLatin1String("Regisseur"),
      QLatin1String("Drehbuchautoren"),
      QString(),
      QString::fromUtf8("Länge:.*(\\d+)\\s+min"),
      QString::fromUtf8("Seitenverhältnis:"),
      QLatin1String("Auch bekannt als"),
      QString::fromUtf8("Produktionsfirmen"),
      QLatin1String("Besetzung"),
      QLatin1String("cast overview"), // couldn't get phrase
      QLatin1String("credited cast"), // couldn't get phrase
      QLatin1String("episodes"),
      QString::fromUtf8("Genre"),
      QLatin1String("Tonverfahren"),
      QLatin1String("Farbe"),
      QLatin1String("Sprache"),
      QString::fromUtf8("Altersfreigabe"),
      QString::fromUtf8("Land"),
      QLatin1String("Handlung\\s*")
    }, {
      i18n("Internet Movie Database (Italian)"),
      QLatin1String("www.imdb.it"),
      QString::fromUtf8("findSectionHeader"),
      QString::fromUtf8("risultati esatti"),
      QString::fromUtf8("risultati parziali"),
      QString::fromUtf8("risultati approssimati"),
      QString::fromUtf8("findSectionHeader"),
      QString::fromUtf8("Resultados Otros"),
      QString::fromUtf8("otro título"),
      QLatin1String("Regista"),
      QLatin1String("Sceneggiatori"),
      QString(),
      QString::fromUtf8("Durata:.*(\\d+)\\s+min"),
      QString::fromUtf8("Aspect Ratio:"),
      QLatin1String("Alias"),
      QString::fromUtf8("Società di produzione"),
      QLatin1String("Cast"),
      QLatin1String("cast overview"), // couldn't get phrase
      QLatin1String("credited cast"), // couldn't get phrase
      QLatin1String("episodes"),
      QString::fromUtf8("Genere"),
      QLatin1String("Sonoro"),
      QLatin1String("Colore"),
      QLatin1String("Lingua"),
      QString::fromUtf8("Divieti"),
      QString::fromUtf8("Nazionalità"),
      QLatin1String("Trama\\s*")
    }, {
      i18n("Internet Movie Database (Portuguese)"),
      QLatin1String("www.imdb.pt"),
      QString::fromUtf8("findSectionHeader"),
      QString::fromUtf8("Exato"),
      QString::fromUtf8("Combinação Parcial"),
      QString::fromUtf8("Combinação Aproximada"),
      QString::fromUtf8("findSectionHeader"),
      QString::fromUtf8("Combinação Otros"),
      QString::fromUtf8("otro título"),
      QLatin1String("Diretor"),
      QLatin1String("Escritores"),
      QString(),
      QString::fromUtf8("Duração:.*(\\d+)\\s+min"),
      QString::fromUtf8("Resolução:"),
      QString::fromUtf8("Também Conhecido Como"),
      QString::fromUtf8("Companhias de Produção"),
      QLatin1String("Elenco"),
      QLatin1String("cast overview"), // couldn't get phrase
      QLatin1String("credited cast"), // couldn't get phrase
      QLatin1String("episodes"),
      QString::fromUtf8("Gênero"),
      QLatin1String("Mixagem de Som"),
      QLatin1String("Cor"),
      QLatin1String("Lingua"),
      QString::fromUtf8("Certificação"),
      QString::fromUtf8("País"),
      QLatin1String("Argumento\\s*")
    }
  };

  return dataVector[qBound(0, lang_, static_cast<int>(sizeof(dataVector)/sizeof(LangData)))];
}

IMDBFetcher::IMDBFetcher(QObject* parent_) : Fetcher(parent_),
    m_job(0), m_started(false), m_fetchImages(true),
    m_numCast(10), m_limit(IMDB_MAX_RESULTS), m_lang(EN), m_countOffset(0) {
  if(!s_tagRx) {
    initRegExps();
  }
  m_host = langData(m_lang).siteHost;
}

IMDBFetcher::~IMDBFetcher() {
}

QString IMDBFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool IMDBFetcher::canFetch(int type) const {
  return type == Data::Collection::Video;
}

// imdb can search title, person in english
bool IMDBFetcher::canSearch(FetchKey k) const {
  return k == Title || (m_lang == EN && k == Person);
}

void IMDBFetcher::readConfigHook(const KConfigGroup& config_) {
  /*
  const int lang = config_.readEntry("Lang", int(EN));
  m_lang = static_cast<Lang>(lang);
  */
  if(m_name.isEmpty()) {
    m_name = langData(m_lang).siteTitle;
  }
  QString h = config_.readEntry("Host");
  if(h.isEmpty()) {
    m_host = langData(m_lang).siteHost;
  } else {
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

  m_url = QUrl();
  m_url.setScheme(QLatin1String("http"));
  m_url.setHost(m_host);
  m_url.setPath(QLatin1String("/find"));

  // as far as I can tell, the url encoding should always be iso-8859-1?
  m_url.addQueryItem(QLatin1String("q"), request().value);

  switch(request().key) {
    case Title:
      m_url.addQueryItem(QLatin1String("s"), QLatin1String("tt"));
      break;

    case Person:
      m_url.addQueryItem(QLatin1String("s"), QLatin1String("nm"));
      break;

    case Raw:
      m_url = request().value;
      break;

    default:
      myWarning() << "not supported:" << request().key;
      stop();
      return;
  }

//  myDebug() << m_url;

  m_job = KIO::storedGet(m_url, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job, SIGNAL(result(KJob*)),
          SLOT(slotComplete(KJob*)));
  connect(m_job, SIGNAL(redirection(KIO::Job*, const QUrl&)),
          SLOT(slotRedirection(KIO::Job*, const QUrl&)));
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

void IMDBFetcher::slotRedirection(KIO::Job*, const QUrl& toURL_) {
  m_url = toURL_;
  if(m_url.path().contains(QRegExp(QLatin1String("/tt\\d+/$"))))  {
    m_url.setPath(m_url.path() + QLatin1String("combined"));
  }
  m_redirected = true;
}

void IMDBFetcher::slotComplete(KJob*) {
  if(m_job->error()) {
    m_job->ui()->showErrorMessage();
    stop();
    return;
  }

  m_text = Tellico::fromHtmlData(m_job->data(), "UTF-8");
  if(m_text.isEmpty()) {
    myLog() << "No data returned";
    stop();
    return;
  }
  // see bug 319662. If fetcher is cancelled, job is killed
  // if the pointer is retained, it gets double-deleted
  m_job = 0;

#if 0
  myWarning() << "Remove debug from imdbfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/testimdbresults.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << m_text;
  }
  f.close();
#endif

  // a single result was found if we got redirected
  switch(request().key) {
    case Title:
      if(m_redirected) {
        parseSingleTitleResult();
      } else {
        parseMultipleTitleResults();
      }
      break;

    case Person:
      if(m_redirected) {
        parseSingleNameResult();
      } else {
        parseMultipleNameResults();
      }
      break;

    case Raw:
      parseSingleTitleResult();
      break;

    default:
      myWarning() << "skipping results";
      break;
  }
}

void IMDBFetcher::parseSingleTitleResult() {
  s_titleRx->indexIn(Tellico::decodeHTML(m_text));
  // split title at parenthesis
  const QString cap1 = s_titleRx->cap(1);
  int pPos = cap1.indexOf(QLatin1Char('('));
  // FIXME: maybe remove parentheses here?
  FetchResult* r = new FetchResult(Fetcher::Ptr(this),
                                   pPos == -1 ? cap1 : cap1.left(pPos),
                                   pPos == -1 ? QString() : cap1.mid(pPos));
  // IMDB returns different HTML for single title results and has a query in the url
  // clear the query so we download the "canonical" page for the title
  QUrl url(m_url);
  url.setEncodedQuery(QByteArray());
  m_matches.insert(r->uid, url);
  m_allMatches.insert(r->uid, url);
  emit signalResultFound(r);

  m_hasMoreResults = false;
  stop();
}

void IMDBFetcher::parseMultipleTitleResults() {
  QString output = Tellico::decodeHTML(m_text);

  const LangData& data = langData(m_lang);
  // IMDb can return three title lists, popular, exact, and partial
  // the popular titles are in the first table
  int pos_popular = output.indexOf(data.title_popular, 0,                    Qt::CaseInsensitive);
  int pos_exact   = output.indexOf(data.match_exact,   qMax(pos_popular, 0), Qt::CaseInsensitive);
  int pos_partial = output.indexOf(data.match_partial, qMax(pos_exact,   0), Qt::CaseInsensitive);
  int pos_approx  = output.indexOf(data.match_approx,  qMax(pos_partial, 0), Qt::CaseInsensitive);

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
    m_partialTitles = output.mid(pos_partial, end_partial-pos_partial);
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
    myLog() << "no matches found.";
  }

  stop();
}

void IMDBFetcher::parseTitleBlock(const QString& str_) {
  if(str_.isEmpty()) {
    m_countOffset = 0;
    return;
  }

  QRegExp akaRx(QString::fromLatin1("%1 (.*)(</li>|</td>|<br)").arg(langData(m_lang).aka), Qt::CaseInsensitive);
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
    QUrl u = QUrl(m_url).resolved(cap1);
    u.setQuery(QString());
    m_matches.insert(r->uid, u);
    m_allMatches.insert(r->uid, u);
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
    QUrl u = QUrl(m_url).resolved(s_anchorTitleRx->cap(1)); // relative URL
    u.setQuery(QString());
    m_matches.insert(r->uid, u);
    m_allMatches.insert(r->uid, u);
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

  const LangData& data = langData(m_lang);
  // the exact results are in the first table after the "exact results" text
  QString output = Tellico::decodeHTML(m_text);
  int pos = output.indexOf(data.result_popular, 0, Qt::CaseInsensitive);
  if(pos == -1) {
    pos = output.indexOf(data.match_exact, 0, Qt::CaseInsensitive);
  }

  // find beginning of partial matches
  int end = output.indexOf(data.result_other, qMax(pos, 0), Qt::CaseInsensitive);
  if(end == -1) {
    end = output.indexOf(data.match_partial, qMax(pos, 0), Qt::CaseInsensitive);
    if(end == -1) {
      end = output.indexOf(data.match_approx, qMax(pos, 0), Qt::CaseInsensitive);
      if(end == -1) {
        end = output.length();
      }
    }
  }

  QMap<QString, QUrl> map;
  QHash<QString, int> nameMap;

  QString s;
  // if found exact matches
  if(pos > -1) {
    pos = s_anchorNameRx->indexIn(output, pos+13);
    while(pos > -1 && pos < end && m_matches.size() < m_limit) {
      QUrl u = QUrl(m_url).resolved(s_anchorNameRx->cap(1));
      s = s_anchorNameRx->cap(2).trimmed() + QLatin1Char(' ');
      // if more than one exact, add parentheses
      if(nameMap.contains(s) && nameMap[s] > 0) {
        // fix the first one that didn't have a number
        if(nameMap[s] == 1) {
          QUrl u2 = map[s];
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
    QUrl u = QUrl(m_url).resolved(s_anchorNameRx->cap(1)); // relative URL
    s = s_anchorNameRx->cap(2).trimmed();
    if(nameMap.contains(s) && nameMap[s] > 0) {
    // fix the first one that didn't have a number
      if(nameMap[s] == 1) {
        QUrl u2 = map[s];
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
    pos = s_anchorNameRx->indexIn(output, pos+s_anchorNameRx->matchedLength());
  }

  if(map.count() == 0) {
    myLog() << "no name matches found.";
    stop();
    return;
  }

  KDialog dlg(GUI::Proxy::widget());
  dlg.setCaption(i18n("Select IMDb Result"));
  dlg.setModal(false);
  dlg.setButtons(KDialog::Ok|KDialog::Cancel);

  QWidget* box = new QWidget(&dlg);
  QVBoxLayout* boxVBoxLayout = new QVBoxLayout(box);
  boxVBoxLayout->setMargin(0);
  boxVBoxLayout->setSpacing(10);
  (void) new QLabel(i18n("<qt>Your search returned multiple matches. Please select one below.</qt>"), box);

  QListWidget* listWidget = new QListWidget(box);
  boxVBoxLayout->addWidget(listWidget);
  listWidget->setMinimumWidth(400);
  listWidget->setWrapping(true);

  QMapIterator<QString, QUrl> i(map);
  while(i.hasNext()) {
    i.next();
    const QString& value = i.key();
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
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job, SIGNAL(result(KJob*)),
          SLOT(slotComplete(KJob*)));
  connect(m_job, SIGNAL(redirection(KIO::Job *, const QUrl&)),
          SLOT(slotRedirection(KIO::Job*, const QUrl&)));

  // do not stop() here
}

Tellico::Data::EntryPtr IMDBFetcher::fetchEntryHook(uint uid_) {
  // if we already grabbed this one, then just pull it out of the dict
  Data::EntryPtr entry = m_entries[uid_];
  if(entry) {
    return entry;
  }

  if(!m_matches.contains(uid_) && !m_allMatches.contains(uid_)) {
    myLog() << "no url found";
    return Data::EntryPtr();
  }
  QUrl url = m_matches.contains(uid_) ? m_matches[uid_]
                                      : m_allMatches[uid_];
  if(url.path().contains(QRegExp(QLatin1String("/tt\\d+/$"))))  {
    url.setPath(url.path() + QLatin1String("combined"));
  }

  QUrl origURL = m_url; // keep to switch back
  QString results;
  // if the url matches the current one, no need to redownload it
  if(url == m_url) {
//    myDebug() << "matches previous URL, no downloading needed.";
    results = Tellico::decodeHTML(m_text);
  } else {
    // now it's sychronous
    // be quiet about failure
    results = Tellico::fromHtmlData(FileHandler::readDataFile(url, true));
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
    results = Tellico::decodeHTML(results);
  }
  if(results.isEmpty()) {
    myLog() << "no text results";
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
  if(m_lang == EN) {
    doLists(str_, entry);
  } else {
    doLists2(str_, entry);
  }
  doStudio(str_, entry);
  doPerson(str_, entry, langData(m_lang).director, QLatin1String("director"));
  doPerson(str_, entry, langData(m_lang).writer, QLatin1String("writer"));
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
    // we want to strip the "/combined" from the url
    QString url = m_url.url();
    if(url.endsWith(QLatin1String("/combined"))) {
      url = m_url.adjusted(QUrl::RemoveFilename).url();
    }
    entry->setField(imdb, url);
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
  QRegExp runtimeRx(langData(m_lang).runtime, Qt::CaseInsensitive);
  runtimeRx.setMinimal(true);

  if(runtimeRx.indexIn(str_) > -1) {
    entry_->setField(QLatin1String("running-time"), runtimeRx.cap(1));
  }
}

void IMDBFetcher::doAspectRatio(const QString& str_, Tellico::Data::EntryPtr entry_) {
  QRegExp rx(QString::fromLatin1("%1.*([\\d\\.\\,]+\\s*:\\s*[\\d\\.\\,]+)").arg(langData(m_lang).aspect_ratio), Qt::CaseInsensitive);
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
  QRegExp akaRx(QString::fromLatin1("%1(.*)<span[>\\s/]").arg(langData(m_lang).also_known_as), Qt::CaseInsensitive);
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
    const QRegExp dashEndRx(QLatin1String("\\s*-\\s+.+$"));
    QStringList values;
    for(QStringList::Iterator it = list.begin(); it != list.end(); ++it) {
      QString s = *it;
      // sometimes, the word "more" gets linked to the releaseinfo page, check that
      if(s.contains(QLatin1String("releaseinfo"))) {
        continue;
      }
      s.remove(*s_tagRx);
      s.remove(brackRx);
      // remove country
      s.remove(dashEndRx);
      s.remove(QLatin1Char('"'));
      s = s.trimmed();
      // the first value ends up being or starting with the colon after "Also known as"
      // I'm too lazy to figure out a better regexp
      if(s.startsWith(QLatin1Char(':'))) {
        s = s.mid(1);
        s = s.trimmed();
      }
      if(!s.isEmpty()) {
        values += s;
      }
    }
    if(!values.isEmpty()) {
      entry_->setField(QLatin1String("alttitle"), values.join(FieldFormat::rowDelimiterString()));
    }
  } else {
//    myLog() << "'Also Known As' not found";
  }
}

void IMDBFetcher::doPlot(const QString& str_, Tellico::Data::EntryPtr entry_, const QUrl& baseURL_) {
  // plot summaries provided by users are on a separate page
  // should those be preferred?

  bool useUserSummary = false;

  QString thisPlot;
  // match until next opening tag
  QString plotRxStr = langData(m_lang).plot + QLatin1String(":(.*)<[^/].*</");
  QRegExp plotRx(plotRxStr, Qt::CaseInsensitive);
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
  } else {
    useUserSummary = true;
  }

  if(useUserSummary) {
    QRegExp idRx(QLatin1String("title/(tt\\d+)"));
    idRx.indexIn(baseURL_.path());
    QUrl plotURL = baseURL_;
    plotURL.setPath(QLatin1String("/title/") + idRx.cap(1) + QLatin1String("/plotsummary"));
    // be quiet about failure
    QString plotPage = Tellico::fromHtmlData(FileHandler::readDataFile(plotURL, true));

    if(!plotPage.isEmpty()) {
      QRegExp plotRx(QLatin1String("<p\\s+class\\s*=\\s*\"plotpar\">(.*)</p"));
      plotRx.setMinimal(true);
      QRegExp plotRx2(QLatin1String("<div\\s+id\\s*=\\s*\"swiki.2.1\">(.*)</d"));
      plotRx2.setMinimal(true);
      QString userPlot;
      if(plotRx.indexIn(plotPage) > -1) {
        userPlot = plotRx.cap(1);
      } else if(plotRx2.indexIn(plotPage) > -1) {
        userPlot = plotRx2.cap(1);
      }
      userPlot.remove(*s_tagRx); // remove HTML tags
      // remove last little "written by", if there
      userPlot.remove(QRegExp(QLatin1String("\\s*written by.*$"), Qt::CaseInsensitive));
      if(!userPlot.isEmpty()) {
        entry_->setField(QLatin1String("plot"), Tellico::decodeHTML(userPlot));
      }
    }
  }
}

void IMDBFetcher::doStudio(const QString& str_, Tellico::Data::EntryPtr entry_) {
  // match until next opening tag
//  QRegExp productionRx(langData(m_lang).studio, Qt::CaseInsensitive);
  QRegExp productionRx(langData(m_lang).studio);
  productionRx.setMinimal(true);

  QRegExp blackcatRx(QLatin1String("blackcatheader"), Qt::CaseInsensitive);
  blackcatRx.setMinimal(true);

  const int pos1 = str_.indexOf(productionRx);
  if(pos1 == -1) {
//    myLog() << "No studio found";
    return;
  }

  int pos2 = str_.indexOf(blackcatRx, pos1);
  if(pos2 == -1) {
    pos2 = str_.length();
  }

  const QString text = str_.mid(pos1, pos2-pos1);
  const QString company = QLatin1String("/company/");
  QStringList studios;
  for(int pos = s_anchorRx->indexIn(text); pos > -1; pos = s_anchorRx->indexIn(text, pos+s_anchorRx->matchedLength())) {
    const QString cap1 = s_anchorRx->cap(1);
    if(cap1.contains(company)) {
      studios += s_anchorRx->cap(2).trimmed();
    }
  }

  entry_->setField(QLatin1String("studio"), studios.join(FieldFormat::delimiterString()));
}

void IMDBFetcher::doPerson(const QString& str_, Tellico::Data::EntryPtr entry_,
                           const QString& imdbHeader_, const QString& fieldName_) {
  QRegExp br2Rx(QLatin1String("<br[\\s/]*>\\s*<br[\\s/]*>"), Qt::CaseInsensitive);
  br2Rx.setMinimal(true);
  QRegExp divRx(QLatin1String("<div\\s[^>]*class\\s*=\\s*\"(?:info|txt-block)\"[^>]*>(.*)</div"), Qt::CaseInsensitive);
  divRx.setMinimal(true);
  QString name = QLatin1String("/name/");

  StringSet people;
  for(int pos = str_.indexOf(divRx); pos > -1; pos = str_.indexOf(divRx, pos+divRx.matchedLength())) {
    const QString infoBlock = divRx.cap(1);
    if(infoBlock.contains(imdbHeader_, Qt::CaseInsensitive)) {
      int pos2 = s_anchorRx->indexIn(infoBlock);
      while(pos2 > -1) {
        if(s_anchorRx->cap(1).contains(name)) {
          people.add(s_anchorRx->cap(2).trimmed());
        }
        pos2 = s_anchorRx->indexIn(infoBlock, pos2+s_anchorRx->matchedLength());
      }
      break;
    }
  }
  if(!people.isEmpty()) {
    entry_->setField(fieldName_, people.toList().join(FieldFormat::delimiterString()));
  }
}

void IMDBFetcher::doCast(const QString& str_, Tellico::Data::EntryPtr entry_, const QUrl& baseURL_) {
  // the extended cast list is on a separate page
  // that's usually a lot of people
  // but since it can be in billing order, the main actors might not
  // be in the short list
  QRegExp idRx(QLatin1String("title/(tt\\d+)"));
  idRx.indexIn(baseURL_.path());
  QUrl castURL = baseURL_;
  castURL.setPath(QLatin1String("/title/") + idRx.cap(1) + QLatin1String("/fullcredits"));

  // be quiet about failure and be sure to translate entities
  const QString castPage = Tellico::decodeHTML(FileHandler::readTextFile(castURL, true));
#if 0
  myWarning() << "Remove debug from imdbfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/testimdbcast.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << castPage;
  }
  f.close();
#endif

  const LangData& data = langData(m_lang);

  int pos = -1;
  // the text to search, depends on which page is being read
  QString castText = castPage;
  if(castText.isEmpty()) {
    // fall back to short list
    castText = str_;
    pos = castText.indexOf(data.cast1, 0, Qt::CaseInsensitive);
    if(pos == -1) {
      pos = castText.indexOf(data.cast2, 0, Qt::CaseInsensitive);
    }
  } else {
    // first look for anchor
    QRegExp castAnchorRx(QLatin1String("<a\\s+name\\s*=\\s*\"cast\""), Qt::CaseInsensitive);
    pos = castAnchorRx.indexIn(castText);
    if(pos < 0) {
      QRegExp tableClassRx(QLatin1String("<table\\s+class\\s*=\\s*\"cast_list\""), Qt::CaseInsensitive);
      pos = tableClassRx.indexIn(castText);
      if(pos < 0) {
        // fragile, the word "cast" appears in the title, but need to find
        // the one right above the actual cast table
        // for TV shows, there's a link on the sidebar for "episodes case"
        // so need to not match that one
        const QString castEnd = data.cast + QLatin1String("</");
        pos = castText.indexOf(castEnd, 0, Qt::CaseInsensitive);
        if(pos > 9) {
          // back up 9 places
          if(castText.mid(pos-9, 9).startsWith(data.episodes)) {
            // find next cast list
            pos = castText.indexOf(castEnd, pos+6, Qt::CaseInsensitive);
          }
        }
      }
    }
  }
  if(pos == -1) { // no cast list found
    myLog() << "no cast list found";
    return;
  }

  const QString name = QLatin1String("/name/");
  QRegExp tdRx(QLatin1String("<td[^>]*>(.*)</td>"), Qt::CaseInsensitive);
  tdRx.setMinimal(true);

  QRegExp tdActorRx(QLatin1String("<td\\s+[^>]*itemprop=\"actor\"[^>]*>(.*)</td>"), Qt::CaseInsensitive);
  tdActorRx.setMinimal(true);

  QRegExp tdCharRx(QLatin1String("<td\\s+[^>]*class=\"character\"[^>]*>(.*)</td>"), Qt::CaseInsensitive);
  tdCharRx.setMinimal(true);

  QStringList cast;
  // loop until closing table tag
  int endPos = castText.indexOf(QLatin1String("</table"), pos, Qt::CaseInsensitive);
  castText = castText.mid(pos, endPos-pos+1);
  pos = tdActorRx.indexIn(castText);
  while(pos > -1 && cast.count() < m_numCast) {
    QString actorText = tdActorRx.cap(1).remove(*s_tagRx).simplified();
    const int pos2 = tdCharRx.indexIn(castText, pos+1);
    if(pos2 > -1) {
      cast += actorText
            + FieldFormat::columnDelimiterString()
            + tdCharRx.cap(1).remove(*s_tagRx).simplified();
    }
    pos = tdActorRx.indexIn(castText, qMax(pos+1, pos2));
  }

  if(!cast.isEmpty()) {
    entry_->setField(QLatin1String("cast"), cast.join(FieldFormat::rowDelimiterString()));
  }

  // also do other items from fullcredits page, like producer
  QStringList producers;
  pos = castPage.indexOf(data.producer, 0, Qt::CaseInsensitive);
  if(pos > -1) {
    endPos = castText.indexOf(QLatin1String("</table"), pos, Qt::CaseInsensitive);
    if(endPos == -1) {
      endPos = castText.length();
    }
    const QString prodText = castPage.mid(pos, endPos-pos+1);
    tdCharRx.setPattern(QLatin1String("<td\\s+[^>]*class=\"credit\"[^>]*>(.*)</td>"));

    pos = s_anchorNameRx->indexIn(prodText);
    while(pos > -1) {
      const int pos2 = tdCharRx.indexIn(prodText, pos+1);
      const QString credit = tdCharRx.cap(1).trimmed();
      if(pos2 > -1 && (credit == QLatin1String("producer") ||
                       credit == QLatin1String("co-producer") ||
                       credit == QLatin1String("associate producer"))) {
        producers += s_anchorNameRx->cap(2).trimmed();
      }
      pos = s_anchorNameRx->indexIn(prodText, pos+1);
    }
  }

  if(!producers.isEmpty()) {
    entry_->setField(QLatin1String("producer"), producers.join(FieldFormat::delimiterString()));
  }
#if 0
  myWarning() << "Remove debug from imdbfetcher.cpp";
  QFile f2(QString::fromLatin1("/tmp/testimdbcast2.html"));
  if(f2.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << producers.join(FieldFormat::delimiterString());
  }
  f2.close();
#endif
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

void IMDBFetcher::doCover(const QString& str_, Tellico::Data::EntryPtr entry_, const QUrl& baseURL_) {
  QRegExp imgRx(QLatin1String("<img\\s+[^>]*src\\s*=\\s*\"([^\"]*)\"[^>]*>"), Qt::CaseInsensitive);
  imgRx.setMinimal(true);

  QRegExp posterRx(QLatin1String("<a\\s+[^>]*name\\s*=\\s*\"poster\"[^>]*>(.*)</a>"), Qt::CaseInsensitive);
  posterRx.setMinimal(true);

  const QString cover = QLatin1String("cover");

  int pos = posterRx.indexIn(str_);
  while(pos > -1) {
    if(posterRx.cap(1).contains(imgRx)) {
      QUrl u = QUrl(baseURL_).resolved(imgRx.cap(1));
      QString id = ImageFactory::addImage(u, true);
      if(!id.isEmpty()) {
        entry_->setField(cover, id);
        return;
      }
    }
    pos = posterRx.indexIn(str_, pos+posterRx.matchedLength());
  }

  // didn't find the cover, IMDb also used to put "cover" inside the url
  // cover is the img with the "cover" alt text

  pos = imgRx.indexIn(str_);
  while(pos > -1) {
    const QString url = imgRx.cap(0).toLower();
    if(url.contains(cover)) {
      QUrl u = QUrl(baseURL_).resolved(imgRx.cap(1));
      QString id = ImageFactory::addImage(u, true);
      if(!id.isEmpty()) {
        entry_->setField(cover, id);
        return;
      }
    }
    pos = imgRx.indexIn(str_, pos+imgRx.matchedLength());
  }

  // also check for <link rel='image_src'
  QRegExp linkRx(QLatin1String("<link (.*)>"), Qt::CaseInsensitive);
  linkRx.setMinimal(true);

  const QString src = QLatin1String("image_src");

  pos = linkRx.indexIn(str_);
  while(pos > -1) {
    const QString tag = linkRx.cap(1);
    if(tag.contains(src, Qt::CaseInsensitive)) {
      QRegExp hrefRx(QLatin1String("href=['\"](.*)['\"]"), Qt::CaseInsensitive);
      hrefRx.setMinimal(true);
      if(hrefRx.indexIn(tag) > -1) {
        QUrl u = QUrl(baseURL_).resolved(hrefRx.cap(1));
        QString id = ImageFactory::addImage(u, true);
        if(!id.isEmpty()) {
          entry_->setField(cover, id);
          return;
        }
      }
    }
    pos = linkRx.indexIn(str_, pos+linkRx.matchedLength());
  }
}

void IMDBFetcher::doLists2(const QString& str_, Tellico::Data::EntryPtr entry_) {
  QRegExp divInfoRx(QLatin1String("<div class=\"info\">(.*)</div"), Qt::CaseInsensitive);
  divInfoRx.setMinimal(true);

  const LangData& data = langData(m_lang);

  QStringList genres, countries, langs, certs, tracks;
  for(int pos = divInfoRx.indexIn(str_); pos > -1; pos = divInfoRx.indexIn(str_, pos+divInfoRx.matchedLength())) {
    const QString text = divInfoRx.cap(1).remove(*s_tagRx);
    const QString tag = text.section(QLatin1Char(':'), 0, 0).simplified();
    QString value = text.section(QLatin1Char(':'), 1, -1).simplified();
    if(tag == data.genre) {
      foreach(const QString& token, value.split(QLatin1Char('|'))) {
        genres << token.trimmed();
      }
    } else if(tag == data.language) {
      foreach(const QString& token, value.split(QRegExp(QLatin1String("[,|]")))) {
        langs << token.trimmed();
      }
    } else if(tag == data.sound) {
      foreach(const QString& token, value.split(QLatin1Char('|'))) {
        tracks << token.trimmed();
      }
    } else if(tag == data.country) {
      countries << value;
    } else if(tag == data.certification) {
      foreach(const QString& token, value.split(QLatin1Char('|'))) {
        certs << token.trimmed();
      }
    } else if(tag == data.color) {
      // cut off any parentheses
      value = value.section(QLatin1Char('('), 0, 0).trimmed();
      // change "black and white" to "black & white"
      value.replace(QLatin1String("and"), QLatin1String("&"));
      if(value == data.color) {
        entry_->setField(QLatin1String("color"), i18n("Color"));
      } else {
        entry_->setField(QLatin1String("color"), value);
      }
    }
  }

  entry_->setField(QLatin1String("genre"), genres.join(FieldFormat::delimiterString()));
  entry_->setField(QLatin1String("nationality"), countries.join(FieldFormat::delimiterString()));
  entry_->setField(QLatin1String("language"), langs.join(FieldFormat::delimiterString()));
  entry_->setField(QLatin1String("audio-track"), tracks.join(FieldFormat::delimiterString()));
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
      if(certsAllowed.contains(lcert)) {
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

// look at every anchor tag in the string
void IMDBFetcher::doLists(const QString& str_, Tellico::Data::EntryPtr entry_) {
  const QString genre = QLatin1String("/Genres/");
  const QString genre2 = QLatin1String("/genre/");
  const QString country = QLatin1String("/country/");
  const QString lang = QLatin1String("/language/");
  const QString colorInfo = QLatin1String("colors=");
  const QString cert = QLatin1String("certificates=");
  const QString soundMix = QLatin1String("sound_mixes=");
  const QString year = QLatin1String("/Years/");

  // if we reach faqs or user comments, we can stop
  const QString faqs = QLatin1String("/faq");
  const QString users = QLatin1String("/user/");
  // IMdb also has links with the word "sections" in them, remove that
  // for genres and nationalities

  int startPos = str_.indexOf(QLatin1String("<div id=\"pagecontent\">"));
  if(startPos == -1) {
    startPos = 0;
  }

  QStringList genres, countries, langs, certs, tracks;
  for(int pos = s_anchorRx->indexIn(str_, startPos); pos > -1; pos = s_anchorRx->indexIn(str_, pos+s_anchorRx->matchedLength())) {
    const QString cap1 = s_anchorRx->cap(1);
    if(cap1.contains(genre) || cap1.contains(genre2)) {
      if(!s_anchorRx->cap(2).contains(QLatin1String(" section"), Qt::CaseInsensitive)) {
        genres += s_anchorRx->cap(2).trimmed();
      }
    } else if(cap1.contains(country)) {
      if(!s_anchorRx->cap(2).contains(QLatin1String(" section"), Qt::CaseInsensitive)) {
        countries += s_anchorRx->cap(2).trimmed();
      }
    } else if(cap1.contains(lang)) {
      langs += s_anchorRx->cap(2).trimmed();
    } else if(cap1.contains(colorInfo)) {
      // change "black and white" to "black & white"
      entry_->setField(QLatin1String("color"),
                       s_anchorRx->cap(2).replace(QLatin1String("and"), QLatin1String("&")).trimmed());
    } else if(cap1.contains(cert)) {
      certs += s_anchorRx->cap(2).trimmed();
    } else if(cap1.contains(soundMix)) {
      tracks += s_anchorRx->cap(2).trimmed();
      // if year field wasn't set before, do it now
    } else if(entry_->field(QLatin1String("year")).isEmpty() && cap1.contains(year)) {
      entry_->setField(QLatin1String("year"), s_anchorRx->cap(2).trimmed());
    } else if((cap1.contains(faqs) || cap1.contains(users)) && !genres.isEmpty()) {
      break;
    }
  }

  // since we have multiple genre search strings
  genres.removeDuplicates();

  entry_->setField(QLatin1String("genre"), genres.join(FieldFormat::delimiterString()));
  entry_->setField(QLatin1String("nationality"), countries.join(FieldFormat::delimiterString()));
  entry_->setField(QLatin1String("language"), langs.join(FieldFormat::delimiterString()));
  entry_->setField(QLatin1String("audio-track"), tracks.join(FieldFormat::delimiterString()));
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
      if(certsAllowed.contains(lcert)) {
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
  QUrl link = entry_->field(QLatin1String("imdb"));

  if(!link.isEmpty() && link.isValid()) {
    if(link.host() != m_host) {
//      myLog() << "switching hosts to " << m_host;
      link.setHost(m_host);
    }
    return FetchRequest(Fetch::Raw, link.url());
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
  hash[QLatin1String("imdb")]            = i18n("IMDb Link");
  hash[QLatin1String("imdb-rating")]     = i18n("IMDb Rating");
  hash[QLatin1String("alttitle")]        = i18n("Alternative Titles");
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
  /*
  IMDB.fr and others now redirects to imdb.com
  QLabel* label = new QLabel(i18n("Country: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_langCombo = new GUI::ComboBox(optionsWidget());
  m_langCombo->addItem(i18n("United States"), EN);
  m_langCombo->addItem(i18n("France"), FR);
  m_langCombo->addItem(i18n("Spain"), ES);
  m_langCombo->addItem(i18n("Germany"), DE);
  m_langCombo->addItem(i18n("Italy"), IT);
  m_langCombo->addItem(i18n("Portugal"), PT);
  connect(m_langCombo, SIGNAL(activated(int)), SLOT(slotSetModified()));
  connect(m_langCombo, SIGNAL(activated(int)), SLOT(slotSiteChanged()));
  l->addWidget(m_langCombo, row, 1);
  QString w = i18n("The Internet Movie Database provides data from several different localized sites. "
                   "Choose the one you wish to use for this data source.");
  label->setWhatsThis(w);
  m_langCombo->setWhatsThis(w);
  label->setBuddy(m_langCombo);
  */
  
  QLabel* label = new QLabel(i18n("&Maximum cast: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_numCast = new KIntSpinBox(0, 99, 1, 10, optionsWidget());
  connect(m_numCast, SIGNAL(valueChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_numCast, row, 1);
  QString w = i18n("The list of cast members may include many people. Set the maximum number returned from the search.");
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
  KAcceleratorManager::manage(optionsWidget());

  if(fetcher_) {
    // m_langCombo->setCurrentData(fetcher_->m_lang);
    m_numCast->setValue(fetcher_->m_numCast);
    m_fetchImageCheck->setChecked(fetcher_->m_fetchImages);
  } else { //defaults
    // m_langCombo->setCurrentData(EN);
    m_numCast->setValue(10);
    m_fetchImageCheck->setChecked(true);
  }
}

void IMDBFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  // int n = m_langCombo->currentData().toInt();
  // config_.writeEntry("Lang", n);
  config_.writeEntry("Host", QString()); // clear old host entry
  config_.writeEntry("Max Cast", m_numCast->value());
  config_.writeEntry("Fetch Images", m_fetchImageCheck->isChecked());
}

QString IMDBFetcher::ConfigWidget::preferredName() const {
  // return IMDBFetcher::langData(m_langCombo->currentData().toInt()).siteTitle;
  return IMDBFetcher::langData(EN).siteTitle;
}

void IMDBFetcher::ConfigWidget::slotSiteChanged() {
  emit signalName(preferredName());
}

