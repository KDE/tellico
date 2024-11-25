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

#include "springerfetcher.h"
#include "../entry.h"
#include "../utils/isbnvalidator.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>
#include <QUrl>

#include <QLabel>
#include <QVBoxLayout>
#include <QDomDocument>
#include <QUrlQuery>

namespace {
  static const char* SPRINGER_BASE_URL = "http://api.springer.com/metadata/pam";
  static const char* SPRINGER_API_KEY = "m2z42cbw68qhhjcrm8tbj2hc";
  static const int SPRINGER_QUERY_COUNT = 10;
}

using namespace Tellico;
using Tellico::Fetch::SpringerFetcher;

SpringerFetcher::SpringerFetcher(QObject* parent_)
    : XMLFetcher(parent_), m_start(0), m_total(-1) {
  setLimit(SPRINGER_QUERY_COUNT);
  setXSLTFilename(QStringLiteral("springer2tellico.xsl"));
}

SpringerFetcher::~SpringerFetcher() {
}

QString SpringerFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

QString SpringerFetcher::attribution() const {
  return i18n("This data is licensed under <a href=""%1"">specific terms</a>.",
              QLatin1String("https://dev.springernature.com/"));
}

bool SpringerFetcher::canSearch(Fetch::FetchKey k) const  {
  return k == Title || k == Person || k == Keyword || k == ISBN || k == DOI || k == Raw;
}

bool SpringerFetcher::canFetch(int type) const {
  return type == Data::Collection::Bibtex;
}

void SpringerFetcher::readConfigHook(const KConfigGroup&) {
}

void SpringerFetcher::resetSearch() {
  m_start = 0;
  m_total = -1;
}

QUrl SpringerFetcher::searchUrl() {
  QUrl u(QString::fromLatin1(SPRINGER_BASE_URL));
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("api_key"), QLatin1String(SPRINGER_API_KEY));
  q.addQueryItem(QStringLiteral("s"), QString::number(m_start + 1));
  q.addQueryItem(QStringLiteral("p"), QString::number(SPRINGER_QUERY_COUNT));

  switch(request().key()) {
    case Title:
      q.addQueryItem(QStringLiteral("q"), QStringLiteral("title:\"%1\" OR book:\"%1\"").arg(request().value()));
      break;

    case Person:
      q.addQueryItem(QStringLiteral("q"), QStringLiteral("name:%1").arg(request().value()));
      break;

    case Keyword:
      q.addQueryItem(QStringLiteral("q"), QStringLiteral("\"%1\"").arg(request().value()));
      break;

    case ISBN:
      {
        // only grab first value
        QString v = request().value().section(QLatin1Char(';'), 0);
        v = ISBNValidator::isbn13(v);
        q.addQueryItem(QStringLiteral("q"), QStringLiteral("isbn:%1").arg(v));
      }
      break;

    case DOI:
      q.addQueryItem(QStringLiteral("q"), QStringLiteral("doi:%1").arg(request().value()));
      break;

    case Raw:
      q.addQueryItem(QStringLiteral("q"), request().value());
      break;

    default:
      return QUrl();
  }
  u.setQuery(q);

//  myDebug() << "url:" << u.url();
  return u;
}

void SpringerFetcher::parseData(QByteArray& data_) {
  QDomDocument dom;
#if (QT_VERSION < QT_VERSION_CHECK(6, 5, 0))
  if(!dom.setContent(data_, false)) {
#else
  if(!dom.setContent(data_, QDomDocument::ParseOption::Default)) {
#endif
    myWarning() << "server did not return valid XML.";
    return;
  }
  // total is /response/result/total
  QDomNode n = dom.documentElement().namedItem(QStringLiteral("result"))
                                    .namedItem(QStringLiteral("total"));
  QDomElement e = n.toElement();
  if(!e.isNull()) {
    m_total = e.text().toInt();
//    myDebug() << "total = " << m_total;
  }
}

void SpringerFetcher::checkMoreResults(int count_) {
  m_start = count_;
  m_hasMoreResults = m_start < m_total;
}

Tellico::Fetch::FetchRequest SpringerFetcher::updateRequest(Data::EntryPtr entry_) {
  const QString doi = entry_->field(QStringLiteral("doi"));
  if(!doi.isEmpty()) {
    return FetchRequest(Fetch::DOI, doi);
  }

  const QString isbn = entry_->field(QStringLiteral("isbn"));
  if(!isbn.isEmpty()) {
    return FetchRequest(Fetch::ISBN, isbn);
  }

  const QString title = entry_->field(QStringLiteral("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Fetch::Title, title);
  }

  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* SpringerFetcher::configWidget(QWidget* parent_) const {
  return new SpringerFetcher::ConfigWidget(parent_, this);
}

QString SpringerFetcher::defaultName() {
  return QStringLiteral("SpringerLink");
}

QString SpringerFetcher::defaultIcon() {
  return favIcon("https://link.springer.com/static/sites/link/images/favicon-32x32.png");
}

SpringerFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const SpringerFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  // now add additional fields widget
  addFieldsWidget(SpringerFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

void SpringerFetcher::ConfigWidget::saveConfigHook(KConfigGroup&) {
}

QString SpringerFetcher::ConfigWidget::preferredName() const {
  return SpringerFetcher::defaultName();
}
