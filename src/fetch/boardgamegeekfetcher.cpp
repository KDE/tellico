/***************************************************************************
    Copyright (C) 2014-2021 Robby Stephenson <robby@periapsis.org>
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

#include "boardgamegeekfetcher.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../utils/string_utils.h"
#include "../core/tellico_strings.h"
#include "../gui/combobox.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QUrlQuery>

namespace {
  // a lot of overlap with boardgamegeekimporter.h
  static const int BGG_MAX_RETURNS_TOTAL = 10;
  static const char* BGG_SEARCH_URL  = "https://boardgamegeek.com/xmlapi2/search";
  static const char* BGG_THING_URL  = "https://boardgamegeek.com/xmlapi2/thing";
}

using namespace Tellico;
using Tellico::Fetch::BoardGameGeekFetcher;

BoardGameGeekFetcher::BoardGameGeekFetcher(QObject* parent_)
    : XMLFetcher(parent_), m_imageSize(SmallImage) {
  setLimit(BGG_MAX_RETURNS_TOTAL);
  setXSLTFilename(QStringLiteral("boardgamegeek2tellico.xsl"));
}

BoardGameGeekFetcher::~BoardGameGeekFetcher() {
}

QString BoardGameGeekFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool BoardGameGeekFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Title || k == Keyword;
}

// https://boardgamegeek.com/wiki/page/XML_API_Terms_of_Use
QString BoardGameGeekFetcher::attribution() const {
  return TC_I18N3(providedBy, QLatin1String("https://boardgamegeek.com"), QLatin1String("BoardGameGeek"));
}

bool BoardGameGeekFetcher::canFetch(int type) const {
  return type == Data::Collection::BoardGame;
}

void BoardGameGeekFetcher::readConfigHook(const KConfigGroup& config_) {
  const int imageSize = config_.readEntry("Image Size", -1);
  if(imageSize > -1) {
    m_imageSize = static_cast<ImageSize>(imageSize);
  }
}

QUrl BoardGameGeekFetcher::searchUrl() {
  QUrl u(QString::fromLatin1(BGG_SEARCH_URL));

  QUrlQuery q;
  switch(request().key()) {
    case Title:
      q.addQueryItem(QStringLiteral("query"), request().value());
      q.addQueryItem(QStringLiteral("type"), QStringLiteral("boardgame,boardgameexpansion"));
      q.addQueryItem(QStringLiteral("exact"), QStringLiteral("1"));
      break;

    case Keyword:
      q.addQueryItem(QStringLiteral("query"), request().value());
      q.addQueryItem(QStringLiteral("type"), QStringLiteral("boardgame,boardgameexpansion"));
      break;

    case Raw:
      u.setUrl(QLatin1String(BGG_THING_URL));
      q.addQueryItem(QStringLiteral("id"), request().value());
      q.addQueryItem(QStringLiteral("type"), QStringLiteral("boardgame,boardgameexpansion"));
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      return QUrl();
  }
  u.setQuery(q);

//  myDebug() << "url: " << u.url();
  return u;
}

Tellico::Data::EntryPtr BoardGameGeekFetcher::fetchEntryHookData(Data::EntryPtr entry_) {
  Q_ASSERT(entry_);

  const QString id = entry_->field(QStringLiteral("bggid"));
  if(id.isEmpty()) {
    myDebug() << "no bgg id found";
    return entry_;
  }

  QUrl u(QString::fromLatin1(BGG_THING_URL));
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("id"), id);
  q.addQueryItem(QStringLiteral("type"), QStringLiteral("boardgame,boardgameexpansion"));
  u.setQuery(q);
//  myDebug() << "url: " << u;

  // quiet
  QString output = FileHandler::readXMLFile(u, true);

#if 0
  myWarning() << "Remove output debug from boardgamegeekfetcher.cpp";
  QFile f(QLatin1String("/tmp/test.xml"));
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

  // don't want to include id
  coll->removeField(QStringLiteral("bggid"));
  return coll->entries().front();
}

Tellico::Fetch::FetchRequest BoardGameGeekFetcher::updateRequest(Data::EntryPtr entry_) {
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

Tellico::Fetch::ConfigWidget* BoardGameGeekFetcher::configWidget(QWidget* parent_) const {
  return new BoardGameGeekFetcher::ConfigWidget(parent_, this);
}

QString BoardGameGeekFetcher::defaultName() {
  return QStringLiteral("BoardGameGeek");
}

QString BoardGameGeekFetcher::defaultIcon() {
  return favIcon("https://cf.geekdo-static.com/icons/favicon2.ico");
}

Tellico::StringHash BoardGameGeekFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("artist")]             = i18nc("Comic Book Illustrator", "Artist");
  hash[QStringLiteral("boardgamegeek-link")] = i18n("BoardGameGeek Link");
  return hash;
}

BoardGameGeekFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const BoardGameGeekFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;

  QLabel* label = new QLabel(i18n("&Image size: "), optionsWidget());
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

  // now add additional fields widget
  addFieldsWidget(BoardGameGeekFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());

  if(fetcher_) {
    m_imageCombo->setCurrentData(fetcher_->m_imageSize);
  } else { // defaults
    m_imageCombo->setCurrentData(SmallImage);
  }
}

void BoardGameGeekFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  const int n = m_imageCombo->currentData().toInt();
  config_.writeEntry("Image Size", n);
}

QString BoardGameGeekFetcher::ConfigWidget::preferredName() const {
  return BoardGameGeekFetcher::defaultName();
}
