/***************************************************************************
    Copyright (C) 2007-2009 Robby Stephenson <robby@periapsis.org>
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

#include "pdfimporter.h"
#include "tellicoimporter.h"
#include "xslthandler.h"
#include "xmphandler.h"
#include "../collections/bibtexcollection.h"
#include "../fieldformat.h"
#include "../core/filehandler.h"
#include "../core/netaccess.h"
#include "../images/imagefactory.h"
#include "../utils/guiproxy.h"
#include "../fetch/fetchmanager.h"
#include "../progressmanager.h"
#include "../utils/cursorsaver.h"
#include "../entryupdatejob.h"
#include "../utils/datafileregistry.h"
#include "../tellico_debug.h"

#include <KMessageBox>
#include <KLocalizedString>

#include <QString>
#include <QPixmap>
#include <QApplication>
#include <QFile>

#include <config.h>
#ifdef HAVE_POPPLER
#include <poppler-qt5.h>
#endif

#include <memory>

namespace {
  static const int PDF_FILE_PREVIEW_SIZE = 196;
}

using Tellico::Import::PDFImporter;

PDFImporter::PDFImporter(const QUrl& url_) : Importer(url_), m_cancelled(false) {
}

PDFImporter::PDFImporter(const QList<QUrl>& urls_) : Importer(urls_), m_cancelled(false) {
}

bool PDFImporter::canImport(int type_) const {
  return type_ == Data::Collection::Bibtex;
}

Tellico::Data::CollPtr PDFImporter::collection() {
  QString xsltFile = DataFileRegistry::self()->locate(QLatin1String("xmp2tellico.xsl"));
  if(xsltFile.isEmpty()) {
    myWarning() << "can not locate xmp2tellico.xsl";
    return Data::CollPtr();
  }

  ProgressItem& item = ProgressManager::self()->newProgressItem(this, progressLabel(), true);
  item.setTotalSteps(urls().count());
  connect(&item, SIGNAL(signalCancelled(ProgressItem*)), SLOT(slotCancel()));
  ProgressItem::Done done(this);
  const bool showProgress = options() & ImportProgress;

  QUrl u = QUrl::fromLocalFile(xsltFile);

  XSLTHandler xsltHandler(u);
  if(!xsltHandler.isValid()) {
    myWarning() << "invalid xslt in xmp2tellico.xsl";
    return Data::CollPtr();
  }

  bool hasDOI = false;
  bool hasArxiv = false;

  uint j = 0;

  Data::CollPtr coll;
  XMPHandler xmpHandler;
  QList<QUrl> list = urls();
  for(QList<QUrl>::Iterator it = list.begin(); it != list.end() && !m_cancelled; ++it, ++j) {
    const std::auto_ptr<FileHandler::FileRef> ref(FileHandler::fileRef(*it));
    if(!ref->isValid()) {
      continue;
    }

    Data::CollPtr newColl;
    Data::EntryPtr entry;

    QString xmp = xmpHandler.extractXMP(ref->fileName());
    //  myDebug() << xmp;
    if(xmp.isEmpty()) {
      setStatusMessage(i18n("Tellico was unable to read any metadata from the PDF file."));
    } else {
      setStatusMessage(QString());
#if 0
      myWarning() << "Remove debug from pdfimporter.cpp";
      QFile f(QString::fromLatin1("/tmp/test-xmp.xml"));
      if(f.open(QIODevice::WriteOnly)) {
        QTextStream t(&f);
        t.setCodec("UTF-8");
        t << xmp;
      }
      f.close();
#endif
      Import::TellicoImporter importer(xsltHandler.applyStylesheet(xmp));
      newColl = importer.collection();
      if(!newColl || newColl->entryCount() == 0) {
        myWarning() << "no collection found";
        setStatusMessage(i18n("Tellico was unable to read any metadata from the PDF file."));
      } else {
        entry = newColl->entries().front();
        hasDOI |= !entry->field(QLatin1String("doi")).isEmpty();
        // the XMP handler has a habit of inserting empty values surrounded by parentheses
        QRegExp rx(QLatin1String("\\(\\s*\\)"));
        foreach(Data::FieldPtr field, newColl->fields()) {
          QString value = entry->field(field);
          if(rx.exactMatch(value)) {
            entry->setField(field, QString());
          }
        }
      }
    }

    if(!newColl) {
      newColl = new Data::BibtexCollection(true);
    }
    if(!entry) {
      entry = new Data::Entry(newColl);
      newColl->addEntries(entry);
    }

#ifdef HAVE_POPPLER

    // now load from poppler
    Poppler::Document* doc = Poppler::Document::load(ref->fileName());
    if(doc && !doc->isLocked()) {
      // now the question is, do we overwrite XMP data with Poppler data?
      // for now, let's say yes conditionally
      QString s = doc->info(QLatin1String("Title")).simplified();
      if(!s.isEmpty()) {
        entry->setField(QLatin1String("title"), s);
      }
      // author could be separated by commas, "and" or whatever
      // we're not going to overwrite it
      if(entry->field(QLatin1String("author")).isEmpty()) {
        QRegExp rx(QLatin1String("\\s*(\\s+and\\s+|,|;)\\s*"));
        QStringList authors = doc->info(QLatin1String("Author")).simplified().split(rx);
        entry->setField(QLatin1String("author"), authors.join(FieldFormat::delimiterString()));
      }
      s = doc->info(QLatin1String("Keywords")).simplified();
      if(!s.isEmpty()) {
        // keywords are also separated by semi-colons in poppler
        entry->setField(QLatin1String("keyword"), s);
      }

      // now parse the first page text and try to guess
      Poppler::Page* page = doc->page(0);
      if(page) {
        // a null rectangle means get all text on page
        QString text = page->text(QRectF());
        // borrowed from Referencer
        QRegExp rx(QLatin1String("(?:"
                                       "(?:[Dd][Oo][Ii]:? *)"
                                       "|"
                                       "(?:[Dd]igital *[Oo]bject *[Ii]dentifier:? *)"
                                       ")"
                                       "("
                                       "[^\\.\\s]+"
                                       "\\."
                                       "[^\\/\\s]+"
                                       "\\/"
                                       "[^\\s]+"
                                       ")"));
        if(rx.indexIn(text) > -1) {
          QString doi = rx.cap(1);
          myLog() << "in PDF file, found DOI:" << doi;
          entry->setField(QLatin1String("doi"), doi);
          hasDOI = true;
        }
        rx = QRegExp(QLatin1String("arXiv:"
                                         "("
                                         "[^\\/\\s]+"
                                         "[\\/\\.]"
                                         "[^\\s]+"
                                         ")"));
        if(rx.indexIn(text) > -1) {
          QString arxiv = rx.cap(1);
          myLog() << "in PDF file, found arxiv:" << arxiv;
          if(!entry->collection()->hasField(QLatin1String("arxiv"))) {
            Data::FieldPtr field(new Data::Field(QLatin1String("arxiv"), i18n("arXiv ID")));
            field->setCategory(i18n("Publishing"));
            entry->collection()->addField(field);
          }
          entry->setField(QLatin1String("arxiv"), arxiv);
          hasArxiv = true;
        }

        delete page;
      }
    } else {
      myDebug() << "unable to read PDF info (poppler)";
    }
    delete doc;
#endif

    entry->setField(QLatin1String("url"), (*it).url());
    // always an article?
    entry->setField(QLatin1String("entry-type"), QLatin1String("article"));

    QPixmap pix = NetAccess::filePreview(QUrl::fromLocalFile(ref->fileName()), PDF_FILE_PREVIEW_SIZE);
    if(pix.isNull()) {
      myDebug() << "No file preview from pdf";
    } else {
      // is png best option?
      QString id = ImageFactory::addImage(pix, QLatin1String("PNG"));
      if(!id.isEmpty()) {
        Data::FieldPtr field = newColl->fieldByName(QLatin1String("cover"));
        if(!field && !newColl->imageFields().isEmpty()) {
          field = newColl->imageFields().front();
        } else if(!field) {
          field = new Data::Field(QLatin1String("cover"), i18n("Front Cover"), Data::Field::Image);
          newColl->addField(field);
        }
        entry->setField(field, id);
      }
    }
    if(coll) {
      coll->addEntries(newColl->entries());
    } else {
      coll = newColl;
    }

    if(showProgress) {
      ProgressManager::self()->setProgress(this, j);
      qApp->processEvents();
    }
  }

  if(m_cancelled) {
    return Data::CollPtr();
  }

  if(hasDOI) {
    Fetch::FetcherVec vec = Fetch::Manager::self()->createUpdateFetchers(coll->type(), Fetch::DOI);
    if(vec.isEmpty() && GUI::Proxy::widget()) {
      GUI::CursorSaver cs(Qt::ArrowCursor);
      KMessageBox::information(GUI::Proxy::widget(),
                              i18n("Tellico is able to download information about entries with a DOI from "
                                   "CrossRef.org. However, you must create an CrossRef account and add a new "
                                   "data source with your account information."),
                              QString(),
                              QLatin1String("CrossRefSourceNeeded"));
    } else {
      foreach(Fetch::Fetcher::Ptr fetcher, vec) {
        foreach(Data::EntryPtr entry, coll->entries()) {
          KJob* job = new EntryUpdateJob(this, entry, fetcher);
          job->exec();
        }
      }
    }
  }

  if(m_cancelled) {
    return Data::CollPtr();
  }

  if(hasArxiv) {
    Fetch::FetcherVec vec = Fetch::Manager::self()->createUpdateFetchers(coll->type(), Fetch::ArxivID);
    foreach(Fetch::Fetcher::Ptr fetcher, vec) {
      foreach(Data::EntryPtr entry, coll->entries()) {
        KJob* job = new EntryUpdateJob(this, entry, fetcher);
        job->exec();
      }
    }
  }

  // finally
  foreach(Data::EntryPtr entry, coll->entries()) {
    if(entry->title().isEmpty()) {
      // use file name
      QUrl u = QUrl::fromLocalFile(entry->field(QLatin1String("url")));
      entry->setField(QLatin1String("title"), u.fileName());
    }
  }

  if(m_cancelled) {
    return Data::CollPtr();
  }
  return coll;
}

void PDFImporter::slotCancel() {
  m_cancelled = true;
}
