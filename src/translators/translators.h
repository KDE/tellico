/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TRANSLATORS_H
#define TRANSLATORS_H

namespace Tellico {
  namespace Import {
    enum Format {
      TellicoXML,
      Bibtex,
      Bibtexml,
      CSV,
      XSLT,
      AudioFile,
      MODS,
      Alexandria,
      FreeDB,
      RIS,
      GCfilms,
      FileListing,
      GRS1
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
      TellicoXML,
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
      GCfilms
    };

    enum Target {
      None,
      File,
      Dir
    };
  }
}

#endif
