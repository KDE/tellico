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

#ifndef TELLICO_KERNEL_H
#define TELLICO_KERNEL_H

namespace Tellico {
  class QRegExpDict;
  namespace Data {
    class Document;
    class Collection;
  }
}

class KURL;

class QWidget;

#include <qobject.h>

namespace Tellico {

/**
 * @author Robby Stephenson
 * @version $Id: tellico_kernel.h 979 2004-12-01 05:26:42Z robby $
 */
class Kernel : public QObject {
Q_OBJECT

public:
  /**
   * Flags used for searching. The options should be bit-wise OR'd together.
   * @li AllFields - Search through all fields
   * @li AsRegExp - Use the text as the pattern for a regexp search
   * @li FindBackwards - search backwards
   * @li CaseSensitive - Case sensitive search
   */
  enum SearchOptions {
    AllFields     = 1 << 0,
    AsRegExp      = 1 << 1,
    FindBackwards = 1 << 2,
    CaseSensitive = 1 << 3
  };

  ~Kernel();

  static Kernel* self() { return s_self; }
  /**
   * Initializes the singleton. Should just be called once, from Tellico::MainWindow
   */
  static void init(QObject* parent, const char* name=0) { if(!s_self) s_self = new Kernel(parent, name); }

  /**
   * Returns a pointer to the parent widget. This is mainly used for error dialogs and the like.
   *
   * @return The widget pointer
   */
  QWidget* widget() { return m_widget; }
  /**
   * Returns a pointer to the document object.
   *
   * @return The document pointer
   */
  Data::Document* doc() { return m_doc; }
  /**
   * Returns the url of the current document.
   *
   * @return The URL
   */
  const KURL& URL();
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
  /**
   * @param text Text to search for
   * @param title Title of field to search, or empty for all
   * @param options The options, bit-wise OR'd together
   */
  void searchDocument(const QString& text, const QString& title, int options);

private:
  Kernel(QObject* parent, const char* name);
  static Kernel* s_self;

  QWidget* m_widget;
  Data::Document* m_doc;
};

} // end namespace
#endif
