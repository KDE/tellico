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

#ifndef IMPORTDIALOG_H
#define IMPORTDIALOG_H

#include "translators/translators.h"
#include "datavectors.h"

#include <kdialogbase.h>

class KURL;

class QRadioButton;
class QCheckBox;
class QShowEvent;

namespace Tellico {
  namespace Import {
    class Importer;
    typedef QMap<Import::Format, QString> FormatMap;
  }

/**
 * @author Robby Stephenson
 */
class ImportDialog : public KDialogBase {
Q_OBJECT

public:
  ImportDialog(Import::Format format, const KURL& url, QWidget* parent, const char* name);
  ~ImportDialog();

  Data::CollPtr collection();
  QString statusMessage() const;
  Import::Action action() const;

  static QString fileFilter(Import::Format format);
  static Import::Target importTarget(Import::Format format);
  static QString startDir(Import::Format format);
  static Import::FormatMap formatMap();

  static Import::Importer* importer(Import::Format format, const KURL& url);
  static Data::CollPtr importURL(Import::Format format, const KURL& url);

private slots:
  void slotUpdateAction();

private:
  Data::CollPtr m_coll;
  Import::Importer* m_importer;
  QRadioButton* m_radioAppend;
  QRadioButton* m_radioReplace;
  QRadioButton* m_radioMerge;
};

} // end namespace
#endif
