/***************************************************************************
                              bcexportdialog.h
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

#ifndef BCEXPORTDIALOG_H
#define BCEXPORTDIALOG_H

class Bookcase;
class BCCollection;
class Exporter;

class QCheckBox;
class QRadioButton;

#include <kdialogbase.h>

/**
 * @author Robby Stephenson
 * @version $Id: bcexportdialog.h 267 2003-11-08 09:18:46Z robby $
 */
class BCExportDialog : public KDialogBase {
Q_OBJECT

public: 
  enum ExportFormat {
    XML,
    Bibtex,
    Bibtexml,
    HTML,
    CSV,
    XSLT,
    Text,
    PilotDB
  };

  BCExportDialog(ExportFormat format, BCCollection* coll, Bookcase* parent, const char* name);

  QString fileFilter();
  QString text();
  bool encodeUTF8() const;

private slots:
  void slotSaveOptions();

private:
  void readOptions();
  Exporter* exporter(ExportFormat format, Bookcase* bookcase_);

  BCCollection* m_coll;
  Exporter* m_exporter;
  QCheckBox* m_formatAttributes;
  QRadioButton* m_encodeUTF8;
  QRadioButton* m_encodeLocale;
};

#endif
