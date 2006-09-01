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

#include "gcfilmsexporter.h"
#include "../collection.h"
#include "../document.h"
#include "../filehandler.h"
#include "../latin1literal.h"
#include "../tellico_utils.h"
#include "../stringset.h"
#include "../tellico_kernel.h"
#include "../imagefactory.h"

#include <klocale.h>
#include <kio/netaccess.h>

namespace {
  char GCFILMS_DELIMITER = '|';
}

using Tellico::Export::GCfilmsExporter;

GCfilmsExporter::GCfilmsExporter() : Tellico::Export::Exporter() {
}

QString GCfilmsExporter::formatString() const {
  return i18n("GCfilms");
}

QString GCfilmsExporter::fileFilter() const {
  return i18n("*.gcf|GCfilms Data Files (*.gcf)") + QChar('\n') + i18n("*|All Files");
}

bool GCfilmsExporter::exec() {
  Data::CollPtr coll = collection();
  if(!coll) {
    return false;
  }

  QString text;
  QTextOStream ts(&text);

  ts << "GCfilms|" << coll->entryCount() << "|";
  if(options() & Export::ExportUTF8) {
    ts << "UTF8" << endl;
  }

  char d = GCFILMS_DELIMITER;
  bool format = options() & Export::ExportFormatted;
  // when importing GCfilms, a url field is added
  bool hasURL = coll->hasField(QString::fromLatin1("url"))
                && coll->fieldByName(QString::fromLatin1("url"))->type() == Data::Field::URL;

  uint minRating = 1;
  uint maxRating = 5;
  Data::FieldPtr f = coll->fieldByName(QString::fromLatin1("rating"));
  if(f) {
    bool ok;
    uint n = Tellico::toUInt(f->property(QString::fromLatin1("minimum")), &ok);
    if(ok) {
      minRating = n;
    }
    n = Tellico::toUInt(f->property(QString::fromLatin1("maximum")), &ok);
    if(ok) {
      maxRating = n;
    }
  }

  // only going to export images if it's a local path
  KURL imageDir;
  if(url().isLocalFile()) {
    imageDir = url();
    imageDir.cd(QString::fromLatin1(".."));
    imageDir.addPath(url().fileName().section('.', 0, 0) + QString::fromLatin1("_images/"));
    if(!KIO::NetAccess::exists(imageDir, false, 0)) {
      bool success = KIO::NetAccess::mkdir(imageDir, Kernel::self()->widget());
      if(!success) {
        imageDir = KURL(); // means don't write images
      }
    }
  }

  QStringList images;
  for(Data::EntryVec::ConstIterator entry = entries().begin(); entry != entries().end(); ++entry) {
    ts << entry->id() << d;
    push(ts, "title", entry, format);
    push(ts, "year", entry, format);
    push(ts, "running-time", entry, format);
    push(ts, "director", entry, format);
    push(ts, "nationality", entry, format);
    push(ts, "genre", entry, format);
    // do image
    QString tmp = entry->field(QString::fromLatin1("cover"));
    if(!tmp.isEmpty() && !imageDir.isEmpty()) {
      images << tmp;
      ts << imageDir.path() << tmp;
    }
    ts << d;

    // do not format cast since the commas could get mixed up
    const QStringList cast = entry->fields(QString::fromLatin1("cast"), false);
    for(QStringList::ConstIterator it = cast.begin(); it != cast.end(); ++it) {
      ts << (*it).section(QString::fromLatin1("::"), 0, 0);
      if(it != cast.fromLast()) {
        ts << ", ";
      }
    }
    ts << d;

    // values[9] is the original title
    ts << d;

    push(ts, "plot", entry, format);

    if(hasURL) {
      push(ts, "url", entry, format);
    } else {
      ts << d;
    }

    // values[12] is whether the film has been viewed or not
    ts << d;

    push(ts, "medium", entry, format);
    // values[14] is number of DVDS?
    ts << d;
    // values[15] is place?
    ts << d;

    // gcfilms's ratings go 0-10, just multiply by two
    bool ok;
    int rat = Tellico::toUInt(entry->field(QString::fromLatin1("rating"), format), &ok);
    if(ok) {
      ts << rat * 10/(maxRating-minRating);
    }
    ts << d;

    push(ts, "comments", entry, format);
    push(ts, "language", entry, format); // ignoring audio-tracks

    push(ts, "subtitle", entry, format);

    // values[20] is borrower name, values[21] is loan date
    if(entry->field(QString::fromLatin1("loaned")).isEmpty()) {
      ts << d << d;
    } else {
      // find loan
      bool found = false;
      const Data::BorrowerVec& borrowers = Data::Document::self()->collection()->borrowers();
      for(Data::BorrowerVec::ConstIterator b = borrowers.begin(); b != borrowers.end() && !found; ++b) {
        const Data::LoanVec& loans = b->loans();
        for(Data::LoanVec::ConstIterator loan = loans.begin(); loan != loans.end(); ++loan) {
          if(entry.data() == loan->entry()) {
            ts << b->name() << d;
            ts << loan->loanDate().day() << '/'
               << loan->loanDate().month() << '/'
               << loan->loanDate().year() << d;
            found = true;
            break;
          }
        }
      }
    }

    // values[22] is history ?
    ts << d;

    // for certification, only thing we can do is assume default american ratings
    tmp = entry->field(QString::fromLatin1("certification"),  format);
    int age = 0;
    if(tmp == Latin1Literal("U (USA)")) {
      age = 1;
    } else if(tmp == Latin1Literal("G (USA)")) {
      age = 2;
    } else if(tmp == Latin1Literal("PG (USA)")) {
      age = 5;
    } else if(tmp == Latin1Literal("PG-13 (USA)")) {
      age = 13;
    } else if(tmp == Latin1Literal("R (USA)")) {
      age = 17;
    }
    if(age > 0) {
      ts << age << d;
    }
    ts << d;

    // all done
    ts << endl;
  }

  StringSet imageSet;
  for(QStringList::ConstIterator it = images.begin(); it != images.end(); ++it) {
    if(imageSet.has(*it)) {
      continue;
    }
    if(ImageFactory::writeImage(*it, imageDir)) {
      imageSet.add(*it);
    } else {
      kdWarning() << "GCfilmsExporter::exec() - unable to write image file: "
                  << imageDir << *it << endl;
    }
  }

  return FileHandler::writeTextURL(url(), text, options() & Export::ExportUTF8, options() & Export::ExportForce);
}

void GCfilmsExporter::push(QTextOStream& ts_, QCString fieldName_, Data::EntryVec::ConstIterator entry_, bool format_) {
  Data::FieldPtr f = collection()->fieldByName(QString::fromLatin1(fieldName_));
  // don't format multiple names cause commas will cause problems
  if(f->formatFlag() == Data::Field::FormatName && (f->flags() & Data::Field::AllowMultiple)) {
    format_ = false;
  }
  QString s = entry_->field(QString::fromLatin1(fieldName_), format_);
  if(f->flags() & Data::Field::AllowMultiple) {
    ts_ << s.replace(QString::fromLatin1("; "), QChar(','));
  } else {
    ts_ << s;
  }
  ts_ << GCFILMS_DELIMITER;
}

#include "gcfilmsexporter.moc"
