/***************************************************************************
    copyright            : (C) 2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

// this class is largely copied from kdelibs/knewstuff/provider.cpp
// which is Copyright (c) 2002 Cornelius Schumacher <schumacher@kde.org>
// and licensed under GPL v2, just like Tellico

#include "providerloader.h"
#include "../tellico_debug.h"
#include "../latin1literal.h"

#include <kio/job.h>
#include <knewstuff/provider.h>
#include <kglobal.h>
#include <kconfig.h>
#include <kmessagebox.h>
#include <klocale.h>

#include <qdom.h>

using Tellico::NewStuff::ProviderLoader;

ProviderLoader::ProviderLoader( QWidget *parentWidget ) :
  mParentWidget( parentWidget ), mTryAlt(true)
{
  mProviders.setAutoDelete( true );
}

void ProviderLoader::load( const QString &type, const QString &providersList )
{
  mProviders.clear();
  mJobData.truncate(0);

//  myLog() << "ProviderLoader::load(): providersList: " << providersList << endl;

  KIO::TransferJob *job = KIO::get( KURL( providersList ), false, false );
  connect( job, SIGNAL( result( KIO::Job * ) ),
           SLOT( slotJobResult( KIO::Job * ) ) );
  connect( job, SIGNAL( data( KIO::Job *, const QByteArray & ) ),
           SLOT( slotJobData( KIO::Job *, const QByteArray & ) ) );
  connect( job, SIGNAL( percent (KIO::Job *, unsigned long) ),
           SIGNAL( percent (KIO::Job *, unsigned long) ) );

//  job->dumpObjectInfo();
}

void ProviderLoader::slotJobData( KIO::Job *, const QByteArray &data )
{
  if ( data.size() == 0 ) return;
  QCString str( data, data.size() + 1 );
  mJobData.append( QString::fromUtf8( str ) );
}

void ProviderLoader::slotJobResult( KIO::Job *job )
{
  if ( job->error() ) {
    job->showErrorDialog( mParentWidget );
    if(mTryAlt && !mAltProvider.isEmpty()) {
      mTryAlt = false;
      load(QString(), mAltProvider);
    } else {
      emit error();
    }
    return;
  }

  QDomDocument doc;
  if ( !doc.setContent( mJobData ) ) {
    myDebug() << "ProviderLoader::slotJobResult() - error parsing providers list." << endl;
    if(mTryAlt && !mAltProvider.isEmpty()) {
      mTryAlt = false;
      load(QString(), mAltProvider);
    } else {
      emit error();
    }
    return;
  }

  QDomElement providers = doc.documentElement();
  QDomNode n;
  for ( n = providers.firstChild(); !n.isNull(); n = n.nextSibling() ) {
    QDomElement p = n.toElement();

    if ( p.tagName() == Latin1Literal("provider") ) {
      mProviders.append( new KNS::Provider( p ) );
    }
  }

  emit providersLoaded( &mProviders );
}

#include "providerloader.moc"
