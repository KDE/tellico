/***************************************************************************
    Copyright (C) 2025 Robby Stephenson <robby@periapsis.org>
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

#include "isfdbfetcher.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../core/tellico_strings.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>

#include <QLabel>
#include <QVBoxLayout>
#include <QUrlQuery>

namespace {
  static const char* ISFDB_API_URL = "https://www.isfdb.org/cgi-bin/rest/";
}

using Tellico::Fetch::ISFDBFetcher;

ISFDBFetcher::ISFDBFetcher(QObject* parent_)
    : XMLFetcher(parent_) {
  setLimit(10); // only ISBN and LCCN are used right now, but multiple records could be returned for a single value
  setXSLTFilename(QStringLiteral("isfdb2tellico.xsl"));
}

ISFDBFetcher::~ISFDBFetcher() {
}

QString ISFDBFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

QString ISFDBFetcher::attribution() const {
  return TC_I18N3(providedBy, QLatin1String("https://isfdb.org"), QLatin1String("The Internet Speculative Fiction Database"));
}

bool ISFDBFetcher::canSearch(Fetch::FetchKey k) const {
  return k == ISBN || k == LCCN;
}

bool ISFDBFetcher::canFetch(int type) const {
  return type == Data::Collection::Book;
}

void ISFDBFetcher::readConfigHook(const KConfigGroup& config_) {
  Q_UNUSED(config_);
}

QUrl ISFDBFetcher::searchUrl() {
  QUrl u(QString::fromLatin1(ISFDB_API_URL));
  QUrlQuery q;

  // we only search for first isbn or lccn value
  const QStringList searchTerms = FieldFormat::splitValue(request().value());
  if(searchTerms.size() > 1) {
    myLog() << "ISFDB search only uses the first ISBN or LCCN value";
  }

  switch(request().key()) {
    case ISBN:
      u.setPath(u.path() + QStringLiteral("getpub.cgi"));
      q.addQueryItem(searchTerms.first(), QString());
      break;

    case LCCN:
      u.setPath(u.path() + QStringLiteral("getpub_by_ID.cgi"));
      q.addQueryItem(QLatin1String("LCCN+") + searchTerms.first(), QString());
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      return QUrl();
  }
  u.setQuery(q);

//  myDebug() << "url: " << u.url();
  return u;
}

void ISFDBFetcher::parseData(QByteArray& data_) {
  Q_UNUSED(data_);
}

Tellico::Data::EntryPtr ISFDBFetcher::fetchEntryHookData(Data::EntryPtr entry_) {
  Q_ASSERT(entry_);

  // clean up pages value
  static const QRegularExpression digits(QLatin1String("^[^\\d]*(\\d+)"));
  const QString pageField(QStringLiteral("pages"));
  const QString pages = entry_->field(pageField);
  auto match = digits.match(pages);
  if(match.hasMatch()) {
    entry_->setField(pageField, match.captured(1));
  }

  // manually split publisher
  const QString pubField(QStringLiteral("publisher"));
  const QString pub = entry_->field(pubField);
  const auto list = pub.split(QLatin1String(" / "));
  entry_->setField(pubField, list.join(FieldFormat::delimiterString()));

  return entry_;
}

Tellico::Fetch::FetchRequest ISFDBFetcher::updateRequest(Data::EntryPtr entry_) {
  QString isbn = entry_->field(QStringLiteral("isbn"));
  if(!isbn.isEmpty()) {
    return FetchRequest(ISBN, isbn);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* ISFDBFetcher::configWidget(QWidget* parent_) const {
  return new ISFDBFetcher::ConfigWidget(parent_, this);
}

QString ISFDBFetcher::defaultName() {
  return QStringLiteral("The Internet Speculative Fiction Database (ISFDB)");
}

QString ISFDBFetcher::defaultIcon() {
  return favIcon("http://www.isfdb.org");
}

Tellico::StringHash ISFDBFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("isfdb")] = i18n("ISFDB Link");
  return hash;
}

ISFDBFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const ISFDBFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  addFieldsWidget(ISFDBFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

QString ISFDBFetcher::ConfigWidget::preferredName() const {
  return ISFDBFetcher::defaultName();
}
