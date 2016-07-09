/***************************************************************************
    Copyright (C) 2004-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_RISIMPORTER_H
#define TELLICO_RISIMPORTER_H

#include "importer.h"
#include "../datavectors.h"

#include <QString>
#include <QHash>

namespace Tellico {
  namespace Import {

/**
 * @author Robby Stephenson
 */
class RISImporter : public Importer {
Q_OBJECT

public:
  /**
   */
  RISImporter(const QList<QUrl>& urls);
  RISImporter(const QString& text);

  /**
   * @return A pointer to a @ref Data::Collection, or 0 if none can be created.
   */
  virtual Data::CollPtr collection();
  /**
   */
  virtual QWidget* widget(QWidget*) { return 0; }
  virtual bool canImport(int type) const;

  static bool maybeRIS(const QUrl& url);

public Q_SLOTS:
  void slotCancel();

private:
  static void initTagMap();
  static void initTypeMap();

  Data::FieldPtr fieldByTag(const QString& tag);
  void readURL(const QUrl& url, int n, const QHash<QString, Data::FieldPtr>& risFields);
  void readText(const QString& text, int n, const QHash<QString, Data::FieldPtr>& risFields);

  Data::CollPtr m_coll;
  bool m_cancelled;

  static QHash<QString, QString>* s_tagMap;
  static QHash<QString, QString>* s_typeMap;
};

  } // end namespace
} // end namespace
#endif
