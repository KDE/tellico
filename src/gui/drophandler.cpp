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
#include "../utils/guiproxy.h"
#include "../translators/bibteximporter.h"
#include "../translators/risimporter.h"
#include "../translators/ciwimporter.h"
#include "../tellico_debug.h"

#include <KIO/MimetypeJob>
#include <KJobWidgets>

#include <QEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QMimeDatabase>
#include <QMimeType>

using Tellico::DropHandler;

DropHandler::DropHandler(QObject* parent_) : QObject(parent_) {
}

DropHandler::~DropHandler() = default;

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
  const bool accept = event_->mimeData()->hasUrls() || event_->mimeData()->hasText();
  if(accept) {
    event_->acceptProposedAction();
  }
  return accept;
}

bool DropHandler::drop(QDropEvent* event_) {
  QList<QUrl> urls = event_->mimeData()->urls();

  if(urls.isEmpty() && event_->mimeData()->hasText()) {
    QUrl u(event_->mimeData()->text());
    if(!u.isRelative() && (u.isLocalFile() || !u.host().isEmpty())) {
      urls << u;
    } else {
      return handleText(event_->mimeData()->text());
    }
  }
  return !urls.isEmpty() && handleURL(urls);
}

bool DropHandler::handleURL(const QList<QUrl>& urls_) {
  bool hasUnknown = false;
  QMimeDatabase db;
  QList<QUrl> tc, pdf, bib, ris, ciw, ebook;
  for(const QUrl& url: urls_) {
    QMimeType ptr;
    // findByURL doesn't work for http, so actually query
    // the url itself
    if(url.scheme() != QLatin1String("http")) {
      ptr = db.mimeTypeForUrl(url);
    } else {
      KIO::MimetypeJob* job = KIO::mimetype(url, KIO::HideProgressInfo);
      KJobWidgets::setWindow(job, GUI::Proxy::widget());
      job->exec();
      ptr = db.mimeTypeForName(job->mimetype());
    }
    if(ptr.inherits(QStringLiteral("application/x-tellico"))) {
      tc << url;
    } else if(ptr.inherits(QStringLiteral("application/pdf"))) {
      pdf << url;
    } else if(ptr.inherits(QStringLiteral("text/x-bibtex")) ||
              ptr.inherits(QStringLiteral("application/x-bibtex")) ||
              ptr.inherits(QStringLiteral("application/bibtex"))) {
      bib << url;
    } else if(ptr.inherits(QStringLiteral("application/x-research-info-systems"))) {
      ris << url;
    } else if(ptr.inherits(QStringLiteral("application/epub+zip")) ||
              ptr.inherits(QStringLiteral("application/x-mobipocket-ebook")) ||
              ptr.inherits(QStringLiteral("application/x-fictionbook+xml")) ||
              ptr.inherits(QStringLiteral("application/x-zip-compressed-fb2"))) {
      ebook << url;
    } else if(url.fileName().endsWith(QLatin1String(".bib"))) {
      bib << url;
    } else if(url.fileName().endsWith(QLatin1String(".ris"))) {
      ris << url;
    } else if(url.fileName().endsWith(QLatin1String(".ciw"))) {
      ciw << url;
    } else if(ptr.inherits(QStringLiteral("text/plain")) && Import::BibtexImporter::maybeBibtex(url)) {
      bib << url;
    } else if(ptr.inherits(QStringLiteral("text/plain")) && Import::RISImporter::maybeRIS(url)) {
      ris << url;
    } else if(ptr.inherits(QStringLiteral("text/plain")) && Import::CIWImporter::maybeCIW(url)) {
      ciw << url;
    } else {
      myDebug() << "unrecognized type: " << ptr.name() << " (" << url << ")";
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
  if(!ebook.isEmpty()) {
    mainWindow->importFile(Import::EBook, ebook);
  }
  // any unknown urls get passed
  return !hasUnknown;
}

bool DropHandler::handleText(const QString& text_) {
  MainWindow* mainWindow = ::qobject_cast<MainWindow*>(GUI::Proxy::widget());
  if(!mainWindow) {
    myDebug() << "no main window!";
    return false;
  }

  //TODO: handle TellicoXML, RIS, and CIW too
  if(Import::BibtexImporter::maybeBibtex(text_)) {
    mainWindow->importText(Import::Bibtex, text_);
  }
  return true;
}
