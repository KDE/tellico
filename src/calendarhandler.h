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

#ifndef TELLICO_CALENDARHANDLER_H
#define TELLICO_CALENDARHANDLER_H

#include <config.h>
#include "borrower.h"

#include <kdeversion.h>

// libkcal is not binary compatible between versions
// for now, just support KDE 3.4 and higher
#if HAVE_KCAL && KDE_IS_VERSION(3,3,90)
#define USE_KCAL
#endif

namespace KCal {
  class CalendarResources;
  class Todo;
}

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class CalendarHandler {
public:
  static void addLoans(Data::LoanVec loans);
  static void modifyLoans(Data::LoanVec loans);
  static void removeLoans(Data::LoanVec loans);

private:
  static QString timezone();

#ifdef USE_KCAL
  // helper function
  static void addLoans(Data::LoanVec loans, KCal::CalendarResources* resources);
  static bool checkCalendar(KCal::CalendarResources* resources);
  static void populateTodo(KCal::Todo* todo, Data::LoanPtr loan);
#endif
};

} // end namespace

#endif
