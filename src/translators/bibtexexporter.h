/***************************************************************************
                              bibtexexporter.h
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

#ifndef BIBTEXEXPORTER_H
#define BIBTEXEXPORTER_H

class QCheckBox;

#include "exporter.h"
#include "bibtexhandler.h"

/**
 * The Bibtex exporter shows a list of possible Bibtex fields next to a combobox of all
 * the current attributes in the collection. I had thought about the reverse - having a list
 * of all the attributes, with comboboxes for each Bibtex field, but I think this way is more obvious.
 *
 * @author Robby Stephenson
 * @version $Id: bibtexexporter.h 216 2003-10-24 00:58:22Z robby $
 */
class BibtexExporter : public Exporter {
public: 
  BibtexExporter(const BCCollection* coll, const BCUnitList& list);

  virtual QWidget* widget(QWidget* parent, const char* name=0);
  virtual QString formatString() const;
  virtual QString text(bool format, bool encodeUTF8);
  virtual QString fileFilter() const;
  virtual void readOptions(KConfig*);
  virtual void saveOptions(KConfig*);

private:
  bool m_expandMacros;
  bool m_packageURL;
  BibtexHandler::QuoteStyle m_quoteStyle;

  QWidget* m_widget;
  QCheckBox* m_checkExpandMacros;
  QCheckBox* m_checkPackageURL;
};

#endif
