/***************************************************************************
    Copyright (C) 2003-2021 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_STRINGS_H
#define TELLICO_STRINGS_H

#include <Qt>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#define TC_STR const char*
#define TC_I18N1 i18n
#define TC_I18N2 i18n
#else
#include <KLazyLocalizedString>
#define TC_STR KLazyLocalizedString
#define TC_I18N1(str) str.toString();
#define TC_I18N2(str1, str2) str1.subs(str2).toString()
#endif

namespace Tellico {
  extern TC_STR errorOpen;
  extern TC_STR errorLoad;
  extern TC_STR errorWrite;
  extern TC_STR errorUpload;
  extern TC_STR errorAppendType;
  extern TC_STR errorMergeType;
  extern TC_STR errorImageLoad;
  extern TC_STR untitledFilename;
  extern TC_STR providedBy;

  extern TC_STR categoryGeneral;
  extern TC_STR categoryFeatures;
  extern TC_STR categoryPeople;
  extern TC_STR categoryPublishing;
  extern TC_STR categoryClassification;
  extern TC_STR categoryCondition;
  extern TC_STR categoryPersonal;
  extern TC_STR categoryMisc;
}

#undef TC_STR

#endif
