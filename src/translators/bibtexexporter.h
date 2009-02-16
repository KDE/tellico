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

#ifndef TELLICO_BIBTEXEXPORTER_H
#define TELLICO_BIBTEXEXPORTER_H

class QCheckBox;
class KComboBox;

#include "exporter.h"

namespace Tellico {
  namespace Export {

/**
 * The Bibtex exporter shows a list of possible Bibtex fields next to a combobox of all
 * the current attributes in the collection. I had thought about the reverse - having a list
 * of all the attributes, with comboboxes for each Bibtex field, but I think this way is more obvious.
 *
 * @author Robby Stephenson
 */
class BibtexExporter : public Exporter {
Q_OBJECT

public:
  BibtexExporter();

  virtual bool exec();
  virtual QString formatString() const;
  virtual QString fileFilter() const;

  virtual QWidget* widget(QWidget* parent);
  virtual void readOptions(KSharedConfigPtr);
  virtual void saveOptions(KSharedConfigPtr);

private:
  void writeEntryText(QString& text, const Data::FieldList& field, const Data::Entry& entry,
                      const QString& type, const QString& key);

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
