/***************************************************************************
                               finddialog.h
                             -------------------
    begin                : Wed Feb 27 2002
    copyright            : (C) 2002 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef FINDDIALOG_H
#define FINDDIALOG_H

class Bookcase;

class KComboBox;
class KHistoryCombo;

class QCheckBox;

#include <kdialogbase.h>

/**
 * The find dialog allows the user to search for a string in the document.
 *
 * @author Robby Stephenson
 * @version $Id: finddialog.h,v 1.4 2003/03/08 18:24:47 robby Exp $
 */
class FindDialog : public KDialogBase  {
Q_OBJECT

public: 
  /**
   * The constructor sets up the dialog.
   *
   * @param parent A pointer to the parent widget, a Bookcase object
   * @param name The widget name
   */
  FindDialog(Bookcase* parent, const char* name=0);

public slots:
  /**
   * Find the next match
   */
  void slotFindNext();

protected slots:
  /**
   * Called when the Find button is clicked.
   */
  void slotUser1();
  /**
   * Called when the search pattern changes
   *
   * @param text The text in the pattern
   */
  void slotPatternChanged(const QString& text);
  void showEvent(QShowEvent* e);

private:
  Bookcase* m_bookcase;
  
  KHistoryCombo* m_pattern;
  KComboBox* m_attributes;
  QCheckBox* m_caseSensitive;
  QCheckBox* m_findBackwards;
  QCheckBox* m_asRegExp;
  QCheckBox* m_wholeWords;
  QCheckBox* m_fromBeginning;
};

#endif
