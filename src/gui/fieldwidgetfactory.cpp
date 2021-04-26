/***************************************************************************
    Copyright (C) 2021 Robby Stephenson <robby@periapsis.org>
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

#include "fieldwidgetfactory.h"
#include "fieldwidget.h"
#include "linefieldwidget.h"
#include "parafieldwidget.h"
#include "boolfieldwidget.h"
#include "choicefieldwidget.h"
#include "numberfieldwidget.h"
#include "urlfieldwidget.h"
#include "imagefieldwidget.h"
#include "datefieldwidget.h"
#include "tablefieldwidget.h"
#include "ratingfieldwidget.h"
#include "../tellico_debug.h"

Tellico::GUI::FieldWidget* Tellico::GUI::FieldWidgetFactory::create(Tellico::Data::FieldPtr field_, QWidget* parent_) {
  if(field_->hasFlag(Data::Field::NoEdit) ||
     field_->hasFlag(Data::Field::Derived)) {
    myWarning() << "read-only/dependent field, this shouldn't have been called";
    return nullptr;
  }
  switch(field_->type()) {
    case Data::Field::Line:
      return new LineFieldWidget(field_, parent_);

    case Data::Field::Para:
      return new ParaFieldWidget(field_, parent_);

    case Data::Field::Bool:
      return new BoolFieldWidget(field_, parent_);

    case Data::Field::Number:
      return new NumberFieldWidget(field_, parent_);

    case Data::Field::Choice:
      return new GUI::ChoiceFieldWidget(field_, parent_);

    case Data::Field::Table:
    case Data::Field::Table2:
      return new TableFieldWidget(field_, parent_);

    case Data::Field::Date:
      return new DateFieldWidget(field_, parent_);

    case Data::Field::URL:
      return new URLFieldWidget(field_, parent_);

    case Data::Field::Image:
      return new ImageFieldWidget(field_, parent_);

    case Data::Field::Rating:
      return new RatingFieldWidget(field_, parent_);

    default:
      myWarning() << "unknown field type = " << field_->type();
      return nullptr;
  }
}
