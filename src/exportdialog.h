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

#include "translators/translators.h"

#include <kdialogbase.h>
#include <kurl.h>

namespace Tellico {
  class MainWindow;
  namespace Data {
    class Collection;
  }
  namespace Export {
    class Exporter;
  }

/**
 * @author Robby Stephenson
 * @version $Id: exportdialog.h 867 2004-09-15 03:04:49Z robby $
 */
class ExportDialog : public KDialogBase {
Q_OBJECT

public:
  ExportDialog(Export::Format format, Data::Collection* coll, MainWindow* parent, const char* name);
  ~ExportDialog();

  QString fileFilter();
  bool exportURL(const KURL& url=KURL()) const;

  static Export::Target exportTarget(Export::Format format);

private slots:
  void slotSaveOptions();

private:
  static Export::Exporter* exporter(Export::Format format, MainWindow* mainwindow, Data::Collection* coll);

  void readOptions();

  Export::Format m_format;
  Data::Collection* m_coll;
  Export::Exporter* m_exporter;
  QCheckBox* m_formatFields;
  QCheckBox* m_exportSelected;
  QRadioButton* m_encodeUTF8;
  QRadioButton* m_encodeLocale;
};

} // end namespace
#endif
