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
#include "../images/imagefactory.h"
#include "../utils/string_utils.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>
#include <KIO/Job>
#include <KJobUiDelegate>
#include <KAcceleratorManager>
#include <KJobWidgets/KJobWidgets>

#include <QSpinBox>
#include <QRegExp>
#include <QFile>
#include <QMap>
#include <QLabel>
#include <QCheckBox>
#include <QGroupBox>
#include <QGridLayout>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QRegularExpression>

namespace {
  static const uint IMDB_MAX_RESULTS = 20;
  static const uint IMDB_DEFAULT_CAST_SIZE = 10;
  static const int IMDB_MAX_PERSON_COUNT = 5; // limit number of directors, writers, etc, esp for TV series
  static const int IMDB_MAX_SEASON_COUNT = 5; // simply takes too long otherwise
}

using namespace Tellico;
using Tellico::Fetch::IMDBFetcher;

QRegExp* IMDBFetcher::s_tagRx = nullptr;
QRegExp* IMDBFetcher::s_anchorRx = nullptr;
QRegExp* IMDBFetcher::s_anchorTitleRx = nullptr;
QRegExp* IMDBFetcher::s_anchorNameRx = nullptr;
QRegExp* IMDBFetcher::s_titleRx = nullptr;
const QRegularExpression* IMDBFetcher::s_titleIdRx = nullptr;
int IMDBFetcher::s_instanceCount = 0;

// static
void IMDBFetcher::initRegExps() {
  s_tagRx = new QRegExp(QStringLiteral("<.*>"));
  s_tagRx->setMinimal(true);

  s_anchorRx = new QRegExp(QStringLiteral("<a\\s+[^>]*href\\s*=\\s*\"([^\"]+)\"[^<]*>([^<]+)</a>"), Qt::CaseInsensitive);
  s_anchorRx->setMinimal(true);

  s_anchorTitleRx = new QRegExp(QStringLiteral("<a\\s+[^>]*href\\s*=\\s*\"([^\"]*/title/[^\"]*)\"[^<]*>([^<]*)</a>"), Qt::CaseInsensitive);
  s_anchorTitleRx->setMinimal(true);

  s_anchorNameRx = new QRegExp(QStringLiteral("<a\\s+[^>]*href\\s*=\\s*\"([^\"]*/name/[^\"]*)\"[^<]*>(.+)</a>"), Qt::CaseInsensitive);
  s_anchorNameRx->setMinimal(true);

  s_titleRx = new QRegExp(QStringLiteral("<title>(.*)</title>"), Qt::CaseInsensitive);
  s_titleRx->setMinimal(true);

  s_titleIdRx = new QRegularExpression(QStringLiteral("title/(tt\\d+)"));
}

void IMDBFetcher::deleteRegExps() {
  delete s_tagRx;
  s_tagRx = nullptr;

  delete s_anchorRx;
  s_anchorRx = nullptr;

  delete s_anchorTitleRx;
  s_anchorTitleRx = nullptr;

  delete s_anchorNameRx;
  s_anchorNameRx = nullptr;

  delete s_titleRx;
  s_titleRx = nullptr;

  delete s_titleIdRx;
  s_titleIdRx = nullptr;
}

// static
const IMDBFetcher::LangData& IMDBFetcher::langData(int lang_) {
  Q_ASSERT(lang_ >= 0);
  Q_ASSERT(lang_ <  6);
  static LangData dataVector[6] = {
    {
      i18n("Internet Movie Database"),
      QStringLiteral("findSectionHeader"),
      QStringLiteral("Exact Matches"),
      QStringLiteral("Partial Matches"),
      QStringLiteral("Approx Matches"),
      QStringLiteral("findSectionHeader"),
      QStringLiteral("Other Results"),
      QStringLiteral("aka"),
      QStringLiteral("Directed by"),
      QStringLiteral("Written by"),
      QStringLiteral("Produced by"),
      QStringLiteral("runtime.*(\\d+)\\s+min"),
      QStringLiteral("aspect ratio"),
      QStringLiteral("also known as"),
      QStringLiteral("Production Co"),
      QStringLiteral("cast"),
      QStringLiteral("cast overview"),
      QStringLiteral("credited cast"),
      QStringLiteral("episodes"),
      QStringLiteral("Genre"),
      QStringLiteral("Sound"),
      QStringLiteral("Color"),
      QStringLiteral("Language"),
      QStringLiteral("Certification"),
      QStringLiteral("Country"),
      QStringLiteral("plot\\s+(outline|summary)(?!/)"),
      QStringLiteral("Music by")
    }, {
      i18n("Internet Movie Database (French)"),
      QStringLiteral("findSectionHeader"),
      QStringLiteral("Résultats Exacts"),
      QStringLiteral("Résultats Partiels"),
      QStringLiteral("Résultats Approximatif"),
      QStringLiteral("findSectionHeader"),
      QStringLiteral("Résultats Autres"),
      QStringLiteral("autre titre"),
      QStringLiteral("Réalisateur"),
      QStringLiteral("Scénarist"),
      QString(),
      QStringLiteral("Durée.*(\\d+)\\s+heur.*\\s+(\\d+)\\s+min"),
      QStringLiteral("Proportions de l’image"),
      QStringLiteral("Alias"),
      QStringLiteral("Sociétés de production"),
      QStringLiteral("Ensemble"),
      QStringLiteral("cast overview"), // couldn't get phrase
      QStringLiteral("credited cast"), // couldn't get phrase
      QStringLiteral("episodes"),
      QStringLiteral("Genre"),
      QStringLiteral("Mixage audio"),
      QStringLiteral("Couleur"),
      QStringLiteral("Langue"),
      QStringLiteral("Classification"),
      QStringLiteral("Pays d’origine"),
      QStringLiteral("Intrigue\\s*"),
      QString() // reference page doesn't seem to have localized composer
    }, {
      i18n("Internet Movie Database (Spanish)"),
      QStringLiteral("findSectionHeader"),
      QStringLiteral("Resultados Exactos"),
      QStringLiteral("Resultados Parciales"),
      QStringLiteral("Resultados Aproximados"),
      QStringLiteral("findSectionHeader"),
      QStringLiteral("Resultados Otros"),
      QStringLiteral("otro título"),
      QStringLiteral("Director"),
      QStringLiteral("Escritores"),
      QString(),
      QStringLiteral("Duración.*(\\d+)\\s+min"),
      QStringLiteral("Relación de Aspecto"),
      QStringLiteral("Conocido como"),
      QStringLiteral("Compañías Productores"),
      QStringLiteral("Reparto"),
      QStringLiteral("cast overview"), // couldn't get phrase
      QStringLiteral("credited cast"), // couldn't get phrase
      QStringLiteral("episodes"),
      QStringLiteral("Género"),
      QStringLiteral("Sonido"),
      QStringLiteral("Color"),
      QStringLiteral("Idioma"),
      QStringLiteral("Clasificación"),
      QStringLiteral("País"),
      QStringLiteral("Trama\\s*"),
      QString() // reference page doesn't seem to have localized composer
    }, {
      i18n("Internet Movie Database (German)"),
      QStringLiteral("findSectionHeader"),
      QStringLiteral("genaue Übereinstimmung"),
      QStringLiteral("teilweise Übereinstimmung"),
      QStringLiteral("näherungsweise Übereinstimmung"),
      QStringLiteral("findSectionHeader"),
      QStringLiteral("andere Übereinstimmung"),
      QStringLiteral("andere titel"),
      QStringLiteral("Regisseur"),
      QStringLiteral("Drehbuchautoren"),
      QString(),
      QStringLiteral("Länge.*(\\d+)\\s+min"),
      QStringLiteral("Seitenverhältnis"),
      QStringLiteral("Auch bekannt als"),
      QStringLiteral("Produktionsfirmen"),
      QStringLiteral("Besetzung"),
      QStringLiteral("cast overview"), // couldn't get phrase
      QStringLiteral("credited cast"), // couldn't get phrase
      QStringLiteral("episodes"),
      QStringLiteral("Genre"),
      QStringLiteral("Tonverfahren"),
      QStringLiteral("Farbe"),
      QStringLiteral("Sprache"),
      QStringLiteral("Altersfreigabe"),
      QStringLiteral("Land"),
      QStringLiteral("Handlung\\s*"),
      QString() // reference page doesn't seem to have localized composer
    }, {
      i18n("Internet Movie Database (Italian)"),
      QStringLiteral("findSectionHeader"),
      QStringLiteral("risultati esatti"),
      QStringLiteral("risultati parziali"),
      QStringLiteral("risultati approssimati"),
      QStringLiteral("findSectionHeader"),
      QStringLiteral("Resultados Otros"),
      QStringLiteral("otro título"),
      QStringLiteral("Regista"),
      QStringLiteral("Sceneggiatori"),
      QString(),
      QStringLiteral("Durata.*(\\d+)\\s+min"),
      QStringLiteral("Aspect Ratio"),
      QStringLiteral("Alias"),
      QStringLiteral("Società di produzione"),
      QStringLiteral("Cast"),
      QStringLiteral("cast overview"), // couldn't get phrase
      QStringLiteral("credited cast"), // couldn't get phrase
      QStringLiteral("episodes"),
      QStringLiteral("Genere"),
      QStringLiteral("Sonoro"),
      QStringLiteral("Colore"),
      QStringLiteral("Lingua"),
      QStringLiteral("Divieti"),
      QStringLiteral("Nazionalità"),
      QStringLiteral("Trama\\s*"),
      QString() // reference page doesn't seem to have localized composer
    }, {
      i18n("Internet Movie Database (Portuguese)"),
      QStringLiteral("findSectionHeader"),
      QStringLiteral("Exato"),
      QStringLiteral("Combinação Parcial"),
      QStringLiteral("Combinação Aproximada"),
      QStringLiteral("findSectionHeader"),
      QStringLiteral("Combinação Otros"),
      QStringLiteral("otro título"),
      QStringLiteral("Diretor"),
      QStringLiteral("Escritores"),
      QString(),
      QStringLiteral("Duração.*(\\d+)\\s+min"),
      QStringLiteral("Resolução"),
      QStringLiteral("Também Conhecido Como"),
      QStringLiteral("Companhias de Produção"),
      QStringLiteral("Elenco"),
      QStringLiteral("cast overview"), // couldn't get phrase
      QStringLiteral("credited cast"), // couldn't get phrase
      QStringLiteral("episodes"),
      QStringLiteral("Gênero"),
      QStringLiteral("Mixagem de Som"),
      QStringLiteral("Cor"),
      QStringLiteral("Lingua"),
      QStringLiteral("Certificação"),
      QStringLiteral("País"),
      QStringLiteral("Argumento\\s*"),
      QString() // reference page doesn't seem to have localized composer
    }
  };

  return dataVector[qBound(0, lang_, static_cast<int>(sizeof(dataVector)/sizeof(LangData)))];
}

