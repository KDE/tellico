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

#ifndef IMPORTDIALOG_H
#define IMPORTDIALOG_H

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
 * @version $Id: importdialog.h 633 2004-05-01 03:16:22Z robby $
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

  enum ImportAction {
    Replace,
    Append,
    Merge
  };

  ImportDialog(ImportFormat format, const KURL& url, MainWindow* parent, const char* name);
  ~ImportDialog();

  Data::Collection* collection();
  QString statusMessage() const;
  ImportAction action() const;

  static QString fileFilter(ImportFormat format);
  static bool selectFileFirst(ImportFormat format);

private slots:
  void slotUpdateAction();

private:
  Import::Importer* importer(ImportFormat format, const KURL& url);

  Data::Collection* m_coll;
  Import::Importer* m_importer;
  QRadioButton* m_radioAppend;
  QRadioButton* m_radioReplace;
  QRadioButton* m_radioMerge;
};

} // end namespace
#endif
