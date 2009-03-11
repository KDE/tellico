/***************************************************************************
    copyright            : (C) 2005-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_URLFIELDWIDGET_H
#define TELLICO_URLFIELDWIDGET_H

#include "fieldwidget.h"

#include <krun.h>
#include <kurlcompletion.h>

#include <QPointer>

class KUrlRequester;

namespace Tellico {
  namespace GUI {

/**
 * @author Robby Stephenson
 */
class URLFieldWidget : public FieldWidget {
Q_OBJECT

public:
  URLFieldWidget(Data::FieldPtr field, QWidget* parent);
  virtual ~URLFieldWidget();

  virtual QString text() const;
  virtual void setTextImpl(const QString& text);

public slots:
  virtual void clearImpl();

protected:
  virtual QWidget* widget();
  virtual void updateFieldHook(Data::FieldPtr oldField, Data::FieldPtr newField);

protected slots:
  void slotOpenURL(const QString& url);

private:
  class URLCompletion : public KUrlCompletion {
  public:
    URLCompletion() : KUrlCompletion() {}
    virtual QString makeCompletion(const QString& text);
  };

  KUrlRequester* m_requester;
  bool m_isRelative : 1;
  QPointer<KRun> m_run;
};

  } // end GUI namespace
} // end namespace
#endif
