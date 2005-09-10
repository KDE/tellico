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

#ifndef CSVEXPORTER_H
#define CSVEXPORTER_H

class KLineEdit;
class KConfig;

class QWidget;
class QCheckBox;
class QRadioButton;

#include "exporter.h"

namespace Tellico {
  namespace Export {

/**
 * @author Robby Stephenson
 */
class CSVExporter : public Exporter {
public:
  CSVExporter();

  virtual bool exec();
  virtual QString formatString() const;
  virtual QString fileFilter() const;

  virtual QWidget* widget(QWidget* parent, const char* name=0);
  virtual void readOptions(KConfig* config);
  virtual void saveOptions(KConfig* config);

private:
  QString& escapeText(QString& text);

  bool m_includeTitles;
  QString m_delimiter;

  QWidget* m_widget;
  QCheckBox* m_checkIncludeTitles;
  QRadioButton* m_radioComma;
  QRadioButton* m_radioSemicolon;
  QRadioButton* m_radioTab;
  QRadioButton* m_radioOther;
  KLineEdit* m_editOther;
};

  } // end namespace
} // end namespace
#endif
