/*  -*- c++ -*-

  kwidgetlister.cpp

  This file is part of libkdepim.
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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

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

#include <KDebug>
#include <KDialog>
#include <KLocale>
#include <KGuiItem>
#include <KHBox>
#include <KPushButton>

#include <QPushButton>
#include <QVBoxLayout>

#include <assert.h>

KWidgetLister::KWidgetLister( int minWidgets, int maxWidgets, QWidget *parent, const char *name )
  : QWidget( parent )
{
  setObjectName( name );

  mMinWidgets = qMax( minWidgets, 1 );
  mMaxWidgets = qMax( maxWidgets, mMinWidgets + 1 );

  //--------- the button box
  mLayout = new QVBoxLayout( this );
  mLayout->setMargin( 0 );
  mLayout->setSpacing( 4 );

  mButtonBox = new KHBox( this );
  mButtonBox->setSpacing( KDialog::spacingHint() );
  mLayout->addWidget( mButtonBox );

  mBtnMore = new KPushButton( KGuiItem( i18nc( "more widgets", "More" ),
                                        "button_more" ), mButtonBox );
  mButtonBox->setStretchFactor( mBtnMore, 0 );

  mBtnFewer = new KPushButton( KGuiItem( i18nc( "fewer widgets", "Fewer" ),
                                         "button_fewer" ), mButtonBox );
  mButtonBox->setStretchFactor( mBtnFewer, 0 );

  QWidget *spacer = new QWidget( mButtonBox );
  mButtonBox->setStretchFactor( spacer, 1 );

  mBtnClear = new KPushButton( KStandardGuiItem::clear(), mButtonBox );
  // FIXME a useful whats this. KStandardGuiItem::clear() returns a text with an edit box
  mBtnClear->setWhatsThis( "" );
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
  qDeleteAll( mWidgetList );
  mWidgetList.clear();
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
  foreach ( QWidget *widget, mWidgetList ) {
    clearWidget( widget );
  }

  //  adjustSize();
  enableControls();
  emit clearWidgets();
}

void KWidgetLister::addWidgetAtEnd( QWidget *w )
{
  if ( !w ) {
    w = this->createWidget( this );
  }

  mLayout->insertWidget( mLayout->indexOf( mButtonBox ), w );
  mWidgetList.append( w );
  w->show();
  enableControls();
  emit widgetAdded();
  emit widgetAdded( w );
}

void KWidgetLister::removeLastWidget()
{
  // The layout will take care that the
  // widget is removed from screen, too.
  delete mWidgetList.takeLast();
  enableControls();
  emit widgetRemoved();
}

void KWidgetLister::clearWidget( QWidget *w )
{
  Q_UNUSED( w );
}

QWidget *KWidgetLister::createWidget( QWidget *parent )
{
  return new QWidget( parent );
}

void KWidgetLister::setNumberOfShownWidgetsTo( int aNum )
{
  int superfluousWidgets = qMax( (int)mWidgetList.count() - aNum, 0 );
  int missingWidgets     = qMax( aNum - (int)mWidgetList.count(), 0 );

  // remove superfluous widgets
  for ( ; superfluousWidgets ; superfluousWidgets-- ) {
    removeLastWidget();
  }

  // add missing widgets
  for ( ; missingWidgets ; missingWidgets-- ) {
    addWidgetAtEnd();
  }
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
