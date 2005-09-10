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

#ifndef TELLICO_UTILS_H
#define TELLICO_UTILS_H

class QCursor;
class QStringList;
class QFontMetrics;
class QScrollView;

#include <qstring.h>
#include <qcolor.h>

/**
 * This file contains utility functions.
 *
 * @author Robby Stephenson
 */
namespace Tellico {
  /**
   * Decode HTML entities. Only numeric entities are handled currently.
   */
  QString decodeHTML(QString text);
  /**
   * Return a random, and almost certainly unique UID.
   *
   * @param length The UID starts with "Tellico" and adds enough letters to be @p length long.
   */
  QString uid(int length=20);
  uint toUInt(const QString& string, bool* ok);
  /**
   * Replace all occurences  of <i18n>text</i18n> with i18n("text")
   */
  QString i18nReplace(QString text);
  /**
   * Returns a list of the subdirectories in @param dir
   *.Symbolic links are ignored
   */
  QStringList findAllSubDirs(const QString& dir);

  extern QColor contrastColor;
  void updateContrastColor(const QColorGroup& cg);

  QString rPixelSqueeze(const QString& str, const QFontMetrics& fm, uint pixels);

namespace GUI {
  class CursorSaver {
  public:
    CursorSaver(const QCursor& cursor);
    ~CursorSaver();
    void restore();
  private:
    bool m_restored : 1;
  };

  class WidgetUpdateBlocker {
  public:
    WidgetUpdateBlocker(QScrollView* widget);
    ~WidgetUpdateBlocker();
  private:
    QScrollView* m_widget;
    bool m_wasEnabled : 1;
  };
}

}

#endif
