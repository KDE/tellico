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

#ifndef TELLICO_RATINGFIELDWIDGET_H
#define TELLICO_RATINGFIELDWIDGET_H

#include "fieldwidget.h"
#include "../datavectors.h"

class FieldWidgetTest;

namespace Tellico {
  namespace GUI {
    class RatingWidget;

/**
 * @author Robby Stephenson
 */
class RatingFieldWidget : public FieldWidget {
Q_OBJECT

friend class ::FieldWidgetTest;

public:
  RatingFieldWidget(Data::FieldPtr field, QWidget* parent);
  virtual ~RatingFieldWidget() {}

  virtual QString text() const Q_DECL_OVERRIDE;
  virtual void setTextImpl(const QString& text) Q_DECL_OVERRIDE;

public Q_SLOTS:
  virtual void clearImpl() Q_DECL_OVERRIDE;

protected:
  virtual QWidget* widget() Q_DECL_OVERRIDE;
  virtual void updateFieldHook(Data::FieldPtr oldField, Data::FieldPtr newField) Q_DECL_OVERRIDE;

private:
  RatingWidget* m_rating;
};

  } // end GUI namespace
} // end namespace
#endif
