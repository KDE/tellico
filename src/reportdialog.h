/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef REPORTDIALOG_H
#define REPORTDIALOG_H

#include "gui/comboboxproxy.h"

#include <kdialogbase.h>

class KHTMLPart;

namespace Tellico {
  namespace Export {
    class HTMLExporter;
  }

/**
 * @author Robby Stephenson
 */
class ReportDialog : public KDialogBase {
Q_OBJECT

public:
  /**
   * The constructor sets up the dialog.
   *
   * @param parent A pointer to the parent widget
   * @param name The widget name
   */
  ReportDialog(QWidget* parent, const char* name=0);
  virtual ~ReportDialog();

public slots:
  /**
   * Regenerate the report.
   */
  void slotRefresh();

private slots:
  void slotGenerate();
  void slotPrint();
  void slotSaveAs();

private:
  KHTMLPart* m_HTMLPart;
  typedef Tellico::GUI::ComboBoxProxy<QString> CBProxy;
  CBProxy* m_templateCombo;
  Export::HTMLExporter* m_exporter;
  QString m_xsltFile;
};

} // end namespace
#endif
