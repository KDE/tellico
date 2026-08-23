/***************************************************************************
    Copyright (C) 2026 Robby Stephenson <robby@periapsis.org>
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

#include "goodreadsfetcher.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../core/tellico_strings.h"
#include "../utils/string_utils.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>

#include <QLabel>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QUrlQuery>

namespace {
static const char* GOODREADS_TITLE_URL = "https://www.goodreads.com/book/title.xml";
static const char* GOODREADS_ISBN_URL = "https://www.goodreads.com/book/isbn/";
  static const char* GOODREADS_API_KEY = "23626735f5a498f2f2c003300549c0b73f5caa9e87e9faca81eac394eaa5a0d66b3a6d0f563182f29efa";
}

using Tellico::Fetch::GoodreadsFetcher;

GoodreadsFetcher::GoodreadsFetcher(QObject* parent_)
    : XMLFetcher(parent_)
    , m_apiKey(Tellico::reverseObfuscate(GOODREADS_API_KEY)) {
  setLimit(10);
  setXSLTFilename(QStringLiteral("goodreads2tellico.xsl"));
}

GoodreadsFetcher::~GoodreadsFetcher() = default;

QString GoodreadsFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

QString GoodreadsFetcher::attribution() const {
  return TC_I18N3(providedBy, QStringLiteral("https://www.goodreads.com"), QStringLiteral("Goodreads"));
}

bool GoodreadsFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Title || k == ISBN;
}

// only single values
bool GoodreadsFetcher::canSearchMultiple() const {
  return false;
}

bool GoodreadsFetcher::canFetch(int type) const {
  return type == Data::Collection::Book;
}

void GoodreadsFetcher::readConfigHook(const KConfigGroup& config_) {
  QString k = config_.readEntry("API Key");
  if(!k.isEmpty()) {
    m_apiKey = k;
  }
}

QUrl GoodreadsFetcher::searchUrl() {
  QUrl u;
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("key"), m_apiKey);

  switch(request().key()) {
    case Title:
      u.setUrl(QString::fromLatin1(GOODREADS_TITLE_URL));
      q.addQueryItem(QStringLiteral("title"), request().value());
      break;

    case ISBN:
      u.setUrl(QString::fromLatin1(GOODREADS_ISBN_URL));
      {
        const QStringList searchTerms = FieldFormat::splitValue(request().value());
        if(searchTerms.size() > 1) {
          myLog() << "Goodreads search only uses the first ISBN value";
        }
        u.setPath(u.path() + searchTerms.first());
      }
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      return QUrl();
  }
  u.setQuery(q);

//  myLog() << "url: " << u.url();
  return u;
}

Tellico::Data::EntryPtr GoodreadsFetcher::fetchEntryHookData(Data::EntryPtr entry_) {
  Q_ASSERT(entry_);
  const QString isbn(QStringLiteral("isbn"));
  if(request().key() == ISBN && entry_->field(isbn).isEmpty()) {
    const auto terms = FieldFormat::splitValue(request().value());
    entry_->setField(isbn, terms.first());
  }
  return entry_;
}

Tellico::Fetch::FetchRequest GoodreadsFetcher::updateRequest(Data::EntryPtr entry_) {
  QString isbn = entry_->field(QStringLiteral("isbn"));
  if(!isbn.isEmpty()) {
    return FetchRequest(ISBN, isbn);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* GoodreadsFetcher::configWidget(QWidget* parent_) const {
  return new GoodreadsFetcher::ConfigWidget(parent_, this);
}

QString GoodreadsFetcher::defaultName() {
  return QStringLiteral("Goodreads");
}

QString GoodreadsFetcher::defaultIcon() {
  return favIcon("http://www.goodreads.com");
}

Tellico::StringHash GoodreadsFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("goodreads")] = i18n("Goodreads Link");
  return hash;
}

GoodreadsFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const GoodreadsFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;

  QLabel* al = new QLabel(i18n("Registration is required for accessing this data source. "
                               "If you agree to the terms and conditions, <a href='%1'>sign "
                               "up for an account</a>, and enter your information below.",
                                QLatin1String("https://www.goodreads.com/api/keys")),
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
  addFieldsWidget(GoodreadsFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());

  if(fetcher_) {
    // only show the key if it is not the default Tellico one...
    // that way the user is prompted to apply for their own
    if(fetcher_->m_apiKey != Tellico::reverseObfuscate(GOODREADS_API_KEY)) {
      m_apiKeyEdit->setText(fetcher_->m_apiKey);
    }
  }
}

QString GoodreadsFetcher::ConfigWidget::preferredName() const {
  return GoodreadsFetcher::defaultName();
}
