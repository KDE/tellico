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

#ifndef BCEXPORTDIALOG_H
#define BCEXPORTDIALOG_H

class QCheckBox;
class QRadioButton;

#include <kdialogbase.h>

namespace Bookcase {
  class MainWindow;
  namespace Data {
    class Collection;
  }
  namespace Export {
    class Exporter;
  }

/**
 * @author Robby Stephenson
 * @version $Id: exportdialog.h 386 2004-01-24 05:12:28Z robby $
 */
class ExportDialog : public KDialogBase {
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

  ExportDialog(ExportFormat format, Data::Collection* coll, MainWindow* parent, const char* name);

  QString fileFilter();
  bool isText() const;
  QString text();
  QByteArray data();
  bool encodeUTF8() const;

private slots:
  void slotSaveOptions();

private:
  void readOptions();
  Export::Exporter* exporter(ExportFormat format, MainWindow* bookcase_);

  Data::Collection* m_coll;
  Export::Exporter* m_exporter;
  QCheckBox* m_formatFields;
  QRadioButton* m_encodeUTF8;
  QRadioButton* m_encodeLocale;
};

} // end namespace
#endif
