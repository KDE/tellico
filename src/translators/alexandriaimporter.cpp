/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#include "alexandriaimporter.h"
#include "../collections/bookcollection.h"
#include "../entry.h"
#include "../field.h"
#include "../images/imagefactory.h"
#include "../tellico_debug.h"
#include "../utils/isbnvalidator.h"

#include <kcombobox.h>
#include <kapplication.h>
#include <kstringhandler.h>

#include <QLabel>
#include <QGroupBox>
#include <QTextStream>
#include <QByteArray>
#include <QHBoxLayout>

using Tellico::Import::AlexandriaImporter;

bool AlexandriaImporter::canImport(int type) const {
  return type == Data::Collection::Book;
}

Tellico::Data::CollPtr AlexandriaImporter::collection() {
  QDir dataDir;
  if(m_libraryDir.exists() && m_library && m_library->count() > 0) {
    dataDir = m_libraryDir;
    dataDir.cd(m_library->currentText());
  } else if(!m_libraryPath.isEmpty()) {
    dataDir.setPath(m_libraryPath);
  } else {
    // no widget and no explicit set of the library path means we fail
    myWarning() << "no widget and no library path";
    return Data::CollPtr();
  }
  // just a sanity check
  if(!dataDir.exists()) {
    myDebug() << dataDir.path() << "doesn't exist";
    return Data::CollPtr();
  }
  dataDir.setFilter(QDir::Files | QDir::Readable | QDir::NoSymLinks);

  m_coll = new Data::BookCollection(true);

  const QString title = QLatin1String("title");
  const QString author = QLatin1String("author");
  const QString year = QLatin1String("pub_year");
  const QString binding = QLatin1String("binding");
  const QString isbn = QLatin1String("isbn");
  const QString pub = QLatin1String("publisher");
  const QString rating = QLatin1String("rating");
  const QString cover = QLatin1String("cover");
  const QString comments = QLatin1String("comments");

  // start with yaml files
  dataDir.setNameFilters(QStringList() << QLatin1String("*.yaml"));
  const QStringList files = dataDir.entryList();
  const uint numFiles = files.count();
  const uint stepSize = qMax(s_stepSize, numFiles/100);
  const bool showProgress = options() & ImportProgress;

  emit signalTotalSteps(this, numFiles);

  QStringList covers;
  covers << QLatin1String(".cover")
         << QLatin1String("_medium.jpg")
         << QLatin1String("_small.jpg");

  QTextStream ts;
  ts.setCodec("UTF-8"); // YAML is always utf8?
  uint j = 0;
  for(QStringList::ConstIterator it = files.begin(); !m_cancelled && it != files.end(); ++it, ++j) {
    QFile file(dataDir.absoluteFilePath(*it));
    if(!file.open(QIODevice::ReadOnly)) {
      continue;
    }

    Data::EntryPtr entry(new Data::Entry(m_coll));

    bool readNextLine = true;
    ts.setDevice(&file);
    ts.setCodec("UTF-8"); // YAML is always utf8?
    QString line;
    while(!ts.atEnd()) {
      if(readNextLine) {
        line = ts.readLine();
      } else {
        readNextLine = true;
      }
      // skip the line that starts with ---
      if(line.isEmpty() || line.startsWith(QLatin1String("---"))) {
        continue;
      }
      if(line.endsWith(QLatin1Char('\\'))) {
        line.truncate(line.length()-1); // remove last character
        line += ts.readLine();
      }

      cleanLine(line);
      QString alexField = line.section(QLatin1Char(':'), 0, 0);
      QString alexValue = line.section(QLatin1Char(':'), 1).trimmed();
      clean(alexValue);

      // Alexandria uses "n/a for empty values, and it is translated
      // only thing we can do is check for english value and continue
      if(alexValue == QLatin1String("n/a") || alexValue == QLatin1String("false")) {
        continue;
      }

      if(alexField == QLatin1String("redd"))  {
        alexField = QLatin1String("read");
      }

      if(alexField == QLatin1String("authors")) {
        QStringList authors;
        line = ts.readLine();
        QRegExp begin(QLatin1String("^\\s*-\\s+"));
        while(!line.isNull() && line.indexOf(begin) > -1) {
          line.remove(begin);
          authors += clean(line);
          line = ts.readLine();
        }
        entry->setField(author, authors.join(QLatin1String("; ")));
        // the next line has already been read
        readNextLine = false;

        // Alexandria calls the edition the binding
      } else if(alexField == QLatin1String("edition")) {
        // special case if it's "Hardcover"
        if(alexValue.toLower() == QLatin1String("hardcover")) {
          alexValue = i18n("Hardback");
        }
        entry->setField(binding, alexValue);

      } else if(alexField == QLatin1String("publishing_year")) {
        entry->setField(year, alexValue);

      } else if(alexField == QLatin1String("isbn")) {
        const ISBNValidator val(0);
        val.fixup(alexValue);
        entry->setField(isbn, alexValue);

        // now find cover image
        KUrl u;
        alexValue.remove(QLatin1Char('-'));
        for(QStringList::Iterator ext = covers.begin(); ext != covers.end(); ++ext) {
          u.setPath(dataDir.absoluteFilePath(alexValue + *ext));
          if(!QFile::exists(u.path())) {
            continue;
          }
          QString id = ImageFactory::addImage(u, true);
          if(!id.isEmpty()) {
            entry->setField(cover, id);
            break;
          }
        }
      } else if(alexField == QLatin1String("notes")) {
        entry->setField(comments, alexValue);

      // now try by name then title
      } else if(m_coll->fieldByName(alexField)) {
        entry->setField(alexField, alexValue);

      } else if(m_coll->fieldByTitle(alexField)) {
        entry->setField(m_coll->fieldByTitle(alexField), alexValue);
      }
    }
    m_coll->addEntries(entry);

    if(showProgress && j%stepSize == 0) {
      emit signalProgress(this, j);
      kapp->processEvents();
    }
  }

  return m_coll;
}

