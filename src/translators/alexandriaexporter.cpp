/***************************************************************************
    Copyright (C) 2003-2020 Robby Stephenson <robby@periapsis.org>
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

#include "alexandriaexporter.h"
#include "../collection.h"
#include "../fieldformat.h"
#include "../images/imagefactory.h"
#include "../images/image.h"
#include "../utils/string_utils.h"
#include "../progressmanager.h"
#include "../utils/guiproxy.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KMessageBox>

#include <QDir>
#include <QTextStream>
#include <QApplication>

namespace {
  static const int ALEXANDRIA_MAX_SIZE_SMALL = 60;
  static const int ALEXANDRIA_MAX_SIZE_MEDIUM = 140;
}

using namespace Tellico;
using Tellico::Export::AlexandriaExporter;

AlexandriaExporter::AlexandriaExporter(Data::CollPtr coll_) : Exporter(coll_) {
}

QString AlexandriaExporter::escapeText(const QString& str_) {
  // no control characters at all which is more strict than XML (which is what string_utils::removeControlCharacters has)
  QString result;
  result.reserve(str_.size());
  for(int i = 0; i < str_.size(); ++i) {
    const ushort c = str_.at(i).unicode();
    if((c > 0x1F && c < 0x7F) || (c > 0xA0)) {
      result += str_.at(i);
    }
  }
  result.replace(QLatin1String("\""), QLatin1String("\\\""));
  return result;
}

QString AlexandriaExporter::formatString() const {
  return QStringLiteral("Alexandria");
}

bool AlexandriaExporter::exec() {
  Data::CollPtr coll = collection();
  if(!coll || (coll->type() != Data::Collection::Book && coll->type() != Data::Collection::Bibtex)) {
    myLog() << "bad collection";
    return false;
  }

  const QString alexDirName = QStringLiteral(".alexandria");

  // create if necessary
  const QUrl u = url();
  QDir libraryDir;
  if(u.isEmpty()) {
    libraryDir = QDir::home();
  } else {
    if(u.isLocalFile()) {
      if(!libraryDir.cd(u.toLocalFile())) {
        myWarning() << "can't change to directory:" << u.path();
        return false;
      }
    } else {
      myWarning() << "can't write to remote directory";
      return false;
    }
  }

  if(!libraryDir.cd(alexDirName)) {
    if(!libraryDir.mkdir(alexDirName) || !libraryDir.cd(alexDirName)) {
      myLog() << "can't locate directory";
      return false;
    }
  }

  // the collection title is the name of the directory to create
  if(libraryDir.cd(coll->title())) {
    int ret = KMessageBox::warningContinueCancel(GUI::Proxy::widget(),
                                                 i18n("<qt>An Alexandria library called <i>%1</i> already exists. "
                                                      "Any existing books in that library could be overwritten.</qt>",
                                                      coll->title()));
    if(ret == KMessageBox::Cancel) {
      return false;
    }
  } else if(!libraryDir.mkdir(coll->title()) || !libraryDir.cd(coll->title())) {
    return false; // could not create and cd to the dir
  }

  ProgressItem& item = ProgressManager::self()->newProgressItem(this, QString(), false);
  item.setTotalSteps(entries().count());
  ProgressItem::Done done(this);
  const uint stepSize = qMax(1, entries().count()/100);
  const bool showProgress = options() & ExportProgress;

  bool success = true;
  uint j = 0;
  foreach(const Data::EntryPtr& entry, entries()) {
    success &= writeFile(libraryDir, entry);
    if(showProgress && j%stepSize == 0) {
      item.setProgress(j);
      qApp->processEvents();
    }
    ++j;
  }
  return success;
}

// this isn't true YAML export, of course
// everything is put between quotes except for the rating, just to be sure it's interpreted as a string
bool AlexandriaExporter::writeFile(const QDir& dir_, Tellico::Data::EntryPtr entry_) {
  // the filename is the isbn without dashes, followed by .yaml
  QString isbn = entry_->field(QStringLiteral("isbn"));
  if(isbn.isEmpty()) {
    return false; // can't write it since Alexandria uses isbn as name of file
  }
  isbn.remove(QLatin1Char('-')); // remove dashes

  QFile file(dir_.absolutePath() + QDir::separator() + isbn + QLatin1String(".yaml"));
  if(!file.open(QIODevice::WriteOnly)) {
    return false;
  }

  // do we format?
  FieldFormat::Request format = (options() & Export::ExportFormatted ?
                                                FieldFormat::ForceFormat :
                                                FieldFormat::AsIsFormat);

  QTextStream ts(&file);
  // alexandria uses utf-8 all the time
  ts.setEncoding(QStringConverter::Utf8);

  ts << "--- !ruby/object:Alexandria::Book\n";
  ts << "authors:\n";
  QStringList authors = FieldFormat::splitValue(entry_->formattedField(QStringLiteral("author"), format));
  for(QStringList::Iterator it = authors.begin(); it != authors.end(); ++it) {
    ts << "  - " << escapeText(*it) << "\n";
  }
  // Alexandria crashes when no authors, and uses n/a when none
  if(authors.count() == 0) {
    ts << "  - n/a\n";
  }

  QString tmp = entry_->title(format);
  ts << "title: \"" << escapeText(tmp) << "\"\n";

  // Alexandria refers to the binding as the edition
  tmp = entry_->formattedField(QStringLiteral("binding"), format);
  ts << "edition: \"" << escapeText(tmp) << "\"\n";

  // sometimes Alexandria interprets the isbn as a number instead of a string
  // I have no idea how to debug ruby, so err on safe side and add quotes
  ts << "isbn: \"" << isbn << "\"\n";

  static const QRegularExpression rx(QLatin1String("<br/?>"), QRegularExpression::CaseInsensitiveOption);
  tmp = entry_->formattedField(QStringLiteral("comments"), format);
  tmp.replace(rx, QStringLiteral("\n"));
  ts << "notes: |-\n";
  foreach(const QString& line, tmp.split(QLatin1Char('\n'))) {
    ts << "  " << escapeText(line) << "\n";
  }

  tmp = entry_->formattedField(QStringLiteral("publisher"), format);
  // publisher uses n/a when empty
  ts << "publisher: \"" << (tmp.isEmpty() ? QStringLiteral("n/a") : escapeText(tmp)) << "\"\n";

  tmp = entry_->formattedField(QStringLiteral("pub_year"), format);
  if(!tmp.isEmpty()) {
    ts << "publishing_year: \"" << escapeText(tmp) << "\"\n";
  }

  tmp = entry_->field(QStringLiteral("rating"));
  bool ok;
  int rating = Tellico::toUInt(tmp, &ok);
  if(ok) {
    ts << "rating: " << rating << "\n";
  }

  tmp = entry_->field(QStringLiteral("read"));
  if(!tmp.isEmpty()) {
    ts << "redd: true\n";
  }

  file.close();

  QString cover = entry_->field(QStringLiteral("cover"));
  if(cover.isEmpty() || !(options() & Export::ExportImages)) {
    return true; // all done
  }

  QImage img1(ImageFactory::imageById(cover));
  QImage img2;
  QString filename = dir_.absolutePath() + QDir::separator() + isbn;
  if(img1.height() > ALEXANDRIA_MAX_SIZE_SMALL) {
    if(img1.height() > ALEXANDRIA_MAX_SIZE_MEDIUM) { // limit maximum size
      img1 = img1.scaled(ALEXANDRIA_MAX_SIZE_MEDIUM, ALEXANDRIA_MAX_SIZE_MEDIUM, Qt::KeepAspectRatio);
    }
    img2 = img1.scaled(ALEXANDRIA_MAX_SIZE_SMALL, ALEXANDRIA_MAX_SIZE_SMALL, Qt::KeepAspectRatio);
  } else {
    // img2 is the small image
    img2 = img1;
    img1 = QImage();
  }
  return (img1.isNull() || img1.save(filename + QLatin1String("_medium.jpg"), "JPEG"))
      && img2.save(filename + QLatin1String("_small.jpg"), "JPEG");
}
