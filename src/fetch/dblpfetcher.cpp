/***************************************************************************
    Copyright (C) 2012 Robby Stephenson <robby@periapsis.org>
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

#include "dblpfetcher.h"
#include "../tellico_debug.h"

#include <KLocale>
#include <KConfigGroup>

#include <QLabel>
#include <QVBoxLayout>

namespace {
  static const char* DBLP_API_URL = "http://www.dblp.org/search/api/";
  static const int DBLP_MAX_RETURNS_TOTAL = 20;
}

using namespace Tellico;
using Tellico::Fetch::DBLPFetcher;

DBLPFetcher::DBLPFetcher(QObject* parent_) : XMLFetcher(parent_) {
  setLimit(DBLP_MAX_RETURNS_TOTAL);
  setXSLTFilename(QLatin1String("dblp2tellico.xsl"));
}

DBLPFetcher::~DBLPFetcher() {
}

QString DBLPFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool DBLPFetcher::canFetch(int type) const {
  return type == Data::Collection::Bibtex;
}

void DBLPFetcher::readConfigHook(const KConfigGroup&) {
}

KUrl DBLPFetcher::searchUrl() {
  KUrl u(DBLP_API_URL);

  switch(request().key) {
    case Keyword:
      u.addQueryItem(QLatin1String("q"), request().value);
      u.addQueryItem(QLatin1String("h"), QString::number(DBLP_MAX_RETURNS_TOTAL));
      u.addQueryItem(QLatin1String("c"), QString::number(0));
      break;

    default:
      myWarning() << "key not recognized:" << request().key;
      return KUrl();
  }
  // has to be after query
  u.addQueryItem(QLatin1String("format"), QLatin1String("xml"));

//  myDebug() << "url:" << u.url();
  return u;
}

void DBLPFetcher::resetSearch() {
}

void DBLPFetcher::parseData(QByteArray& data_) {
  // weird XML
  // remove the CDATA and the dblp:: namespace
  data_.replace("<![CDATA[", ""); // krazy:exclude=doublequote_chars
  data_.replace("]]", ""); // krazy:exclude=doublequote_chars
  data_.replace("dblp:", ""); // krazy:exclude=doublequote_chars
}

Tellico::Data::EntryPtr DBLPFetcher::fetchEntryHookData(Data::EntryPtr entry_) {
  Q_ASSERT(entry_);
  return entry_;
}

Tellico::Fetch::FetchRequest DBLPFetcher::updateRequest(Data::EntryPtr entry_) {
  QString title = entry_->field(QLatin1String("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Keyword, title);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* DBLPFetcher::configWidget(QWidget* parent_) const {
  return new DBLPFetcher::ConfigWidget(parent_, this);
}

QString DBLPFetcher::defaultName() {
  return QLatin1String("DBLP");
}

QString DBLPFetcher::defaultIcon() {
  return favIcon("http://dblp.uni-trier.de");
}

Tellico::StringHash DBLPFetcher::allOptionalFields() {
  StringHash hash;
  return hash;
}

DBLPFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const DBLPFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  // now add additional fields widget
  addFieldsWidget(DBLPFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

void DBLPFetcher::ConfigWidget::saveConfigHook(KConfigGroup&) {
}

QString DBLPFetcher::ConfigWidget::preferredName() const {
  return DBLPFetcher::defaultName();
}

#include "dblpfetcher.moc"
