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

#ifndef TELLICO_XSLTEXPORTER_H
#define TELLICO_XSLTEXPORTER_H

class KUrlRequester;

#include "exporter.h"

namespace Tellico {
  namespace Export {

/**
 * @author Robby Stephenson
 */
class XSLTExporter : public Exporter {
public:
  XSLTExporter(Data::CollPtr coll, const QUrl& baseUrl);

  virtual bool exec() override;
  virtual QString formatString() const override;
  virtual QString fileFilter() const override;

  virtual QWidget* widget(QWidget* parent) override;

  virtual void readOptions(KSharedConfigPtr config) override;
  virtual void saveOptions(KSharedConfigPtr config) override;

private:
  QWidget* m_widget;
  KUrlRequester* m_URLRequester;
  QUrl m_xsltFile;
};

  } // end namespace
} // end namespace
#endif
