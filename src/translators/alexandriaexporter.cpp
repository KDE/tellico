/***************************************************************************
    copyright            : (C) 2003-2007 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "alexandriaexporter.h"
#include "../document.h"
#include "../collection.h"
#include "../tellico_kernel.h"
#include "../imagefactory.h"
#include "../image.h"
#include "../tellico_utils.h"
//#include "../tellico_debug.h"
#include "../progressmanager.h"

#include <klocale.h>
#include <kmessagebox.h>
#include <kapplication.h>

#include <qdir.h>

namespace {
  static const int ALEXANDRIA_MAX_SIZE_SMALL = 60;
  static const int ALEXANDRIA_MAX_SIZE_MEDIUM = 140;
}

using Tellico::Export::AlexandriaExporter;

QString& AlexandriaExporter::escapeText(QString& str_) {
  str_.replace('"', QString::fromLatin1("\\\""));
  return str_;
}

QString AlexandriaExporter::formatString() const {
  return i18n("Alexandria");
}

bool AlexandriaExporter::exec() {
  Data::CollPtr coll = collection();
  if(!coll || (coll->type() != Data::Collection::Book && coll->type() != Data::Collection::Bibtex)) {
    myLog() << "AlexandriaExporter::exec() - bad collection" << endl;
    return false;
  }

  const QString alexDirName = QString::fromLatin1(".alexandria");

  // create if necessary
  QDir libraryDir = QDir::home();
  if(!libraryDir.cd(alexDirName)) {
    if(!libraryDir.mkdir(alexDirName) || !libraryDir.cd(alexDirName)) {
      myLog() << "AlexandriaExporter::exec() - can't locate directory" << endl;
      return false;
    }
  }

  // the collection title is the name of the directory to create
  if(libraryDir.cd(coll->title())) {
    int ret = KMessageBox::warningContinueCancel(Kernel::self()->widget(),
                                                 i18n("<qt>An Alexandria library called <i>%1</i> already exists. "
                                                      "Any existing books in that library could be overwritten.</qt>")
                                                      .arg(coll->title()));
    if(ret == KMessageBox::Cancel) {
      return false;
    }
  } else if(!libraryDir.mkdir(coll->title()) || !libraryDir.cd(coll->title())) {
    return false; // could not create and cd to the dir
  }

  ProgressItem& item = ProgressManager::self()->newProgressItem(this, QString::null, false);
  item.setTotalSteps(entries().count());
  ProgressItem::Done done(this);
  const uint stepSize = QMIN(1, entries().count()/100);

  GUI::CursorSaver cs;
  bool success = true;
  uint j = 0;
  for(Data::EntryVec::ConstIterator entryIt = entries().begin(); entryIt != entries().end(); ++entryIt, ++j) {
    success &= writeFile(libraryDir, entryIt.data());
    if(j%stepSize == 0) {
      item.setProgress(j);
      kapp->processEvents();
    }
  }
  return success;
}

// this isn't true YAML export, of course
// everything is put between quotes except for the rating, just to be sure it's interpreted as a string
bool AlexandriaExporter::writeFile(const QDir& dir_, Data::ConstEntryPtr entry_) {
  // the filename is the isbn without dashes, followed by .yaml
  QString isbn = entry_->field(QString::fromLatin1("isbn"));
  if(isbn.isEmpty()) {
    return false; // can't write it since Alexandria uses isbn as name of file
  }
  isbn.remove('-'); // remove dashes

  QFile file(dir_.absPath() + QDir::separator() + isbn + QString::fromLatin1(".yaml"));
  if(!file.open(IO_WriteOnly)) {
    return false;
  }

  // do we format?
  bool format = options() & Export::ExportFormatted;

  QTextStream ts(&file);
  // alexandria uses utf-8 all the time
  ts.setEncoding(QTextStream::UnicodeUTF8);
  ts << "--- !ruby/object:Alexandria::Book\n";
  ts << "authors:\n";
  QStringList authors = entry_->fields(QString::fromLatin1("author"), format);
  for(QStringList::Iterator it = authors.begin(); it != authors.end(); ++it) {
    ts << "  - " << escapeText(*it) << "\n";
  }
  // Alexandria crashes when no authors, and uses n/a when none
  if(authors.count() == 0) {
    ts << "  - n/a\n";
  }

  QString tmp = entry_->field(QString::fromLatin1("title"), format);
  ts << "title: \"" << escapeText(tmp) << "\"\n";

  // Alexandria refers to the binding as the edition
  tmp = entry_->field(QString::fromLatin1("binding"), format);
  ts << "edition: \"" << escapeText(tmp) << "\"\n";

  // sometimes Alexandria interprets the isbn as a number instead of a string
  // I have no idea how to debug ruby, so err on safe side and add quotes
  ts << "isbn: \"" << isbn << "\"\n";

  tmp = entry_->field(QString::fromLatin1("comments"), format);
  ts << "notes: \"" << escapeText(tmp) << "\"\n";

  tmp = entry_->field(QString::fromLatin1("publisher"), format);
  // publisher uses n/a when empty
  ts << "publisher: \"" << (tmp.isEmpty() ? QString::fromLatin1("n/a") : escapeText(tmp)) << "\"\n";

  tmp = entry_->field(QString::fromLatin1("pub_year"), format);
  if(!tmp.isEmpty()) {
    ts << "publishing_year: \"" << escapeText(tmp) << "\"\n";
  }

  tmp = entry_->field(QString::fromLatin1("rating"));
  bool ok;
  int rating = Tellico::toUInt(tmp, &ok);
  if(ok) {
    ts << "rating: " << rating << "\n";
  }

  file.close();

  QString cover = entry_->field(QString::fromLatin1("cover"));
  if(cover.isEmpty() || !(options() & Export::ExportImages)) {
    return true; // all done
  }

  QImage img1(ImageFactory::imageById(cover));
  QImage img2;
  QString filename = dir_.absPath() + QDir::separator() + isbn;
  if(img1.height() > ALEXANDRIA_MAX_SIZE_SMALL) {
    if(img1.height() > ALEXANDRIA_MAX_SIZE_MEDIUM) { // limit maximum size
      img1 = img1.scale(ALEXANDRIA_MAX_SIZE_MEDIUM, ALEXANDRIA_MAX_SIZE_MEDIUM, QImage::ScaleMin);
    }
    img2 = img1.scale(ALEXANDRIA_MAX_SIZE_SMALL, ALEXANDRIA_MAX_SIZE_SMALL, QImage::ScaleMin);
  } else {
    img2 = img1.smoothScale(ALEXANDRIA_MAX_SIZE_MEDIUM, ALEXANDRIA_MAX_SIZE_MEDIUM, QImage::ScaleMin); // scale up
  }
  if(!img1.save(filename + QString::fromLatin1("_medium.jpg"), "JPEG")
     || !img2.save(filename + QString::fromLatin1("_small.jpg"), "JPEG")) {
    return false;
  }
  return true;
}

#include "alexandriaexporter.moc"
