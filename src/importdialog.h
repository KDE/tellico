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

#include "translators/translators.h"

#include <kdialogbase.h>
#include <kurl.h>

namespace Tellico {
  class MainWindow;
  namespace Data {
    class Collection;
  }
  namespace Import {
    class Importer;
  }

/**
 * @author Robby Stephenson
 * @version $Id: importdialog.h 964 2004-11-19 06:54:49Z robby $
 */
class ImportDialog : public KDialogBase {
Q_OBJECT

public:
  ImportDialog(Import::Format format, const KURL& url, MainWindow* parent, const char* name);
  ~ImportDialog();

  Data::Collection* collection();
  QString statusMessage() const;
  Import::Action action() const;

  static QString fileFilter(Import::Format format);
  static Import::Target importTarget(Import::Format format);

  static Data::Collection* importURL(Import::Format format, const KURL& url);

private slots:
  void slotUpdateAction();

private:
  static Import::Importer* importer(Import::Format format, const KURL& url);

  Data::Collection* m_coll;
  Import::Importer* m_importer;
  QRadioButton* m_radioAppend;
  QRadioButton* m_radioReplace;
  QRadioButton* m_radioMerge;
};

} // end namespace
#endif
