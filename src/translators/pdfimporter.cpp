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

#include "pdfimporter.h"
#include "tellicoimporter.h"
#include "xslthandler.h"
#include "../collections/bibtexcollection.h"
#include "../xmphandler.h"
#include "../filehandler.h"
#include "../imagefactory.h"
#include "../tellico_kernel.h"
#include "../fetch/fetchmanager.h"
#include "../fetch/crossreffetcher.h"
#include "../tellico_utils.h"
#include "../progressmanager.h"
#include "../core/netaccess.h"
#include "../tellico_debug.h"

#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <kapplication.h>

#include <QString>
#include <QPixmap>

#include <config.h>
#ifdef HAVE_POPPLER
#include <qt4/poppler-qt4.h>
#endif

namespace {
  static const int PDF_FILE_PREVIEW_SIZE = 196;
}

using Tellico::Import::PDFImporter;

PDFImporter::PDFImporter(const KUrl::List& urls_) : Importer(urls_), m_cancelled(false) {
}

bool PDFImporter::canImport(int type_) const {
  return type_ == Data::Collection::Bibtex;
}

Tellico::Data::CollPtr PDFImporter::collection() {
  QString xsltfile = KStandardDirs::locate("appdata", QString::fromLatin1("xmp2tellico.xsl"));
  if(xsltfile.isEmpty()) {
    kWarning() << "DropHandler::handleURL() - can not locate xmp2tellico.xsl";
    return Data::CollPtr();
  }

  ProgressItem& item = ProgressManager::self()->newProgressItem(this, progressLabel(), true);
  item.setTotalSteps(urls().count());
  connect(&item, SIGNAL(signalCancelled(ProgressItem*)), SLOT(slotCancel()));
  ProgressItem::Done done(this);
  const bool showProgress = options() & ImportProgress;

  KUrl u;
  u.setPath(xsltfile);

  XSLTHandler xsltHandler(u);
  if(!xsltHandler.isValid()) {
    kWarning() << "DropHandler::handleURL() - invalid xslt in xmp2tellico.xsl";
    return Data::CollPtr();
  }

  bool hasDOI = false;
  bool hasArxiv = false;

  uint j = 0;

  Data::CollPtr coll;
  XMPHandler xmpHandler;
  KUrl::List list = urls();
  for(KUrl::List::Iterator it = list.begin(); it != list.end() && !m_cancelled; ++it, ++j) {
    FileHandler::FileRef* ref = FileHandler::fileRef(*it);
    if(!ref) {
      continue;
    }

    Data::CollPtr newColl;
    Data::EntryPtr entry;

    QString xmp = xmpHandler.extractXMP(ref->fileName());
    //  myDebug() << xmp << endl;
    if(xmp.isEmpty()) {
      setStatusMessage(i18n("Tellico was unable to read any metadata from the PDF file."));
    } else {
      setStatusMessage(QString());

      Import::TellicoImporter importer(xsltHandler.applyStylesheet(xmp));
      newColl = importer.collection();
      if(!newColl || newColl->entryCount() == 0) {
        kWarning() << "DropHandler::handleURL() - no collection found";
        setStatusMessage(i18n("Tellico was unable to read any metadata from the PDF file."));
      } else {
        entry = newColl->entries().front();
        hasDOI |= !entry->field(QString::fromLatin1("doi")).isEmpty();
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
      QString s = doc->info(QString::fromLatin1("Title")).simplified();
      if(!s.isEmpty()) {
        entry->setField(QString::fromLatin1("title"), s);
      }
      // author could be separated by commas, "and" or whatever
      // we're not going to overwrite it
      if(entry->field(QString::fromLatin1("author")).isEmpty()) {
        QRegExp rx(QString::fromLatin1("\\s*(and|,|;)\\s*"));
        QStringList authors = doc->info(QString::fromLatin1("Author")).simplified().split(rx);
        entry->setField(QString::fromLatin1("author"), authors.join(QString::fromLatin1("; ")));
      }
      s = doc->info(QString::fromLatin1("Keywords")).simplified();
      if(!s.isEmpty()) {
        // keywords are also separated by semi-colons in poppler
        entry->setField(QString::fromLatin1("keyword"), s);
      }

      // now parse the first page text and try to guess
      Poppler::Page* page = doc->page(0);
      if(page) {
        // a null rectangle means get all text on page
        QString text = page->text(QRectF());
        // borrowed from Referencer
        QRegExp rx(QString::fromLatin1("(?:"
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
          myDebug() << "PDFImporter::collection() - in PDF file, found DOI: " << doi << endl;
          entry->setField(QString::fromLatin1("doi"), doi);
          hasDOI = true;
        }
        rx = QRegExp(QString::fromLatin1("arXiv:"
                                         "("
                                         "[^\\/\\s]+"
                                         "[\\/\\.]"
                                         "[^\\s]+"
                                         ")"));
        if(rx.indexIn(text) > -1) {
          QString arxiv = rx.cap(1);
          myDebug() << "PDFImporter::collection() - in PDF file, found arxiv: " << arxiv << endl;
          if(!entry->collection()->hasField(QString::fromLatin1("arxiv"))) {
            Data::FieldPtr field(new Data::Field(QString::fromLatin1("arxiv"), i18n("arXiv ID")));
            field->setCategory(i18n("Publishing"));
            entry->collection()->addField(field);
          }
          entry->setField(QString::fromLatin1("arxiv"), arxiv);
          hasArxiv = true;
        }

        delete page;
      }
    } else {
      myDebug() << "PDFImporter::collection() - unable to read PDF info (poppler)" << endl;
    }
    delete doc;
#endif

    entry->setField(QString::fromLatin1("url"), (*it).url());
    // always an article?
    entry->setField(QString::fromLatin1("entry-type"), QString::fromLatin1("article"));

    QPixmap pix = NetAccess::filePreview(ref->fileName(), PDF_FILE_PREVIEW_SIZE);
    delete ref; // removes temp file

    if(!pix.isNull()) {
      // is png best option?
      QString id = ImageFactory::addImage(pix, QString::fromLatin1("PNG"));
      if(!id.isEmpty()) {
        Data::FieldPtr field = newColl->fieldByName(QString::fromLatin1("cover"));
        if(!field && !newColl->imageFields().isEmpty()) {
          field = newColl->imageFields().front();
        } else if(!field) {
          field = new Data::Field(QString::fromLatin1("cover"), i18n("Front Cover"), Data::Field::Image);
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
      kapp->processEvents();
    }
  }

  if(m_cancelled) {
    return Data::CollPtr();
  }

  if(hasDOI) {
    myDebug() << "looking for DOI" << endl;
    Fetch::FetcherVec vec = Fetch::Manager::self()->createUpdateFetchers(coll->type(), Fetch::DOI);
    if(vec.isEmpty()) {
      GUI::CursorSaver cs(Qt::ArrowCursor);
      KMessageBox::information(Kernel::self()->widget(),
                              i18n("Tellico is able to download information about entries with a DOI from "
                                   "CrossRef.org. However, you must create an CrossRef account and add a new "
                                   "data source with your account information."),
                              QString::null,
                              QString::fromLatin1("CrossRefSourceNeeded"));
    } else {
      foreach(Fetch::Fetcher::Ptr fetcher, vec) {
        foreach(Data::EntryPtr entry, coll->entries()) {
          fetcher->updateEntrySynchronous(entry);
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
        fetcher->updateEntrySynchronous(entry);
      }
    }
  }

// finally
  foreach(Data::EntryPtr entry, coll->entries()) {
    if(entry->title().isEmpty()) {
      // use file name
      KUrl u = entry->field(QString::fromLatin1("url"));
      entry->setField(QString::fromLatin1("title"), u.fileName());
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

#include "pdfimporter.moc"
