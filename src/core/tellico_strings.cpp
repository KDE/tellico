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

#include "tellico_strings.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <KLocalizedString>
#define TC_STR const char*
#define STR_NOOP I18N_NOOP
#else
#define TC_STR KLazyLocalizedString
#define STR_NOOP kli18n
#endif

TC_STR Tellico::errorOpen = STR_NOOP("Tellico is unable to open the file - %1.");
TC_STR Tellico::errorLoad = STR_NOOP("Tellico is unable to load the file - %1.");
TC_STR Tellico::errorWrite = STR_NOOP("Tellico is unable to write the file - %1.");
TC_STR Tellico::errorUpload = STR_NOOP("Tellico is unable to upload the file - %1.");
TC_STR Tellico::errorAppendType = STR_NOOP("Only collections with the same type of entries as "
                                                 "the current one can be appended. No changes are being "
                                                 "made to the current collection.");
TC_STR Tellico::errorMergeType = STR_NOOP("Only collections with the same type of entries as "
                                                "the current one can be merged. No changes are being "
                                                "made to the current collection.");
TC_STR Tellico::errorImageLoad = STR_NOOP("Tellico is unable to load an image from the file - %1.");

TC_STR Tellico::untitledFilename = STR_NOOP("Untitled");
TC_STR Tellico::providedBy = STR_NOOP("This information was freely provided by <a href=\"%1\">%2</a>.");

#undef TC_STR
#undef STR_NOOP
