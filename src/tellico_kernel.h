/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_KERNEL_H
#define TELLICO_KERNEL_H

#include "datavectors.h"
#include "borrower.h"

#include <kcommand.h>

class KURL;

class QWidget;
class QString;
class QStringList;

namespace Tellico {
  class MainWindow;
  class Filter;
  namespace Command {
    class Group;
  }
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
  const KURL& URL() const;
  /**
   * Returns a list of the field titles, wraps the call to the collection itself.
   *
   * @return the field titles
   */
  const QStringList& fieldTitles() const;
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

  void addEntries(Data::EntryVec entries, bool checkFields);
  void modifyEntries(Data::EntryVec oldEntries, Data::EntryVec newEntries);
  void updateEntry(Data::EntryPtr oldEntry, Data::EntryPtr newEntry, bool overWrite);
  void removeEntries(Data::EntryVec entries);

  bool addLoans(Data::EntryVec entries);
  bool modifyLoan(Data::LoanPtr loan);
  bool removeLoans(Data::LoanVec loans);

  void addFilter(FilterPtr filter);
  bool modifyFilter(FilterPtr filter);
  bool removeFilter(FilterPtr filter);

  void reorderFields(const Data::FieldVec& fields);

  void appendCollection(Data::CollPtr coll);
  void mergeCollection(Data::CollPtr coll);
  void replaceCollection(Data::CollPtr coll);

  // adds new fields into collection if any values in entries are not empty
  // first object is modified fields, second is new fields
  QPair<Data::FieldVec, Data::FieldVec> mergeFields(Data::CollPtr coll,
                                                    Data::FieldVec fields,
                                                    Data::EntryVec entries);

  void renameCollection();
  const KCommandHistory* commandHistory() { return &m_commandHistory; }

  int askAndMerge(Data::EntryPtr entry1, Data::EntryPtr entry2, Data::FieldPtr field,
                  QString value1 = QString(), QString value2 = QString());

private:
  static Kernel* s_self;

  // all constructors are private
  Kernel(MainWindow* parent);
  Kernel(const Kernel&);
  Kernel& operator=(const Kernel&);

  void doCommand(KCommand* command);

  QWidget* m_widget;
  KCommandHistory m_commandHistory;
  Command::Group* m_commandGroup;
};

} // end namespace
#endif
