/***************************************************************************
    Copyright (C) 2006-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_IMPORT_GRS1IMPORTER_H
#define TELLICO_IMPORT_GRS1IMPORTER_H

#include "textimporter.h"

#include <QVariant>
#include <QMap>
#include <QPair>

namespace Tellico {
  namespace Import {

/**
 * @author Robby Stephenson
 */
class GRS1Importer : public TextImporter {
Q_OBJECT

public:
  GRS1Importer(const QString& text);
  virtual ~GRS1Importer() {}

  /**
   * @return A pointer to a @ref Data::Collection, or 0 if none can be created.
   */
  virtual Data::CollPtr collection() override;
  /**
   */
  virtual QWidget* widget(QWidget*) override { return nullptr; }
  virtual bool canImport(int type) const override;

public Q_SLOTS:
  void slotCancel() override {}

private:
  static void initTagMap();

  class TagPair : public QPair<int, QVariant> {
  public:
    TagPair() : QPair<int, QVariant>(-1, QVariant()) {}
    TagPair(int n, const QVariant& v) : QPair<int, QVariant>(n, v) {}
    QString toString() const { return QString::number(first) + second.toString(); }
    bool operator< (const TagPair& p) const {
      return toString() < p.toString();
    }
  };

  typedef QMap<TagPair, QString> TagMap;
  static TagMap* s_tagMap;
};

  } // end namespace
} // end namespace
#endif
