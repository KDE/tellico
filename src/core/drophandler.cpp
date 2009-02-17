/***************************************************************************
    copyright            : (C) 2007-2008 by Robby Stephenson
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
#include "../translators/bibteximporter.h"
#include "../translators/risimporter.h"

#include <kmimetype.h>
#include <kio/netaccess.h>
#include <kio/job.h>

#include <QEvent>
#include <QDragEnterEvent>
#include <QDropEvent>

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
  bool accept = KUrl::List::canDecode(event_->mimeData()) || event_->mimeData()->hasText();
  if(accept) {
    event_->acceptProposedAction();
  }
  return accept;
}

bool DropHandler::drop(QDropEvent* event_) {
  KUrl::List urls = KUrl::List::fromMimeData(event_->mimeData());

  if(urls.isEmpty() && event_->mimeData()->hasText()) {
    urls << KUrl(event_->mimeData()->text());
  }
  return !urls.isEmpty() && handleURL(urls);
}

bool DropHandler::handleURL(const KUrl::List& urls_) {
  bool hasUnknown = false;
  KUrl::List tc, pdf, bib, ris;
  foreach(const KUrl& url, urls_) {
    KMimeType::Ptr ptr;
    // findByURL doesn't work for http, so actually query
    // the url itself
    if(url.protocol() != QLatin1String("http")) {
      ptr = KMimeType::findByUrl(url);
    } else {
      KIO::MimetypeJob* job = KIO::mimetype(url, KIO::HideProgressInfo);
      KIO::NetAccess::synchronousRun(job, Kernel::self()->widget());
      ptr = KMimeType::mimeType(job->mimetype());
    }
    if(ptr->is(QLatin1String("application/x-tellico"))) {
      tc << url;
    } else if(ptr->is(QLatin1String("application/pdf"))) {
      pdf << url;
    } else if(ptr->is(QLatin1String("text/x-bibtex")) ||
              ptr->is(QLatin1String("application/x-bibtex")) ||
              ptr->is(QLatin1String("application/bibtex"))) {
      bib << url;
    } else if(ptr->is(QLatin1String("application/x-research-info-systems"))) {
      ris << url;
    } else if(url.fileName().endsWith(QLatin1String(".bib"))) {
      bib << url;
    } else if(url.fileName().endsWith(QLatin1String(".ris"))) {
      ris << url;
    } else if(ptr->is(QLatin1String("text/plain")) && Import::BibtexImporter::maybeBibtex(url)) {
      bib << url;
    } else if(ptr->is(QLatin1String("text/plain")) && Import::RISImporter::maybeRIS(url)) {
      ris << url;
    } else {
      myDebug() << "DropHandler::handleURL() - unrecognized type: " << ptr->name() << " (" << url << ")" << endl;
      hasUnknown = true;
    }
  }
  MainWindow* mainWindow = ::qobject_cast<MainWindow*>(Kernel::self()->widget());
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
