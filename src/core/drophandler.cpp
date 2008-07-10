/***************************************************************************
    copyright            : (C) 2007 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "drophandler.h"
#include "../mainwindow.h"
#include "../tellico_kernel.h"
#include "../tellico_debug.h"

#include <kurldrag.h>
#include <kmimetype.h>
#include <kio/netaccess.h>
#include <kio/job.h>

using Tellico::DropHandler;

DropHandler::DropHandler(QObject* parent_) : QObject(parent_) {
}

DropHandler::~DropHandler() {
}

// assume the object is always the main window, that's the
// only object with this event filter
bool DropHandler::eventFilter(QObject* obj_, QEvent* ev_) {
  Q_UNUSED(obj_);
  if(ev_->type() == QEvent::DragEnter) {
    return dragEnter(static_cast<QDragEnterEvent*>(ev_));
  } else if(ev_->type() == QEvent::Drop) {
    return drop(static_cast<QDropEvent*>(ev_));
  }
  return false;
}

bool DropHandler::dragEnter(QDragEnterEvent* event_) {
  bool accept = KURLDrag::canDecode(event_) || QTextDrag::canDecode(event_);
  if(accept) {
    event_->accept(accept);
  }
  return accept;
}

bool DropHandler::drop(QDropEvent* event_) {
  KURL::List urls;
  QString text;

  if(KURLDrag::decode(event_, urls)) {
  } else if(QTextDrag::decode(event_, text) && !text.isEmpty()) {
    urls << KURL(text);
  }
  return !urls.isEmpty() && handleURL(urls);
}

bool DropHandler::handleURL(const KURL::List& urls_) {
  bool hasUnknown = false;
  KURL::List tc, pdf, bib, ris;
  for(KURL::List::ConstIterator it = urls_.begin(); it != urls_.end(); ++it) {
    KMimeType::Ptr ptr;
    // findByURL doesn't work for http, so actually query
    // the url itself
    if((*it).protocol() != QString::fromLatin1("http")) {
      ptr = KMimeType::findByURL(*it);
    } else {
      KIO::MimetypeJob* job = KIO::mimetype(*it, false /*progress*/);
      KIO::NetAccess::synchronousRun(job, Kernel::self()->widget());
      ptr = KMimeType::mimeType(job->mimetype());
    }
    if(ptr->is(QString::fromLatin1("application/x-tellico"))) {
      tc << *it;
    } else if(ptr->is(QString::fromLatin1("application/pdf"))) {
      pdf << *it;
    } else if(ptr->is(QString::fromLatin1("text/x-bibtex")) ||
              ptr->is(QString::fromLatin1("application/x-bibtex"))) {
      bib << *it;
    } else if(ptr->is(QString::fromLatin1("application/x-research-info-systems"))) {
      ris << *it;
    } else {
      myDebug() << "DropHandler::handleURL() - unrecognized type: " << ptr->name() << " (" << *it << ")" << endl;
      hasUnknown = true;
    }
  }
  MainWindow* mainWindow = ::qt_cast<MainWindow*>(Kernel::self()->widget());
  if(!mainWindow) {
    myDebug() << "DropHandler::handleURL() - no main window!" << endl;
    return !hasUnknown;
  }
  if(!tc.isEmpty()) {
    mainWindow->importFile(Import::TellicoXML, tc);
  }
  if(!pdf.isEmpty()) {
    mainWindow->importFile(Import::PDF, pdf);
  }
  if(!bib.isEmpty()) {
    mainWindow->importFile(Import::Bibtex, bib);
  }
  if(!ris.isEmpty()) {
    mainWindow->importFile(Import::RIS, ris);
  }
  // any unknown urls get passed
  return !hasUnknown;
}

#include "drophandler.moc"
