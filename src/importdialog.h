/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef BCIMPORTDIALOG_H
#define BCIMPORTDIALOG_H

class QRadioButton;
class QCheckBox;
class QShowEvent;

#include <kdialogbase.h>
#include <kurl.h>

namespace Bookcase {
  class MainWindow;
  namespace Data {
    class Collection;
  }
  namespace Import {
    class Importer;
  }

/**
 * @author Robby Stephenson
 * @version $Id: importdialog.h 386 2004-01-24 05:12:28Z robby $
 */
class ImportDialog : public KDialogBase {
Q_OBJECT

public:
  enum ImportFormat {
    BookcaseXML,
    Bibtex,
    Bibtexml,
    CSV,
    XSLT,
    AudioFile
  };

  ImportDialog(ImportFormat format, const KURL& url, MainWindow* parent, const char* name);

  Data::Collection* collection();
  QString statusMessage() const;
  bool replaceCollection() const;

  static QString fileFilter(ImportFormat format);
  static bool selectFileFirst(ImportFormat format);

private:
  Import::Importer* importer(ImportFormat format, const KURL& url);

  Data::Collection* m_coll;
  Import::Importer* m_importer;
  QRadioButton* m_radioAppend;
  QRadioButton* m_radioReplace;
};

} // end namespace
#endif
