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

#include "calendarhandler.h"
#include "../entry.h"
#include "../tellico_kernel.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <kstandarddirs.h>
#include <kconfiggroup.h>

#ifdef HAVE_KCAL
#include <KSystemTimeZones>
#include <kcal/todo.h>
#include <kcal/resourcelocal.h>
#endif

// most of this code came from konsolekalendar in kdepim

using Tellico::CalendarHandler;

void CalendarHandler::addLoans(Tellico::Data::LoanList loans_) {
#ifdef HAVE_KCAL
  addLoans(loans_, 0);
#else
  Q_UNUSED(loans_);
#endif
}

#ifdef HAVE_KCAL
void CalendarHandler::addLoans(Data::LoanList loans_, KCal::ResourceCalendar* resource_) {
  if(loans_.isEmpty()) {
    return;
  }

  StdCalendar* calendar = 0;
  KCal::ResourceCalendar* resource;
  if(resource_) {
    resource = resource_;
  } else {
    calendar = new StdCalendar();
    resource = calendar->resource();
  }
  if(!resource) {
    myWarning() << " no resource calendar";
    delete calendar;
    return;
  }

  foreach(Data::LoanPtr loan, loans_) {
    // only add loans with a due date
    if(loan->dueDate().isNull()) {
      continue;
    }

    KCal::Todo* todo = new KCal::Todo();
    populateTodo(todo, resource, loan);

    if(resource->addTodo(todo)) {
      loan->setInCalendar(true);
    } else {
      myWarning() << "failed to add todo to calendar";
    }
  }
  resource->save();
  resource->close();

  // ok to delete null
  delete calendar;
}
#endif

void CalendarHandler::modifyLoans(Tellico::Data::LoanList loans_) {
#ifndef HAVE_KCAL
  Q_UNUSED(loans_);
  return;
#else
  if(loans_.isEmpty()) {
    return;
  }

  StdCalendar calendar;
  KCal::ResourceCalendar* resource = calendar.resource();
  Q_ASSERT(resource);
  if(!resource) {
    return;
  }

  foreach(Data::LoanPtr loan, loans_) {
    KCal::Todo* todo = resource->todo(loan->uid());
    if(!todo) {
      myDebug() << "couldn't find existing todo, adding a new todo";
      Data::LoanList newLoans;
      newLoans.append(loan);
      addLoans(newLoans, resource); // add loan
      continue;
    }
    if(loan->dueDate().isNull()) {
      myDebug() << "removing todo";
      resource->deleteIncidence(todo);
      continue;
    }

    populateTodo(todo, resource, loan);
    todo->updated();

    loan->setInCalendar(true);
  }
  resource->save();
#endif
}

void CalendarHandler::removeLoans(Tellico::Data::LoanList loans_) {
#ifndef HAVE_KCAL
  Q_UNUSED(loans_);
  return;
#else
  if(loans_.isEmpty()) {
    return;
  }

  StdCalendar calendar;
  KCal::ResourceCalendar* resource = calendar.resource();
  Q_ASSERT(resource);
  if(!resource) {
    return;
  }

  foreach(Data::LoanPtr loan, loans_) {
    KCal::Todo* todo = resource->todo(loan->uid());
    if(todo) {
      // maybe this is too much, we could just set the todo as done
      resource->deleteIncidence(todo);
    }
  }
  resource->save();
#endif
}

#ifdef HAVE_KCAL

// taken from kpimprefs.cpp
KDateTime::Spec CalendarHandler::timeSpec() {
  KTimeZone zone;

  // Read TimeZoneId from korganizerrc.
  KConfig korgcfg(KStandardDirs::locate("config", QLatin1String("korganizerrc")));
  KConfigGroup group(&korgcfg, "Time & Date");
  QString tz( group.readEntry("TimeZoneId"));
  if(!tz.isEmpty()) {
    zone = KSystemTimeZones::zone(tz);
  }

  // If timeSpec not found in KOrg, use the system's default timeSpec.
  if(!zone.isValid()) {
    zone = KSystemTimeZones::local();
  }

  return zone.isValid() ? KDateTime::Spec(zone) : KDateTime::ClockTime;
}

