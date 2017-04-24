/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_ONIXEXPORTER_H
#define TELLICO_ONIXEXPORTER_H

class QCheckBox;

#include "exporter.h"

namespace Tellico {
  namespace Data {
    class Collection;
  }
  class XSLTHandler;
  namespace Export {

/**
 * @author Robby Stephenson
 */
class ONIXExporter : public Exporter {
Q_OBJECT

public:
  ONIXExporter(Data::CollPtr coll);
  ~ONIXExporter();

  virtual bool exec() Q_DECL_OVERRIDE;
  virtual QString formatString() const Q_DECL_OVERRIDE;
  virtual QString fileFilter() const Q_DECL_OVERRIDE;

  virtual QWidget* widget(QWidget*) Q_DECL_OVERRIDE;
  virtual void readOptions(KSharedConfigPtr) Q_DECL_OVERRIDE;
  virtual void saveOptions(KSharedConfigPtr) Q_DECL_OVERRIDE;

  QString text();

private:
  XSLTHandler* m_handler;
  QString m_xsltFile;
  bool m_includeImages;

  QWidget* m_widget;
  QCheckBox* m_checkIncludeImages;
};

  } // end namespace
} // end namespace
#endif
