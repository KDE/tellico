/***************************************************************************
    Copyright (C) 2023 Robby Stephenson <robby@periapsis.org>
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

#include "filmaffinityfetcher.h"
#include "../utils/guiproxy.h"
#include "../utils/string_utils.h"
#include "../collections/videocollection.h"
#include "../entry.h"
#include "../fieldformat.h"
#include "../core/filehandler.h"
#include "../images/imagefactory.h"
#include "../gui/combobox.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KIO/StoredTransferJob>
#include <KJobUiDelegate>
#include <KJobWidgets>

#include <QRegularExpression>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QSpinBox>
#include <QUrlQuery>
#include <QStandardPaths>

namespace {
  static const char* FILMAFFINITY_SEARCH_URL = "https://www.filmaffinity.com";
  static const uint FILMAFFINITY_DEFAULT_CAST_SIZE = 10;
}

using namespace Tellico;
using Tellico::Fetch::FilmAffinityFetcher;

FilmAffinityFetcher::FilmAffinityFetcher(QObject* parent_)
    : Fetcher(parent_), m_started(false), m_locale(ES), m_numCast(FILMAFFINITY_DEFAULT_CAST_SIZE) {
}

FilmAffinityFetcher::~FilmAffinityFetcher() {
}

// static
const FilmAffinityFetcher::LocaleData& FilmAffinityFetcher::localeData(int locale_) {
  Q_ASSERT(locale_ >= ES);
  Q_ASSERT(locale_ <  Last);
  static LocaleData dataVector[6] = {
    {
      QStringLiteral("es"),
      QStringLiteral("(Serie de TV)"),
      QString::fromUtf8("Año"),
      QStringLiteral("Título original"),
      QStringLiteral("País"),
      QString::fromUtf8("Duración"),
      QString::fromUtf8("Dirección"),
      QStringLiteral("Reparto"),
      QString::fromUtf8("Género"),
      QStringLiteral("Guion"),
      QStringLiteral("Historia:"),
      QString::fromUtf8("Compañías"),
      QStringLiteral("Distribuidora"),
      QStringLiteral("Emitida por:"),
      QString::fromUtf8("Música"),
      QStringLiteral("Sinopsis")
    },
    {
      QStringLiteral("us"),
      QStringLiteral("(TV Series)"),
      QStringLiteral("Year"),
      QStringLiteral("Original title"),
      QStringLiteral("Country"),
      QStringLiteral("Running time"),
      QStringLiteral("Director"),
      QStringLiteral("Cast"),
      QStringLiteral("Genre"),
      QStringLiteral("Screenwriter"),
      QStringLiteral("Story:"),
      QStringLiteral("Producer"),
      QStringLiteral("Distributor:"),
      QStringLiteral("Broadcast by:"),
      QStringLiteral("Music"),
      QStringLiteral("Synopsis")
    }
  };

  return dataVector[qBound(0, locale_, static_cast<int>(sizeof(dataVector)/sizeof(LocaleData)))];
}

QString FilmAffinityFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool FilmAffinityFetcher::canFetch(int type) const {
  return type == Data::Collection::Video;
}

bool FilmAffinityFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Title;
}

void FilmAffinityFetcher::readConfigHook(const KConfigGroup& config_) {
  const int locale = config_.readEntry("Locale", int(ES));
  m_locale = static_cast<Locale>(locale);
  m_numCast = config_.readEntry("Max Cast", FILMAFFINITY_DEFAULT_CAST_SIZE);
}

void FilmAffinityFetcher::search() {
  m_started = true;
  m_matches.clear();

  QUrl u(QString::fromLatin1(FILMAFFINITY_SEARCH_URL));
  u.setPath(QLatin1String("/") + localeData(m_locale).siteSlug + QLatin1String("/advsearch.php"));
  QString searchValue = request().value();
  QUrlQuery q;
  // extract the year from the end of the search string, accept the possible corner case of a movie
  // having some other year in the title?
  QRegularExpression yearRx(QStringLiteral("\\s(19|20)\\d\\d$"));
  auto match = yearRx.match(searchValue);
  if(match.hasMatch()) {
    searchValue.remove(match.captured());
    const auto& year = match.captured().simplified();
    q.addQueryItem(QStringLiteral("fromyear"), year);
    q.addQueryItem(QStringLiteral("toyear"), year);
  }
  q.addQueryItem(QStringLiteral("stext"), searchValue);

  switch(request().key()) {
    case Title:
      //q.addQueryItem(QStringLiteral("year"), QStringLiteral("yes"));
      q.addQueryItem(QStringLiteral("stype[]"), QLatin1String("title"));
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      stop();
      return;
  }
  u.setQuery(q);
  myLog() << "Reading" << u.toDisplayString();

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result, this, &FilmAffinityFetcher::slotComplete);
}

void FilmAffinityFetcher::stop() {
  if(!m_started) {
    return;
  }

  if(m_job) {
    m_job->kill();
    m_job = nullptr;
  }
  m_started = false;
  Q_EMIT signalDone(this);
}

void FilmAffinityFetcher::slotComplete(KJob*) {
  if(m_job->error()) {
    m_job->uiDelegate()->showErrorMessage();
    stop();
    return;
  }

  QByteArray data = m_job->data();
  if(data.isEmpty()) {
    myDebug() << "no data";
    stop();
    return;
  }

  const QString output = Tellico::decodeHTML(data);
#if 0
  myWarning() << "Remove debug from filmaffinityfetcher.cpp";
  QFile f(QStringLiteral("/tmp/test1.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << output;
  }
  f.close();
#endif

  // look for a specific div, with an href and title, sometime uses single-quote, sometimes double-quotes
  QRegularExpression resultRx(QStringLiteral("data-movie-id(.+?)mc-actions"),
                              QRegularExpression::DotMatchesEverythingOption);
  QRegularExpression titleRx(QStringLiteral("mc-title\">.*?<a.+?href=\"(.+?)\".+?>(.+?)</a"),
                             QRegularExpression::DotMatchesEverythingOption);
  // the year is within the title text as a 4-digit number, starting with 1 or 2
  static const QRegularExpression yearRx(QStringLiteral("mc-year\".*?>([12]\\d\\d\\d)</span"));
  static const QRegularExpression tagRx(QStringLiteral("<.+?>"));

  QString href, title, year;
  auto i = resultRx.globalMatch(output);
  while(i.hasNext() && m_started) {
    auto topMatch = i.next();
    auto anchorMatch = titleRx.match(topMatch.captured(1));
    if(anchorMatch.hasMatch()) {
      href = anchorMatch.captured(1);
      title = anchorMatch.captured(2).remove(tagRx).trimmed();
      auto yearMatch = yearRx.match(topMatch.captured(1));
      if(yearMatch.hasMatch()) {
        year = yearMatch.captured(1);
      } else {
        year.clear();
      }
    } else {
      href.clear();
    }
    if(!href.isEmpty()) {
      QUrl url(QString::fromLatin1(FILMAFFINITY_SEARCH_URL));
      url = url.resolved(QUrl(href));
//      myDebug() << url << title << year;
      FetchResult* r = new FetchResult(this, title, year);
      m_matches.insert(r->uid, url);
      Q_EMIT signalResultFound(r);
    }
  }

  // since the fetch is done, don't worry about holding the job pointer
  m_job = nullptr;
  stop();
}

Tellico::Data::EntryPtr FilmAffinityFetcher::fetchEntryHook(uint uid_) {
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

  const QString results = Tellico::decodeHTML(FileHandler::readDataFile(url, true));
  if(results.isEmpty()) {
    myDebug() << "no text results";
    return Data::EntryPtr();
  }

#if 0
  myDebug() << url.url();
  myWarning() << "Remove debug2 from filmaffinityfetcher.cpp";
  QFile f(QStringLiteral("/tmp/test-filmaffinity.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << results;
  }
  f.close();
#endif

  entry = parseEntry(results);
  if(!entry) {
    myDebug() << "error in processing entry";
    return Data::EntryPtr();
  }

  const QString fa = QStringLiteral("filmaffinity");
  if(optionalFields().contains(fa)) {
    Data::FieldPtr field(new Data::Field(fa, i18n("FilmAffinity Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    entry->collection()->addField(field);
    entry->setField(fa, url.url());
  }

  m_entries.insert(uid_, entry); // keep for later
  return entry;
}

Tellico::Data::EntryPtr FilmAffinityFetcher::parseEntry(const QString& str_) {
  Data::CollPtr coll(new Data::VideoCollection(true));
  Data::EntryPtr entry(new Data::Entry(coll));
  coll->addEntries(entry);

  const LocaleData& data = localeData(m_locale);

  QRegularExpression titleRx(QStringLiteral("<span itemprop=\"name\">(.+?)</span"));
  QRegularExpressionMatch match = titleRx.match(str_);
  if(match.hasMatch()) {
    // remove anything in parentheses
    QString title = match.captured(1).simplified();
    title.remove(data.tvSeries);
    title = title.trimmed();
    entry->setField(QStringLiteral("title"), title);
  }

  const QString origtitle = QStringLiteral("origtitle");
  QRegularExpression tagRx(QStringLiteral("<.+?>"));
  QRegularExpression spanRx(QStringLiteral("<span.*?>(.+?),*\\s*</span"));
  QRegularExpression divRx(QStringLiteral("<div [^>]*?class=\"name\"[^>]*?>(.+?)</div"));
  QRegularExpression defRx(QStringLiteral("<dt>(.+?)</dt>\\s*?<dd.*?>(.+?)</dd>"),
                           QRegularExpression::DotMatchesEverythingOption);
  QRegularExpressionMatchIterator i = defRx.globalMatch(str_);
  while(i.hasNext()) {
    auto match = i.next();
    const auto& term = match.captured(1);
    if(term == data.year) {
      entry->setField(QStringLiteral("year"), match.captured(2).trimmed());
    } else if(term == data.origTitle &&
              optionalFields().contains(origtitle)) {
      Data::FieldPtr f(new Data::Field(origtitle, i18n("Original Title")));
      f->setFormatType(FieldFormat::FormatTitle);
      coll->addField(f);
      // might have an aka in a span
      QString oTitle = match.captured(2);
      const int start = oTitle.indexOf(QLatin1String("<span"));
      if(start > -1) oTitle = oTitle.left(start);
      entry->setField(origtitle, oTitle.remove(tagRx).simplified());
    } else if(term == data.runningTime) {
      QRegularExpression timeRx(QStringLiteral("\\d+"));
      auto timeMatch = timeRx.match(match.captured(2));
      if(timeMatch.hasMatch()) {
        entry->setField(QStringLiteral("running-time"), timeMatch.captured());
      }
    } else if(term == data.country) {
      QRegularExpression countryRx(QStringLiteral("alt=\"(.+?)\""));
      auto countryMatch = countryRx.match(match.captured(2));
      if(countryMatch.hasMatch()) {
        entry->setField(QStringLiteral("nationality"), countryMatch.captured(1));
      }
    } else if(term == data.director) {
      QStringList directors;
      auto iSpan = spanRx.globalMatch(match.captured(2));
      while(iSpan.hasNext()) {
        auto spanMatch = iSpan.next();
        directors += spanMatch.captured(1).remove(tagRx).simplified();
      }
      if(!directors.isEmpty()) {
        entry->setField(QStringLiteral("director"), directors.join(FieldFormat::delimiterString()));
      }
    } else if(term == data.cast) {
      QStringList cast;
      const auto& captured = match.captured(2);
      // only read up to the hidden credits
      auto end = captured.indexOf(QLatin1String("hidden-credit"));
      if(end == -1) end = captured.indexOf(QLatin1String("see-more-cre"));
      if(end == -1) end = captured.size();
      auto iDiv = divRx.globalMatch(captured.left(end));
      while(iDiv.hasNext() && cast.size() < m_numCast) {
        auto spanMatch = iDiv.next();
        cast += spanMatch.captured(1).remove(tagRx).simplified();
      }
      if(!cast.isEmpty()) {
        entry->setField(QStringLiteral("cast"), cast.join(FieldFormat::rowDelimiterString()));
      }
    } else if(term == data.genre) {
      QStringList genres;
      auto iSpan = spanRx.globalMatch(match.captured(2));
      while(iSpan.hasNext()) {
        auto spanMatch = iSpan.next();
        genres += spanMatch.captured(1).remove(tagRx).simplified();
      }
      if(!genres.isEmpty()) {
        entry->setField(QStringLiteral("genre"), genres.join(FieldFormat::delimiterString()));
      }
    } else if(term == data.writer) {
      QStringList writers;
      const auto& captured = match.captured(2);
      // skip ahead to "Story"
      const auto start = captured.indexOf(data.story);
      auto iSpan = spanRx.globalMatch(captured.mid(qMax(0,start)));
      while(iSpan.hasNext()) {
        auto spanMatch = iSpan.next();
        writers += spanMatch.captured(1).remove(tagRx).simplified();
      }
      if(!writers.isEmpty()) {
        entry->setField(QStringLiteral("writer"), writers.join(FieldFormat::delimiterString()));
      }
    } else if(term == data.producer) {
      // producer seems to be all the studio, use distributor as the main
      QStringList studios;
      const auto& captured = match.captured(2);
      // skip ahead to "Story"
      const auto start1 = captured.indexOf(data.distributor);
      const auto start2 = captured.indexOf(data.broadcast);
      auto iSpan = spanRx.globalMatch(captured.mid(qMax(0,qMax(start1,start2))));
      while(iSpan.hasNext()) {
        auto spanMatch = iSpan.next();
        studios += spanMatch.captured(1).remove(tagRx).simplified();
      }
      if(!studios.isEmpty()) {
        entry->setField(QStringLiteral("studio"), studios.join(FieldFormat::delimiterString()));
      }
    } else if(term == data.music) {
      entry->setField(QStringLiteral("composer"), match.captured(2).remove(tagRx).trimmed());
    } else if(term == data.plot) {
      entry->setField(QStringLiteral("plot"), match.captured(2).trimmed());
    }
  }

  QString cover;
  QRegularExpression coverRx(QStringLiteral("<img\\s.*?itemprop=\"image\".+?src=\"(.+?)\".*?>"));
  match = coverRx.match(str_);
  if(match.hasMatch()) {
    cover = match.captured(1);
  } else {
    coverRx.setPattern(QStringLiteral("<meta property=\"og:image\" content=\"(.+?)\""));
    match = coverRx.match(str_);
    if(match.hasMatch()) {
      cover = match.captured(1);
    }
  }
  if(!cover.isEmpty()) {
//    myDebug() << "cover:" << cover;
    const QString id = ImageFactory::addImage(QUrl::fromUserInput(cover), true /* quiet */);
    if(id.isEmpty()) {
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    }
    // empty image ID is ok
    entry->setField(QStringLiteral("cover"), id);
  }

  return entry;
}