QWidget* AlexandriaImporter::widget(QWidget* parent_) {
  if(m_widget) {
    return m_widget;
  }

  m_libraryDir = QDir::home();
  m_libraryDir.setFilter(QDir::Dirs | QDir::Readable | QDir::NoSymLinks | QDir::NoDotAndDotDot);

  m_widget = new QWidget(parent_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* gbox = new QGroupBox(i18n("Alexandria Options"), m_widget);
  QHBoxLayout* hlay = new QHBoxLayout(gbox);

  QLabel* label = new QLabel(i18n("&Library:"), gbox);
  m_library = new KComboBox(gbox);
  label->setBuddy(m_library);

  hlay->addWidget(label);
  hlay->addWidget(m_library);

  // .alexandria might not exist
  if(m_libraryDir.cd(QLatin1String(".alexandria"))) {
    m_library->addItems(m_libraryDir.entryList());
  }

  l->addWidget(gbox);
  l->addStretch(1);

  // now that we set a widget, it should override library path
  m_libraryPath.clear();

  return m_widget;
}

QString& AlexandriaImporter::cleanLine(QString& str_) {
  static QRegExp escRx(QLatin1String("\\\\x(\\w\\w)"), Qt::CaseInsensitive);
  str_.remove(QLatin1String("\\r"));
  str_.replace(QLatin1String("\\n"), QLatin1String("\n"));
  str_.replace(QLatin1String("\\t"), QLatin1String("\t"));

  // YAML uses escape sequences like \xC3
  int pos = escRx.indexIn(str_);
  int origPos = pos;
  QByteArray bytes;
  while(pos > -1) {
    bool ok;
    char c = static_cast<char>(escRx.cap(1).toInt(&ok, 16));
    if(ok) {
      bytes += c;
    } else {
      bytes.clear();
      break;
    }
    pos = escRx.indexIn(str_, pos+1);
  }
  if(!bytes.isEmpty()) {
    str_.replace(origPos, bytes.length()*4, QString::fromUtf8(bytes.data()));
  }
  return str_;
}

QString& AlexandriaImporter::clean(QString& str_) {
  const QRegExp quote(QLatin1String("\\\\\"")); // equals \"
  if(str_.startsWith(QLatin1Char('\'')) || str_.startsWith(QLatin1Char('"'))) {
    str_.remove(0, 1);
  }
  if(str_.endsWith(QLatin1Char('\'')) || str_.endsWith(QLatin1Char('"'))) {
    str_.truncate(str_.length()-1);
  }
  // we ignore YAML tags, this is not actually a good parser, but will do for now
  str_.remove(QRegExp(QLatin1String("^![^\\s]*\\s+")));
  return str_.replace(quote, QLatin1String("\""));
}

void AlexandriaImporter::slotCancel() {
  m_cancelled = true;
}

#include "alexandriaimporter.moc"
