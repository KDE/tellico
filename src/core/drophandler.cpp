/***************************************************************************
    Copyright (C) 2007-2012 Robby Stephenson <robby@periapsis.org>
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

#include "drophandler.h"
#include "../mainwindow.h"
#include "../gui/guiproxy.h"
#include "../translators/bibteximporter.h"
#include "../translators/risimporter.h"
#include "../translators/ciwimporter.h"
#include "../tellico_debug.h"

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
  KUrl::List tc, pdf, bib, ris, ciw;
  foreach(const KUrl& url, urls_) {
    KMimeType::Ptr ptr;
    // findByURL doesn't work for http, so actually query
    // the url itself
    if(url.protocol() != QLatin1String("http")) {
      ptr = KMimeType::findByUrl(url);
    } else {
      KIO::MimetypeJob* job = KIO::mimetype(url, KIO::HideProgressInfo);
      KIO::NetAccess::synchronousRun(job, GUI::Proxy::widget());
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
    } else if(url.fileName().endsWith(QLatin1String(".ciw"))) {
      ciw << url;
    } else if(ptr->is(QLatin1String("text/plain")) && Import::BibtexImporter::maybeBibtex(url)) {
      bib << url;
    } else if(ptr->is(QLatin1String("text/plain")) && Import::RISImporter::maybeRIS(url)) {
      ris << url;
    } else if(ptr->is(QLatin1String("text/plain")) && Import::CIWImporter::maybeCIW(url)) {
      ciw << url;
    } else {
      myDebug() << "unrecognized type: " << ptr->name() << " (" << url << ")";
      hasUnknown = true;
    }
  }
  MainWindow* mainWindow = ::qobject_cast<MainWindow*>(GUI::Proxy::widget());
  if(!mainWindow) {
    myDebug() << "no main window!";
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
  if(!ciw.isEmpty()) {
    mainWindow->importFile(Import::CIW, ciw);
  }
  // any unknown urls get passed
  return !hasUnknown;
}

#include "drophandler.moc"
