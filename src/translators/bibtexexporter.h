/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
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
  BibtexExporter(Data::CollPtr coll);

  virtual bool exec() override;
  virtual QString formatString() const override;
  virtual QString fileFilter() const override;
  QString text();

  virtual QWidget* widget(QWidget* parent) override;
  virtual void readOptions(KSharedConfigPtr) override;
  virtual void saveOptions(KSharedConfigPtr) override;

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
