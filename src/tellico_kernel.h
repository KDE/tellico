/***************************************************************************
    copyright            : (C) 2003-2005 by Robby Stephenson
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
  const QStringList& Kernel::fieldTitles() const;
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

  const QString& entryName() const;
  int collectionType() const;

  void sorry(const QString& text, QWidget* widget=0);

  void beginCommandGroup(const QString& name);
  void endCommandGroup();
  void resetHistory();

  bool addField(Data::Field* field);
  bool modifyField(Data::Field* field);
  bool removeField(Data::Field* field);

  void saveEntries(Data::EntryVec oldEntries, Data::EntryVec entries);
  void removeEntries(Data::EntryVec entries);

  bool addLoans(Data::EntryVec entries);
  bool modifyLoan(Data::Loan* loan);
  bool removeLoans(Data::LoanVec loans);

  void addFilter(Filter* filter);
  bool modifyFilter(Filter* filter);
  bool removeFilter(Filter* filter);

  void reorderFields(const Data::FieldVec& fields);

  void appendCollection(Data::Collection* coll);
  void mergeCollection(Data::Collection* coll);
  void replaceCollection(Data::Collection* coll);

  void renameCollection();
  const KCommandHistory* commandHistory() { return &m_commandHistory; }

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
