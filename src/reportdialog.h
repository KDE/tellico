/***************************************************************************
    copyright            : (C) 2005-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_REPORTDIALOG_H
#define TELLICO_REPORTDIALOG_H

#include <kdialog.h>

class KHTMLPart;

namespace Tellico {
  namespace Export {
    class HTMLExporter;
  }
  namespace GUI {
    class ComboBox;
  }

/**
 * @author Robby Stephenson
 */
class ReportDialog : public KDialog {
Q_OBJECT

public:
  /**
   * The constructor sets up the dialog.
   *
   * @param parent A pointer to the parent widget
   */
  ReportDialog(QWidget* parent);
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
  GUI::ComboBox* m_templateCombo;
  Export::HTMLExporter* m_exporter;
  QString m_xsltFile;
};

} // end namespace
#endif
