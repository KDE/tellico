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

#ifndef TELLICO_IMPORT_IMPORTER_H
#define TELLICO_IMPORT_IMPORTER_H

#include "../datavectors.h"
#include "../collection.h"

#include <QObject>
#include <QString>
#include <QUrl>

class QWidget;

namespace Tellico {
  namespace Import {
    enum Options {
      ImportProgress        = 1 << 0,   // show progress bar
      ImportShowImageErrors = 1 << 1,
      ImportImagesAsLinks   = 1 << 2
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
   * Any warnings or errors should be added to the status message queue.
   *
   * @param url The URL of the file to import
   */
  Importer(const QUrl& url);
  Importer(const QList<QUrl>& urls);
  Importer(const QString& text);
  /**
   */
  virtual ~Importer() = default;

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
  virtual QWidget* widget(QWidget*) { return nullptr; }
  /**
   * Checks to see if the importer can return a collection of this type
   *
   * @param type The collection type to check
   * @return Whether the importer could return a collection of that type
   */
  virtual bool canImport(int type) const = 0;
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

public Q_SLOTS:
  /**
   * The import action was changed in the import dialog
   */
  virtual void slotActionChanged(int) {}
  virtual void slotCancel() = 0;

Q_SIGNALS:
  void signalTotalSteps(QObject* obj, qulonglong steps);
  void signalProgress(QObject* obj, qulonglong progress);

protected:
  /**
   * Return the URL of the imported file.
   *
   * @return the file URL
   */
  QUrl url() const { return m_urls.isEmpty() ? QUrl() : m_urls[0]; }
  QList<QUrl> urls() const { return m_urls; }
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
  QList<QUrl> m_urls;
  QString m_text;
  QString m_statusMsg;
  Data::CollPtr m_currentCollection;
};

  } // end namespace
} // end namespace

#endif
