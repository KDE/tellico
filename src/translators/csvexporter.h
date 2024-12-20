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

#ifndef TELLICO_CSVEXPORTER_H
#define TELLICO_CSVEXPORTER_H

#include "exporter.h"

class QLineEdit;
class QWidget;
class QCheckBox;
class QRadioButton;

namespace Tellico {
  namespace Export {

/**
 * @author Robby Stephenson
 */
class CSVExporter : public Exporter {
Q_OBJECT

public:
  CSVExporter(Data::CollPtr coll);

  virtual bool exec() override;
  virtual QString formatString() const override;
  virtual QString fileFilter() const override;
  QString text() const;

  virtual QWidget* widget(QWidget* parent) override;
  virtual void readOptions(KSharedConfigPtr config) override;
  virtual void saveOptions(KSharedConfigPtr config) override;

private:
  QString& escapeText(QString& text) const;

  bool m_includeTitles;
  QString m_delimiter;
  QString m_colDelimiter;
  QString m_rowDelimiter;

  QWidget* m_widget;
  QCheckBox* m_checkIncludeTitles;
  QRadioButton* m_radioComma;
  QRadioButton* m_radioSemicolon;
  QRadioButton* m_radioTab;
  QRadioButton* m_radioOther;
  QLineEdit* m_editOther;
  QLineEdit* m_colDelimiterEdit;
  QLineEdit* m_rowDelimiterEdit;
};

  } // end namespace
} // end namespace
#endif
