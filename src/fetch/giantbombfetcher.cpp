/***************************************************************************
    Copyright (C) 2010 Robby Stephenson <robby@periapsis.org>
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

#include "giantbombfetcher.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../utils/guiproxy.h"
#include "../utils/string_utils.h"
#include "../core/tellico_strings.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QDomDocument>
#include <QUrlQuery>

namespace {
  static const int GIANTBOMB_MAX_RETURNS_TOTAL = 20;
  static const char* GIANTBOMB_API_URL = "https://www.giantbomb.com/api";
  static const char* GIANTBOMB_API_KEY = "291bfe4b2d77a460e67dd8f90c1e7e56c3e4f05a";
}

using namespace Tellico;
using Tellico::Fetch::GiantBombFetcher;

GiantBombFetcher::GiantBombFetcher(QObject* parent_)
    : XMLFetcher(parent_)
    , m_total(-1)
    , m_apiKey(QLatin1String(GIANTBOMB_API_KEY)) {
  setLimit(GIANTBOMB_MAX_RETURNS_TOTAL);
  setXSLTFilename(QStringLiteral("giantbomb2tellico.xsl"));
}

GiantBombFetcher::~GiantBombFetcher() {
}

QString GiantBombFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

QString GiantBombFetcher::attribution() const {
  return TC_I18N3(providedBy, QLatin1String("https://giantbomb.com"), QLatin1String("Giant Bomb"));
}

bool GiantBombFetcher::canFetch(int type) const {
  return type == Data::Collection::Game;
}

void GiantBombFetcher::readConfigHook(const KConfigGroup& config_) {
  QString k = config_.readEntry("API Key", GIANTBOMB_API_KEY);
  if(!k.isEmpty()) {
    m_apiKey = k;
  }
}

void GiantBombFetcher::resetSearch() {
  m_total = -1;
}

QUrl GiantBombFetcher::searchUrl() {
  QUrl u(QString::fromLatin1(GIANTBOMB_API_URL));
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("format"), QStringLiteral("xml"));
  q.addQueryItem(QStringLiteral("api_key"), m_apiKey);

  switch(request().key()) {
    case Keyword:
      u.setPath(u.path() + QStringLiteral("/search"));
      q.addQueryItem(QStringLiteral("query"), request().value());
      q.addQueryItem(QStringLiteral("resources"), QStringLiteral("game"));
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      return QUrl();
  }
  u.setQuery(q);

//  myDebug() << "url: " << u.url();
  return u;
}

void GiantBombFetcher::parseData(QByteArray& data_) {
  Q_UNUSED(data_);
#if 0
  if(m_total == -1) {
    QDomDocument dom;
    if(!dom.setContent(data, false)) {
      myWarning() << "server did not return valid XML.";
      return;
    }
    // total is /resp/fetchresults/@numResults
    QDomNode n = dom.documentElement().namedItem(QLatin1String("resp"))
                                      .namedItem(QLatin1String("fetchresults"));
    QDomElement e = n.toElement();
    if(!e.isNull()) {
      m_total = e.attribute(QLatin1String("numResults")).toInt();
      myDebug() << "total = " << m_total;
    }
  }
  m_start = m_entries.count() + 1;
  // not sure how to specify start in the REST url
  //  m_hasMoreResults = m_start <= m_total;
#endif
}

Tellico::Data::EntryPtr GiantBombFetcher::fetchEntryHookData(Data::EntryPtr entry_) {
  Q_ASSERT(entry_);

  const QString id = entry_->field(QStringLiteral("giantbomb-id"));
  if(id.isEmpty()) {
    myDebug() << "no giantbomb id found";
    return entry_;
  }

  QUrl u(QString::fromLatin1(GIANTBOMB_API_URL));
  u.setPath(u.path() + QStringLiteral("/game/%1/").arg(id));
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("format"), QStringLiteral("xml"));
  q.addQueryItem(QStringLiteral("api_key"), m_apiKey);
  u.setQuery(q);
//  myDebug() << "url: " << u;

  // quiet
  QString output = FileHandler::readXMLFile(u, true);

#if 0
  myWarning() << "Remove output debug from giantbombfetcher.cpp";
  QFile f(QStringLiteral("/tmp/test2.xml"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << output;
  }
  f.close();
#endif

  Import::TellicoImporter imp(xsltHandler()->applyStylesheet(output));
  // be quiet when loading images
  imp.setOptions(imp.options() ^ Import::ImportShowImageErrors);
  Data::CollPtr coll = imp.collection();
//  getTracks(entry);
  if(!coll) {
    myWarning() << "no collection pointer";
    return entry_;
  }

  if(coll->entryCount() > 1) {
    myDebug() << "weird, more than one entry found";
  }

  // don't want to include id
  coll->removeField(QStringLiteral("giantbomb-id"));
  return coll->entries().front();
}

Tellico::Fetch::FetchRequest GiantBombFetcher::updateRequest(Data::EntryPtr entry_) {
  QString title = entry_->field(QStringLiteral("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Keyword, title);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* GiantBombFetcher::configWidget(QWidget* parent_) const {
  return new GiantBombFetcher::ConfigWidget(parent_, this);
}

QString GiantBombFetcher::defaultName() {
  return QStringLiteral("Giant Bomb");
}

QString GiantBombFetcher::defaultIcon() {
  return favIcon("http://www.giantbomb.com");
}

Tellico::StringHash GiantBombFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("giantbomb")] = i18n("GiantBomb Link");
  hash[QStringLiteral("pegi")] = i18n("PEGI Rating");
  return hash;
}

GiantBombFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const GiantBombFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;

  QLabel* al = new QLabel(i18n("Registration is required for accessing this data source. "
                               "If you agree to the terms and conditions, <a href='%1'>sign "
                               "up for an account</a>, and enter your information below.",
                                QLatin1String("http://api.giantbomb.com")),
                          optionsWidget());
  al->setOpenExternalLinks(true);
  al->setWordWrap(true);
  ++row;
  l->addWidget(al, row, 0, 1, 2);
  // richtext gets weird with size
  al->setMinimumWidth(al->sizeHint().width());

  QLabel* label = new QLabel(i18n("Access key: "), optionsWidget());
  l->addWidget(label, ++row, 0);

  m_apiKeyEdit = new QLineEdit(optionsWidget());
  connect(m_apiKeyEdit, &QLineEdit::textChanged, this, &ConfigWidget::slotSetModified);
  l->addWidget(m_apiKeyEdit, row, 1);
  QString w = i18n("The default Tellico key may be used, but searching may fail due to reaching access limits.");
  label->setWhatsThis(w);
  m_apiKeyEdit->setWhatsThis(w);
  label->setBuddy(m_apiKeyEdit);

  l->setRowStretch(++row, 10);

  // now add additional fields widget
  addFieldsWidget(GiantBombFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());

  if(fetcher_) {
    // only show the key if it is not the default Tellico one...
    // that way the user is prompted to apply for their own
    if(fetcher_->m_apiKey != QLatin1String(GIANTBOMB_API_KEY)) {
      m_apiKeyEdit->setText(fetcher_->m_apiKey);
    }
  }
}

void GiantBombFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  QString apiKey = m_apiKeyEdit->text().trimmed();
  if(!apiKey.isEmpty()) {
    config_.writeEntry("API Key", apiKey);
  }
}

QString GiantBombFetcher::ConfigWidget::preferredName() const {
  return GiantBombFetcher::defaultName();
}
