/***************************************************************************
    copyright            : (C) 2004-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
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
  RISImporter(const KUrl::List& urls);

  /**
   * @return A pointer to a @ref Data::Collection, or 0 if none can be created.
   */
  virtual Data::CollPtr collection();
  /**
   */
  virtual QWidget* widget(QWidget*) { return 0; }
  virtual bool canImport(int type) const;

  static bool maybeRIS(const KUrl& url);

public slots:
  void slotCancel();

private:
  static void initTagMap();
  static void initTypeMap();

  Data::FieldPtr fieldByTag(const QString& tag);
  void readURL(const KUrl& url, int n, const QHash<QString, Data::FieldPtr>& risFields);

  Data::CollPtr m_coll;
  bool m_cancelled;

  static QHash<QString, QString>* s_tagMap;
  static QHash<QString, QString>* s_typeMap;
};

  } // end namespace
} // end namespace
#endif