Tellico::Fetch::FetchRequest FilmAffinityFetcher::updateRequest(Data::EntryPtr entry_) {
  QString t = entry_->field(QStringLiteral("title"));
  if(!t.isEmpty()) {
    return FetchRequest(Fetch::Title, t);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* FilmAffinityFetcher::configWidget(QWidget* parent_) const {
  return new FilmAffinityFetcher::ConfigWidget(parent_);
}

QString FilmAffinityFetcher::defaultName() {
  return QStringLiteral("FilmAffinity");
}

QString FilmAffinityFetcher::defaultIcon() {
  return favIcon("https://www.filmaffinity.com");
}

Tellico::StringHash FilmAffinityFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("origtitle")] = i18n("Original Title");
  hash[QStringLiteral("filmaffinity")] = i18n("FilmAffinity Link");
  return hash;
}

FilmAffinityFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const FilmAffinityFetcher* fetcher_)
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
  m_numCast->setValue(FILMAFFINITY_DEFAULT_CAST_SIZE);
  void (QSpinBox::* textChanged)(const QString&) = &QSpinBox::textChanged;
  connect(m_numCast, textChanged, this, &ConfigWidget::slotSetModified);
  l->addWidget(m_numCast, row, 1);
  QString w = i18n("The list of cast members may include many people. Set the maximum number returned from the search.");
  label->setWhatsThis(w);
  m_numCast->setWhatsThis(w);
  label->setBuddy(m_numCast);

  label = new QLabel(i18n("Language: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_localeCombo = new GUI::ComboBox(optionsWidget());
  QIcon iconES(QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                      QStringLiteral("kf5/locale/countries/es/flag.png")));
  m_localeCombo->addItem(iconES, i18nc("Country", "Spain"), int(FilmAffinityFetcher::ES));
  QIcon iconUS(QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                      QStringLiteral("kf5/locale/countries/us/flag.png")));
  m_localeCombo->addItem(iconUS, i18nc("Country", "USA"), int(FilmAffinityFetcher::US));
  void (GUI::ComboBox::* activatedInt)(int) = &GUI::ComboBox::activated;
  connect(m_localeCombo, activatedInt, this, &ConfigWidget::slotSetModified);
  l->addWidget(m_localeCombo, row, 1);
  label->setBuddy(m_localeCombo);

  l->setRowStretch(++row, 10);

  addFieldsWidget(FilmAffinityFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());

  if(fetcher_) {
    m_localeCombo->setCurrentData(fetcher_->m_locale);
    m_numCast->setValue(fetcher_->m_numCast);
  }
}

void FilmAffinityFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  config_.writeEntry("Locale", m_localeCombo->currentData().toInt());
  config_.writeEntry("Max Cast", m_numCast->value());
}

QString FilmAffinityFetcher::ConfigWidget::preferredName() const {
  return FilmAffinityFetcher::defaultName();
}
