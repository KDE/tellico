/***************************************************************************
    copyright            : (C) 2003-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_EXPORTDIALOG_H
#define TELLICO_EXPORTDIALOG_H

#include "translators/translators.h"
#include "datavectors.h"

#include <kdialog.h>
#include <kurl.h>

class QCheckBox;
class QRadioButton;

namespace Tellico {
  namespace Export {
    class Exporter;
  }

/**
 * @author Robby Stephenson
 */
class ExportDialog : public KDialog {
Q_OBJECT

public:
  ExportDialog(Export::Format format, Data::CollPtr coll, QWidget* parent);
  ~ExportDialog();

  QString fileFilter();
  bool exportURL(const KUrl& url=KUrl()) const;

  static Export::Target exportTarget(Export::Format format);
  static bool exportCollection(Export::Format format, const KUrl& url);

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
