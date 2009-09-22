/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TRANSLATORS_H
#define TRANSLATORS_H

namespace Tellico {
  namespace Import {
    enum Format {
      TellicoXML = 0,
      Bibtex,
      Bibtexml,
      CSV,
      XSLT,
      AudioFile,
      MODS,
      Alexandria,
      FreeDB,
      RIS,
      GCstar,
      FileListing,
      GRS1,
      AMC,
      Griffith,
      PDF,
      Referencer,
      Delicious
    };

    enum Action {
      Replace,
      Append,
      Merge
    };

    enum Target {
      None,
      File,
      Dir
    };
  }

  namespace Export {
    enum Format {
      TellicoXML = 0,
      TellicoZip,
      Bibtex,
      Bibtexml,
      HTML,
      CSV,
      XSLT,
      Text,
      PilotDB,
      Alexandria,
      ONIX,
      GCstar
    };

    enum Target {
      None,
      File,
      Dir
    };
  }
}

#endif
