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

#include "alexandriaimporter.h"
#include "../collections/bookcollection.h"
#include "../latin1literal.h"
#include "../isbnvalidator.h"
#include "../imagefactory.h"

#include <kcombobox.h>
#include <kdebug.h>

#include <qlayout.h>
#include <qlabel.h>
#include <qgroupbox.h>

using Bookcase::Import::AlexandriaImporter;

Bookcase::Data::Collection* AlexandriaImporter::collection() {
  if(!m_widget || m_library->count() == 0) {
    return 0;
  }

  m_coll = new Data::BookCollection(true);

  QDir dataDir = m_libraryDir;
  dataDir.cd(m_library->currentText());
  dataDir.setFilter(QDir::Files | QDir::Readable | QDir::NoSymLinks);

  const QString title = QString::fromLatin1("title");
  const QString author = QString::fromLatin1("author");
  const QString binding = QString::fromLatin1("binding");
  const QString isbn = QString::fromLatin1("isbn");
  const QString pub = QString::fromLatin1("publisher");
  const QString rating = QString::fromLatin1("rating");
  const QString cover = QString::fromLatin1("cover");
  const QString comments = QString::fromLatin1("comments");
  const QStringList allowed = m_coll->fieldByName(QString::fromLatin1("rating"))->allowed();
  const QRegExp quote(QString::fromLatin1("\\\\\"")); // equals \"

  // start with yaml files
  dataDir.setNameFilter(QString::fromLatin1("*.yaml"));
  QTextStream ts;
  const QStringList files = dataDir.entryList();
  for(QStringList::ConstIterator it = files.begin(); it != files.end(); ++it) {
    QFile file(dataDir.absFilePath(*it));
    if(!file.open(IO_ReadOnly)) {
      continue;
    }

    Data::Entry* entry = new Data::Entry(m_coll);

    bool readingAuthors = false;
    QStringList authors;

    ts.unsetDevice();
    ts.setDevice(&file);
    for(QString line = ts.readLine(); !line.isNull(); line = ts.readLine()) {
      // skip the line that starts with ---
      if(line.startsWith(QString::fromLatin1("---"))) {
        continue;
      }

      if(readingAuthors) {
        if(line.startsWith(QString::fromLatin1("  - "))) {
          line.remove(0, 4);
          authors += line;
          continue;
        } else {
          entry->setField(author, authors.join(QString::fromLatin1("; ")));
          readingAuthors = false;
        }
      }

      QString alexField = line.section(':', 0, 0);
      QString alexValue = line.section(':', 1, 1).stripWhiteSpace();

      if(alexField == Latin1Literal("authors")) {
        readingAuthors = true;
        // Alexandria calls the edition the binding
      } else if(alexField == Latin1Literal("edition")) {
        entry->setField(binding, alexValue);

      } else if(alexField == Latin1Literal("isbn")) {
        KURL u;
        u.setPath(dataDir.absFilePath(alexValue + QString::fromLatin1("_medium.jpg")));
        const Data::Image& img = ImageFactory::addImage(u, true);
        if(!img.isNull()) {
          entry->setField(cover, img.id());
        } else {
          u.setPath(dataDir.absFilePath(alexValue + QString::fromLatin1("_small.jpg")));
          const Data::Image& img = ImageFactory::addImage(u, true);
          if(!img.isNull()) {
            entry->setField(cover, img.id());
          }
        }
        ISBNValidator val(0);
        val.fixup(alexValue);
        entry->setField(isbn, alexValue);

      } else if(alexField == Latin1Literal("notes")) {
        if(alexValue.startsWith(QChar('\'')) || alexValue.startsWith(QChar('"'))) {
          alexValue.remove(0, 1);
        }
        if(alexValue.endsWith(QChar('\'')) || alexValue.endsWith(QChar('"'))) {
          alexValue.truncate(alexValue.length()-1);
        }
        alexValue.replace(quote, QChar('"'));
        entry->setField(comments, alexValue);

      } else if(alexField == Latin1Literal("rating")) {
        // assume the rating is a number, and find the allowed value with that number
        entry->setField(rating, allowed.grep(alexValue)[0]);

      } else if(alexField == Latin1Literal("title")) {
        if(alexValue.startsWith(QChar('\'')) || alexValue.startsWith(QChar('"'))) {
          alexValue.remove(0, 1);
        }
        if(alexValue.endsWith(QChar('\'')) || alexValue.endsWith(QChar('"'))) {
          alexValue.truncate(alexValue.length()-1);
        }
        alexValue.replace(quote, QChar('"'));
        entry->setField(title, alexValue);

      } else if(m_coll->fieldByName(alexField)) {
        entry->setField(alexField, alexValue);
      } else if(m_coll->fieldByTitle(alexField)) {
        entry->setField(m_coll->fieldNameByTitle(alexField), alexValue);
      }
    }
    m_coll->addEntry(entry);
  }

  return m_coll;
}

QWidget* AlexandriaImporter::widget(QWidget* parent_, const char* name_/*=0*/) {
  if(m_widget) {
    return m_widget;
  }

  m_libraryDir = QDir::home();
  m_libraryDir.setFilter(QDir::Dirs | QDir::Readable | QDir::NoSymLinks);

  m_widget = new QWidget(parent_, name_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* box = new QGroupBox(2, Qt::Horizontal, i18n("Alexandria Options"), m_widget);
  (void) new QLabel(i18n("Library:"), box);
  m_library = new KComboBox(box);

  // .alexandria might not exist
  if(m_libraryDir.cd(QString::fromLatin1(".alexandria"))) {
    QStringList dirs = m_libraryDir.entryList();
    dirs.remove(QString::fromLatin1(".")); // why can't I tell QDir not to include these? QDir::Hidden doesn't work
    dirs.remove(QString::fromLatin1(".."));
    m_library->insertStringList(dirs);
  }

  l->addWidget(box);
  l->addStretch(1);
  return m_widget;
}

#include "alexandriaimporter.moc"
