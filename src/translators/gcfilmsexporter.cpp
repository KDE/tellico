/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
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

#include "gcfilmsexporter.h"
#include "../collection.h"
#include "../document.h"
#include "../core/filehandler.h"
#include "../tellico_utils.h"
#include "../utils/stringset.h"
#include "../gui/guiproxy.h"
#include "../images/image.h"
#include "../images/imagefactory.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <kio/netaccess.h>

#include <QTextStream>

namespace {
  const char GCFILMS_DELIMITER = '|';
}

using Tellico::Export::GCfilmsExporter;

GCfilmsExporter::GCfilmsExporter() : Tellico::Export::Exporter() {
}

QString GCfilmsExporter::formatString() const {
  return i18n("GCfilms");
}

QString GCfilmsExporter::fileFilter() const {
  return i18n("*.gcf|GCfilms Data Files (*.gcf)") + QLatin1Char('\n') + i18n("*|All Files");
#if 0
  i18n("*.gcs|GCstar Data Files (*.gcs)")
#endif
}

bool GCfilmsExporter::exec() {
  Data::CollPtr coll = collection();
  if(!coll) {
    return false;
  }

  QString text;
  QTextStream ts(&text);

  ts << "GCfilms|" << coll->entryCount() << "|";
  if(options() & Export::ExportUTF8) {
    ts << "UTF8" << endl;
  }

  char d = GCFILMS_DELIMITER;
  bool format = options() & Export::ExportFormatted;
  // when importing GCfilms, a url field is added
  bool hasURL = coll->hasField(QLatin1String("url"))
                && coll->fieldByName(QLatin1String("url"))->type() == Data::Field::URL;

  uint minRating = 1;
  uint maxRating = 5;
  Data::FieldPtr f = coll->fieldByName(QLatin1String("rating"));
  if(f) {
    bool ok;
    uint n = Tellico::toUInt(f->property(QLatin1String("minimum")), &ok);
    if(ok) {
      minRating = n;
    }
    n = Tellico::toUInt(f->property(QLatin1String("maximum")), &ok);
    if(ok) {
      maxRating = n;
    }
  }

  // only going to export images if it's a local path
  KUrl imageDir;
  if(url().isLocalFile()) {
    imageDir = url();
    imageDir.cd(QLatin1String(".."));
    imageDir.addPath(url().fileName().section(QLatin1Char('.'), 0, 0) + QLatin1String("_images/"));
    if(!KIO::NetAccess::exists(imageDir, KIO::NetAccess::DestinationSide, 0)) {
      bool success = KIO::NetAccess::mkdir(imageDir, GUI::Proxy::widget());
      if(!success) {
        imageDir = KUrl(); // means don't write images
      }
    }
  }

  StringSet images;
  foreach(Data::EntryPtr entry, entries()) {
    ts << entry->id() << d;
    push(ts, "title", entry, format);
    push(ts, "year", entry, format);
    push(ts, "running-time", entry, format);
    push(ts, "director", entry, format);
    push(ts, "nationality", entry, format);
    push(ts, "genre", entry, format);
    // do image
    QString tmp = entry->field(QLatin1String("cover"));
    if(!tmp.isEmpty() && !imageDir.isEmpty()) {
      images.add(tmp);
      ts << imageDir.path() << tmp;
    }
    ts << d;

    // do not format cast since the commas could get mixed up
    const QStringList cast = entry->fields(QLatin1String("cast"), false);
    for(QStringList::ConstIterator it = cast.begin(); it != cast.end(); ++it) {
      ts << (*it).section(QLatin1String("::"), 0, 0);
      if(it != --cast.end()) {
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
    int rat = Tellico::toUInt(entry->field(QLatin1String("rating"), format), &ok);
    if(ok) {
      ts << rat * 10/(maxRating-minRating);
    }
    ts << d;

    push(ts, "comments", entry, format);
    push(ts, "language", entry, format); // ignoring audio-tracks

    push(ts, "subtitle", entry, format);

    // values[20] is borrower name, values[21] is loan date
    if(entry->field(QLatin1String("loaned")).isEmpty()) {
      ts << d << d;
    } else {
      // find loan
      bool found = false;
      Data::BorrowerList borrowers = Data::Document::self()->collection()->borrowers();
      foreach(Data::BorrowerPtr b, borrowers) {
        foreach(Data::LoanPtr loan, b->loans()) {
          if(entry == loan->entry()) {
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
    tmp = entry->field(QLatin1String("certification"),  format);
    int age = 0;
    if(tmp == QLatin1String("U (USA)")) {
      age = 1;
    } else if(tmp == QLatin1String("G (USA)")) {
      age = 2;
    } else if(tmp == QLatin1String("PG (USA)")) {
      age = 5;
    } else if(tmp == QLatin1String("PG-13 (USA)")) {
      age = 13;
    } else if(tmp == QLatin1String("R (USA)")) {
      age = 17;
    }
    if(age > 0) {
      ts << age << d;
    }
    ts << d;

    // all done
    ts << endl;
  }

  if(!imageDir.isEmpty()) {
    foreach(const QString& image, images) {
      const Data::Image& img = ImageFactory::imageById(image);
      KUrl target = imageDir;
      target.addPath(image);
      if(img.isNull() || !FileHandler::writeDataURL(target, img.byteArray(), false)) {
        myWarning() << "unable to write image file:"
                    << imageDir << image;
      }
    }
  }

  return FileHandler::writeTextURL(url(), text, options() & Export::ExportUTF8, options() & Export::ExportForce);
}

void GCfilmsExporter::push(QTextStream& ts_, const QByteArray& fieldName_, Tellico::Data::EntryPtr entry_, bool format_) {
  Data::FieldPtr f = collection()->fieldByName(QLatin1String(fieldName_));
  // don't format multiple names cause commas will cause problems
  if(f->formatFlag() == Data::Field::FormatName && (f->hasFlag(Data::Field::AllowMultiple))) {
    format_ = false;
  }
  QString s = entry_->field(QLatin1String(fieldName_), format_);
  if(f->hasFlag(Data::Field::AllowMultiple)) {
    ts_ << s.replace(QLatin1String("; "), QLatin1String(","));
  } else {
    ts_ << s;
  }
  ts_ << GCFILMS_DELIMITER;
}

#include "gcfilmsexporter.moc"