IMDBFetcher::IMDBFetcher(QObject* parent_) : Fetcher(parent_),
    m_job(nullptr), m_started(false), m_fetchImages(true),
    m_numCast(IMDB_DEFAULT_CAST_SIZE), m_redirected(false), m_limit(IMDB_MAX_RESULTS), m_lang(EN),
    m_currentTitleBlock(Unknown), m_countOffset(0) {
  if(!s_instanceCount++) {
    initRegExps();
  }
  m_host = QStringLiteral("www.imdb.com");
}

IMDBFetcher::~IMDBFetcher() {
  if(!--s_instanceCount) {
    deleteRegExps();
  }
}

QString IMDBFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool IMDBFetcher::canFetch(int type) const {
  return type == Data::Collection::Video;
}

// imdb can search title only
bool IMDBFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Title;
}

void IMDBFetcher::readConfigHook(const KConfigGroup& config_) {
  const int lang = config_.readEntry("Lang", int(EN));
  m_lang = static_cast<Lang>(lang);
  if(m_name.isEmpty()) {
    m_name = langData(m_lang).siteTitle;
  }

  m_numCast = config_.readEntry("Max Cast", IMDB_DEFAULT_CAST_SIZE);
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
  m_url.setScheme(QStringLiteral("https"));
  m_url.setHost(m_host);
  m_url.setPath(QStringLiteral("/find/"));

  // as far as I can tell, the url encoding should always be iso-8859-1?
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("q"), request().value());

  switch(request().key()) {
    case Title:
      q.addQueryItem(QStringLiteral("s"), QStringLiteral("tt"));
      m_url.setQuery(q);
      break;

    case Raw:
      m_url = QUrl(request().value());
      break;

    default:
      myWarning() << "not supported:" << request().key();
      stop();
      return;
  }
//  myDebug() << m_url;

  m_job = KIO::storedGet(m_url, KIO::NoReload, KIO::HideProgressInfo);
  configureJob(m_job);
  connect(m_job.data(), &KJob::result,
          this, &IMDBFetcher::slotComplete);
  connect(m_job.data(), &KIO::TransferJob::redirection,
          this, &IMDBFetcher::slotRedirection);
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

  m_hasMoreResults = false;
  stop();
}

void IMDBFetcher::stop() {
  if(!m_started) {
    return;
  }
  if(m_job) {
    m_job->kill();
    m_job = nullptr;
  }

  m_started = false;
  m_redirected = false;

  emit signalDone(this);
}

void IMDBFetcher::slotRedirection(KIO::Job*, const QUrl& toURL_) {
  static const QRegularExpression ttEndRx(QStringLiteral("/tt\\d+/$"));
  m_url = toURL_;
  if(m_url.path().contains(ttEndRx)) {
    m_url.setPath(m_url.path() + QStringLiteral("reference"));
  }
  m_redirected = true;
}

