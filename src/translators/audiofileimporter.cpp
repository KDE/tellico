/***************************************************************************
    copyright            : (C) 2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "audiofileimporter.h"
#include "../collections/musiccollection.h"
#include "../latin1literal.h"
#include "../../config.h"
#include "../imagefactory.h"

#if HAVE_TAGLIB
#include <taglib/fileref.h>
#include <taglib/tag.h>
#endif

#include <klocale.h>
#include <kdebug.h>
#include <kapplication.h>
#include <kprogress.h>

#include <qlabel.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qdict.h>
#include <qcheckbox.h>
#include <qdir.h>
#include <qwhatsthis.h>

using Bookcase::Import::AudioFileImporter;

AudioFileImporter::AudioFileImporter(const KURL& url_) : Bookcase::Import::Importer(url_),
    m_coll(0),
    m_widget(0) {
}

Bookcase::Data::Collection* AudioFileImporter::collection() {
#if !HAVE_TAGLIB
  return 0;
#else

  if(m_coll) {
    return m_coll;
  }

  m_coll = new Data::MusicCollection(true);
  QDict<Data::Entry> dict;

  // TODO: allow remote audio file importing
  QStringList dirs = url().path();
  if(m_recursive->isChecked()) {
    dirs += findAllSubDirs(dirs[0]);
  }

  QStringList files;
  for(QStringList::ConstIterator it = dirs.begin(); it != dirs.end(); ++it) {
    if((*it).isEmpty()) {
      continue;
    }

    QDir dir(*it);
    dir.setFilter(QDir::Files | QDir::Readable | QDir::Hidden); // hidden since I want directory files
    const QStringList list = dir.entryList();
    for(QStringList::ConstIterator it2 = list.begin(); it2 != list.end(); ++it2) {
      files += dir.absFilePath(*it2);
    }
    kapp->processEvents();
  }

  const QString title = QString::fromLatin1("title");
  const QString artist = QString::fromLatin1("artist");
  const QString year = QString::fromLatin1("year");
  const QString genre = QString::fromLatin1("genre");
  const QString track = QString::fromLatin1("track");
  const QString comments = QString::fromLatin1("comments");

  KProgressDialog progressDlg(m_widget);
  progressDlg.setLabel(i18n("Scanning audio files..."));
  progressDlg.progressBar()->setTotalSteps(files.count());
  progressDlg.setMinimumDuration(files.count() > 20 ? 500 : 2000); // default is 2000

  QStringList directoryFiles;
  const uint stepSize = QMAX(1, files.count() / 100);
  uint step = 1;
  for(QStringList::ConstIterator it = files.begin(); it != files.end(); ++it, ++step) {
    if(progressDlg.wasCancelled()) {
      break;
    }
    TagLib::FileRef f(QFile::encodeName(*it));
    if(f.isNull() || !f.tag()) {
      if((*it).endsWith(QString::fromLatin1("/.directory"))) {
        directoryFiles += *it;
      }
      continue;
    }

    TagLib::Tag* tag = f.tag();
    Data::Entry* entry = 0;
    QString album = TStringToQString(tag->album());
    if(album.isEmpty()) {
      // can't do anything
      continue;
    }
    bool exists = true;
    if(!(entry = dict[album])) {
      entry = new Data::Entry(m_coll);
      dict.insert(album, entry);
      exists = false;
    }
    // album entries use the album name as the title
    entry->setField(title, album);
    if(!tag->artist().isEmpty()) {
      entry->setField(artist, TStringToQString(tag->artist()));
    }
    if(tag->year() > 0) {
      entry->setField(year, QString::number(tag->year()));
    }
    if(!tag->genre().isEmpty()) {
      entry->setField(genre, TStringToQString(tag->genre()));
    }
    if(!tag->title().isEmpty() && tag->track() > 0) {
      entry->setField(track, insertValue(entry->field(track), TStringToQString(tag->title()), tag->track()));
    } else {
      kdDebug() << *it << " contains no track number, so the song title is not imported." << endl;
    }
    if(!tag->comment().isEmpty()) {
      QString c = entry->field(comments);
      c += QString::fromLatin1("<em>") + TStringToQString(tag->title()) + QString::fromLatin1("</em> - ");
      c += TStringToQString(tag->comment()) + QString::fromLatin1("<br/>");
      entry->setField(comments, c);
    }
    if(!exists) {
      m_coll->addEntry(entry);
    }
    if(step % stepSize == 0) {
      progressDlg.progressBar()->setProgress(step);
      kapp->processEvents();
    }

/*    kdDebug() << "-- TAG --" << endl;
    kdDebug() << "title   - \"" << tag->title().to8Bit()   << "\"" << endl;
    kdDebug() << "artist  - \"" << tag->artist().to8Bit()  << "\"" << endl;
    kdDebug() << "album   - \"" << tag->album().to8Bit()   << "\"" << endl;
    kdDebug() << "year    - \"" << tag->year()             << "\"" << endl;
    kdDebug() << "comment - \"" << tag->comment().to8Bit() << "\"" << endl;
    kdDebug() << "track   - \"" << tag->track()            << "\"" << endl;
    kdDebug() << "genre   - \"" << tag->genre().to8Bit()   << "\"" << endl;*/
  }
  if(progressDlg.wasCancelled()) {
    delete m_coll;
    return 0;
  }

  QTextStream ts;
  QRegExp iconRx(QString::fromLatin1("Icon\\s*=\\s*(.*)"));
  for(QStringList::ConstIterator it = directoryFiles.begin(); it != directoryFiles.end(); ++it) {
    QFile file(*it);
    if(!file.open(IO_ReadOnly)) {
      continue;
    }
    ts.unsetDevice();
    ts.setDevice(&file);
    for(QString line = ts.readLine(); !line.isNull(); line = ts.readLine()) {
      if(!iconRx.exactMatch(line)) {
        continue;
      }
      QDir thisDir(*it);
      thisDir.cdUp();
      QFileInfo fi(thisDir, iconRx.cap(1));
      Data::Entry* entry = dict[thisDir.dirName()];
      if(!entry) {
        continue;
      }
      KURL u;
      u.setPath(fi.absFilePath());
      const Data::Image& img = ImageFactory::addImage(u, true);
      if(!img.isNull()) {
        entry->setField(QString::fromLatin1("cover"), img.id());
      }
      break;
    }
  }

  for(Data::EntryListIterator it(m_coll->entryList()); it.current(); ++it) {
    m_coll->updateDicts(it.current());
  }
  return m_coll;
