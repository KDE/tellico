/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
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
#include "../collection.h"
#include "../kernel.h"
#include "../imagefactory.h"

#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>

#include <qdir.h>

static const int ALEXANDRIA_MAX_SIZE_SMALL = 60;
static const int ALEXANDRIA_MAX_SIZE_MEDIUM = 140;

using Bookcase::Export::AlexandriaExporter;

QString AlexandriaExporter::formatString() const {
  return i18n("Alexandria");
}

bool AlexandriaExporter::exportEntries(bool formatFields_) const {
  const Data::Collection* coll = collection();

  QDir libraryDir = QDir::home();
  if(!libraryDir.cd(QString::fromLatin1(".alexandria"))) {
    if(!libraryDir.mkdir(QString::fromLatin1(".alexandria"))) {
      return false;
    }
    libraryDir.cd(QString::fromLatin1(".alexandria"));
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
  } else {
    if(!libraryDir.mkdir(libraryDir.absPath() + QDir::separator() + coll->title())) {
      return false;
    }
    libraryDir.cd(coll->title());
  }

  for(Data::EntryListIterator it(entryList()); it.current(); ++it) {
    writeFile(libraryDir, it.current(), formatFields_);
  }

  return true;
}

// this isn't true YAML export, of course
// everything is put between quotes except for the rating, just to be sure it's interpreted as a string
bool AlexandriaExporter::writeFile(const QDir& dir_, Data::Entry* entry_, bool formatFields_) const {
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

  QTextStream ts(&file);
  // alexandria uses utf-8 all the time
  ts.setEncoding(QTextStream::UnicodeUTF8);
  ts << "--- !ruby/object:Alexandria::Book\n";
  ts << "authors:\n";
  QStringList authors = entry_->fields(QString::fromLatin1("author"), formatFields_);
  for(QStringList::Iterator it = authors.begin(); it != authors.end(); ++it) {
    ts << "  - " << (*it).replace('"', QString::fromLatin1("\\\"")) << "\n";
  }
  // Alexandria crashes when no authors, and uses n/a when none
  if(authors.count() == 0) {
    ts << "  - n/a\n";
  }
  // Alexandria calls the binding, the edition
  ts << "edition: \"" << (formatFields_ ?
                          entry_->formattedField(QString::fromLatin1("binding")) :
                          entry_->field(QString::fromLatin1("binding")))
                    << "\"\n";
  // sometimes Alexandria interprets the isbn as a number instead of a string
  // I have no idea how to debug ruby, so err on safe side and add quotes
  ts << "isbn: \"" << isbn << "\"\n";
  QString notes = formatFields_ ?
                    entry_->formattedField(QString::fromLatin1("comments")) :
                    entry_->field(QString::fromLatin1("comments"));
  notes.replace('"', QString::fromLatin1("\\\""));
  ts << "notes: \"" << notes << "\"\n";
  QString pub = formatFields_ ?
                  entry_->formattedField(QString::fromLatin1("publisher")) :
                  entry_->field(QString::fromLatin1("publisher"));
  pub.replace('"', QString::fromLatin1("\\\""));
  // publisher uses n/a when empty
  ts << "publisher: \"" << (pub.isEmpty() ? "n/a" : pub) << "\"\n";
  QString rating = entry_->field(QString::fromLatin1("rating"));
  for(uint pos = 0; pos < rating.length(); ++pos) {
    if(rating[pos].isDigit()) {
      ts << "rating: " << rating[pos] << "\n";
      break;
    }
  }
  QString title = formatFields_ ?
                    entry_->formattedField(QString::fromLatin1("title")) :
                    entry_->field(QString::fromLatin1("title"));
  title.replace('"', QString::fromLatin1("\\\""));
  ts << "title: \"" << title << "\"\n";
  file.close();

  QString cover = entry_->field(QString::fromLatin1("cover"));
  if(cover.isEmpty()) {
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
  if(!img1.save(filename + QString::fromLatin1("_medium.jpg"), "JPEG") ||
     !img2.save(filename + QString::fromLatin1("_small.jpg"), "JPEG")) {
    return false;
  }
  return true;
}