void IMDBFetcher::slotComplete(KJob*) {
  if(m_job->error()) {
    myDebug() << m_job->errorString();
    m_job->uiDelegate()->showErrorMessage();
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
  m_job = nullptr;

#if 0
  myWarning() << "Remove debug from imdbfetcher.cpp for /tmp/testimdbresults.html";
  QFile f(QString::fromLatin1("/tmp/testimdbresults.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << m_text;
  }
  f.close();
#endif

  // a single result was found if we got redirected
  switch(request().key()) {
    case Title:
      if(m_redirected) {
        parseSingleTitleResult();
      } else {
        parseMultipleTitleResults();
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
  FetchResult* r = new FetchResult(this,
                                   pPos == -1 ? cap1 : cap1.left(pPos),
                                   pPos == -1 ? QString() : cap1.mid(pPos));
  // IMDB returns different HTML for single title results and has a query in the url
  // clear the query so we download the "canonical" page for the title
  QUrl url(m_url);
  url.setQuery(QString());
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

  // last resort
  if(m_matches.size() < m_limit) {
    const int pos_header = output.indexOf(QStringLiteral("ipc-page-content-container"));
    const int end_header = output.indexOf(QStringLiteral("cornerstone"), qMax(0, pos_header));
    if(pos_header > -1) {
      parseTitleBlock(output.mid(pos_header, end_header == -1 ? output.length() : end_header));
    }
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

  static const QRegularExpression akaRx(QStringLiteral("%1 (.*?)(</li>|</td>|<br)").arg(langData(m_lang).aka),
                                        QRegularExpression::CaseInsensitiveOption);
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
      const int end2 = str_.indexOf(QStringLiteral("<img"), start);
      const int end3 = str_.indexOf(QStringLiteral("</ul"), start);
      if(end2 > -1) end = qMin(end, end2);
      if(end3 > -1) end = qMin(end, end3);
      if(end == -1) {
        end = str_.length();
      }
      const QString text = str_.mid(start, end-start);
      pPos = text.indexOf(QLatin1Char('('));
      if(pPos > -1) {
        const int pNewLine = text.indexOf(QStringLiteral("<br"));
        if(pNewLine == -1 || pPos < pNewLine) {
          const int pPos2 = text.indexOf(QLatin1Char(')'), pPos);
          desc = text.mid(pPos+1, pPos2-pPos-1);
        }
        // IMDB occasionally has (I) in results. If so, continue parsing string
        if(desc == QStringLiteral("I") || desc == QStringLiteral("II")) {
          pPos = text.indexOf(QLatin1Char('('), pPos+1);
          if(pPos > -1 && (pNewLine == -1 || pPos < pNewLine)) {
            const int pPos2 = text.indexOf(QLatin1Char(')'), pPos);
            desc = text.mid(pPos+1, pPos2-pPos-1);
          }
        }
        pPos = -1;
      } else {
        static const QRegularExpression digitsRx(QStringLiteral(">([-–\\d]+)\\s*<"));
        QRegularExpressionMatch digitsMatch = digitsRx.match(text);
        if(digitsMatch.hasMatch()) {
          desc = digitsMatch.captured(1);
        }
      }
    }
    auto akaMatch = akaRx.match(str_, start+1, QRegularExpression::NormalMatch);
    if(akaMatch.hasMatch()) {
      // limit to 50 chars
      desc += QLatin1Char(' ') + akaMatch.captured(1).trimmed().remove(*s_tagRx);
      if(desc.length() > 50) {
        desc = desc.left(50) + QStringLiteral("...");
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

    FetchResult* r = new FetchResult(this, pPos == -1 ? cap2 : cap2.left(pPos), desc);
    QUrl u = QUrl(m_url).resolved(QUrl(cap1));
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
  static const QRegularExpression ttEndRx(QStringLiteral("/tt\\d+/$"));
  if(m_lang == EN && url.path().contains(ttEndRx))  {
    url.setPath(url.path() + QStringLiteral("reference"));
  }

  QUrl origURL = m_url; // keep to switch back
  QString results;
  // if the url matches the current one, no need to redownload it
  if(url == m_url) {
    results = Tellico::decodeHTML(m_text);
  } else {
    // now it's synchronous
    // be quiet about failure
    QPointer<KIO::StoredTransferJob> getJob = KIO::storedGet(url, KIO::NoReload, KIO::HideProgressInfo);
    configureJob(getJob);
    if(!getJob->exec()) {
      myWarning() << "...unable to read" << url;
      return Data::EntryPtr();
    }
    results = Tellico::fromHtmlData(getJob->data(), "UTF-8");
    m_url = url; // needed for processing
#if 0
    myWarning() << "Remove debug from imdbfetcher.cpp for /tmp/testimdbresult.html";
    myDebug() << m_url;
    QFile f(QStringLiteral("/tmp/testimdbresult.html"));
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

  doJson(str_, entry);

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
  doPerson(str_, entry, langData(m_lang).director, QStringLiteral("director"));
  doPerson(str_, entry, langData(m_lang).writer, QStringLiteral("writer"));
  doPerson(str_, entry, langData(m_lang).composer, QStringLiteral("composer"));
  doRating(str_, entry);
  doCast(str_, entry, m_url);
  if(m_fetchImages) {
    // needs base URL
    doCover(str_, entry, m_url);
  }
  if(optionalFields().contains(QStringLiteral("episode"))) {
    doEpisodes(str_, entry, m_url);
  }

  const QString imdb = QStringLiteral("imdb");
  if(!coll->hasField(imdb) && optionalFields().contains(imdb)) {
    coll->addField(Data::Field::createDefaultField(Data::Field::ImdbField));
  }
  if(coll->hasField(imdb) && coll->fieldByName(imdb)->type() == Data::Field::URL) {
    m_url.setQuery(QString());
    // we want to strip the "/reference" from the url
    QString url = m_url.url();
    if(url.endsWith(QStringLiteral("/reference"))) {
      url = m_url.adjusted(QUrl::RemoveFilename).url();
    }
    entry->setField(imdb, url);
  }
  return entry;
}

void IMDBFetcher::doJson(const QString& str_, Tellico::Data::EntryPtr entry_) {
  static const QRegularExpression jsonRx(QStringLiteral("<script[^>]+?type=\"application/ld\\+json\".*?>(.+?)</script>"));
  QRegularExpressionMatch jsonMatch = jsonRx.match(str_);
  if(!jsonMatch.hasMatch()) {
    return;
  }

  QJsonParseError parseError;
  QJsonDocument doc = QJsonDocument::fromJson(jsonMatch.captured(1).toUtf8(), &parseError);
  if(doc.isNull()) {
    myDebug() << "Bad json data:" << parseError.errorString();
    return;
  }

  QVariantMap objectMap = doc.object().toVariantMap();
  entry_->setField(QStringLiteral("title"), mapValue(objectMap, "name"));
  entry_->setField(QStringLiteral("director"), mapValue(objectMap, "director", "name"));
  entry_->setField(QStringLiteral("plot"), mapValue(objectMap, "description"));
  entry_->setField(QStringLiteral("genre"), mapValue(objectMap, "genre"));

  QStringList writers;
  foreach(QVariant v, objectMap.value(QStringLiteral("creator")).toList()) {
    auto vmap = v.toMap();
    if(vmap.value(QLatin1String("@type")) == QLatin1String("Person")) {
      writers += vmap.value(QLatin1String("name")).toString();
    }
  }
  entry_->setField(QStringLiteral("writer"), writers.join(FieldFormat::delimiterString()));

  QString cert = mapValue(objectMap, "contentRating");
  // set default certification, assuming US for now
  if(cert == QStringLiteral("Unrated")) {
    cert = QLatin1Char('U');
  }
  cert += QStringLiteral(" (USA)");
  const QStringList& certsAllowed = entry_->collection()->fieldByName(QStringLiteral("certification"))->allowed();
  if(certsAllowed.contains(cert)) {
    entry_->setField(QStringLiteral("certification"), cert);
  }

  const QString imageUrl = mapValue(objectMap,"image");
  if(!imageUrl.isEmpty()) {
    QString id = ImageFactory::addImage(QUrl::fromUserInput(imageUrl), true);
    if(!id.isEmpty()) {
      entry_->setField(QStringLiteral("cover"), id);
    }
  }

  if(optionalFields().contains(QStringLiteral("imdb-rating"))) {
    if(!entry_->collection()->hasField(QStringLiteral("imdb-rating"))) {
      Data::FieldPtr f(new Data::Field(QStringLiteral("imdb-rating"), i18n("IMDb Rating"), Data::Field::Rating));
      f->setCategory(i18n("General"));
      f->setProperty(QStringLiteral("maximum"), QStringLiteral("10"));
      entry_->collection()->addField(f);
    }

    const QString ratingString = mapValue(objectMap, "aggregateRating", "ratingValue");
    bool ok = true;
    float value = ratingString.toFloat(&ok);
    if(!ok) {
      value = QLocale().toFloat(ratingString, &ok);
    }
    if(ok) {
      entry_->setField(QStringLiteral("imdb-rating"), QString::number(value));
    }
  }
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
    entry_->setField(QStringLiteral("title"), title);

    // now for movies with original non-english titles, the <title> is english
    // but the page header is the original title. Grab the orig title
    static const QRegularExpression h3TitleRx(QStringLiteral("<h3[^>]+itemprop=\"name\"\\s*>(.*?)<"),
                                              QRegularExpression::DotMatchesEverythingOption);
    auto h3Match = h3TitleRx.match(str_);
    if(h3Match.hasMatch()) {
      QString possibleOrigTitle;
      const QString h3Title = h3Match.captured(1).trimmed();
      if(h3Title == title) {
        // some tv series have a original title label
        static const QRegularExpression origTitleRx(QLatin1String("/h3>(.*?)<span class=\"titlereference-original-title-label"),
                                                    QRegularExpression::DotMatchesEverythingOption);
        auto origTitleMatch = origTitleRx.match(str_);
        if(origTitleMatch.hasMatch()) {
          possibleOrigTitle = origTitleMatch.captured(1).trimmed();
        }
      } else {
        // mis-matching titles. If the user has requested original title,
        // put it in origtitle field and keep english as title
        // otherwise replace
        if(optionalFields().contains(QStringLiteral("origtitle"))) {
          possibleOrigTitle = h3Title;
        } else {
          entry_->setField(QStringLiteral("title"), h3Title);
        }
      }
      if(!possibleOrigTitle.isEmpty() && optionalFields().contains(QStringLiteral("origtitle"))) {
        Data::FieldPtr f(new Data::Field(QStringLiteral("origtitle"), i18n("Original Title")));
        f->setFormatType(FieldFormat::FormatTitle);
        entry_->collection()->addField(f);
        entry_->setField(QStringLiteral("origtitle"), possibleOrigTitle);
      }
    }

    // remove parentheses and extract year, tv shows can have (TV Series 2002-2003) for example
    int pPos2 = pPos+1;
    // find the closing parenthesis
    while(pPos2 < cap1.length() && cap1[pPos2] != QLatin1Char(')')) {
      ++pPos2;
    }
    const auto inParentheses = cap1.midRef(pPos+1, pPos2-pPos-1);
    if(!inParentheses.isEmpty()) {
      static const QRegularExpression yearRx(QLatin1String("\\d{4}")); // ignore ending year for tv series
      auto match = yearRx.match(inParentheses);
      if(match.hasMatch()) {
        entry_->setField(QStringLiteral("year"), match.captured());
      }
    }
  }
}

void IMDBFetcher::doRunningTime(const QString& str_, Tellico::Data::EntryPtr entry_) {
  // running time
  QRegExp runtimeRx(langData(m_lang).runtime, Qt::CaseInsensitive);
  runtimeRx.setMinimal(true);

  QString text = str_;
  text.remove(*s_tagRx);
  if(runtimeRx.indexIn(text) > -1) {
    if(m_lang == EN) {
      entry_->setField(QStringLiteral("running-time"), runtimeRx.cap(1));
    }
    else {
      const int hours = runtimeRx.cap(1).toInt();
      const int minutes = runtimeRx.cap(2).toInt();
      entry_->setField(QStringLiteral("running-time"), QString::number(hours*60+minutes));
    }
  }
}

void IMDBFetcher::doAspectRatio(const QString& str_, Tellico::Data::EntryPtr entry_) {
  QRegExp rx(QStringLiteral("%1.*([\\d\\.\\,]+\\s*:\\s*[\\d\\.\\,]+)").arg(langData(m_lang).aspect_ratio), Qt::CaseInsensitive);
  rx.setMinimal(true);

  if(rx.indexIn(str_) > -1) {
    entry_->setField(QStringLiteral("aspect-ratio"), rx.cap(1).trimmed());
  }
}

void IMDBFetcher::doAlsoKnownAs(const QString& str_, Tellico::Data::EntryPtr entry_) {
  if(!optionalFields().contains(QStringLiteral("alttitle"))) {
    return;
  }

  // match until next b tag
//  QRegExp akaRx(QStringLiteral("also known as(.*)<b(?:\\s.*)?>"));
  QRegExp akaRx(QStringLiteral("%1(.*)(<a|<span)[>\\s/]").arg(langData(m_lang).also_known_as), Qt::CaseInsensitive);
  akaRx.setMinimal(true);

  if(akaRx.indexIn(str_) > -1 && !akaRx.cap(1).isEmpty()) {
    Data::FieldPtr f = entry_->collection()->fieldByName(QStringLiteral("alttitle"));
    if(!f) {
      f = new Data::Field(QStringLiteral("alttitle"), i18n("Alternative Titles"), Data::Field::Table);
      f->setFormatType(FieldFormat::FormatTitle);
      entry_->collection()->addField(f);
    }

    // split by </li>
    QStringList list = akaRx.cap(1).split(QStringLiteral("</li>"));
    // lang could be included with [fr]
//    const QRegExp parRx(QStringLiteral("\\(.+\\)"));
    const QRegExp brackRx(QStringLiteral("\\[\\w+\\]"));
    const QRegExp countryRx(QStringLiteral("\\s*\\(.+\\)\\s*$"));
    QStringList values;
    for(QStringList::Iterator it = list.begin(); it != list.end(); ++it) {
      // sometimes the regexp doesn't work and grabs too much text
      // limit to reasonable length
      QString s = (*it).left(1000);
      // sometimes, the word "more" gets linked to the releaseinfo page, check that
      if(s.contains(QStringLiteral("releaseinfo"))) {
        continue;
      }
      s.remove(*s_tagRx);
      s.remove(brackRx);
      // remove country
      s.remove(countryRx);
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
      entry_->setField(QStringLiteral("alttitle"), values.join(FieldFormat::rowDelimiterString()));
    }
//  } else {
//    myLog() << "'Also Known As' not found";
  }
}

void IMDBFetcher::doPlot(const QString& str_, Tellico::Data::EntryPtr entry_, const QUrl& baseURL_) {
  if(!entry_->field(QStringLiteral("plot")).isEmpty()) return;
  // before using localized plot string, look for DOM component
  const QRegularExpression sectionRx(QStringLiteral("<section class=\"titlereference-section-overview\">(.+?)</div"),
                                     QRegularExpression::DotMatchesEverythingOption);
  auto sectionMatch = sectionRx.match(str_);
  if(sectionMatch.hasMatch()) {
    QString thisPlot = sectionMatch.captured(1);
    // TV Series include the episode link first, before the plot, so don't be fooled
    if(!thisPlot.contains(QLatin1String("<a href"))) {
      thisPlot.remove(*s_tagRx); // remove HTML tags
      entry_->setField(QStringLiteral("plot"), thisPlot.simplified());
      return;
    }
  }

  // plot summaries provided by users are on a separate page
  // should those be preferred?
  bool useUserSummary = false;

  // match until next <p> tag
  QString plotRxStr = langData(m_lang).plot + QStringLiteral("(.*)</(p|div|li)");
  QRegExp plotRx(plotRxStr, Qt::CaseInsensitive);
  plotRx.setMinimal(true);
  const QRegularExpression plotUrlRx(QStringLiteral("<a\\s+?[^>]*href\\s*=\\s*\"[^\"]*?/title/[^\"]*?/plotsummary\""),
                                     QRegularExpression::CaseInsensitiveOption);
  if(plotRx.indexIn(str_) > -1) {
    QString thisPlot = plotRx.cap(2);
    // if ends with "Written by", remove it. It has an em tag
    thisPlot.remove(QRegExp(QStringLiteral("<em class=\"nobr\".*</em>")));
    thisPlot.remove(*s_tagRx); // remove HTML tags
    thisPlot = thisPlot.simplified();
    // if thisPlot ends with (more) or contains
    // a url that ends with plotsummary, then we'll grab it, otherwise not
    if(thisPlot.isEmpty() ||
       plotRx.cap(0).endsWith(QStringLiteral("(more)</")) ||
       plotRx.cap(0).contains(plotUrlRx)) {
      useUserSummary = true;
    } else {
      entry_->setField(QStringLiteral("plot"), thisPlot);
    }
  } else {
    useUserSummary = true;
  }

  if(useUserSummary) {
    auto idMatch = s_titleIdRx->match(baseURL_.path());
    Q_ASSERT(idMatch.hasMatch());
    QUrl plotURL = baseURL_;
    plotURL.setPath(QStringLiteral("/title/") + idMatch.captured(1) + QStringLiteral("/plotsummary"));
    QPointer<KIO::StoredTransferJob> getJob = KIO::storedGet(plotURL, KIO::NoReload, KIO::HideProgressInfo);
    configureJob(getJob);
    if(!getJob->exec()) {
      myWarning() << "...unable to read" << plotURL;
    }
    QString plotPage = Tellico::fromHtmlData(getJob->data(), "UTF-8");

    if(!plotPage.isEmpty()) {
      const QRegularExpression plotRx1(QStringLiteral("id=\"plot-summaries-content\">(.+)</p"),
                                       QRegularExpression::DotMatchesEverythingOption);
      QString userPlot;
      auto plotMatch = plotRx1.match(plotPage);
      if(plotMatch.hasMatch()) {
        userPlot = plotMatch.captured(1);
      } else {
        const QRegularExpression plotRx2(QStringLiteral("<div\\s+id\\s*=\\s*\"swiki.2.1\">(.+?)</d"),
                                         QRegularExpression::DotMatchesEverythingOption);
        plotMatch = plotRx2.match(plotPage);
        if(plotMatch.hasMatch()) {
          userPlot = plotMatch.captured(1);
         }
      }
      userPlot.remove(*s_tagRx); // remove HTML tags
      // remove last little "written by", if there
      userPlot.remove(QRegExp(QStringLiteral("\\s*written by.*$"), Qt::CaseInsensitive));
      if(!userPlot.isEmpty()) {
        entry_->setField(QStringLiteral("plot"), Tellico::decodeHTML(userPlot.simplified()));
      }
    }
  }
//  myDebug() << "Plot:" << entry_->field(QStringLiteral("plot"));
}

void IMDBFetcher::doStudio(const QString& str_, Tellico::Data::EntryPtr entry_) {
  // match until next opening tag
//  QRegExp productionRx(langData(m_lang).studio, Qt::CaseInsensitive);
  QRegExp productionRx(langData(m_lang).studio);
  productionRx.setMinimal(true);

  const int pos1 = str_.indexOf(productionRx);
  if(pos1 == -1) {
//    myLog() << "No studio found";
    return;
  }

  int pos2 = str_.indexOf(QStringLiteral("blackcatheader"), pos1, Qt::CaseInsensitive);
  if(pos2 == -1) {
    pos2 = str_.length();
  }
  // stop matching when getting to Distributors
  int pos3 = str_.indexOf(QStringLiteral("Distributors"), pos1);
  if(pos3 > -1 && pos3 < pos2) {
    pos2 = pos3;
  }

  const QString text = str_.mid(pos1, pos2-pos1);
  const QString company = QStringLiteral("/company/");
  QStringList studios;
  for(int pos = s_anchorRx->indexIn(text); pos > -1; pos = s_anchorRx->indexIn(text, pos+s_anchorRx->matchedLength())) {
    const QString cap1 = s_anchorRx->cap(1);
    if(cap1.contains(company)) {
      studios += s_anchorRx->cap(2).trimmed();
    }
  }

  entry_->setField(QStringLiteral("studio"), studios.join(FieldFormat::delimiterString()));
}

void IMDBFetcher::doPerson(const QString& str_, Tellico::Data::EntryPtr entry_,
                           const QString& imdbHeader_, const QString& fieldName_) {
  // only read if the field value is currently empty
  if(!entry_->field(fieldName_).isEmpty()) return;
  QRegExp br2Rx(QStringLiteral("<br[\\s/]*>\\s*<br[\\s/]*>"), Qt::CaseInsensitive);
  br2Rx.setMinimal(true);
  QRegExp divRx(QStringLiteral("<div\\s[^>]*class\\s*=\\s*\"(?:ipl-header__content|info|txt-block)\"[^>]*>(.*)</table"), Qt::CaseInsensitive);
  divRx.setMinimal(true);

  const QString name = QStringLiteral("/name/");
  QStringList people;
  for(int pos = str_.indexOf(divRx); pos > -1; pos = str_.indexOf(divRx, pos+divRx.matchedLength())) {
    const QString infoBlock = divRx.cap(1);
    if(infoBlock.contains(imdbHeader_, Qt::CaseInsensitive)) {
      int pos2 = s_anchorRx->indexIn(infoBlock);
      while(pos2 > -1) {
        if(s_anchorRx->cap(1).contains(name)) {
          people += s_anchorRx->cap(2).trimmed();
        }
        pos2 = s_anchorRx->indexIn(infoBlock, pos2+s_anchorRx->matchedLength());
      }
      break;
    }
  }
  if(!people.isEmpty()) {
    people.removeDuplicates();
    entry_->setField(fieldName_, people.join(FieldFormat::delimiterString()));
  }
}

void IMDBFetcher::doCast(const QString& str_, Tellico::Data::EntryPtr entry_, const QUrl& baseURL_) {
  // the extended cast list is on a separate page
  // that's usually a lot of people
  // but since it can be in billing order, the main actors might not
  // be in the short list
  auto idMatch = s_titleIdRx->match(baseURL_.path());
  Q_ASSERT(idMatch.hasMatch());
  QUrl castURL = baseURL_;
  castURL.setPath(QStringLiteral("/title/") + idMatch.captured(1) + QStringLiteral("/fullcredits"));

  // be quiet about failure and be sure to translate entities
  QPointer<KIO::StoredTransferJob> getJob = KIO::storedGet(castURL, KIO::NoReload, KIO::HideProgressInfo);
  configureJob(getJob);
  if(!getJob->exec()) {
    myWarning() << "...unable to read" << castURL;
  }
  const QString castPage = Tellico::decodeHTML(Tellico::fromHtmlData(getJob->data(), "UTF-8"));
#if 0
  myWarning() << "Remove debug from imdbfetcher.cpp (/tmp/testimdbcast.html)";
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
    QRegExp castAnchorRx(QStringLiteral("<a\\s+name\\s*=\\s*\"cast\""), Qt::CaseInsensitive);
    pos = castAnchorRx.indexIn(castText);
    if(pos < 0) {
      QRegExp tableClassRx(QStringLiteral("<table\\s+class\\s*=\\s*\"cast_list\""), Qt::CaseInsensitive);
      pos = tableClassRx.indexIn(castText);
      if(pos < 0) {
        // fragile, the word "cast" appears in the title, but need to find
        // the one right above the actual cast table
        // for TV shows, there's a link on the sidebar for "episodes case"
        // so need to not match that one
        const QString castEnd = data.cast + QStringLiteral("</");
        pos = castText.indexOf(castEnd, 0, Qt::CaseInsensitive);
        if(pos > 9) {
          // back up 9 places
          if(castText.midRef(pos-9, 9).startsWith(data.episodes)) {
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
  // loop until closing table tag
  int endPos = castText.indexOf(QStringLiteral("</table"), pos, Qt::CaseInsensitive);
  castText = castText.mid(pos, endPos-pos+1);

  QStringList actorList, characterList;
  QRegularExpression tdActorRx(QStringLiteral("<td>.*?<a href=\"/name.+?\".*?>(.+?)</a"),
                               QRegularExpression::DotMatchesEverythingOption);
  QRegularExpression tdCharRx(QStringLiteral("<td class=\"character\">(.+?)</"),
                              QRegularExpression::DotMatchesEverythingOption);

  QRegularExpressionMatchIterator i = tdActorRx.globalMatch(castText);
  while(i.hasNext()) {
    QRegularExpressionMatch match = i.next();
    actorList += match.captured(1).simplified();
  }
  i = tdCharRx.globalMatch(castText);
  while(i.hasNext()) {
    QRegularExpressionMatch match = i.next();
    characterList += match.captured(1).remove(*s_tagRx).simplified();
  }

  // sanity check
  while(characterList.length() > actorList.length()) {
    myDebug() << "Too many characters";
    characterList.removeLast();
  }
  while(characterList.length() < actorList.length()) {
    characterList += QString();
  }

  QStringList cast;
  cast.reserve(actorList.size());
  for(int i = 0; i < actorList.size(); ++i) {
    cast += actorList.at(i)
          + FieldFormat::columnDelimiterString()
          + characterList.at(i);
    if(cast.count() >= m_numCast) {
      break;
    }
  }

  if(cast.isEmpty()) {
    QRegExp tdRx(QStringLiteral("<td[^>]*>(.*)</td>"), Qt::CaseInsensitive);
    tdRx.setMinimal(true);

    QRegExp tdActorRx(QStringLiteral("<td\\s+[^>]*itemprop=\"actor\"[^>]*>(.*)</td>"), Qt::CaseInsensitive);
    tdActorRx.setMinimal(true);

    QRegExp tdCharRx(QStringLiteral("<td\\s+[^>]*class=\"character\"[^>]*>(.*)</td>"), Qt::CaseInsensitive);
    tdCharRx.setMinimal(true);

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
  }

  if(!cast.isEmpty()) {
    entry_->setField(QStringLiteral("cast"), cast.join(FieldFormat::rowDelimiterString()));
  }

  // also do other items from fullcredits page, like producer
  pos = castPage.indexOf(QLatin1String("id=\"producer\""), 0, Qt::CaseInsensitive);
  if(pos > -1) {
    int endPos = castPage.indexOf(QStringLiteral("</table"), pos, Qt::CaseInsensitive);
    if(endPos == -1) {
      endPos = castPage.length();
    }
    const QString prodText = castPage.mid(pos, endPos-pos+1);
    QRegExp tdCharRx(QStringLiteral("<td\\s+[^>]*class=\"credit\"[^>]*>(.*)</td>"));
    tdCharRx.setMinimal(true);

    QStringList producers;
    pos = s_anchorNameRx->indexIn(prodText);
    while(pos > -1 && producers.count() < IMDB_MAX_PERSON_COUNT) {
      const int pos2 = tdCharRx.indexIn(prodText, pos+1);
      const QString credit = tdCharRx.cap(1).trimmed();
      if(pos2 > -1 && (credit.startsWith(QStringLiteral("producer")) ||
                       credit.startsWith(QStringLiteral("co-producer")) ||
                       credit.startsWith(QStringLiteral("associate producer")))) {
        producers += s_anchorNameRx->cap(2).trimmed();
      }
      pos = s_anchorNameRx->indexIn(prodText, pos+1);
    }
    if(!producers.isEmpty()) {
      entry_->setField(QStringLiteral("producer"), producers.join(FieldFormat::delimiterString()));
    }
  }

  const QString director = QStringLiteral("director");
  // only try to read director if its already empty, which means it wasn't found on main page
  if(entry_->field(director).isEmpty()) {
    QStringList directors;
    pos = castPage.indexOf(QLatin1String("id=\"director\""), 0, Qt::CaseInsensitive);
    if(pos > -1 && directors.count() < IMDB_MAX_PERSON_COUNT) {
      int endPos = castPage.indexOf(QStringLiteral("</table"), pos, Qt::CaseInsensitive);
      if(endPos == -1) {
        endPos = castPage.length();
      }
      const QString midText = castPage.mid(pos, endPos-pos+1);
      pos = s_anchorNameRx->indexIn(midText);
      while(pos > -1) {
        directors += s_anchorNameRx->cap(2).trimmed();
        pos = s_anchorNameRx->indexIn(midText, pos+1);
      }
    }
    if(!directors.isEmpty()) {
      entry_->setField(director, directors.join(FieldFormat::delimiterString()));
    }
  }

  const QString writer = QStringLiteral("writer");
  // only try to read director if its already empty, which means it wasn't found on main page
  if(entry_->field(writer).isEmpty()) {
    QStringList writers;
    pos = castPage.indexOf(QLatin1String("id=\"writer\""), 0, Qt::CaseInsensitive);
    if(pos > -1 && writers.count() < IMDB_MAX_PERSON_COUNT) {
      int endPos = castPage.indexOf(QStringLiteral("</table"), pos, Qt::CaseInsensitive);
      if(endPos == -1) {
        endPos = castPage.length();
      }
      const QString midText = castPage.mid(pos, endPos-pos+1);
      pos = s_anchorNameRx->indexIn(midText);
      while(pos > -1) {
        writers += s_anchorNameRx->cap(2).trimmed();
        pos = s_anchorNameRx->indexIn(midText, pos+1);
      }
    }
    writers.removeDuplicates(); // some editor/writer duplicates
    if(!writers.isEmpty()) {
      entry_->setField(writer, writers.join(FieldFormat::delimiterString()));
    }
  }

  const QString composer = QStringLiteral("composer");
  // only try to read director if its already empty, which means it wasn't found on main page
  if(entry_->field(composer).isEmpty()) {
    QStringList composers;
    pos = castPage.indexOf(QLatin1String("id=\"composer\""), 0, Qt::CaseInsensitive);
    if(pos > -1 && composers.count() < IMDB_MAX_PERSON_COUNT) {
      int endPos = castPage.indexOf(QStringLiteral("</table"), pos, Qt::CaseInsensitive);
      if(endPos == -1) {
        endPos = castPage.length();
      }
      const QString midText = castPage.mid(pos, endPos-pos+1);
      pos = s_anchorNameRx->indexIn(midText);
      while(pos > -1) {
        composers += s_anchorNameRx->cap(2).trimmed();
        pos = s_anchorNameRx->indexIn(midText, pos+1);
      }
    }
    if(!composers.isEmpty()) {
      entry_->setField(composer, composers.join(FieldFormat::delimiterString()));
    }
  }
}

void IMDBFetcher::doRating(const QString& str_, Tellico::Data::EntryPtr entry_) {
  if(!optionalFields().contains(QStringLiteral("imdb-rating"))) {
    return;
  }

  QRegExp divRx(QStringLiteral("<div class=\"ipl-rating-star[\\s\"]+>(.*)</div"), Qt::CaseInsensitive);
  divRx.setMinimal(true);

  if(divRx.indexIn(str_) > -1) {
    if(!entry_->collection()->hasField(QStringLiteral("imdb-rating"))) {
      Data::FieldPtr f(new Data::Field(QStringLiteral("imdb-rating"), i18n("IMDb Rating"), Data::Field::Rating));
      f->setCategory(i18n("General"));
      f->setProperty(QStringLiteral("maximum"), QStringLiteral("10"));
      entry_->collection()->addField(f);
    }

    QString text = divRx.cap(0);
    text.remove(*s_tagRx);

    QRegExp ratingRx(QStringLiteral("\\s(\\d+.?\\d*)\\s"));
    if(ratingRx.indexIn(text) > -1) {
      bool ok;
      float value = ratingRx.cap(1).toFloat(&ok);
      if(!ok) {
        value = QLocale().toFloat(ratingRx.cap(1), &ok);
      }
      if(ok) {
        entry_->setField(QStringLiteral("imdb-rating"), QString::number(value));
      }
    }
  }
}

void IMDBFetcher::doCover(const QString& str_, Tellico::Data::EntryPtr entry_, const QUrl& baseURL_) {
  QRegExp imgRx(QStringLiteral("<img\\s+[^>]*src\\s*=\\s*\"([^\"]*)\"[^>]*>"), Qt::CaseInsensitive);
  imgRx.setMinimal(true);

  QRegExp posterRx(QStringLiteral("<a\\s+[^>]*name\\s*=\\s*\"poster\"[^>]*>(.*)</a>"), Qt::CaseInsensitive);
  posterRx.setMinimal(true);

  const QString cover = QStringLiteral("cover");

  int pos = posterRx.indexIn(str_);
  while(pos > -1) {
    if(posterRx.cap(1).contains(imgRx)) {
      QUrl u = QUrl(baseURL_).resolved(QUrl(imgRx.cap(1)));
      QString id = ImageFactory::addImage(u, true);
      if(!id.isEmpty()) {
        entry_->setField(cover, id);
        return;
      }
    }
    pos = posterRx.indexIn(str_, pos+posterRx.matchedLength());
  }

  // <link rel='image_src'
  const QRegularExpression linkRx(QStringLiteral("<link (.+?)>"));
  const QRegularExpression hrefRx(QStringLiteral("href=['\"](.+?)['\"]"));

  const QString src = QStringLiteral("image_src");
  auto i = linkRx.globalMatch(str_);
  while(i.hasNext()) {
    auto match = i.next();
    const auto tag = match.capturedRef(1);
    if(tag.contains(src, Qt::CaseInsensitive)) {
      auto hrefMatch = hrefRx.match(tag);
      if(hrefMatch.hasMatch()) {
        QUrl u = QUrl(baseURL_).resolved(QUrl(hrefMatch.captured(1)));
        // imdb uses amazon media image, where the img src "encodes" requests for image sizing and cropping
        // strip everything after the "@." and add UY64 to limit the max image dimension to 640
        int n = u.url().indexOf(QStringLiteral("@."));
        if(n > -1) {
          const QString newLink = u.url().left(n) + QStringLiteral("@.UY640.jpg");
          const QString id = ImageFactory::addImage(QUrl(newLink), true);
          if(!id.isEmpty()) {
            entry_->setField(cover, id);
            return;
          }
        }
        const QString id = ImageFactory::addImage(u, true);
        if(!id.isEmpty()) {
          entry_->setField(cover, id);
          return;
        }
      }
    }
  }

  // <img alt="poster"
  posterRx.setPattern(QStringLiteral("<img\\s+[^>]*alt\\s*=\\s*\"poster\"[^>]+src\\s*=\\s*\"([^\"]+)\""));
  pos = posterRx.indexIn(str_);
  if(pos > -1) {
    QUrl u = QUrl(baseURL_).resolved(QUrl(posterRx.cap(1)));
    QString id = ImageFactory::addImage(u, true);
    if(!id.isEmpty()) {
      entry_->setField(cover, id);
      return;
    }
  }

  // didn't find the cover, IMDb also used to put "cover" inside the url
  // cover is the img with the "cover" alt text
  pos = imgRx.indexIn(str_);
  while(pos > -1) {
    const QString url = imgRx.cap(0).toLower();
    if(url.contains(cover)) {
      QUrl u = QUrl(baseURL_).resolved(QUrl(imgRx.cap(1)));
      QString id = ImageFactory::addImage(u, true);
      if(!id.isEmpty()) {
        entry_->setField(cover, id);
        return;
      }
    }
    pos = imgRx.indexIn(str_, pos+imgRx.matchedLength());
  }
}

void IMDBFetcher::doLists2(const QString& str_, Tellico::Data::EntryPtr entry_) {
  QRegExp divInfoRx(QStringLiteral("<li role=\"presentation\".*>(.*)</div"), Qt::CaseInsensitive);
  divInfoRx.setMinimal(true);

  const LangData& data = langData(m_lang);

  QStringList genres, countries, langs, certs, tracks;
  for(int pos = divInfoRx.indexIn(str_); pos > -1; pos = divInfoRx.indexIn(str_, pos+divInfoRx.matchedLength())) {
    QString divMatch = divInfoRx.cap(1);
    int pos2 = 0;
    if((pos2=s_anchorRx->indexIn(divMatch)) == -1) continue;
    const QString text = divMatch.remove(*s_tagRx);
    QString value = s_anchorRx->cap(2);

    if(text.startsWith(data.genre)) {
      foreach(const QString& token, value.split(QLatin1Char('|'))) {
        genres << token.trimmed();
      }
    } else if(text.startsWith(data.language)) {
      foreach(const QString& token, value.split(QRegExp(QLatin1String("[,|]")))) {
        langs << token.trimmed();
      }
    } else if(text.startsWith(data.sound)) {
      foreach(const QString& token, value.split(QLatin1Char('|'))) {
        tracks << token.trimmed();
      }
    } else if(text.startsWith(data.country)) {
      countries << value;
    } else if(text.startsWith(data.certification)) {
      foreach(const QString& token, value.split(QLatin1Char('|'))) {
        certs << token.trimmed();
      }
    } else if(text.startsWith(data.color)) {
      // cut off any parentheses
      value = value.section(QLatin1Char('('), 0, 0).trimmed();
      // change "black and white" to "black & white"
      value.replace(QStringLiteral("and"), QStringLiteral("&"));
      if(value == data.color) {
        entry_->setField(QStringLiteral("color"), i18n("Color"));
      } else {
        entry_->setField(QStringLiteral("color"), value);
      }
    }
  }

  if(!genres.isEmpty()) {
    entry_->setField(QStringLiteral("genre"), genres.join(FieldFormat::delimiterString()));
  }
  if(!countries.isEmpty()) {
    entry_->setField(QStringLiteral("nationality"), countries.join(FieldFormat::delimiterString()));
  }
  if(!langs.isEmpty()) {
    entry_->setField(QStringLiteral("language"), langs.join(FieldFormat::delimiterString()));
  }
  if(!tracks.isEmpty()) {
    entry_->setField(QStringLiteral("audio-track"), tracks.join(FieldFormat::delimiterString()));
  }
  if(!certs.isEmpty()) {
    // first try to set default certification
    const QStringList& certsAllowed = entry_->collection()->fieldByName(QStringLiteral("certification"))->allowed();
    foreach(const QString& cert, certs) {
      QString country = cert.section(QLatin1Char(':'), 0, 0);
      QString lcert = cert.section(QLatin1Char(':'), 1, 1);
      if(lcert == QStringLiteral("Unrated")) {
        lcert = QLatin1Char('U');
      }
      lcert += QStringLiteral(" (") + country + QLatin1Char(')');
      if(certsAllowed.contains(lcert)) {
        entry_->setField(QStringLiteral("certification"), lcert);
        break;
      }
    }

    // now add new field for all certifications
    const QString allc = QStringLiteral("allcertification");
    if(optionalFields().contains(allc)) {
      Data::FieldPtr f = entry_->collection()->fieldByName(allc);
      if(!f) {
        f = new Data::Field(allc, i18n("Certifications"), Data::Field::Table);
        f->setFlags(Data::Field::AllowGrouped);
        entry_->collection()->addField(f);
      }
      entry_->setField(QStringLiteral("allcertification"), certs.join(FieldFormat::rowDelimiterString()));
    }
  }
}

// look at every anchor tag in the string
void IMDBFetcher::doLists(const QString& str_, Tellico::Data::EntryPtr entry_) {
  const QString genre = QStringLiteral("/Genres/");
  const QString genre2 = QStringLiteral("/genre/");
  const QString country = QStringLiteral("/country/");
  const QString lang = QStringLiteral("/language/");
  const QString colorInfo = QStringLiteral("colors=");
  const QString cert = QStringLiteral("certificates=");
  const QString soundMix = QStringLiteral("sound_mixes=");
  const QString year = QStringLiteral("/Years/");

  // if we reach faqs or user comments, we can stop
  const QString faqs = QStringLiteral("/faq");
  const QString users = QStringLiteral("/user/");
  // IMdb also has links with the word "sections" in them, remove that
  // for genres and nationalities

  int startPos = str_.indexOf(QStringLiteral("<div id=\"pagecontent\">"));
  if(startPos == -1) {
    startPos = 0;
  }

  QStringList genres, countries, langs, certs, tracks;
  for(int pos = s_anchorRx->indexIn(str_, startPos); pos > -1; pos = s_anchorRx->indexIn(str_, pos+s_anchorRx->matchedLength())) {
    const QString cap1 = s_anchorRx->cap(1);
    if(cap1.contains(genre) || cap1.contains(genre2)) {
      const QString g = s_anchorRx->cap(2);
      if(!g.contains(QStringLiteral(" section"), Qt::CaseInsensitive) &&
         !g.contains(QStringLiteral(" genre"), Qt::CaseInsensitive)) {
        // ignore "Most Popular by Genre"
        genres += g.trimmed();
      }
    } else if(cap1.contains(country)) {
      if(!s_anchorRx->cap(2).contains(QStringLiteral(" section"), Qt::CaseInsensitive)) {
        countries += s_anchorRx->cap(2).trimmed();
      }
    } else if(cap1.contains(lang) && !cap1.contains(QStringLiteral("contribute"))) {
      langs += s_anchorRx->cap(2).trimmed();
    } else if(cap1.contains(colorInfo)) {
      QString value = s_anchorRx->cap(2);
      // cut off any parentheses
      value = value.section(QLatin1Char('('), 0, 0).trimmed();
      // change "black and white" to "black & white"
      value.replace(QStringLiteral("and"), QStringLiteral("&"));
      entry_->setField(QStringLiteral("color"), value.trimmed());
    } else if(cap1.contains(cert)) {
      certs += s_anchorRx->cap(2).trimmed();
    } else if(cap1.contains(soundMix)) {
      tracks += s_anchorRx->cap(2).trimmed();
      // if year field wasn't set before, do it now
    } else if(entry_->field(QStringLiteral("year")).isEmpty() && cap1.contains(year)) {
      entry_->setField(QStringLiteral("year"), s_anchorRx->cap(2).trimmed());
    } else if((cap1.contains(faqs) || cap1.contains(users)) && !genres.isEmpty()) {
      break;
    }
  }

  // since we have multiple genre search strings
  genres.removeDuplicates();

  entry_->setField(QStringLiteral("genre"), genres.join(FieldFormat::delimiterString()));
  entry_->setField(QStringLiteral("nationality"), countries.join(FieldFormat::delimiterString()));
  entry_->setField(QStringLiteral("language"), langs.join(FieldFormat::delimiterString()));
  entry_->setField(QStringLiteral("audio-track"), tracks.join(FieldFormat::delimiterString()));
  if(!certs.isEmpty()) {
    // first try to set default certification
    const QStringList& certsAllowed = entry_->collection()->fieldByName(QStringLiteral("certification"))->allowed();
    foreach(const QString& cert, certs) {
      QString country = cert.section(QLatin1Char(':'), 0, 0);
      if(country == QStringLiteral("United States")) {
        country = QStringLiteral("USA");
      }
      QString lcert = cert.section(QLatin1Char(':'), 1, 1);
      if(lcert == QStringLiteral("Unrated")) {
        lcert = QLatin1Char('U');
      }
      lcert += QStringLiteral(" (") + country + QLatin1Char(')');
      if(certsAllowed.contains(lcert)) {
        entry_->setField(QStringLiteral("certification"), lcert);
        break;
      }
    }

    // now add new field for all certifications
    const QString allc = QStringLiteral("allcertification");
    if(optionalFields().contains(allc)) {
      Data::FieldPtr f = entry_->collection()->fieldByName(allc);
      if(!f) {
        f = new Data::Field(allc, i18n("Certifications"), Data::Field::Table);
        f->setFlags(Data::Field::AllowGrouped);
        entry_->collection()->addField(f);
      }
      entry_->setField(QStringLiteral("allcertification"), certs.join(FieldFormat::rowDelimiterString()));
    }
  }
}

void IMDBFetcher::doEpisodes(const QString& str_, Tellico::Data::EntryPtr entry_, const QUrl& baseURL_) {
  if(!str_.contains(QStringLiteral("video.tv_show"))) {
    // depend on meta data to indicate TV series
    // should include <meta property='og:type' content="video.tv_show" /> in the reference view
    return;
  }
  const QString episode = QStringLiteral("episode");
  if(!entry_->collection()->hasField(episode)) {
    entry_->collection()->addField(Data::Field::createDefaultField(Data::Field::EpisodeField));
  }

  int currentSeason = 1;
  int totalSeasons = -1;
  QStringList episodes;

  // the episode list is on a separate page
  auto idMatch = s_titleIdRx->match(baseURL_.path());
  Q_ASSERT(idMatch.hasMatch());

  const QRegularExpression episodeRx(QStringLiteral("itemtype=\"http://schema.org/TVEpisode\""));
  const QRegularExpression anchorEpisodeRx(QStringLiteral("<a href=\"/title/.+?_ep(\\d+)\"\\s+title=\"(.+?)\""),
                                           QRegularExpression::DotMatchesEverythingOption);
  QUrl episodeUrl = baseURL_;
  episodeUrl.setPath(QStringLiteral("/title/") + idMatch.captured(1) + QStringLiteral("/episodes/_ajax"));
  QUrlQuery q;
  // loop over the total number of seasons
  do {
    q.clear();
    q.addQueryItem(QLatin1String("season"), QString::number(currentSeason));
    episodeUrl.setQuery(q);

    QPointer<KIO::StoredTransferJob> getJob = KIO::storedGet(episodeUrl, KIO::NoReload, KIO::HideProgressInfo);
    configureJob(getJob);
    if(!getJob->exec()) {
      myWarning() << "...unable to read" << episodeUrl;
    }
    const QString episodeText = Tellico::fromHtmlData(getJob->data(), "UTF-8");
#if 0
    myWarning() << "Remove debug from imdbfetcher.cpp (/tmp/testimdbepisodes.html)";
    QFile f(QString::fromLatin1("/tmp/testimdbepisodes.html"));
    if(f.open(QIODevice::WriteOnly)) {
      QTextStream t(&f);
      t << castPage;
    }
    f.close();
#endif

    if(totalSeasons == -1) {
      // assume never more than 99 seasons, alternative is 4-digit years
      static const QRegularExpression optionRx(QStringLiteral("<option\\s+value=\"(\\d\\d?)\""));
      auto iOption = optionRx.globalMatch(episodeText);
      while(iOption.hasNext()) {
        auto optionMatch = iOption.next();
        const int value = optionMatch.captured(1).toInt();
        if(value > totalSeasons) totalSeasons = value;
      }
      totalSeasons = qMin(totalSeasons, IMDB_MAX_SEASON_COUNT);
     // ok if totalSeasons remains == -1
//      myDebug() << "Total seasons:" << totalSeasons;
    }

    auto i = episodeRx.globalMatch(episodeText);
    while(i.hasNext()) {
      auto match = i.next();
      auto anchorMatch = anchorEpisodeRx.match(episodeText, match.capturedEnd());
      if(anchorMatch.hasMatch()) {
//        myDebug() << "found episode" << anchorMatch.captured(1) << anchorMatch.captured(2);
        episodes << anchorMatch.captured(2) + FieldFormat::columnDelimiterString() +
                    QString::number(currentSeason) + FieldFormat::columnDelimiterString() +
                    anchorMatch.captured(1);
      }
    }
    ++currentSeason;
  } while (totalSeasons > 0 && currentSeason < totalSeasons);

  entry_->setField(episode, episodes.join(FieldFormat::rowDelimiterString()));
}

Tellico::Fetch::FetchRequest IMDBFetcher::updateRequest(Data::EntryPtr entry_) {
  QUrl link = QUrl::fromUserInput(entry_->field(QStringLiteral("imdb")));

  if(!link.isEmpty() && link.isValid()) {
    if(link.host() != m_host) {
//      myLog() << "switching hosts to " << m_host;
      link.setHost(m_host);
    }
    return FetchRequest(Fetch::Raw, link.url());
  }

  // optimistically try searching for title and rely on Collection::sameEntry() to figure things out
  const QString t = entry_->field(QStringLiteral("title"));
  if(!t.isEmpty()) {
    return FetchRequest(Fetch::Title, t);
  }
  return FetchRequest();
}

void IMDBFetcher::configureJob(QPointer<KIO::StoredTransferJob> job_) {
  KJobWidgets::setWindow(job_, GUI::Proxy::widget());
  switch(m_lang) {
    case EN:
      job_->addMetaData(QStringLiteral("Languages"), QStringLiteral("en-US")); break;
    case FR:
      job_->addMetaData(QStringLiteral("Languages"), QStringLiteral("fr-FR")); break;
    case ES:
      job_->addMetaData(QStringLiteral("Languages"), QStringLiteral("es-ES")); break;
    case DE:
      job_->addMetaData(QStringLiteral("Languages"), QStringLiteral("de-DE")); break;
    case IT:
      job_->addMetaData(QStringLiteral("Languages"), QStringLiteral("it-IT")); break;
    case PT:
      job_->addMetaData(QStringLiteral("Languages"), QStringLiteral("pt-PT")); break;
  }
}

QString IMDBFetcher::defaultName() {
  return i18n("Internet Movie Database");
}

QString IMDBFetcher::defaultIcon() {
  return favIcon("https://www.imdb.com");
}

//static
Tellico::StringHash IMDBFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("imdb")]             = i18n("IMDb Link");
  hash[QStringLiteral("imdb-rating")]      = i18n("IMDb Rating");
  hash[QStringLiteral("alttitle")]         = i18n("Alternative Titles");
  hash[QStringLiteral("allcertification")] = i18n("Certifications");
  hash[QStringLiteral("origtitle")]        = i18n("Original Title");
  hash[QStringLiteral("episode")]          = i18n("Episodes");
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

  QLabel* label = new QLabel(i18n("&Maximum cast: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_numCast = new QSpinBox(optionsWidget());
  m_numCast->setMaximum(99);
  m_numCast->setMinimum(0);
  m_numCast->setValue(IMDB_DEFAULT_CAST_SIZE);
#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
  void (QSpinBox::* textChanged)(const QString&) = &QSpinBox::valueChanged;
#else
  void (QSpinBox::* textChanged)(const QString&) = &QSpinBox::textChanged;
#endif
  connect(m_numCast, textChanged, this, &ConfigWidget::slotSetModified);
  l->addWidget(m_numCast, row, 1);
  QString w = i18n("The list of cast members may include many people. Set the maximum number returned from the search.");
  label->setWhatsThis(w);
  m_numCast->setWhatsThis(w);
  label->setBuddy(m_numCast);

  m_fetchImageCheck = new QCheckBox(i18n("Download cover &image"), optionsWidget());
  connect(m_fetchImageCheck, &QAbstractButton::clicked, this, &ConfigWidget::slotSetModified);
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
    m_numCast->setValue(fetcher_->m_numCast);
    m_fetchImageCheck->setChecked(fetcher_->m_fetchImages);
  } else { //defaults
    m_fetchImageCheck->setChecked(true);
  }
}

void IMDBFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  config_.writeEntry("Host", QString()); // clear old host entry
  config_.writeEntry("Max Cast", m_numCast->value());
  config_.writeEntry("Fetch Images", m_fetchImageCheck->isChecked());
}

QString IMDBFetcher::ConfigWidget::preferredName() const {
  return IMDBFetcher::langData(EN).siteTitle;
}

void IMDBFetcher::ConfigWidget::slotSiteChanged() {
  emit signalName(preferredName());
}
