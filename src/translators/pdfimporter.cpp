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
#include "../collections/bookcollection.h"
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
#include <QScopedPointer>

#include <config.h>
#ifdef HAVE_POPPLER
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#include <poppler-qt5.h>
#else
#include <poppler-qt6.h>
#endif
#endif

namespace {
  static const int PDF_FILE_PREVIEW_SIZE = 196;
}

using Tellico::Import::PDFImporter;

PDFImporter::PDFImporter(const QUrl& url_) : Importer(url_), m_cancelled(false) {
}

PDFImporter::PDFImporter(const QList<QUrl>& urls_) : Importer(urls_), m_cancelled(false) {
}

bool PDFImporter::canImport(int type_) const {
  return type_ == Data::Collection::Book || type_ == Data::Collection::Bibtex;
}

Tellico::Data::CollPtr PDFImporter::collection() {
  QString xsltFile = DataFileRegistry::self()->locate(QStringLiteral("xmp2tellico.xsl"));
  if(xsltFile.isEmpty()) {
    myWarning() << "can not locate xmp2tellico.xsl";
    return Data::CollPtr();
  }

  ProgressItem& item = ProgressManager::self()->newProgressItem(this, progressLabel(), true);
  item.setTotalSteps(urls().count());
  connect(&item, &Tellico::ProgressItem::signalCancelled, this, &Tellico::Import::PDFImporter::slotCancel);
  ProgressItem::Done done(this);
  const bool showProgress = options() & ImportProgress;

  QUrl u = QUrl::fromLocalFile(xsltFile);

  XSLTHandler xsltHandler(u);
  if(!xsltHandler.isValid()) {
    myWarning() << "invalid xslt in xmp2tellico.xsl";
    return Data::CollPtr();
  }
  bool isBook = false;
  if(currentCollection() && currentCollection()->type() == Data::Collection::Book) {
    xsltHandler.addStringParam("ctype", "2"); // book if already existing
    isBook = true;
  } else {
    xsltHandler.addStringParam("ctype", "5"); // bibtex by default
  }

  bool hasDOI = false;
  bool hasArxiv = false;

  uint j = 0;

  Data::CollPtr coll;
  XMPHandler xmpHandler;
  QList<QUrl> list = urls();
  for(QList<QUrl>::Iterator it = list.begin(); it != list.end() && !m_cancelled; ++it, ++j) {
    const QScopedPointer<FileHandler::FileRef> ref(FileHandler::fileRef(*it));
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
        hasDOI |= !entry->field(QStringLiteral("doi")).isEmpty();
        // the XMP handler has a habit of inserting empty values surrounded by parentheses
        static const QRegularExpression rx(QLatin1String("^\\(\\s*\\)$"));
        foreach(Data::FieldPtr field, newColl->fields()) {
          QString value = entry->field(field);
          if(value.contains(rx)) {
            entry->setField(field, QString());
          }
        }
      }
    }

