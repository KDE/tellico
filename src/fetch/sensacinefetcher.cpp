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

#include "sensacinefetcher.h"

#include <KLocale>

namespace {
  static const char* SENSACINE_API_URL = "http://api.sensacine.com/rest/v3/";
}

using namespace Tellico;
using Tellico::Fetch::SensaCineFetcher;

SensaCineFetcher::SensaCineFetcher(QObject* parent_)
    : AbstractAllocineFetcher(parent_, QLatin1String(SENSACINE_API_URL)) {
}

QString SensaCineFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

Tellico::Fetch::ConfigWidget* SensaCineFetcher::configWidget(QWidget* parent_) const {
  return new SensaCineFetcher::ConfigWidget(parent_, this);
}

QString SensaCineFetcher::defaultName() {
  return QLatin1String("SensaCine.com");
}

QString SensaCineFetcher::defaultIcon() {
  return favIcon("http://www.sensacine.com");
}

Tellico::StringHash SensaCineFetcher::allOptionalFields() {
  StringHash hash;
  hash[QLatin1String("origtitle")] = i18n("Original Title");
  return hash;
}

SensaCineFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const AbstractAllocineFetcher* fetcher_)
    : AbstractAllocineFetcher::ConfigWidget(parent_, fetcher_) {
  // now add additional fields widget
  addFieldsWidget(SensaCineFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

QString SensaCineFetcher::ConfigWidget::preferredName() const {
  return SensaCineFetcher::defaultName();
}

