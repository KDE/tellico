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

#include "beyazperdefetcher.h"

#include <KLocalizedString>

namespace {
  static const char* BEYAZPERDE_API_URL = "http://api.beyazperde.com/rest/v3/";
}

using namespace Tellico;
using Tellico::Fetch::BeyazperdeFetcher;

BeyazperdeFetcher::BeyazperdeFetcher(QObject* parent_)
    : AbstractAllocineFetcher(parent_, QLatin1String(BEYAZPERDE_API_URL)) {
}

Tellico::Fetch::ConfigWidget* BeyazperdeFetcher::configWidget(QWidget* parent_) const {
  return new BeyazperdeFetcher::ConfigWidget(parent_, this);
}

QString BeyazperdeFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

QString BeyazperdeFetcher::defaultName() {
  return QLatin1String("Beyazperde");
}

QString BeyazperdeFetcher::defaultIcon() {
  return favIcon("http://www.beyazperde.com");
}

Tellico::StringHash BeyazperdeFetcher::allOptionalFields() {
  StringHash hash;
  hash[QLatin1String("origtitle")] = i18n("Original Title");
  return hash;
}

BeyazperdeFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const AbstractAllocineFetcher* fetcher_)
    : AbstractAllocineFetcher::ConfigWidget(parent_, fetcher_) {
  // now add additional fields widget
  addFieldsWidget(BeyazperdeFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

QString BeyazperdeFetcher::ConfigWidget::preferredName() const {
  return BeyazperdeFetcher::defaultName();
}
