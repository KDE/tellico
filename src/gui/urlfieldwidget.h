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

#ifndef URLFIELDWIDGET_H
#define URLFIELDWIDGET_H

class KURLRequester;

#include "fieldwidget.h"

#include <krun.h>
#include <kurlcompletion.h>

#include <qguardedptr.h>

namespace Tellico {
  namespace GUI {

/**
 * @author Robby Stephenson
 */
class URLFieldWidget : public FieldWidget {
Q_OBJECT

public:
  URLFieldWidget(Data::FieldPtr field, QWidget* parent, const char* name=0);
  virtual ~URLFieldWidget();

  virtual QString text() const;
  virtual void setText(const QString& text);

public slots:
  virtual void clear();

protected:
  virtual QWidget* widget();
  virtual void updateFieldHook(Data::FieldPtr oldField, Data::FieldPtr newField);

protected slots:
  void slotOpenURL(const QString& url);

private:
  class URLCompletion : public KURLCompletion {
  public:
    URLCompletion() : KURLCompletion() {}
    virtual QString makeCompletion(const QString& text);
  };

  KURLRequester* m_requester;
  bool m_isRelative : 1;
  QGuardedPtr<KRun> m_run;
};

  } // end GUI namespace
} // end namespace
#endif
