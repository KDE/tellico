/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef BOOKCASEKERNEL_H
#define BOOKCASEKERNEL_H

#include <qwidget.h>

namespace Bookcase {
  namespace Data {
    class Document;
    class Collection;
  }

/**
 * @author Robby Stephenson
 * @version $Id: kernel.h 714 2004-07-29 00:38:46Z robby $
 */
class Kernel : public QObject {
Q_OBJECT

public:
  static Kernel* self() { return s_self; }
  /**
   * Initializes the singleton. Should just be called once, from Bookcase::MainWindow
   */
  static void init(QWidget* parent, const char* name=0) { if(!s_self) s_self = new Kernel(parent, name); }

  /**
   * Returns a pointer to the parent widget. This is mainly used for error dialogs and the like.
   *
   * @return The widget pointer
   */
  QWidget* widget() { return static_cast<QWidget*>(parent()); }
  /**
   * Returns a pointer to the document object.
   *
   * @return The document pointer
   */
  Data::Document* doc() { return m_doc; }
  /**
   * Returns a pointer to the document's collection.
   *
   * @return The collection pointer
   */
  Data::Collection* collection();
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

private:
  static Kernel* s_self;
  Kernel(QWidget* parent, const char* name);

  Data::Document* m_doc;
};

} // end namespace
#endif
