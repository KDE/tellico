/***************************************************************************
    copyright            : (C) 2004-2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include <config.h>

#include "audiofileimporter.h"
#include "../collections/musiccollection.h"
#include "../entry.h"
#include "../field.h"
#include "../latin1literal.h"
#include "../imagefactory.h"
#include "../tellico_utils.h"

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
#include <qcheckbox.h>
#include <qdir.h>
#include <qwhatsthis.h>

using Tellico::Import::AudioFileImporter;

AudioFileImporter::AudioFileImporter(const KURL& url_) : Tellico::Import::Importer(url_),
    m_coll(0),
    m_widget(0) {
}

bool AudioFileImporter::canImport(int type) const {
  return type == Data::Collection::Album;
}

Tellico::Data::Collection* AudioFileImporter::collection() {
#if !HAVE_TAGLIB
  return 0;
#else

  if(m_coll) {
    return m_coll;
  }

  m_coll = new Data::MusicCollection(true);
  QMap<QString, Data::EntryPtr> albumMap;

  // TODO: allow remote audio file importing
  QStringList dirs = url().path();
  if(m_recursive->isChecked()) {
    dirs += Tellico::findAllSubDirs(dirs[0]);
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

  const QString title    = QString::fromLatin1("title");
  const QString artist   = QString::fromLatin1("artist");
  const QString year     = QString::fromLatin1("year");
  const QString genre    = QString::fromLatin1("genre");
  const QString track    = QString::fromLatin1("track");
  const QString comments = QString::fromLatin1("comments");

  KProgressDialog progressDlg(m_widget);
  progressDlg.setLabel(i18n("Scanning audio files..."));
  progressDlg.progressBar()->setTotalSteps(files.count());
  progressDlg.setMinimumDuration(files.count() > 20 ? 500 : 2000); // default is 2000

  QStringList directoryFiles;
  const uint stepSize = KMAX(static_cast<size_t>(1), files.count() / 100);
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
    if(album.stripWhiteSpace().isEmpty()) {
      // can't do anything
      continue;
    }
    bool various = false;
    QString initialArtist;
    bool exists = true;
    if(!(entry = albumMap[album.lower()])) {
      entry = new Data::Entry(m_coll);
      albumMap.insert(album.lower(), entry);
      exists = false;
    }
    // album entries use the album name as the title
    entry->setField(title, album);
    if(!tag->artist().isEmpty()) {
      QString a = TStringToQString(tag->artist());
      if(exists && entry->field(artist) != a) {
        various = true;
        a = i18n("(Various)");
        initialArtist = entry->field(artist);
      }
      entry->setField(artist, a);
    }
    if(tag->year() > 0) {
      entry->setField(year, QString::number(tag->year()));
    }
    if(!tag->genre().isEmpty()) {
      entry->setField(genre, TStringToQString(tag->genre()));
    }
    if(!tag->title().isEmpty() && tag->track() > 0) {
      if(various) {
        Data::Field* f = m_coll->fieldByName(track);
        // make sure it has 2 columns
        f->setProperty(QString::fromLatin1("columns"), QChar('2'));
        if(!initialArtist.isEmpty()) {
          // also need to add artist for the first song added to the collection
          QString other = entry->field(track);
          int i = 0;
          while(i < other.contains(';') && other.section(';', i, i).stripWhiteSpace().isEmpty()) {
            ++i;
          }
          other = other.section(';', i, i).stripWhiteSpace();
          if(!other.isEmpty()) {
            other += QString::fromLatin1("::") + initialArtist;
            // position is not zero-indexed...
            entry->setField(track, insertValue(QString::null, other, i+1));
          }
        }
        QString t = TStringToQString(tag->title()) + QString::fromLatin1("::") + TStringToQString(tag->artist());
        entry->setField(track, insertValue(entry->field(track), t, tag->track()));
      } else {
        entry->setField(track, insertValue(entry->field(track), TStringToQString(tag->title()), tag->track()));
      }
    } else {
      myDebug() << *it << " contains no track number or has an empty title, so the track is not imported." << endl;
    }
    if(!tag->comment().stripWhiteSpace().isEmpty()) {
      QString c = entry->field(comments);
      if(!c.isEmpty()) {
        c += QString::fromLatin1("<br/>");
      }
      if(!tag->title().isEmpty()) {
        c += QString::fromLatin1("<em>") + TStringToQString(tag->title()) + QString::fromLatin1("</em> - ");
      }
      c += TStringToQString(tag->comment().stripWhiteSpace());
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
      Data::Entry* entry = albumMap[thisDir.dirName()];
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

  Data::EntryVec vec = m_coll->entries();
  for(Data::EntryVecIt entryIt = vec.begin(); entryIt != vec.end(); ++entryIt) {
    m_coll->updateDicts(entryIt);
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
  // by default, make it checked
  m_recursive->setChecked(true);

  l->addWidget(box);
  l->addStretch(1);
  return m_widget;
}

// pos_ is NOT zero-indexed!
QString AudioFileImporter::insertValue(const QString& str_, const QString& value_, uint pos_) {
  QStringList list = Data::Field::split(str_, true);
  for(uint i = list.count(); i < pos_; ++i) {
    list += QString::null;
  }
  if(!list[pos_-1].isNull()) {
    myDebug() << "AudioFileImporter::insertValue() - overwriting track " << pos_ << endl;
    myDebug() << "*** Old value: " << list[pos_-1] << endl;
    myDebug() << "*** New value: " << value_ << endl;
  }
  list[pos_-1] = value_;
  return list.join(QString::fromLatin1("; "));
}

#include "audiofileimporter.moc"
