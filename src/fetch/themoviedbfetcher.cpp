/***************************************************************************
    Copyright (C) 2009 Robby Stephenson <robby@periapsis.org>
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

#include "themoviedbfetcher.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../gui/combobox.h"
#include "../tellico_utils.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <KConfigGroup>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QDomDocument>
#include <QTextCodec>

namespace {
  static const int THEMOVIEDB_MAX_RETURNS_TOTAL = 20;
  static const char* THEMOVIEDB_API_URL = "http://api.themoviedb.org";
  static const char* THEMOVIEDB_API_VERSION = "2.1";
  static const char* THEMOVIEDB_API_KEY = "919890b4128d33c729dc368209ece555";
}

using namespace Tellico;
using Tellico::Fetch::TheMovieDBFetcher;

TheMovieDBFetcher::TheMovieDBFetcher(QObject* parent_)
    : XMLFetcher(parent_)
    , m_locale(QLatin1String("en"))
    , m_needPersonId(false)
    , m_apiKey(QLatin1String(THEMOVIEDB_API_KEY)) {
  setLimit(THEMOVIEDB_MAX_RETURNS_TOTAL);
  setXSLTFilename(QLatin1String("tmdb2tellico.xsl"));
}

TheMovieDBFetcher::~TheMovieDBFetcher() {
}

QString TheMovieDBFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

// http://api.themoviedb.org/2.1/terms-of-use
QString TheMovieDBFetcher::attribution() const {
  return QString::fromLatin1("This product uses the TMDb API but is not endorsed or certified by TMDb.");
}

bool TheMovieDBFetcher::canFetch(int type) const {
  return type == Data::Collection::Video;
}

void TheMovieDBFetcher::readConfigHook(const KConfigGroup& config_) {
  QString k = config_.readEntry("API Key", THEMOVIEDB_API_KEY);
  if(!k.isEmpty()) {
    m_apiKey = k;
  }
  k = config_.readEntry("Locale", "en");
  if(!k.isEmpty()) {
    m_locale = k.toLower();
  }
}

KUrl TheMovieDBFetcher::searchUrl() {
  if(m_apiKey.isEmpty()) {
    myDebug() << "empty API key";
    return KUrl();
  }

  KUrl u(THEMOVIEDB_API_URL);
  u.setPath(QLatin1String(THEMOVIEDB_API_VERSION));
  QString queryPath;

  switch(request().key) {
    case Title:
      queryPath = QString::fromLatin1("/Movie.search/%1/xml/%2/%3").arg(m_locale, m_apiKey, request().value);
      break;

    case Person:
      queryPath = QString::fromLatin1("/Person.search/%1/xml/%2/%3").arg(m_locale, m_apiKey, request().value);
      m_needPersonId = true;
      break;

    default:
      myWarning() << "key not recognized:" << request().key;
      return KUrl();
  }

  u.addPath(queryPath);

//  myDebug() << "url:" << u.url();
  return u;
}

void TheMovieDBFetcher::resetSearch() {
  m_needPersonId = false;
  m_total = -1;
}

void TheMovieDBFetcher::parseData(QByteArray& data_) {
  if(m_total == -1) {
    QDomDocument dom;
    if(!dom.setContent(data_, false)) {
      myWarning() << "server did not return valid XML.";
      return;
    }
    // total is /resp/fetchresults/@numResults
    QDomNode n = dom.documentElement().namedItem(QLatin1String("opensearch:totalResults"));
    QDomElement e = n.toElement();
    if(!e.isNull()) {
      m_total = e.text().toInt();
//      myDebug() << "total = " << m_total;
    }

    if(m_needPersonId) {
      m_total = -1;
      m_needPersonId = false;

      // for now, find the person with the highest popularity and "lowest" score
      QDomNode finalPerson;
      int bestTotal = 0;

      QDomNode person = dom.documentElement().namedItem(QLatin1String("people"))
                                             .namedItem(QLatin1String("person"));
      while(!person.isNull()) {
        const int pop = person.namedItem(QLatin1String("popularity")).toElement().text().toInt();
        const int score = person.namedItem(QLatin1String("score")).toElement().text().toInt();
        const int total = 100*pop + 100-score;
        if(total > bestTotal) {
          bestTotal = total;
          finalPerson = person;
          myDebug() << "New total:" << total;
        }
        person = person.nextSibling();
      }
      n = finalPerson.namedItem(QLatin1String("id"));
      e = n.toElement();
      if(e.isNull()) {
        myWarning() << "no person id found";
        stop();
        return;
      }
      KUrl u(THEMOVIEDB_API_URL);
      u.setPath(QLatin1String(THEMOVIEDB_API_VERSION));
      u.addPath(QString::fromLatin1("/Person.getInfo/%1/xml/%2/%3").arg(m_locale, m_apiKey, e.text()));
//      myDebug() << u.url();
      // quiet
      data_ = FileHandler::readXMLFile(u, true).toUtf8();
    }
  }

  // not sure how to specify start in the REST url
  //  m_hasMoreResults = m_start <= m_total;
}

Tellico::Data::EntryPtr TheMovieDBFetcher::fetchEntryHookData(Data::EntryPtr entry_) {
  Q_ASSERT(entry_);

  QString release = entry_->field(QLatin1String("tmdb-id"));
  if(release.isEmpty()) {
    return entry_;
  }

  KUrl u(THEMOVIEDB_API_URL);
  u.setPath(QLatin1String(THEMOVIEDB_API_VERSION));
  u.addPath(QString::fromLatin1("/Movie.getInfo/%1/xml/%2/%3").arg(m_locale, m_apiKey, release));

  // quiet
  QString output = FileHandler::readXMLFile(u, true);
#if 0
  myWarning() << "Remove output debug from themoviedbfetcher.cpp";
  QFile f(QLatin1String("/tmp/test2.xml"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << output;
  }
  f.close();
#endif

  Import::TellicoImporter imp(xsltHandler()->applyStylesheet(output));
  Data::CollPtr coll = imp.collection();
  if(!coll) {
    myWarning() << "no collection pointer";
    return entry_;
  }

  if(coll->entryCount() > 1) {
    myDebug() << "weird, more than one entry found";
  }

  // don't want to include id
  coll->removeField(QLatin1String("tmdb-id"));
  return coll->entries().front();
}

Tellico::Fetch::FetchRequest TheMovieDBFetcher::updateRequest(Data::EntryPtr entry_) {
  QString title = entry_->field(QLatin1String("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* TheMovieDBFetcher::configWidget(QWidget* parent_) const {
  return new TheMovieDBFetcher::ConfigWidget(parent_, this);
}

QString TheMovieDBFetcher::defaultName() {
  return QLatin1String("TheMovieDB.org");
}

QString TheMovieDBFetcher::defaultIcon() {
  return favIcon("http://www.themoviedb.org");
}

Tellico::StringHash TheMovieDBFetcher::allOptionalFields() {
  StringHash hash;
  hash[QLatin1String("tmdb")] = i18n("TMDb Link");
  hash[QLatin1String("imdb")] = i18n("IMDb Link");
  hash[QLatin1String("alttitle")] = i18n("Alternative Titles");
  return hash;
}

TheMovieDBFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const TheMovieDBFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;

  QLabel* al = new QLabel(i18n("Registration is required for accessing the %1 data source. "
                               "If you agree to the terms and conditions, <a href='%2'>sign "
                               "up for an account</a>, and enter your information below.",
                                TheMovieDBFetcher::defaultName(),
                                QLatin1String("http://api.themoviedb.org")),
                          optionsWidget());
  al->setOpenExternalLinks(true);
  al->setWordWrap(true);
  ++row;
  l->addWidget(al, row, 0, 1, 2);
  // richtext gets weird with size
  al->setMinimumWidth(al->sizeHint().width());

  QLabel* label = new QLabel(i18n("Access key: "), optionsWidget());
  l->addWidget(label, ++row, 0);

  m_apiKeyEdit = new KLineEdit(optionsWidget());
  connect(m_apiKeyEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_apiKeyEdit, row, 1);
  QString w = i18n("The default Tellico key may be used, but searching may fail due to reaching access limits.");
  label->setWhatsThis(w);
  m_apiKeyEdit->setWhatsThis(w);
  label->setBuddy(m_apiKeyEdit);

  label = new QLabel(i18n("Language: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_langCombo = new GUI::ComboBox(optionsWidget());
  m_langCombo->addItem(i18nc("Language", "English"), QLatin1String("en"));
  m_langCombo->addItem(i18nc("Language", "French"), QLatin1String("fr"));
  m_langCombo->addItem(i18nc("Language", "German"), QLatin1String("de"));
  m_langCombo->addItem(i18nc("Language", "Spanish"), QLatin1String("es"));
  connect(m_langCombo, SIGNAL(activated(int)), SLOT(slotSetModified()));
  connect(m_langCombo, SIGNAL(activated(int)), SLOT(slotLangChanged()));
  l->addWidget(m_langCombo, row, 1);
  label->setBuddy(m_langCombo);

  l->setRowStretch(++row, 10);

  // now add additional fields widget
  addFieldsWidget(TheMovieDBFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());

  if(fetcher_) {
    // only show the key if it is not the default Tellico one...
    // that way the user is prompted to apply for their own
    if(fetcher_->m_apiKey != QLatin1String(THEMOVIEDB_API_KEY)) {
      m_apiKeyEdit->setText(fetcher_->m_apiKey);
    }
    m_langCombo->setCurrentData(fetcher_->m_locale);
  }
}

void TheMovieDBFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  const QString apiKey = m_apiKeyEdit->text().trimmed();
  if(!apiKey.isEmpty()) {
    config_.writeEntry("API Key", apiKey);
  }
  const QString lang = m_langCombo->currentData().toString();
  config_.writeEntry("Locale", lang);
}

QString TheMovieDBFetcher::ConfigWidget::preferredName() const {
  return i18n("TheMovieDB (%1)", m_langCombo->currentText());
}

void TheMovieDBFetcher::ConfigWidget::slotLangChanged() {
  emit signalName(preferredName());
}

#include "themoviedbfetcher.moc"
