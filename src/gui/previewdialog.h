/***************************************************************************
    copyright            : (C) 2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_GUI_PREVIEWDIALOG_H
#define TELLICO_GUI_PREVIEWDIALOG_H

#include <kdialogbase.h>

#include "../datavectors.h"

class KTempDir;

namespace Tellico {
  class EntryView;
  class StyleOptions;

  namespace GUI {

class PreviewDialog : public KDialogBase {
Q_OBJECT

public:
  PreviewDialog(QWidget* parent);
  ~PreviewDialog();

  QString tempDir() const;

  void setXSLTFile(const QString& file);
  void setXSLTOptions(const StyleOptions& options);
  void showEntry(Data::EntryPtr entry);

private:
  KTempDir* m_tempDir;
  EntryView* m_view;
};

  }
}
#endif
