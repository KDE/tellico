/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
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
class KComboBox;

#include "textexporter.h"
#include "bibtexhandler.h"

namespace Tellico {
  namespace Export {

/**
 * The Bibtex exporter shows a list of possible Bibtex fields next to a combobox of all
 * the current attributes in the collection. I had thought about the reverse - having a list
 * of all the attributes, with comboboxes for each Bibtex field, but I think this way is more obvious.
 *
 * @author Robby Stephenson
 * @version $Id: bibtexexporter.h 905 2004-09-23 04:29:10Z robby $
 */
class BibtexExporter : public TextExporter {
public:
  BibtexExporter(const Data::Collection* coll);

  virtual QWidget* widget(QWidget* parent, const char* name=0);
  virtual QString formatString() const;
  virtual QString text(bool format, bool encodeUTF8);
  virtual QString fileFilter() const;
  virtual void readOptions(KConfig*);
  virtual void saveOptions(KConfig*);

private:
  bool m_expandMacros;
  bool m_packageURL;
  bool m_skipEmptyKeys;

  QWidget* m_widget;
  QCheckBox* m_checkExpandMacros;
  QCheckBox* m_checkPackageURL;
  QCheckBox* m_checkSkipEmpty;
  KComboBox* m_cbBibtexStyle;
};

  } // end namespace
} // end namespace
#endif
