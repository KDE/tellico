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

#ifndef TELLICO_IMPORT_GCSTARIMPORTER_H
#define TELLICO_IMPORT_GCSTARIMPORTER_H

#include "textimporter.h"
#include "../datavectors.h"

class QRegularExpression;
class QCheckBox;

namespace Tellico {
  namespace Import {

/**
 * @author Robby Stephenson
*/
class GCstarImporter : public TextImporter {
Q_OBJECT

public:
  /**
   */
  GCstarImporter(const QUrl& url);
  GCstarImporter(const QString& text);

  virtual Data::CollPtr collection() override;
  virtual bool canImport(int type) const override;
  void setImagePathsAsLinks(bool imagePathsAsLinks);

  virtual QWidget* widget(QWidget*) override;

public Q_SLOTS:
  void slotCancel() override;

private:
  static QString splitJoin(const QRegularExpression& rx, const QString& s);

  void readGCfilms(const QString& text);
  void readGCstar(const QString& text, const QString& collType);

  Data::CollPtr m_coll;
  bool m_cancelled;
  bool m_imageLinksOnly;

  QWidget* m_widget;
  QCheckBox* m_cbImageLink;
};

  } // end namespace
} // end namespace
#endif
