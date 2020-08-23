/***************************************************************************
    Copyright (C) 2017 Robby Stephenson <robby@periapsis.org>
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

#include "dbcfetcher.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>

#include <QLabel>
#include <QVBoxLayout>
#include <QUrlQuery>

namespace {
  static const char* DBC_API_URL = "https://oss-services.dbc.dk/opensearch/5.2/";
  static const int DBC_MAX_RETURNS_TOTAL = 20;
}

using namespace Tellico;
using Tellico::Fetch::DBCFetcher;

DBCFetcher::DBCFetcher(QObject* parent_) : XMLFetcher(parent_) {
  setLimit(DBC_MAX_RETURNS_TOTAL);
  setXSLTFilename(QStringLiteral("dbc2tellico.xsl"));
}

DBCFetcher::~DBCFetcher() {
}

QString DBCFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool DBCFetcher::canSearch(FetchKey k) const {
  return k == Title || k == Person || k == Keyword || k == ISBN;
}

bool DBCFetcher::canFetch(int type) const {
  return type == Data::Collection::Book || type == Data::Collection::Bibtex;
}

void DBCFetcher::readConfigHook(const KConfigGroup&) {
}

QUrl DBCFetcher::searchUrl() {
  QUrl u(QString::fromLatin1(DBC_API_URL));

  QUrlQuery query;
  switch(request().key) {
    case Title:
      query.addQueryItem(QStringLiteral("query"), QLatin1String("dkcclterm.ti=\"") + request().value + QLatin1Char('"'));
      break;

    case Keyword:
      query.addQueryItem(QStringLiteral("query"), QLatin1String("cql.keywords=\"") + request().value + QLatin1Char('"'));
      break;

    case Person:
      query.addQueryItem(QStringLiteral("query"), QLatin1String("term.mainCreator=\"") + request().value + QLatin1Char('"'));
      break;

    case ISBN:
      {
        QString s = request().value;
        s.remove(QLatin1Char('-'));
        QStringList isbnList = FieldFormat::splitValue(s);
        // only search for first ISBN for now
        QString q = isbnList.isEmpty() ? QString() : QLatin1String("dkcclterm.ib=") + isbnList.at(0);
        query.addQueryItem(QStringLiteral("query"), q);
      }
      break;

    default:
      myWarning() << "key not recognized:" << request().key;
      return QUrl();
  }
  query.addQueryItem(QStringLiteral("action"), QStringLiteral("search"));
  // see https://opensource.dbc.dk/services/open-search-web-service
  // agency and profile determine the search collections
//  query.addQueryItem(QLatin1String("agency"), QLatin1String("100200"));
//  query.addQueryItem(QLatin1String("profile"), QLatin1String("test"));
  query.addQueryItem(QStringLiteral("agency"), QStringLiteral("761500"));
  query.addQueryItem(QStringLiteral("profile"), QStringLiteral("opac"));
  query.addQueryItem(QStringLiteral("term.type"), QStringLiteral("bog"));
  query.addQueryItem(QStringLiteral("start"), QStringLiteral("1"));
  query.addQueryItem(QStringLiteral("stepValue"), QStringLiteral("5"));
  query.addQueryItem(QStringLiteral("outputType"), QStringLiteral("xml"));
  u.setQuery(query);

//  myDebug() << "url:" << u.url();
  return u;
}

void DBCFetcher::resetSearch() {
}

void DBCFetcher::parseData(QByteArray& data_) {
  Q_UNUSED(data_);
}

Tellico::Data::EntryPtr DBCFetcher::fetchEntryHookData(Data::EntryPtr entry_) {
  Q_ASSERT(entry_);
  return entry_;
}

Tellico::Fetch::FetchRequest DBCFetcher::updateRequest(Data::EntryPtr entry_) {
  QString title = entry_->field(QStringLiteral("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Keyword, title);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* DBCFetcher::configWidget(QWidget* parent_) const {
  return new DBCFetcher::ConfigWidget(parent_, this);
}

QString DBCFetcher::defaultName() {
//  return QLatin1String("Dansk BiblioteksCenter (DBC)");
  return i18n("Danish Bibliographic Center (DBC.dk)");
}

QString DBCFetcher::defaultIcon() {
  return favIcon("http://dbc.dk");
}

Tellico::StringHash DBCFetcher::allOptionalFields() {
  StringHash hash;
  return hash;
}

DBCFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const DBCFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  // now add additional fields widget
  addFieldsWidget(DBCFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

void DBCFetcher::ConfigWidget::saveConfigHook(KConfigGroup&) {
}

QString DBCFetcher::ConfigWidget::preferredName() const {
  return DBCFetcher::defaultName();
}
