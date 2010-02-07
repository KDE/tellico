/***************************************************************************
    Copyright (C) 2006-2009 Robby Stephenson <robby@periapsis.org>

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

#ifndef TELLICO_CALENDARHANDLER_H
#define TELLICO_CALENDARHANDLER_H

#include <config.h>
#include "../borrower.h"

#include <KDateTime>

#ifdef HAVE_KCAL
#include <kcal/calendarresources.h>
#endif

namespace KCal {
  class Todo;
  class ResourceCalendar;
}

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class CalendarHandler {
public:
  static void addLoans(Data::LoanList loans);
  static void modifyLoans(Data::LoanList loans);
  static void removeLoans(Data::LoanList loans);

private:

#ifdef HAVE_KCAL
  // helper class and functions

  class StdCalendar : public KCal::CalendarResources {
  public:
    StdCalendar();
    ~StdCalendar();
    KCal::ResourceCalendar* resource() { return m_resource; }
  private:
    KCal::ResourceCalendar* m_resource;
  };

  static KDateTime::Spec timeSpec();
  static void addLoans(Data::LoanList loans, KCal::ResourceCalendar* resource);
  static bool checkCalendar(KCal::CalendarResources* resources);
  static void populateTodo(KCal::Todo* todo, KCal::ResourceCalendar* resources, Data::LoanPtr loan);
#endif
};

} // end namespace

#endif
