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

#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

class QCheckBox;
class QRadioButton;

class KURL;

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
 * @version $Id: exportdialog.h 759 2004-08-11 01:28:25Z robby $
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
  ~ExportDialog();

  QString fileFilter();
  bool exportURL(const KURL& url) const;

private slots:
  void slotSaveOptions();

private:
  void readOptions();
  Export::Exporter* exporter(ExportFormat format, MainWindow* bookcase_);

  Data::Collection* m_coll;
  Export::Exporter* m_exporter;
  QCheckBox* m_formatFields;
  QCheckBox* m_exportSelected;
  QRadioButton* m_encodeUTF8;
  QRadioButton* m_encodeLocale;
};

} // end namespace
#endif
