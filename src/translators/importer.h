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

#ifndef IMPORTER_H
#define IMPORTER_H

class QWidget;

#include "../collection.h"

#include <kurl.h>

#include <qobject.h>
#include <qstring.h>

namespace Tellico {
  namespace Import {

/**
 * The top-level abstract class for importing other document formats into Tellico.
 *
 * The Importer classes import a file, and return a pointer to a newly created
 * @ref Data::Collection. Any errors or warnings are added to a status message queue.
 * The calling function owns the collection pointer.
 *
 * @author Robby Stephenson
 * @version $Id: importer.h 997 2004-12-07 14:35:31Z robby $
 */
class Importer : public QObject {
Q_OBJECT

public:
  Importer() : QObject() {}
  /**
   * The constructor should immediately load the contents of the file to be imported.
   * Any warnings or errors should be added the the status message queue.
   *
   * @param url The URL of the file to import
   */
  Importer(const KURL& url) : QObject(), m_url(url) {}
  /**
   */
  virtual ~Importer() {}

  /**
   * Returns a pointer to a @ref Data::Collection containing the contents of the imported file.
   * This function should probably only be called once, but the subclasses may cache the
   * collection. The collection should not be created until this function is called.
   *
   * @return A pointer to a @ref Collection created on the stack, or 0 if none could be created.
   */
  virtual Data::Collection* collection() = 0;
  /**
   * Returns a string containing all the messages added to the queue in the course of loading
   * and importing the file.
   *
   * @return The status message
   */
  const QString& statusMessage() const { return m_statusMsg; }
  /**
   * Returns a widget with the setting specific to this importer, or 0 if no
   * options are needed.
   *
   * @return A pointer to the setting widget
   */
  virtual QWidget* widget(QWidget*, const char*) { return 0; }
  /**
   * Checks to see if the importer can return a collection of this type
   *
   * @param type The collection type to check
   * @return Whether the importer could return a collection of that type
   */
  virtual bool canImport(Data::Collection::Type) { return true; }

public slots:
  virtual void slotActionChanged(int) {}

signals:
  /**
   * Signals that a fraction of an operation has been completed.
   *
   * @param f The fraction, 0 =< f >= 1
   */
  void signalFractionDone(float f);

protected:
  /**
   * Return the URL of the imported file.
   *
   * @return the file URL
   */
  const KURL& url() const { return m_url; }
  /**
   * Adds a message to the status queue.
   *
   * @param msg A string containing a warning or error.
   */
  void setStatusMessage(const QString& msg) { if(!msg.isEmpty()) m_statusMsg += msg + QChar(' '); }

  static const uint s_stepSize;

private:
  KURL m_url;
  QString m_statusMsg;
};

  } // end namespace
} // end namespace

#endif