#endif
}

QWidget* AudioFileImporter::widget(QWidget* parent_, const char* name_) {
  if(m_widget) {
    return m_widget;
  }

  m_widget = new QWidget(parent_, name_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* box = new QGroupBox(2, Qt::Horizontal, i18n("Audio File Options"), m_widget);

  m_recursive = new QCheckBox(i18n("Recursive folder search"), box);
  QWhatsThis::add(m_recursive, i18n("If checked, folders are recursively searched for audio files."));

  l->addWidget(box);
  l->addStretch(1);
  return m_widget;
}

// pos_ is NOT zero-indexed!
QString AudioFileImporter::insertValue(const QString& str_, const QString& value_, uint pos_) {
  static const QString sep = QString::fromLatin1("; ");
  QStringList list = Data::Field::split(str_, true);
  for(uint i = list.count(); i < pos_; ++i) {
    list += QString::null;
  }
  list[pos_-1] = value_;
  return list.join(sep);
}

QStringList AudioFileImporter::findAllSubDirs(const QString& dir_) {
  if(dir_.isEmpty()) {
    return QStringList();
  }

  // TODO: build in symlink chekcing, for now, prohibit
  QDir dir(dir_, QString::null, QDir::Name | QDir::IgnoreCase, QDir::Dirs | QDir::Readable | QDir::NoSymLinks);

  QStringList allSubdirs; // the whole list

  // find immediate sub directories
  const QStringList subdirs = dir.entryList();
  for(QStringList::ConstIterator subdir = subdirs.begin(); subdir != subdirs.end(); ++subdir) {
    if((*subdir).isEmpty() || *subdir == Latin1Literal(".") || *subdir == Latin1Literal("..")) {
      continue;
    }
    QString absSubdir = dir.absFilePath(*subdir);
    allSubdirs += findAllSubDirs(absSubdir);
    allSubdirs += absSubdir;
  }
  return allSubdirs;
}

#include "audiofileimporter.moc"
