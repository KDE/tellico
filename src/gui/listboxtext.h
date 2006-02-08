/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_GUI_LISTBOXTEXT_H
#define TELLICO_GUI_LISTBOXTEXT_H

#include <qlistbox.h>

namespace Tellico {
  namespace GUI {

/**
 * ListBoxText subclasses QListBoxText so that @ref setText() can be made public,
 * and the font color can be changed
 *
 * @author Robby Stephenson
 */
class ListBoxText : public QListBoxText {
public:
  ListBoxText(QListBox* listbox, const QString& text);
  ListBoxText(QListBox* listbox, const QString& text, QListBoxItem* after);

  virtual int width(const QListBox* box) const;
  virtual int height(const QListBox* box) const;

  void setColored(bool colored);
  void setText(const QString& text);

protected:
  virtual void paint(QPainter* painter);

private:
  bool m_colored;
};

  } // end namespace
} // end namespace

#endif
