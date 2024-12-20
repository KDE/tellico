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

#ifndef TELLICO_BIBTEXIMPORTER_H
#define TELLICO_BIBTEXIMPORTER_H

#include "importer.h"
#include "../datavectors.h"

#include <config.h>
#ifdef ENABLE_BTPARSE
extern "C" {
#ifdef HAVE_LIBBTPARSE
/* btparse has a struct member 'class' */
#define class errclass
#include <btparse.h>
#undef class
#else
#include "btparse/btparse.h"
#endif
}
#else
class AST;
#endif

#include <QList>
#include <QHash>

class QRadioButton;

namespace Tellico {
  namespace Import {

/**
 * Bibtex files are used for bibliographies within LaTex. The btparse library is used to
 * parse the text and generate a @ref BibtexCollection.
 *
 * @author Robby Stephenson
 */
class BibtexImporter : public Importer {
Q_OBJECT

public:
  /**
   * Initializes the btparse library
   *
   * @param url The url of the bibtex file
   */
  BibtexImporter(const QList<QUrl>& urls);
  BibtexImporter(const QString& text);
  /*
   * Some cleanup is done for the btparse library
   */
  virtual ~BibtexImporter();

  /**
   * Returns a pointer to a @ref BibtexCollection created on the stack. All entries
   * in the bibtex file are added, including any preamble, all macro strings, and each entry.
   *
   * @return A pointer to a @ref BibtexCollection, or 0 if none can be created.
   */
  virtual Data::CollPtr collection() override;
  virtual QWidget* widget(QWidget* parent) override;
  virtual bool canImport(int type) const override;

  static bool maybeBibtex(const QUrl& url);
  static bool maybeBibtex(const QString& text, const QUrl& url = QUrl());

public Q_SLOTS:
  void slotCancel() override;

private:
  void init();
  Data::CollPtr readCollection(const QString& text, int n);
  void parseText(const QString& text);
  void appendCollection(Data::CollPtr newColl);

  QList<AST*> m_nodes;
  QHash<QString, QString> m_macros;

  Data::CollPtr m_coll;
  QWidget* m_widget;
  QRadioButton* m_readUTF8;
  QRadioButton* m_readLocale;
  bool m_cancelled : 1;

  static int s_initCount;
};

  } // end namespace
} // end namespace
#endif
