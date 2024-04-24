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

#ifndef TELLICO_KERNEL_H
#define TELLICO_KERNEL_H

#include "datavectors.h"
#include "borrower.h"

#include <QStringList>

class QUndoStack;
class QWidget;
class QUndoCommand;
class QUrl;

namespace Tellico {
  class Filter;
  namespace Data {
    class Collection;
  }

/**
 * @author Robby Stephenson
 */
class Kernel : public QObject {
Q_OBJECT

public:
  static Kernel* self() { return s_self; }
  /**
   * Initializes the singleton. Should just be called once, from Tellico::MainWindow
   */
  static void init(QWidget* parent) { if(!s_self) s_self = new Kernel(parent); }

  /**
   * Returns a pointer to the parent widget. This is mainly used for error dialogs and the like.
   *
   * @return The widget pointer
   */
  QWidget* widget() { return m_widget; }

  /**
   * Returns the url of the current document.
   *
   * @return The URL
   */
  QUrl URL() const;

  int collectionType() const;
  QString collectionTypeName() const;

  void sorry(const QString& text, QWidget* widget=nullptr);

  void resetHistory();

  void addEntries(Data::EntryList entries, bool checkFields);
  void modifyEntries(Data::EntryList oldEntries, Data::EntryList newEntries, const QStringList& modifiedFields);
  void updateEntry(Data::EntryPtr oldEntry, Data::EntryPtr newEntry, bool overWrite);
  void removeEntries(Data::EntryList entries);

  bool addLoans(Data::EntryList entries);
  bool modifyLoan(Data::LoanPtr loan);
  bool removeLoans(Data::LoanList loans);

  void addFilter(FilterPtr filter);
  bool modifyFilter(FilterPtr filter);
  bool removeFilter(FilterPtr filter);

  void appendCollection(Data::CollPtr coll);
  void mergeCollection(Data::CollPtr coll);
  void replaceCollection(Data::CollPtr coll);

  void renameCollection();
  QUndoStack* commandHistory() { return m_commandHistory; }

  int askAndMerge(Data::EntryPtr entry1, Data::EntryPtr entry2, Data::FieldPtr field,
                  QString value1 = QString(), QString value2 = QString());

public Q_SLOTS:
  void beginCommandGroup(const QString& name);
  void endCommandGroup();

  bool addField(Tellico::Data::FieldPtr field);
  bool modifyField(Tellico::Data::FieldPtr field);
  bool removeField(Tellico::Data::FieldPtr field);
  void reorderFields(const Tellico::Data::FieldList& fields);

private:
  static Kernel* s_self;

  // all constructors are private
  Kernel(QWidget* parent);
  Q_DISABLE_COPY(Kernel)
  ~Kernel();

  void doCommand(QUndoCommand* command);

  QWidget* m_widget;
  QUndoStack* m_commandHistory;
};

} // end namespace
#endif
