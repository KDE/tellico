/***************************************************************************
    Copyright (C) 2019 Robby Stephenson <robby@periapsis.org>
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

#include "comicvinefetcher.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../utils/objvalue.h"
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
#include <QJsonDocument>
#include <QJsonObject>

namespace {
  static const int COMICVINE_MAX_RETURNS_TOTAL = 20;
  static const char* COMICVINE_API_URL = "https://comicvine.gamespot.com/api";
  static const char* COMICVINE_API_KEY = "6a0fd2ea457262511a7e39092c19ccf4b0877e4b3e58593ded8c162f1021b7d3b68f0c3a4776fcca1d2f3000f690043690f5172fcaa95d38dab998fbcbf3d5b78fea583d1c25556453313602eb8e99af";
}

using namespace Tellico;
using Tellico::Fetch::ComicVineFetcher;

ComicVineFetcher::ComicVineFetcher(QObject* parent_)
    : XMLFetcher(parent_)
    , m_total(-1) {
  setLimit(COMICVINE_MAX_RETURNS_TOTAL);
  setXSLTFilename(QStringLiteral("comicvine2tellico.xsl"));
  m_apiKey = Tellico::reverseObfuscate(COMICVINE_API_KEY);
}

ComicVineFetcher::~ComicVineFetcher() {
}

QString ComicVineFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

QString ComicVineFetcher::attribution() const {
  return TC_I18N3(providedBy, QLatin1String("https://comicvine.gamespot.com"), QLatin1String("Comic Vine"));
}

bool ComicVineFetcher::canFetch(int type) const {
  return type == Data::Collection::ComicBook;
}

void ComicVineFetcher::readConfigHook(const KConfigGroup& config_) {
  QString k = config_.readEntry("API Key", Tellico::reverseObfuscate(COMICVINE_API_KEY));
  if(!k.isEmpty()) {
    m_apiKey = k;
  }
}

void ComicVineFetcher::resetSearch() {
  m_total = -1;
}

QUrl ComicVineFetcher::searchUrl() {
  QUrl u(QString::fromLatin1(COMICVINE_API_URL));
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("format"), QStringLiteral("xml"));
  q.addQueryItem(QStringLiteral("api_key"), m_apiKey);

  switch(request().key()) {
    case Keyword:
      u.setPath(u.path() + QStringLiteral("/search/"));
      q.addQueryItem(QStringLiteral("query"), request().value());
      q.addQueryItem(QStringLiteral("resources"), QStringLiteral("issue"));
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      return QUrl();
  }
  u.setQuery(q);

//  myDebug() << "url: " << u.url();
  return u;
}

void ComicVineFetcher::parseData(QByteArray& data_) {
  Q_UNUSED(data_);
}

Tellico::Data::EntryPtr ComicVineFetcher::fetchEntryHookData(Data::EntryPtr entry_) {
  Q_ASSERT(entry_);

  const QString url = entry_->field(QStringLiteral("comicvine-api"));
  if(url.isEmpty()) {
    myDebug() << "no comicvine api url found";
    return entry_;
  }

  QUrl u(url);
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("format"), QStringLiteral("xml"));
  q.addQueryItem(QStringLiteral("api_key"), m_apiKey);
  u.setQuery(q);
//  myDebug() << "url: " << u;

  // quiet
  QString output = FileHandler::readXMLFile(u, true);

#if 0
  myWarning() << "Remove output debug from comicvinefetcher.cpp";
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
  if(!coll || coll->entryCount() == 0) {
    myWarning() << "no collection pointer";
    return entry_;
  }

  if(coll->entryCount() > 1) {
    myDebug() << "weird, more than one entry found";
  }

  // grab the publisher from the volume link
  const QString volUrl = entry_->field(QStringLiteral("comicvine-volume-api"));
  if(!volUrl.isEmpty()) {
    QUrl vu(volUrl);
    // easier to use JSON here
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    q.addQueryItem(QStringLiteral("api_key"), m_apiKey);
    vu.setQuery(q);
//    myDebug() << "volume url: " << vu;

    QByteArray data = FileHandler::readDataFile(vu, true /* quiet */);
#if 0
    myWarning() << "Remove JSON output debug from comicvinefetcher.cpp";
    QFile f2(QStringLiteral("/tmp/test2.json"));
    if(f2.open(QIODevice::WriteOnly)) {
      QTextStream t(&f2);
      t << data;
    }
    f2.close();
#endif
    QJsonDocument doc = QJsonDocument::fromJson(data);
    const QString pub = objValue(doc.object(), "results", "publisher", "name");
    if(!pub.isEmpty()) {
      Data::EntryPtr e = coll->entries().front();
      if(e) {
        e->setField(QStringLiteral("publisher"), pub);
      }
    }
  }

  // don't want to include api link
  coll->removeField(QStringLiteral("comicvine-api"));
  coll->removeField(QStringLiteral("comicvine-volume-api"));
  return coll->entries().front();
}

Tellico::Fetch::FetchRequest ComicVineFetcher::updateRequest(Data::EntryPtr entry_) {
  QString title = entry_->field(QStringLiteral("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Keyword, title);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* ComicVineFetcher::configWidget(QWidget* parent_) const {
  return new ComicVineFetcher::ConfigWidget(parent_, this);
}

QString ComicVineFetcher::defaultName() {
  return QStringLiteral("Comic Vine");
}

QString ComicVineFetcher::defaultIcon() {
  return favIcon("https://comicvine.gamespot.com");
}

Tellico::StringHash ComicVineFetcher::allOptionalFields() {
  StringHash hash;
//  hash[QStringLiteral("colorist")]  = i18n("Colorist");
  hash[QStringLiteral("comicvine")] = i18n("Comic Vine Link");
  return hash;
}

ComicVineFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const ComicVineFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;

  QLabel* al = new QLabel(i18n("Registration is required for accessing this data source. "
                               "If you agree to the terms and conditions, <a href='%1'>sign "
                               "up for an account</a>, and enter your information below.",
                                QLatin1String("https://comicvine.gamespot.com/api/")),
                          optionsWidget());
  al->setOpenExternalLinks(true);
  al->setWordWrap(true);
  ++row;
  l->addWidget(al, row, 0, 1, 2);
  // richtext gets weird with size
  al->setMinimumWidth(al->sizeHint().width());
  al->setMinimumHeight(al->sizeHint().height());

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
  addFieldsWidget(ComicVineFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());

  if(fetcher_) {
    // only show the key if it is not the default Tellico one...
    // that way the user is prompted to apply for their own
    if(fetcher_->m_apiKey != QLatin1String(COMICVINE_API_KEY)) {
      m_apiKeyEdit->setText(fetcher_->m_apiKey);
    }
  }
}

void ComicVineFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  QString apiKey = m_apiKeyEdit->text().trimmed();
  if(!apiKey.isEmpty()) {
    config_.writeEntry("API Key", apiKey);
  }
}

QString ComicVineFetcher::ConfigWidget::preferredName() const {
  return ComicVineFetcher::defaultName();
}
