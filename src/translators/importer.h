/***************************************************************************
    copyright            : (C) 2003-2009 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_IMPORT_IMPORTER_H
#define TELLICO_IMPORT_IMPORTER_H

#include "../datavectors.h"
#include "../collection.h"

#include <kurl.h>
#include <klocale.h>

#include <QObject>
#include <QString>

class QWidget;

namespace Tellico {
  namespace Import {
    enum Options {
      ImportProgress = 1 << 5   // show progress bar
    };

/**
 * The top-level abstract class for importing other document formats into Tellico.
 *
 * The Importer classes import a file, and return a pointer to a newly created
 * @ref Data::Collection. Any errors or warnings are added to a status message queue.
 * The calling function owns the collection pointer.
 *
 * @author Robby Stephenson
 */
class Importer : public QObject {
Q_OBJECT

public:
  Importer();
  /**
   * The constructor should immediately load the contents of the file to be imported.
   * Any warnings or errors should be added the the status message queue.
   *
   * @param url The URL of the file to import
   */
  Importer(const KUrl& url);
  Importer(const KUrl::List& urls);
  Importer(const QString& text);
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
  virtual Data::CollPtr collection() = 0;
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
  virtual QWidget* widget(QWidget*) { return 0; }
  /**
   * Checks to see if the importer can return a collection of this type
   *
   * @param type The collection type to check
   * @return Whether the importer could return a collection of that type
   */
  virtual bool canImport(int) const { return true; }
  /**
   * Validate the import settings
   */
  virtual bool validImport() const { return true; }
  virtual void setText(const QString& text) { m_text = text; }
  long options() const { return m_options; }
  void setOptions(long options) { m_options = options; }
  /**
   * Returns a string useful for the ProgressManager
   */
  QString progressLabel() const;
  /**
   * Sets a pointer to the existing collection in case importers need to use existing field information
   */
  void setCurrentCollection(Data::CollPtr coll) { m_currentCollection = coll; }

public slots:
  /**
   * The import action was changed in the import dialog
   */
  virtual void slotActionChanged(int) {}

signals:
  void signalTotalSteps(QObject* obj, qulonglong steps);
  void signalProgress(QObject* obj, qulonglong progress);

protected:
  /**
   * Return the URL of the imported file.
   *
   * @return the file URL
   */
  KUrl url() const { return m_urls.isEmpty() ? KUrl() : m_urls[0]; }
  KUrl::List urls() const { return m_urls; }
  QString text() const { return m_text; }
  Data::CollPtr currentCollection() const;
  /**
   * Adds a message to the status queue.
   *
   * @param msg A string containing a warning or error.
   */
  void setStatusMessage(const QString& msg) { if(!msg.isEmpty()) m_statusMsg += msg + QLatin1Char(' '); }

  static const uint s_stepSize;

private:
  long m_options;
  KUrl::List m_urls;
  QString m_text;
  QString m_statusMsg;
  Data::CollPtr m_currentCollection;
};

  } // end namespace
} // end namespace

#endif
