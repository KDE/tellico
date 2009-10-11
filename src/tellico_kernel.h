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

class KUrl;
class KUndoStack;
namespace KWallet {
  class Wallet;
}

class QWidget;
class QString;
class QStringList;
class QUndoCommand;

namespace Tellico {
  class MainWindow;
  class Filter;
  namespace Data {
    class Collection;
  }

/**
 * @author Robby Stephenson
 */
class Kernel {

public:
  static Kernel* self() { return s_self; }
  /**
   * Initializes the singleton. Should just be called once, from Tellico::MainWindow
   */
  static void init(MainWindow* parent) { if(!s_self) s_self = new Kernel(parent); }

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
  KUrl URL() const;
  /**
   * Returns a list of the field titles, wraps the call to the collection itself.
   *
   * @return the field titles
   */
  QStringList fieldTitles() const;
  /**
   * Returns the name of an field, given its title. Wraps the call to the collection itself.
   *
   * @param title The field title
   * @return The field name
   */
  QString fieldNameByTitle(const QString& title) const;
  /**
   * Returns the title of an field, given its name. Wraps the call to the collection itself.
   *
   * @param name The field name
   * @return The field title
   */
  QString fieldTitleByName(const QString& name) const;
  QStringList valuesByFieldName(const QString& name) const;

  int collectionType() const;
  QString collectionTypeName() const;

  void sorry(const QString& text, QWidget* widget=0);

  void beginCommandGroup(const QString& name);
  void endCommandGroup();
  void resetHistory();

  bool addField(Data::FieldPtr field);
  bool modifyField(Data::FieldPtr field);
  bool removeField(Data::FieldPtr field);

  void addEntries(Data::EntryList entries, bool checkFields);
  void modifyEntries(Data::EntryList oldEntries, Data::EntryList newEntries);
  void updateEntry(Data::EntryPtr oldEntry, Data::EntryPtr newEntry, bool overWrite);
  void removeEntries(Data::EntryList entries);

  bool addLoans(Data::EntryList entries);
  bool modifyLoan(Data::LoanPtr loan);
  bool removeLoans(Data::LoanList loans);

  void addFilter(FilterPtr filter);
  bool modifyFilter(FilterPtr filter);
  bool removeFilter(FilterPtr filter);

  void reorderFields(const Data::FieldList& fields);

  void appendCollection(Data::CollPtr coll);
  void mergeCollection(Data::CollPtr coll);
  void replaceCollection(Data::CollPtr coll);

  // adds new fields into collection if any values in entries are not empty
  // first object is modified fields, second is new fields
  QPair<Data::FieldList, Data::FieldList> mergeFields(Data::CollPtr coll,
                                                      Data::FieldList fields,
                                                      Data::EntryList entries);

  void renameCollection();
  KUndoStack* commandHistory() { return m_commandHistory; }

  int askAndMerge(Data::EntryPtr entry1, Data::EntryPtr entry2, Data::FieldPtr field,
                  QString value1 = QString(), QString value2 = QString());

  QByteArray readWalletEntry(const QString& key);
  QMap<QString, QString> readWalletMap(const QString& key);

private:
  static Kernel* s_self;

  // all constructors are private
  Kernel(MainWindow* parent);
  Kernel(const Kernel&);
  Kernel& operator=(const Kernel&);

  void doCommand(QUndoCommand* command);
  bool prepareWallet();

  QWidget* m_widget;
  KUndoStack* m_commandHistory;
  KWallet::Wallet* m_wallet;
};

} // end namespace
#endif
