/*****************************************************************************
 *   Copyright (C) 2003 - 2004 by Frerich Raabe <raabe@kde.org>               *
 *                                Tobias Koenig <tokoe@kde.org>               *
 *   Copyright (C) 2006 by Narayan Newton <narayannewton@gmail.com>           *
 *                                                                            *
 * This program is distributed in the hope that it will be useful, but        *
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE. For licensing and distribution        *
 * details, check the accompanying file 'COPYING.BSD'.                        *
 *****************************************************************************/
/**
  @file
  This file is part of the API for accessing XML-RPC Servers
  and defines the Client class.

  @brief
  Defines the Client class.

  @author Frerich Raabe <raabe@kde.org>
  @author Tobias Koenig <tokoe@kde.org>
  @author Narayan Newton <narayannewton@gmail.com>
*/

#include "client.h"
#include "query.h"

#include <kio/job.h>
#include <kdebug.h>
#include <klocale.h>
#include <kurl.h>

#include <QtCore/QVariant>

using namespace KXmlRpc;

class Client::Private
{
  public:
    Private() : mUserAgent( "KDE XMLRPC resources" ), mDigestAuth( false ){}

    void queryFinished( Query * );

    KUrl mUrl;
    QString mUserAgent;
    bool mDigestAuth;
    QList<Query*> mPendingQueries;
};

void Client::Private::queryFinished( Query *query )
{
  mPendingQueries.removeAll( query );
  query->deleteLater();
}

Client::Client( QObject *parent )
  : QObject( parent ), d( new Private )
{
}

Client::Client( const KUrl &url, QObject *parent )
  : QObject( parent ), d( new Private )
{
  d->mUrl = url;
}

Client::~Client()
{
  QList<Query *>::Iterator it;
  for ( it = d->mPendingQueries.begin(); it != d->mPendingQueries.end(); ++it ) {
    (*it)->deleteLater();
  }

  d->mPendingQueries.clear();

  delete d;
}

void Client::setUrl( const KUrl &url )
{
  d->mUrl = url.isValid() ? url : KUrl();
}

KUrl Client::url() const
{
  return d->mUrl;
}

QString Client::userAgent() const
{
  return d->mUserAgent;
}

void Client::setUserAgent( const QString &userAgent )
{
  d->mUserAgent = userAgent;
}

bool Client::isDigestAuthEnabled() const
{
  return d->mDigestAuth;
}

void Client::setDigestAuthEnabled( bool enabled )
{
  d->mDigestAuth = enabled;
}

void Client::call( const QString &method, const QList<QVariant> &args,
                   QObject *msgObj, const char *messageSlot,
                   QObject *faultObj, const char *faultSlot, const QVariant &id )
{

  QMap<QString, QString> metaData;

  if ( d->mUrl.isEmpty() ) {
    kWarning() << "Cannot execute call to" << method << ": empty server URL";
  }

  //Fill metadata, with userAgent and possible digest auth
  if ( d->mUserAgent.isEmpty() ) {
    metaData["UserAgent"] = "KDE-XMLRPC";
  } else {
    metaData["UserAgent"] = d->mUserAgent;
  }

  if ( d->mDigestAuth ) {
    metaData["WWW-Authenticate:"] = "Digest";
  }

  Query *query = Query::create( id, this );
  connect( query, SIGNAL(message(const QList<QVariant> &,const QVariant &)), msgObj, messageSlot );
  connect( query, SIGNAL(fault(int,const QString &,const QVariant &)), faultObj, faultSlot );
  connect( query, SIGNAL(finished(Query *)), this, SLOT(queryFinished(Query *)) );
  d->mPendingQueries.append( query );

  query->call( d->mUrl.url(), method, args, metaData );
}

void Client::call( const QString &method, const QVariant &arg,
                   QObject *msgObj, const char *messageSlot,
                   QObject *faultObj, const char *faultSlot,
                   const QVariant &id )
{
  QList<QVariant> args;
  args << arg ;
  call( method, args, msgObj, messageSlot, faultObj, faultSlot, id );
}

void Client::call( const QString &method, int arg,
                   QObject *msgObj, const char *messageSlot,
                   QObject *faultObj, const char *faultSlot,
                   const QVariant &id )
{
  QList<QVariant> args;
  args << QVariant( arg );
  call( method, args, msgObj, messageSlot, faultObj, faultSlot, id );
}

void Client::call( const QString &method, bool arg,
                   QObject *msgObj, const char *messageSlot,
                   QObject *faultObj, const char *faultSlot,
                   const QVariant &id )
{
  QList<QVariant> args;
  args << QVariant( arg );
  call( method, args, msgObj, messageSlot, faultObj, faultSlot, id );
}

void Client::call( const QString &method, double arg,
                   QObject *msgObj, const char *messageSlot,
                   QObject *faultObj, const char *faultSlot,
                   const QVariant &id )
{
  QList<QVariant> args;
  args << QVariant( arg );
  call( method, args, msgObj, messageSlot, faultObj, faultSlot, id );
}

void Client::call( const QString &method, const QString &arg,
                   QObject *msgObj, const char *messageSlot,
                   QObject *faultObj, const char *faultSlot,
                   const QVariant &id )
{
  QList<QVariant> args;
  args << QVariant( arg );
  call( method, args, msgObj, messageSlot, faultObj, faultSlot, id );
}

void Client::call( const QString &method, const QByteArray &arg,
                   QObject *msgObj, const char *messageSlot,
                   QObject *faultObj, const char *faultSlot,
                   const QVariant &id )
{
  QList<QVariant> args;
  args << QVariant( arg );
  call( method, args, faultObj, faultSlot, msgObj, messageSlot, id );
}

void Client::call( const QString &method, const QDateTime &arg,
                   QObject *msgObj, const char *messageSlot,
                   QObject *faultObj, const char *faultSlot,
                   const QVariant &id )
{
  QList<QVariant> args;
  args << QVariant( arg );
  call( method, args, msgObj, messageSlot, faultObj, faultSlot, id );
}

void Client::call( const QString &method, const QStringList &arg,
                   QObject *msgObj, const char *messageSlot,
                   QObject *faultObj, const char *faultSlot,
                   const QVariant &id )
{
  QList<QVariant> args;
  for ( int i = 0; i < arg.count(); ++i ) {
    args << QVariant( arg[ i ] );
  }

  call( method, args, msgObj, messageSlot, faultObj, faultSlot, id );
}

#include "client.moc"
