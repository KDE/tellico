/***************************************************************************
    Copyright (C) 2020 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_PRINTHANDLER_H
#define TELLICO_PRINTHANDLER_H

#include "datavectors.h"

class QPrinter;
class QWebEngineView;

namespace Tellico {

class PrintHandler {

public:
  PrintHandler();
  ~PrintHandler();

  void setEntries(const Data::EntryList& entries);
  void setColumns(const QStringList& columns);

  void print();
  void printPreview();

private:
  QString generateHtml() const;
  bool printPrepare();

  std::unique_ptr<QPrinter> m_printer;
  std::unique_ptr<QWebEngineView> m_view;
  bool m_inPrintPreview;
  Data::EntryList m_entries;
  QStringList m_columns;
  QString m_html;
};

}
#endif
