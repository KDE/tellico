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

#ifdef HAVE_KCAL
#include <libkcal/calendarresources.h>
#include <libkcal/todo.h>
#include <libkcal/resourcelocal.h>
// #include <libkcal/resourceremote.h> // this file is moving around, API differences
#endif

// needed for ::readlink
#include <unistd.h>
#include <limits.h>

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
void CalendarHandler::addLoans(Data::LoanList loans_, KCal::CalendarResources* resources_) {
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

  for(Data::LoanList::Iterator loan = loans_.begin(); loan != loans_.end(); ++loan) {
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

void CalendarHandler::modifyLoans(Tellico::Data::LoanList loans_) {
#ifndef HAVE_KCAL
  Q_UNUSED(loans_);
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

  for(Data::LoanList::Iterator loan = loans_.begin(); loan != loans_.end(); ++loan) {
    KCal::Todo* todo = calendarResources.todo(loan->uid());
    if(!todo) {
//      myDebug() << "couldn't find existing todo, adding a new todo" << endl;
      Data::LoanList newLoans;
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

void CalendarHandler::removeLoans(Tellico::Data::LoanList loans_) {
#ifndef HAVE_KCAL
  Q_UNUSED(loans_);
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

  for(Data::LoanList::Iterator loan = loans_.begin(); loan != loans_.end(); ++loan) {
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

#ifdef HAVE_KCAL
bool CalendarHandler::checkCalendar(KCal::CalendarResources* resources) {
  KCal::CalendarResourceManager* manager = resources->resourceManager();
  if(manager->isEmpty()) {
    kWarning() << "Tellico::CalendarHandler::checkCalendar() - adding default calendar";
    KConfig config(QLatin1String("korganizerrc"));
    config.setGroup("General");
    QString fileName = config.readPathEntry("Active Calendar", QString());

    QString resourceName;
    KCal::ResourceCalendar* defaultResource = 0;
    if(fileName.isEmpty()) {
      fileName = KStandardDirs::locateLocal("appdata", QLatin1String("std.ics"));
      resourceName = i18n("Default Calendar");
      defaultResource = new KCal::ResourceLocal(fileName);
    } else {
      KUrl url = KUrl::fromPathOrUrl(fileName);
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

// taken from kpimprefs.cpp
QString CalendarHandler::timezone() {
  QString zone;

  KConfig korgcfg(locate(QLatin1String("config"), QLatin1String("korganizerrc")));
  korgcfg.setGroup("Time & Date");
  QString tz(korgcfg.readEntry("TimeZoneId"));
  if(!tz.isEmpty()) {
    zone = tz;
  } else {
    char zonefilebuf[PATH_MAX];

    int len = ::readlink("/etc/localtime", zonefilebuf, PATH_MAX);
    if(len > 0 && len < PATH_MAX) {
      zone = QString::fromLocal8Bit(zonefilebuf, len);
      zone = zone.mid(zone.find(QLatin1String("zoneinfo/")) + 9);
    } else {
      tzset();
      zone = tzname[0];
    }
  }
  return zone;
}

#endif
