/* *************************************************************************
                          bookcasedoc.h  -  description
                             -------------------
    begin                : Sun Sep 9 2001
    copyright            : (C) 2001 by Robby Stephenson
    email                : robby@radiojodi.com
 * *************************************************************************/

/* *************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 * *************************************************************************/

#ifndef BOOKCASEDOC_H
#define BOOKCASEDOC_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

class BCCollection;

#include "bcunit.h"
#include <kurl.h>
#include <qlist.h>
#include <qfile.h>
#include <qcstring.h>
#include <qobject.h>

/**
  * @author Robby Stephenson
  * @version $Id: bookcasedoc.h,v 1.9 2001/11/05 05:56:43 robby Exp $
  */
class BookcaseDoc : public QObject  {
Q_OBJECT

public:
  BookcaseDoc(QWidget *parent, const char *name=0);
  ~BookcaseDoc();

  void setModified(bool m=true);
  void setURL(const KURL& url);
  bool isModified() const;
  const KURL& URL() const;

  bool newDocument();
  bool openDocument(const KURL& url);
  bool saveModified();
  bool saveDocument(const KURL& url);
  bool closeDocument();
  void deleteContents();
  unsigned collectionCount() const;
  BCCollection* collectionById(int id);
  QList<BCCollection>* collectionList();

public slots:
  void slotAddCollection(BCCollection* coll);
  void slotDeleteCollection(BCCollection* coll);
  void slotRenameCollection(int it, const QString& newName);
  void slotSaveUnit(BCUnit* unit);
  void slotAddUnit(BCUnit* unit);
  void slotDeleteUnit(BCUnit* unit);

protected:
  void writeDocument(QFile& f, const QCString& s);

private:
  QList<BCCollection>* m_collList;
  bool m_isModified;
  KURL m_url;

signals:
  void signalModified();
  void signalStatusMsg(const QString& str);
  void signalNewDoc();
  void signalCollectionAdded(BCCollection*);
  void signalCollectionModified(BCCollection*);
  void signalCollectionDeleted(BCCollection*);
  void signalUnitAdded(BCUnit* unit);
  void signalUnitModified(BCUnit* unit);
  void signalUnitDeleted(BCUnit* unit);
};

#endif
