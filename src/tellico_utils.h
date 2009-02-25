/***************************************************************************
    copyright            : (C) 2003-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_UTILS_H
#define TELLICO_UTILS_H

#include <Qt>
#include <QCursor>

class KLibrary;

class QString;
class QStringList;

/**
 * This file contains utility functions.
 *
 * @author Robby Stephenson
 */
namespace Tellico {
  /**
   * Decode HTML entities. Only numeric entities are handled currently.
   */
  QString decodeHTML(const QByteArray& data);
  QString decodeHTML(QString text);
  /**
   * Return a random, and almost certainly unique UID.
   *
   * @param length The UID starts with "Tellico" and adds enough letters to be @p length long.
   */
  QString uid(int length=20, bool prefix=true);
  uint toUInt(const QString& string, bool* ok);
  /**
   * Replace all occurrences  of <i18n>text</i18n> with i18n("text")
   */
  QString i18nReplace(QString text);
  // copy the KDE 4.2 function to remove accelerators
  QString removeAcceleratorMarker(const QString& label);

  /**
   * Returns a list of the subdirectories in @param dir
   * Symbolic links are ignored
   */
  QStringList findAllSubDirs(const QString& dir);
  int stringHash(const QString& str);
  /** take advantage string collisions to reduce memory
  */
  QString shareString(const QString& str);

  QString minutes(int seconds);
  QString saveLocation(const QString& dir);
  QString fromHtmlData(const QByteArray& data);

  KLibrary* openLibrary(const QString& libName);

namespace GUI {
  class CursorSaver {
  public:
    CursorSaver(const QCursor& cursor = QCursor(Qt::WaitCursor));
    ~CursorSaver();
    void restore();
  private:
    bool m_restored : 1;
  };
}

}

#endif
