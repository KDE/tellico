/***************************************************************************
    copyright            : (C) 2005-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_IMPORT_GCFILMSIMPORTER_H
#define TELLICO_IMPORT_GCFILMSIMPORTER_H

#include "textimporter.h"
#include "../datavectors.h"

class QRegExp;

namespace Tellico {
  namespace Import {

/**
 * @author Robby Stephenson
*/
class GCfilmsImporter : public TextImporter {
Q_OBJECT

public:
  /**
   */
  GCfilmsImporter(const KUrl& url);

  /**
   *
   */
  virtual Data::CollPtr collection();
  /**
   */
  virtual QWidget* widget(QWidget*) { return 0; }
  virtual bool canImport(int type) const;

public slots:
  void slotCancel();

private:
  static QString splitJoin(const QRegExp& rx, const QString& s);

  void readGCfilms(const QString& text);
  void readGCstar(const QString& text);

  Data::CollPtr m_coll;
  bool m_cancelled;
};

  } // end namespace
} // end namespace
#endif
