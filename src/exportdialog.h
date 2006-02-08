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

#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

class QCheckBox;
class QRadioButton;

#include "translators/translators.h"
#include "datavectors.h"

#include <kdialogbase.h>
#include <kurl.h>

namespace Tellico {
  namespace Export {
    class Exporter;
  }

/**
 * @author Robby Stephenson
 */
class ExportDialog : public KDialogBase {
Q_OBJECT

public:
  ExportDialog(Export::Format format, Data::CollPtr coll, QWidget* parent, const char* name);
  ~ExportDialog();

  QString fileFilter();
  bool exportURL(const KURL& url=KURL()) const;

  static Export::Target exportTarget(Export::Format format);
  static bool exportCollection(Export::Format format, const KURL& url);

private slots:
  void slotSaveOptions();

private:
  static Export::Exporter* exporter(Export::Format format);

  void readOptions();

  Export::Format m_format;
  Data::CollPtr m_coll;
  Export::Exporter* m_exporter;
  QCheckBox* m_formatFields;
  QCheckBox* m_exportSelected;
  QRadioButton* m_encodeUTF8;
  QRadioButton* m_encodeLocale;
};

} // end namespace
#endif
