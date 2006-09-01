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

#include "calendarhandler.h"
#include "entry.h"
#include "tellico_kernel.h"
#include "tellico_debug.h"

#include <klocale.h>
#include <kstandarddirs.h>
#include <kconfig.h>

#ifdef USE_KCAL
#include <libkcal/calendarresources.h>
#include <libkcal/todo.h>
#include <libkcal/resourcelocal.h>
// #include <libkcal/resourceremote.h> // this file is moving around, API differences
#endif

// needed for ::readlink
#include <unistd.h>

// most of this code came from konsolekalendar in kdepim

using Tellico::CalendarHandler;

void CalendarHandler::addLoans(Data::LoanVec loans_) {
#ifdef USE_KCAL
  addLoans(loans_, 0);
#endif
}

#ifdef USE_KCAL
void CalendarHandler::addLoans(Data::LoanVec loans_, KCal::CalendarResources* resources_) {
  if(loans_.isEmpty()) {
    return;
  }

  KCal::CalendarResources* calendarResources;
  if(resources_) {
    calendarResources = resources_;
  } else {
    calendarResources = new KCal::CalendarResources(timezone());
    calendarResources->readConfig();
    calendarResources->load();
    if(!checkCalendar(calendarResources)) {
      return;
    }
  }

  for(Data::LoanVec::Iterator loan = loans_.begin(); loan != loans_.end(); ++loan) {
    // only add loans with a due date
    if(loan->dueDate().isNull()) {
      continue;
    }

    KCal::Todo* todo = new KCal::Todo();
    populateTodo(todo, loan);

    calendarResources->addTodo(todo);
    loan->setInCalendar(true);
  }
  calendarResources->save();
  // don't close if a pointer was passed
  if(!resources_) {
    calendarResources->close();
    calendarResources->deleteLater();
  }
}
#endif

void CalendarHandler::modifyLoans(Data::LoanVec loans_) {
#ifndef USE_KCAL
  return;
#else
  if(loans_.isEmpty()) {
    return;
  }

  KCal::CalendarResources calendarResources(timezone());
  calendarResources.readConfig();
  if(!checkCalendar(&calendarResources)) {
    return;
  }
  calendarResources.load();

  for(Data::LoanVec::Iterator loan = loans_.begin(); loan != loans_.end(); ++loan) {
    KCal::Todo* todo = calendarResources.todo(loan->uid());
    if(!todo) {
//      myDebug() << "couldn't find existing todo, adding a new todo" << endl;
      Data::LoanVec newLoans;
      newLoans.append(loan);
      addLoans(newLoans, &calendarResources); // add loan
      continue;
    }
    if(loan->dueDate().isNull()) {
      myDebug() << "removing todo" << endl;
      calendarResources.deleteIncidence(todo);
      continue;
    }

    populateTodo(todo, loan);
    todo->updated();

    loan->setInCalendar(true);
  }
  calendarResources.save();
  calendarResources.close();
#endif
}

void CalendarHandler::removeLoans(Data::LoanVec loans_) {
#ifndef USE_KCAL
  return;
#else
  if(loans_.isEmpty()) {
    return;
  }

  KCal::CalendarResources calendarResources(timezone());
  calendarResources.readConfig();
  if(!checkCalendar(&calendarResources)) {
    return;
  }
  calendarResources.load();

  for(Data::LoanVec::Iterator loan = loans_.begin(); loan != loans_.end(); ++loan) {
    KCal::Todo* todo = calendarResources.todo(loan->uid());
    if(todo) {
      // maybe this is too much, we could just set the todo as done
      calendarResources.deleteIncidence(todo);
    }
  }
  calendarResources.save();
  calendarResources.close();
#endif
}

#ifdef USE_KCAL
bool CalendarHandler::checkCalendar(KCal::CalendarResources* resources) {
  KCal::CalendarResourceManager* manager = resources->resourceManager();
  if(manager->isEmpty()) {
    kdWarning() << "Tellico::CalendarHandler::checkCalendar() - adding default calendar" << endl;
    KConfig config(QString::fromLatin1("korganizerrc"));
    config.setGroup("General");
    QString fileName = config.readPathEntry("Active Calendar");

    QString resourceName;
    KCal::ResourceCalendar* defaultResource = 0;
    if(fileName.isEmpty()) {
      fileName = locateLocal("appdata", QString::fromLatin1("std.ics"));
      resourceName = i18n("Default Calendar");
      defaultResource = new KCal::ResourceLocal(fileName);
    } else {
      KURL url = KURL::fromPathOrURL(fileName);
      if(url.isLocalFile()) {
        defaultResource = new KCal::ResourceLocal(url.path());
      } else {
//        defaultResource = new KCal::ResourceRemote(url);
        Kernel::self()->sorry(i18n("At the moment, Tellico only supports local calendar resources. "
                                   "The active calendar is remotely located, so your loans will not "
                                   "be added."));
        return false;
      }
      resourceName = i18n("Active Calendar");
    }

    defaultResource->setResourceName(resourceName);

    manager->add(defaultResource);
    manager->setStandardResource(defaultResource);
  }
  return true;
}

void CalendarHandler::populateTodo(KCal::Todo* todo_, Data::LoanPtr loan_) {
  if(!todo_ || !loan_) {
    return;
  }

  todo_->setUid(loan_->uid());

  todo_->setDtStart(loan_->loanDate());
  todo_->setHasStartDate(true);
  todo_->setDtDue(loan_->dueDate());
  todo_->setHasDueDate(true);
  QString person = loan_->borrower()->name();
  QString summary = i18n("Tellico: %1 is due to return \"%2\"").arg(person).arg(loan_->entry()->title());
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
                                                QString::null, false /*rsvp*/,
                                                KCal::Attendee::NeedsAction,
                                                KCal::Attendee::ReqParticipant,
                                                loan_->borrower()->uid());
  todo_->addAttendee(attendee);

  todo_->clearAlarms();
  KCal::Alarm* alarm = todo_->newAlarm();
  alarm->setDisplayAlarm(summary);
  alarm->setEnabled(true);
}

// taken from kpimprefs.cpp
QString CalendarHandler::timezone() {
  QString zone;

  KConfig korgcfg(locate(QString::fromLatin1("config"), QString::fromLatin1("korganizerrc")));
  korgcfg.setGroup("Time & Date");
  QString tz(korgcfg.readEntry("TimeZoneId"));
  if(!tz.isEmpty()) {
    zone = tz;
  } else {
    char zonefilebuf[PATH_MAX];

    int len = ::readlink("/etc/localtime", zonefilebuf, PATH_MAX);
    if(len > 0 && len < PATH_MAX) {
      zone = QString::fromLocal8Bit(zonefilebuf, len);
      zone = zone.mid(zone.find(QString::fromLatin1("zoneinfo/")) + 9);
    } else {
      tzset();
      zone = tzname[0];
    }
  }
  return zone;
}

#endif
