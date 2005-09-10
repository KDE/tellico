/*  -*- c++ -*-
    kwidgetlister.h

    This file is part of libkdenetwork.
    Copyright (c) 2001 Marc Mutz <mutz@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2, as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this library with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifndef _KWIDGETLISTER_H_
#define _KWIDGETLISTER_H_

#include <qwidget.h>
#include <qptrlist.h>

class QPushButton;
class QVBoxLayout;
class QHBox;

/** Simple widget that nonetheless does a lot of the dirty work for
    the filter edit widgets (@ref KMSearchpatternEdit and @ref
    KMFilterActionEdit). It provides a growable and shrinkable area
    where widget may be diplayed in rows. Widgets can be added by
    hitting the provided 'More' button, removed by the 'Fewer' button
    and cleared (e.g. reset, if an derived class implements that and
    removed for all but @ref mMinWidgets).

    To use this widget, derive from it with the template changed to
    the type of widgets this class should list. Then reimplement @ref
    addWidgetAtEnd, @ref removeLastWidget, calling the original
    implementation as necessary. Instantate an object of the class and
    put it in your dialog.

    @short Widget that manages a list of other widgets (incl. 'more', 'fewer' and 'clear' buttons).
    @author Marc Mutz <Marc@Mutz.com>
    @see KMSearchPatternEdit::WidgetLister KMFilterActionEdit::WidgetLister

*/

class KWidgetLister : public QWidget
{
  Q_OBJECT
public:
  KWidgetLister( int minWidgets=1, int maxWidgets=8, QWidget* parent=0, const char* name=0 );
  virtual ~KWidgetLister();

protected slots:
  /** Called whenever the user clicks on the 'more' button.
      Reimplementations should call this method, because this
      implementation does all the dirty work with adding the widgets
      to the layout (through @ref addWidgetAtEnd) and enabling/disabling
      the control buttons. */
  virtual void slotMore();
  /** Called whenever the user clicks on the 'fewer' button.
      Reimplementations should call this method, because this
      implementation does all the dirty work with removing the widgets
      from the layout (through @ref removelastWidget) and
      enabling/disabling the control buttons. */
  virtual void slotFewer();
  /** Called whenever the user clicks on the 'clear' button.
      Reimplementations should call this method, because this
      implementation does all the dirty work with removing all but
      @ref mMinWidets widgets from the layout and enabling/disabling
      the control buttons. */
  virtual void slotClear();



protected:
  /** Adds a single widget. Doesn't care if there are already @ref
      mMaxWidgets on screen and whether it should enable/disable any
      controls. It simply does what it is asked to do.  You want to
      reimplement this method if you want to initialize the the widget
      when showing it on screen. Make sure you call this
      implementaion, though, since you cannot put the widget on screen
      from derived classes (@p mLayout is private).
      Make sure the parent of the QWidget to add is this KWidgetLister. */
  virtual void addWidgetAtEnd(QWidget *w =0);
  /** Removes a single (always the last) widget. Doesn't care if there
      are still only @ref mMinWidgets left on screen and whether it
      should enable/disable any controls. It simply does what it is
      asked to do. You want to reimplement this method if you want to
      save the the widget's state before removing it from screen. Make
      sure you call this implementaion, though, since you should not
      remove the widget from screen from derived classes. */
  virtual void removeLastWidget();
  /** Called to clear a given widget. The default implementation does
      nothing. */
  virtual void clearWidget( QWidget* );
  /** Because QT 2.x does not support signals/slots in template
      classes, we are forced to emulate this by forcing the
      implementers of subclasses of KWidgetLister to reimplement this
      function which replaces the "@p new @p T" call. */
  virtual QWidget* createWidget( QWidget *parent );
  /** Sets the number of widgets on scrren to exactly @p aNum. Doesn't
      check if @p aNum is inside the range @p
      [mMinWidgets,mMaxWidgets]. */
  virtual void setNumberOfShownWidgetsTo( int aNum );
  /** The list of widgets. Note that this list is set to auto-delete,
      meaning that widgets that are removed from the screen by either
      @ref slotFewer or @ref slotClear will be destroyed! */
  QPtrList<QWidget> mWidgetList;
  /** The minimum number of widgets that are to stay on screen. */
  int mMinWidgets;
  /** The maximum number of widgets that are to be shown on screen. */
  int mMaxWidgets;

signals:
  /** This signal is emitted whenever a widget was added */
  void widgetAdded();
  /** This signal is emitted whenever a widget was added */
  void widgetAdded(QWidget *);
  /** This signal is emitted whenever a widget was removed */
  void widgetRemoved();
  /** This signal is emitted whenever the clear button is clicked */
  void clearWidgets();

private:
  void enableControls();

  QPushButton *mBtnMore, *mBtnFewer, *mBtnClear;
  QVBoxLayout *mLayout;
  QHBox       *mButtonBox;
};



#endif /* _KWIDGETLISTER_H_ */
