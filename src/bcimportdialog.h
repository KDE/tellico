/***************************************************************************
                              bcimportdialog.h
                             -------------------
    begin                : Sat Jul 12 2003
    copyright            : (C) 2003 by Robby Stephenson
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

class Bookcase;
class BCCollection;
class Importer;

class KURL;

class QRadioButton;
class QCheckBox;
class QShowEvent;

#include <kdialogbase.h>

/**
 * @author Robby Stephenson
 * @version $Id: bcimportdialog.h 233 2003-10-30 03:03:33Z robby $
 */
class BCImportDialog : public KDialogBase {
Q_OBJECT

public:
  enum ImportFormat {
    BookcaseXML,
    Bibtex,
    Bibtexml,
    CSV,
    XSLT
  };

  BCImportDialog(ImportFormat format, const KURL& url, Bookcase* parent, const char* name);

  BCCollection* collection();
  QString statusMessage() const;
  bool replaceCollection() const;

  static QString fileFilter(ImportFormat format);

private:
  Importer* importer(ImportFormat format, const KURL& url);

  BCCollection* m_coll;
  Importer* m_importer;
  QRadioButton* m_radioAppend;
  QRadioButton* m_radioReplace;
};

#endif
