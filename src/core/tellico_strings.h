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

#include <KLazyLocalizedString>
#define TC_I18N1(str) str.toString()
#define TC_I18N2(str1, str2) str1.subs(str2).toString()
#define TC_I18N3(str1, str2, str3) str1.subs(str2).subs(str3).toString()

namespace Tellico {
  extern KLazyLocalizedString errorOpen;
  extern KLazyLocalizedString errorLoad;
  extern KLazyLocalizedString errorWrite;
  extern KLazyLocalizedString errorUpload;
  extern KLazyLocalizedString errorAppendType;
  extern KLazyLocalizedString errorMergeType;
  extern KLazyLocalizedString errorImageLoad;
  extern KLazyLocalizedString untitledFilename;
  extern KLazyLocalizedString providedBy;

  extern KLazyLocalizedString categoryGeneral;
  extern KLazyLocalizedString categoryFeatures;
  extern KLazyLocalizedString categoryPeople;
  extern KLazyLocalizedString categoryPublishing;
  extern KLazyLocalizedString categoryClassification;
  extern KLazyLocalizedString categoryCondition;
  extern KLazyLocalizedString categoryPersonal;
  extern KLazyLocalizedString categoryMisc;
}

#endif
