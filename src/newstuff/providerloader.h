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

// this class is largely copied from kdelibs/knewstuff/provider.h
// which is Copyright (c) 2002 Cornelius Schumacher <schumacher@kde.org>
// and licensed under GPL v2, just like Tellico
//
// I want progress info for the download, and this was the
// easiest way to get it

#ifndef TELLICO_NEWSTUFF_PROVIDERLOADER_H
#define TELLICO_NEWSTUFF_PROVIDERLOADER_H

#include <qobject.h>
#include <qptrlist.h>

namespace KIO {
  class Job;
}
namespace KNS {
  class Provider;
}

namespace Tellico {
  namespace NewStuff {

class ProviderLoader : public QObject {
Q_OBJECT
public:
    /**
     * Constructor.
     *
     * @param parentWidget the parent widget
     */
    ProviderLoader( QWidget *parentWidget );

    /**
     * Starts asynchronously loading the list of providers of the
     * specified type.
     *
     * @param type data type such as 'kdesktop/wallpaper'.
     * @param providerList the URl to the list of providers; if empty
     *    we first try the ProvidersUrl from KGlobal::config, then we
     *    fall back to a hardcoded value.
     */
    void load( const QString &type, const QString &providerList = QString::null );

    void setAlternativeProvider(const QString& alt) { mAltProvider = alt; }

  signals:
    /**
     * Indicates that the list of providers has been successfully loaded.
     */
    void providersLoaded( QPtrList<KNS::Provider>* );
    void percent(KIO::Job *job, unsigned long percent);
    void error();

  protected slots:
    void slotJobData( KIO::Job *, const QByteArray & );
    void slotJobResult( KIO::Job * );

  private:
    QWidget *mParentWidget;

    QString mJobData;

    QPtrList<KNS::Provider> mProviders;
    QString mAltProvider;
    bool mTryAlt;
};

  }
}
#endif
