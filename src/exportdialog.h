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

#ifndef TELLICO_EXPORTDIALOG_H
#define TELLICO_EXPORTDIALOG_H

#include "translators/translators.h"
#include "datavectors.h"

#include <QDialog>
#include <QUrl>

class QCheckBox;
class QRadioButton;

namespace Tellico {
  namespace Export {
    class Exporter;
  }

/**
 * @author Robby Stephenson
 */
class ExportDialog : public QDialog {
Q_OBJECT

public:
  ExportDialog(Export::Format format, Data::CollPtr coll, const QUrl& baseUrl, QWidget* parent);
  ~ExportDialog();

  QString fileFilter();
  bool exportURL(const QUrl& url=QUrl()) const;

  static Export::Target exportTarget(Export::Format format);
  static bool exportCollection(Data::CollPtr coll, Data::EntryList entries, Export::Format format,
                               const QUrl& baseUrl, const QUrl& targetUrl);

private Q_SLOTS:
  void slotSaveOptions();

private:
  static Export::Exporter* exporter(Export::Format format, Data::CollPtr coll, const QUrl& baseUrl);

  void readOptions();

  Export::Format m_format;
  Data::CollPtr m_coll;
  Export::Exporter* m_exporter;
  QCheckBox* m_formatFields;
  QCheckBox* m_exportSelected;
  QCheckBox* m_exportFields;
  QRadioButton* m_encodeUTF8;
  QRadioButton* m_encodeLocale;
};

} // end namespace
#endif
