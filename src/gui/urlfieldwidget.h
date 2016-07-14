/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_URLFIELDWIDGET_H
#define TELLICO_URLFIELDWIDGET_H

#include "fieldwidget.h"

#include <KUrlCompletion>

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

public Q_SLOTS:
  virtual void clearImpl();

protected:
  virtual QWidget* widget();
  virtual void updateFieldHook(Data::FieldPtr oldField, Data::FieldPtr newField);

protected Q_SLOTS:
  void slotOpenURL(const QString& url);

private:
  class URLCompletion : public KUrlCompletion {
  public:
    URLCompletion() : KUrlCompletion() {}
    virtual QString makeCompletion(const QString& text);
  };

  KUrlRequester* m_requester;
  bool m_isRelative;
};

  } // end GUI namespace
} // end namespace
#endif
