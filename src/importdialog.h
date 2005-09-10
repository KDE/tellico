/***************************************************************************
    copyright            : (C) 2003-2005 by Robby Stephenson
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
  namespace Data {
    class Collection;
  }
  namespace Import {
    class Importer;
  }

/**
 * @author Robby Stephenson
 */
class ImportDialog : public KDialogBase {
Q_OBJECT

public:
  ImportDialog(Import::Format format, const KURL& url, QWidget* parent, const char* name);
  ~ImportDialog();

  Data::Collection* collection();
  QString statusMessage() const;
  Import::Action action() const;

  static QString fileFilter(Import::Format format);
  static Import::Target importTarget(Import::Format format);

  static Data::Collection* importURL(Import::Format format, const KURL& url);

signals:
  /**
   * Signals that a fraction of an operation has been completed.
   *
   * @param f The fraction, 0 =< f >= 1
   */
  void signalFractionDone(float f);

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
