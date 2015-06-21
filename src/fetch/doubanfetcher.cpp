/***************************************************************************
    Copyright (C) 2011 Robby Stephenson <robby@periapsis.org>
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

#include "doubanfetcher.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../images/imagefactory.h"
#include "../utils/isbnvalidator.h"
#include "../utils/string_utils.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>

#include <QLineEdit>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QTextCodec>

namespace {
  static const int DOUBAN_MAX_RETURNS_TOTAL = 20;
  static const char* DOUBAN_API_URL = "http://api.douban.com/";
  static const char* DOUBAN_API_KEY = "0bd1672394eb1ebf2374356abec15c3d";
}

using namespace Tellico;
using Tellico::Fetch::DoubanFetcher;

DoubanFetcher::DoubanFetcher(QObject* parent_)
    : XMLFetcher(parent_), m_apiKey(QLatin1String(DOUBAN_API_KEY)) {
  setLimit(DOUBAN_MAX_RETURNS_TOTAL);
  setXSLTFilename(QLatin1String("douban2tellico.xsl"));
}

DoubanFetcher::~DoubanFetcher() {
}

QString DoubanFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool DoubanFetcher::canSearch(FetchKey k) const {
  return k == Keyword || k == ISBN;
}

bool DoubanFetcher::canFetch(int type) const {
  return type == Data::Collection::Book
      || type == Data::Collection::Bibtex
      || type == Data::Collection::Video
      || type == Data::Collection::Album;
}

void DoubanFetcher::readConfigHook(const KConfigGroup& config_) {
  QString k = config_.readEntry("API Key", DOUBAN_API_KEY);
  if(!k.isEmpty()) {
    m_apiKey = k;
  }
}

QUrl DoubanFetcher::searchUrl() {
  QUrl u(QString::fromLatin1(DOUBAN_API_URL));

  switch(request().collectionType) {
    case Data::Collection::Book:
    case Data::Collection::Bibtex:
      u.setPath(u.path() + QLatin1String("book/"));
      break;

    case Data::Collection::Video:
      u.setPath(u.path() + QLatin1String("movie/"));
      break;

    case Data::Collection::Album:
      u.setPath(u.path() + QLatin1String("music/"));
      break;

    default:
      myWarning() << "bad collection type:" << request().collectionType;
      return QUrl();
  }

  switch(request().key) {
    case ISBN:
      u.setPath(u.path() + QLatin1String("subject/isbn/"));
      {
        QStringList isbns = FieldFormat::splitValue(request().value);
        if(isbns.isEmpty()) {
          return QUrl();
        } else {
          u.setPath(u.path() + ISBNValidator::cleanValue(isbns.front()));
        }
      }
      break;

    case Keyword:
      u.setPath(u.path() + QLatin1String("subjects"));
      u.addQueryItem(QLatin1String("q"), request().value);
      break;

    default:
      myWarning() << "key not recognized:" << request().key;
      return QUrl();
  }

  if(!m_apiKey.isEmpty()) {
    u.addQueryItem(QLatin1String("apikey"), m_apiKey);
  }
  u.addQueryItem(QLatin1String("max-results"), QString::number(DOUBAN_MAX_RETURNS_TOTAL));
  u.addQueryItem(QLatin1String("start-index"), QString::number(1));

//  myDebug() << "url:" << u.url();
  return u;
}

Tellico::Data::EntryPtr DoubanFetcher::fetchEntryHookData(Data::EntryPtr entry_) {
  Q_ASSERT(entry_);

  const QString image = entry_->field(QLatin1String("cover"));
  if(image.contains(QLatin1Char('/'))) {
    const QString id = ImageFactory::addImage(QUrl(image), true /* quiet */);
    if(!id.isEmpty()) {
      entry_->setField(QLatin1String("cover"), id);
    }
  }

  if(request().key == ISBN) {
    // don't want to include id
    entry_->collection()->removeField(QLatin1String("douban-id"));
    return entry_;
  }

  const QString id = entry_->field(QLatin1String("douban-id"));
  if(id.isEmpty()) {
    // don't want to include id
    entry_->collection()->removeField(QLatin1String("douban-id"));
    return entry_;
  }

//  myDebug() << id;

  // quiet
  const QString output = FileHandler::readXMLFile(QUrl(id), true /* true */);
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
  coll->removeField(QLatin1String("douban-id"));
  return coll->entries().front();
}

Tellico::Fetch::FetchRequest DoubanFetcher::updateRequest(Data::EntryPtr entry_) {
  QString isbn = entry_->field(QLatin1String("isbn"));
  if(!isbn.isEmpty()) {
    return FetchRequest(ISBN, isbn);
  }

  QString title = entry_->field(QLatin1String("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* DoubanFetcher::configWidget(QWidget* parent_) const {
  return new DoubanFetcher::ConfigWidget(parent_, this);
}

QString DoubanFetcher::defaultName() {
  return QLatin1String("Douban.com");
}

QString DoubanFetcher::defaultIcon() {
  return favIcon("http://www.douban.com");
}

Tellico::StringHash DoubanFetcher::allOptionalFields() {
  StringHash hash;
  hash[QLatin1String("alttitle")] = i18n("Alternative Titles");
  hash[QLatin1String("douban")] = i18n("Douban Link");
  hash[QLatin1String("imdb")] = i18n("IMDb Link");
  return hash;
}

DoubanFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const DoubanFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;
  QLabel* al = new QLabel(i18n("Registration is required for accessing the %1 data source. "
                               "If you agree to the terms and conditions, <a href='%2'>sign "
                               "up for an account</a>, and enter your information below.",
                                preferredName(),
                                QLatin1String("http://www.douban.com/service/apikey/")),
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
  connect(m_apiKeyEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_apiKeyEdit, row, 1);
  label->setBuddy(m_apiKeyEdit);

  l->setRowStretch(++row, 10);

  if(fetcher_ && fetcher_->m_apiKey != QLatin1String(DOUBAN_API_KEY)) {
    m_apiKeyEdit->setText(fetcher_->m_apiKey);
  }

  // now add additional fields widget
  addFieldsWidget(DoubanFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

void DoubanFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  QString apiKey = m_apiKeyEdit->text().trimmed();
  if(!apiKey.isEmpty()) {
    config_.writeEntry("API Key", apiKey);
  }
}

QString DoubanFetcher::ConfigWidget::preferredName() const {
  return DoubanFetcher::defaultName();
}

