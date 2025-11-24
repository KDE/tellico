/***************************************************************************
    Copyright (C) 2014-2025 Robby Stephenson <robby@periapsis.org>
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

#include "abstractbggfetcher.h"
#include "boardgamegeekfetcher.h"
#include "rpggeekfetcher.h"
#include "videogamegeekfetcher.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../utils/guiproxy.h"
#include "../utils/xmlhandler.h"
#include "../utils/string_utils.h"
#include "../core/tellico_strings.h"
#include "../gui/combobox.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>
#include <KIO/StoredTransferJob>
#include <KJobWidgets>

#include <QLabel>
#include <QLineEdit>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QUrlQuery>
#include <QDomDocument>

namespace {
  // a lot of overlap with boardgamegeekimporter.h
  static const int BGG_MAX_RETURNS_TOTAL = 10;
  static const char* BGG_SEARCH_URL  = "https://boardgamegeek.com/xmlapi2/search";
  static const char* BGG_THING_URL  = "https://boardgamegeek.com/xmlapi2/thing";
}

using Tellico::Fetch::AbstractBGGFetcher;

AbstractBGGFetcher::AbstractBGGFetcher(QObject* parent_)
    : XMLFetcher(parent_), m_imageSize(SmallImage) {
  setLimit(BGG_MAX_RETURNS_TOTAL);
  setXSLTFilename(QStringLiteral("boardgamegeek2tellico.xsl"));
}

AbstractBGGFetcher::~AbstractBGGFetcher() = default;

QString AbstractBGGFetcher::source() const {
  auto defaultName = [](int t) {
    switch(t) {
      case BoardGameGeek: return BoardGameGeekFetcher::defaultName();
      case RPGGeek:       return RPGGeekFetcher::defaultName();
      case VideoGameGeek: return VideoGameGeekFetcher::defaultName();
      default: myWarning() << "BGG warning for source()"; return QString();
    }
  };
  return m_name.isEmpty() ? defaultName(type()) : m_name;
}

// https://boardgamegeek.com/wiki/page/XML_API_Terms_of_Use
QString AbstractBGGFetcher::attribution() const {
  return TC_I18N3(providedBy, QLatin1String("https://boardgamegeek.com"), QLatin1String("BoardGameGeek"));
}

bool AbstractBGGFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Title || k == Keyword;
}

QUrl AbstractBGGFetcher::searchUrl() {
  QUrl u(QString::fromLatin1(BGG_SEARCH_URL));

  QUrlQuery q;
  switch(request().key()) {
    case Title:
      q.addQueryItem(QStringLiteral("query"), request().value());
      q.addQueryItem(QStringLiteral("type"), bggType());
      q.addQueryItem(QStringLiteral("exact"), QStringLiteral("1"));
      break;

    case Keyword:
      q.addQueryItem(QStringLiteral("query"), request().value());
      q.addQueryItem(QStringLiteral("type"), bggType());
      break;

    case Raw:
      u.setUrl(QLatin1String(BGG_THING_URL));
      q.addQueryItem(QStringLiteral("id"), request().value());
      // there's an evident bug where the videogameexpansion type is no longer used
      // but some ids still identify as expansions
      if(type() == VideoGameGeek) {
        q.addQueryItem(QStringLiteral("type"), QStringLiteral("videogame,videogameexpansion"));
      } else {
        q.addQueryItem(QStringLiteral("type"), bggType());
      }
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      return QUrl();
  }
  u.setQuery(q);

//  myDebug() << "url: " << u.url();
  return u;
}

void AbstractBGGFetcher::doSearchHook(KIO::Job* job_) {
  if(m_apiKey.isEmpty()) {
    myLog() << "Authorization token required to access BGG data source";
  } else {
    job_->addMetaData(QStringLiteral("customHTTPHeader"), QStringLiteral("Authorization: Bearer ") + m_apiKey);
  }
}

void AbstractBGGFetcher::readConfigHook(const KConfigGroup& config_) {
  const int imageSize = config_.readEntry("Image Size", -1);
  if(imageSize > -1) {
    m_imageSize = static_cast<ImageSize>(imageSize);
  }
  const auto k = config_.readEntry("API Key");
  if(!k.isEmpty()) {
    m_apiKey = k;
  }
}

void AbstractBGGFetcher::parseData(QByteArray& data_) {
  QDomDocument dom;
#if (QT_VERSION < QT_VERSION_CHECK(6, 5, 0))
  if(!dom.setContent(data_, false)) {
#else
  if(!dom.setContent(data_, QDomDocument::ParseOption::Default)) {
#endif
    myWarning() << "BoardGameGeek: server did not return valid XML.";
    return;
  }
  // error comes in a div element apparently
  auto e = dom.documentElement();
  if(e.tagName() == QLatin1StringView("div") &&
     e.attribute(QLatin1String("class")).contains(QLatin1String("error"))) {
    myLog() << "BoardGameGeek error:" << e.text().trimmed();
  }
}

Tellico::Data::EntryPtr AbstractBGGFetcher::fetchEntryHookData(Data::EntryPtr entry_) {
  Q_ASSERT(entry_);

  const QString id = entry_->field(QStringLiteral("bggid"));
  if(id.isEmpty()) {
    myDebug() << "no bgg id found";
    return entry_;
  }

  QUrl u(QString::fromLatin1(BGG_THING_URL));
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("id"), id);
  // there's an evident bug where the videogameexpansion type is no longer used
  // but some video game ids like 139806 still identify as expansions
  if(type() == VideoGameGeek) {
    q.addQueryItem(QStringLiteral("type"), QStringLiteral("videogame,videogameexpansion"));
  } else {
    q.addQueryItem(QStringLiteral("type"), bggType());
  }
  u.setQuery(q);
//  myDebug() << "url: " << u;

  QPointer<KIO::StoredTransferJob> job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  job->addMetaData(QStringLiteral("customHTTPHeader"), QStringLiteral("Authorization: Bearer ") + m_apiKey);
  KJobWidgets::setWindow(job, GUI::Proxy::widget());
  if(!job->exec()) {
    myDebug() << job->errorString() << u;
    return entry_;
  }

  const QString output = XMLHandler::readXMLData(job->data());

#if 0
  myWarning() << "Remove output debug from abstractbggfetcher.cpp";
  QFile f(QStringLiteral("/tmp/test-bgg.xml"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << output;
  }
  f.close();
#endif

  auto handler = xsltHandler();
  handler->addStringParam("image-size", QByteArray::number(m_imageSize));

  Import::TellicoImporter imp(handler->applyStylesheet(output));
  // be quiet when loading images
  imp.setOptions(imp.options() ^ Import::ImportShowImageErrors);
  Data::CollPtr coll = imp.collection();
  if(!coll) {
    myWarning() << "no collection pointer";
    return entry_;
  }
  if(coll->entryCount() == 0) {
    myWarning() << "no entries";
    return entry_;
  }

  if(coll->entryCount() > 1) {
    myDebug() << "weird, more than one entry found";
  }
  // replace HTML entities
  const QString desc(QStringLiteral("description"));
  auto entry = coll->entries().front();
  entry->setField(desc, Tellico::decodeHTML(entry->field(desc)));

  // don't want to include id
  coll->removeField(QStringLiteral("bggid"));
  return entry;
}

Tellico::Fetch::FetchRequest AbstractBGGFetcher::updateRequest(Data::EntryPtr entry_) {
  QString bggid = entry_->field(QStringLiteral("bggid"));
  if(!bggid.isEmpty()) {
    return FetchRequest(Raw, bggid);
  }

  QString title = entry_->field(QStringLiteral("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }
  return FetchRequest();
}

AbstractBGGFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const AbstractBGGFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  auto l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;

  auto al = new QLabel(i18n("Registration is required for accessing this data source. "
                            "If you agree to the terms and conditions, <a href='%1'>sign "
                            "up for an account</a>, and enter your information below.",
                             QStringLiteral("https://boardgamegeek.com/using_the_xml_api#toc10")),
                          optionsWidget());
  al->setOpenExternalLinks(true);
  al->setWordWrap(true);
  ++row;
  l->addWidget(al, row, 0, 1, 2);
  // richtext gets weird with size
  al->setMinimumWidth(al->sizeHint().width());

  auto label = new QLabel(i18n("Access key: "), optionsWidget());
  l->addWidget(label, ++row, 0);

  m_apiKeyEdit = new QLineEdit(optionsWidget());
  connect(m_apiKeyEdit, &QLineEdit::textChanged, this, &ConfigWidget::slotSetModified);
  l->addWidget(m_apiKeyEdit, row, 1);
  label->setBuddy(m_apiKeyEdit);

  label = new QLabel(i18n("&Image size: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_imageCombo = new GUI::ComboBox(optionsWidget());
  m_imageCombo->addItem(i18n("No Image"), NoImage);
  m_imageCombo->addItem(i18n("Small Image"), SmallImage);
  m_imageCombo->addItem(i18n("Large Image"), LargeImage);
  void (GUI::ComboBox::* activatedInt)(int) = &GUI::ComboBox::activated;
  connect(m_imageCombo, activatedInt, this, &ConfigWidget::slotSetModified);
  l->addWidget(m_imageCombo, row, 1);
  label->setBuddy(m_imageCombo);

  l->setRowStretch(++row, 10);

  if(fetcher_) {
    m_apiKeyEdit->setText(fetcher_->m_apiKey);
    m_imageCombo->setCurrentData(fetcher_->m_imageSize);
  } else { // defaults
    m_imageCombo->setCurrentData(SmallImage);
  }
}

void AbstractBGGFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  const auto apiKey = m_apiKeyEdit->text().trimmed();
  if(!apiKey.isEmpty()) {
    config_.writeEntry("API Key", apiKey);
  }
  const auto n = m_imageCombo->currentData().toInt();
  config_.writeEntry("Image Size", n);
}