#ifdef HAVE_POPPLER
    if(!newColl) {
      if(isBook) {
        newColl = new Data::BookCollection(true);
      } else {
        newColl = new Data::BibtexCollection(true);
      }
    }
    if(!entry) {
      entry = new Data::Entry(newColl);
      newColl->addEntries(entry);
    }

    // now load from poppler
    auto doc = Poppler::Document::load(ref->fileName());
    if(doc && !doc->isLocked()) {
      // now the question is, do we overwrite XMP data with Poppler data?
      // for now, let's say yes conditionally
      QString s = doc->info(QStringLiteral("Title")).simplified();
      if(!s.isEmpty()) {
        entry->setField(QStringLiteral("title"), s);
      }
      // author could be separated by commas, "and" or whatever
      // we're not going to overwrite it
      if(entry->field(QStringLiteral("author")).isEmpty()) {
        static const QRegularExpression rx(QLatin1String("\\s*(\\s+and\\s+|,|;)\\s*"));
        QStringList authors = doc->info(QStringLiteral("Author")).simplified().split(rx);
        entry->setField(QStringLiteral("author"), authors.join(FieldFormat::delimiterString()));
      }
      s = doc->info(QStringLiteral("Keywords")).simplified();
      if(s.isEmpty()) {
        s = doc->info(QStringLiteral("Subject")).simplified();
      }
      if(!s.isEmpty()) {
        // keywords are also separated by semi-colons in poppler
        entry->setField(QStringLiteral("keyword"), s);
      }

      // now parse the first page text and try to guess
      auto page = doc->page(0);
      if(page) {
        // a null rectangle means get all text on page
        QString text = page->text(QRectF());
        // borrowed from Referencer
        static const QRegularExpression doiRx(QLatin1String("(?:"
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
        QRegularExpressionMatch m = doiRx.match(text);
        if(!m.hasMatch()) {
          static const QRegularExpression doiUrlRx(QLatin1String("https?://(?:dx\\.)?doi\\.org/(10.\\d{4,9}/[-._;()/:a-zA-Z0-9]+)"));
          m = doiUrlRx.match(text);
        }
        if(m.hasMatch()) {
          const QString doi = m.captured(1);
          myLog() << "In PDF file, found DOI:" << doi;
          entry->setField(QStringLiteral("doi"), doi);
          hasDOI = true;
        }
        static const QRegularExpression arxivRx(QLatin1String("arXiv:"
                                                              "("
                                                              "[^\\/\\s]+"
                                                              "[\\/\\.]"
                                                              "[^\\s]+"
                                                              ")"));
        m = arxivRx.match(text);
        if(m.hasMatch()) {
          const QString arxiv = m.captured(1);
          myLog() << "in PDF file, found arxiv:" << arxiv;
          if(!entry->collection()->hasField(QStringLiteral("arxiv"))) {
            Data::FieldPtr field(new Data::Field(QStringLiteral("arxiv"), i18n("arXiv ID")));
            field->setCategory(i18n("Publishing"));
            entry->collection()->addField(field);
          }
          entry->setField(QStringLiteral("arxiv"), arxiv);
          hasArxiv = true;
        }
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        delete page;
#endif
      }
    } else {
      myDebug() << "unable to read PDF info (poppler)";
    }
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    delete doc;
#endif
#elif defined HAVE_KFILEMETADATA
    if(!newColl || newColl->entryCount() == 0) {
      myDebug() << "Reading with metadata";
      EBookImporter imp(urls());
      auto ebookColl = imp.collection();
      if(ebookColl && ebookColl->type() == Data::Collection::Book && !isBook) {
        newColl = Data::BibtexCollection::convertBookCollection(ebookColl);
      } else {
        newColl = ebookColl;
      }
      if(newColl->entryCount() > 0) {
        entry = new Data::Entry(newColl);
        newColl->addEntries(entry);
      } else {
        entry = newColl->entries().front();
      }
    }
#else
    // only recourse is to create an empty collection
    if(!newColl) {
      if(isBook) {
        newColl = new Data::BookCollection(true);
      } else {
        newColl = new Data::BibtexCollection(true);
      }
    }
    if(!entry) {
      entry = new Data::Entry(newColl);
      newColl->addEntries(entry);
    }
#endif

    if(!isBook) {
      entry->setField(QStringLiteral("url"), (*it).url());
      // always an article?
      entry->setField(QStringLiteral("entry-type"), QStringLiteral("article"));
    }
    QPixmap pix = NetAccess::filePreview(QUrl::fromLocalFile(ref->fileName()), PDF_FILE_PREVIEW_SIZE);
    if(pix.isNull()) {
      myDebug() << "No file preview from pdf";
    } else {
      // is png best option?
      QString id = ImageFactory::addImage(pix, QStringLiteral("PNG"));
      if(!id.isEmpty()) {
        Data::FieldPtr field = newColl->fieldByName(QStringLiteral("cover"));
        if(!field && !newColl->imageFields().isEmpty()) {
          field = newColl->imageFields().front();
        } else if(!field) {
          field = Data::Field::createDefaultField(Data::Field::FrontCoverField);
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
      ProgressManager::self()->setProgress(this, j+1);
      qApp->processEvents();
    }
  }

  if(m_cancelled || !coll) {
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
                              QStringLiteral("CrossRefSourceNeeded"));
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
      QUrl u = QUrl::fromLocalFile(entry->field(QStringLiteral("url")));
      entry->setField(QStringLiteral("title"), u.fileName());
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
