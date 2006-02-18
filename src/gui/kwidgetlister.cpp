/*  -*- c++ -*-
    kwidgetlister.cpp

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
    Foundation, Inc., 51 Franklin Street, Fifh Floor, Boston, MA  02110-1301  USA

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

#include "kwidgetlister.h"

#include <klocale.h>
#include <kdebug.h>
#include <kpushbutton.h>
#include <kiconloader.h>

#include <qlayout.h>
#include <qhbox.h>

#include <assert.h>

KWidgetLister::KWidgetLister( int minWidgets, int maxWidgets, QWidget *parent, const char* name )
  : QWidget( parent, name )
{
  mWidgetList.setAutoDelete(TRUE);

  mMinWidgets = QMAX( minWidgets, 1 );
  mMaxWidgets = QMAX( maxWidgets, mMinWidgets + 1 );

  //--------- the button box
  mLayout = new QVBoxLayout(this, 0, 4);
  mButtonBox = new QHBox(this);
  mButtonBox->setSpacing(4);
  mLayout->addWidget( mButtonBox );

  mBtnMore = new KPushButton( i18n("more widgets","More"), mButtonBox );
  mBtnMore->setIconSet(SmallIconSet(QString::fromLatin1("down")));
  mButtonBox->setStretchFactor( mBtnMore, 0 );

  mBtnFewer = new KPushButton( i18n("fewer widgets","Fewer"), mButtonBox );
  mBtnFewer->setIconSet(SmallIconSet(QString::fromLatin1("up")));
  mButtonBox->setStretchFactor( mBtnFewer, 0 );

  QWidget *spacer = new QWidget( mButtonBox );
  mButtonBox->setStretchFactor( spacer, 1 );

  mBtnClear = new KPushButton( i18n("clear widgets","Clear"), mButtonBox );
  mBtnClear->setIconSet(SmallIconSet(QString::fromLatin1("locationbar_erase")));
  mButtonBox->setStretchFactor( mBtnClear, 0 );

  //---------- connect everything
  connect( mBtnMore, SIGNAL(clicked()),
     this, SLOT(slotMore()) );
  connect( mBtnFewer, SIGNAL(clicked()),
     this, SLOT(slotFewer()) );
  connect( mBtnClear, SIGNAL(clicked()),
     this, SLOT(slotClear()) );

  enableControls();
}

KWidgetLister::~KWidgetLister()
{
}

void KWidgetLister::slotMore()
{
  // the class should make certain that slotMore can't
  // be called when mMaxWidgets are on screen.
  assert( (int)mWidgetList.count() < mMaxWidgets );

  addWidgetAtEnd();
  //  adjustSize();
  enableControls();
}

void KWidgetLister::slotFewer()
{
  // the class should make certain that slotFewer can't
  // be called when mMinWidgets are on screen.
  assert( (int)mWidgetList.count() > mMinWidgets );

  removeLastWidget();
  //  adjustSize();
  enableControls();
}

void KWidgetLister::slotClear()
{
  setNumberOfShownWidgetsTo( mMinWidgets );

  // clear remaining widgets
  QPtrListIterator<QWidget> it( mWidgetList );
  for ( it.toFirst() ; it.current() ; ++it )
    clearWidget( (*it) );

  //  adjustSize();
  enableControls();
  emit clearWidgets();
}

void KWidgetLister::addWidgetAtEnd(QWidget *w)
{
  if (!w) w = this->createWidget(this);

  mLayout->insertWidget( mLayout->findWidget( mButtonBox ), w );
  mWidgetList.append( w );
  w->show();
  enableControls();
  emit widgetAdded();
  emit widgetAdded(w);
}

void KWidgetLister::removeLastWidget()
{
  // The layout will take care that the
  // widget is removed from screen, too.
  mWidgetList.removeLast();
  enableControls();
  emit widgetRemoved();
}

void KWidgetLister::clearWidget( QWidget* /*aWidget*/ )
{
}

QWidget* KWidgetLister::createWidget( QWidget* parent )
{
  return new QWidget( parent );
}

void KWidgetLister::setNumberOfShownWidgetsTo( int aNum )
{
  int superfluousWidgets = QMAX( (int)mWidgetList.count() - aNum, 0 );
  int missingWidgets     = QMAX( aNum - (int)mWidgetList.count(), 0 );

  // remove superfluous widgets
  for ( ; superfluousWidgets ; superfluousWidgets-- )
    removeLastWidget();

  // add missing widgets
  for ( ; missingWidgets ; missingWidgets-- )
    addWidgetAtEnd();
}

void KWidgetLister::enableControls()
{
  int count = mWidgetList.count();
  bool isMaxWidgets = ( count >= mMaxWidgets );
  bool isMinWidgets = ( count <= mMinWidgets );

  mBtnMore->setEnabled( !isMaxWidgets );
  mBtnFewer->setEnabled( !isMinWidgets );
}

#include "kwidgetlister.moc"
