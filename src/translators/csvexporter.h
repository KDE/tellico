/***************************************************************************
                                csvexporter.h
                             -------------------
    begin                : Sat Aug 2 2003
    copyright            : (C) 2003 by Robby Stephenson
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

/**
 * @author Robby Stephenson
 * @version $Id: csvexporter.h 216 2003-10-24 00:58:22Z robby $
 */
class CSVExporter : public Exporter {
public: 
  CSVExporter(const BCCollection* coll, const BCUnitList& list);

  virtual QWidget* widget(QWidget* parent, const char* name=0);
  virtual QString formatString() const;
  virtual QString text(bool format, bool encodeUTF8);
  virtual QString fileFilter() const;
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

#endif