void CalendarHandler::populateTodo(KCal::Todo* todo_, KCal::ResourceCalendar* res_, Data::LoanPtr loan_) {
  if(!todo_ || !loan_) {
    return;
  }

  todo_->setUid(loan_->uid());

  todo_->setDtStart(KDateTime(loan_->loanDate(), res_->timeSpec()));
  todo_->setHasStartDate(true);
  todo_->setDtDue(KDateTime(loan_->dueDate(), res_->timeSpec()));
  todo_->setHasDueDate(true);
  QString person = loan_->borrower()->name();
  QString summary = i18n("Tellico: %1 is due to return \"%2\"", person, loan_->entry()->title());
  todo_->setSummary(summary);
  QString note = loan_->note();
  if(note.isEmpty()) {
    note = summary;
  }
  todo_->setDescription(note);
  todo_->setSecrecy(KCal::Incidence::SecrecyPrivate); // private by default

  todo_->clearAttendees();
  // not adding email of borrower for now, but request RSVP?
  KCal::Attendee* attendee = new KCal::Attendee(loan_->borrower()->name(),
                                                QString(), false /*rsvp*/,
                                                KCal::Attendee::NeedsAction,
                                                KCal::Attendee::ReqParticipant,
                                                loan_->borrower()->uid());
  todo_->addAttendee(attendee);

  todo_->clearAlarms();
  KCal::Alarm* alarm = todo_->newAlarm();
  alarm->setDisplayAlarm(summary);
  alarm->setEnabled(true);
}

CalendarHandler::StdCalendar::StdCalendar() : KCal::CalendarResources(CalendarHandler::timeSpec()), m_resource(0) {
  readConfig();
  load();

  // need to find which calendar resource to add loan information to
  // we're going to require it to be a local file resource

  // first check standard resource
  KCal::ResourceCalendar* resource = resourceManager()->standardResource();
  if(resource && resource->type() == QLatin1String("file")) {
    myDebug() << "found standard resource";
    m_resource = resource;
  } else {
    // check active calendar resources and prefer local files
    KCal::CalendarResourceManager::ActiveIterator it;
    for(it = resourceManager()->activeBegin(); it != resourceManager()->activeEnd(); ++it) {
      if((*it)->type() == QLatin1String("file")) {
        myDebug() << "found local resource";
        m_resource = *it;
        break;
      }
    }
  }
  if(m_resource) {
    m_resource ->load();
    return;
  }

  // so now we need to create one;
  // use the same files and names as konsolekalendar
  KConfig _config(QLatin1String("korganizerrc"));
  KConfigGroup config(&_config, "General");
  QString fileName = config.readPathEntry("Active Calendar", QString());
  QString resourceName = i18n("Active Calendar");
  if(fileName.isEmpty()) {
    fileName = KStandardDirs::locateLocal("data", QLatin1String("korganizer/std.ics"));
    resourceName = i18n("Default Calendar");
  }

  resource = resourceManager()->createResource(QLatin1String("file"));
  if(resource) {
    myDebug() << "created new local calendar:" << fileName;
    m_resource = resource;
    m_resource->setValue(QLatin1String("File"), fileName);
    m_resource->setTimeSpec(CalendarHandler::timeSpec());
    m_resource->setResourceName(resourceName);
    resourceManager()->add(m_resource);
    resourceManager()->setStandardResource(m_resource);
    resourceAdded(m_resource);
    save();
  } else {
    myWarning() << "unable to create local calendar";
  }
}

CalendarHandler::StdCalendar::~StdCalendar() {
  if(m_resource) {
    m_resource->save();
    m_resource->close();
  }
}

#endif
