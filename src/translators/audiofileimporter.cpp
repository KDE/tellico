/***************************************************************************
    copyright            : (C) 2004-2006 by Robby Stephenson
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
#include "../tellico_kernel.h"
#include "../progressmanager.h"

#if HAVE_TAGLIB
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/audioproperties.h>
#endif

#include <klocale.h>
#include <kdebug.h>
#include <kapplication.h>
#include <kglobal.h> // for KMAX

#include <qlabel.h>
#include <qlayout.h>
#include <qvgroupbox.h>
#include <qcheckbox.h>
#include <qdir.h>
#include <qwhatsthis.h>

using Tellico::Import::AudioFileImporter;

AudioFileImporter::AudioFileImporter(const KURL& url_) : Tellico::Import::Importer(url_)
    , m_coll(0)
    , m_widget(0)
    , m_cancelled(false) {
}

bool AudioFileImporter::canImport(int type) const {
  return type == Data::Collection::Album;
}

Tellico::Data::CollPtr AudioFileImporter::collection() {
#if !HAVE_TAGLIB
  return 0;
#else

  if(m_coll) {
    return m_coll;
  }

  ProgressItem& item = ProgressManager::self()->newProgressItem(this, i18n("Scanning audio files..."), true);
  item.setTotalSteps(100);
  connect(&item, SIGNAL(signalCancelled(ProgressItem*)), SLOT(slotCancel()));

  // TODO: allow remote audio file importing
  QStringList dirs = url().path();
  if(m_recursive->isChecked()) {
    dirs += Tellico::findAllSubDirs(dirs[0]);
  }

  if(m_cancelled) {
    return 0;
  }

  QStringList files;
  for(QStringList::ConstIterator it = dirs.begin(); !m_cancelled && it != dirs.end(); ++it) {
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

  if(m_cancelled) {
    return 0;
  }
  item.setTotalSteps(files.count());

  const QString title    = QString::fromLatin1("title");
  const QString artist   = QString::fromLatin1("artist");
  const QString year     = QString::fromLatin1("year");
  const QString genre    = QString::fromLatin1("genre");
  const QString track    = QString::fromLatin1("track");
  const QString comments = QString::fromLatin1("comments");
  const QString file     = QString::fromLatin1("file");

  m_coll = new Data::MusicCollection(true);

  const bool addFile = m_filePath->isChecked();
  if(addFile && !m_coll->hasField(file)) {
    Data::FieldPtr f = new Data::Field(file, i18n("Files"), Data::Field::Table);
    f->setProperty(QString::fromLatin1("columns"), QChar('1'));
    f->setProperty(QString::fromLatin1("column1"), i18n("Files"));
    m_coll->addField(f);
  }

  QMap<QString, Data::EntryPtr> albumMap;

  QStringList directoryFiles;
  const uint stepSize = KMAX(static_cast<size_t>(1), files.count() / 100);

  uint j = 0;
  for(QStringList::ConstIterator it = files.begin(); !m_cancelled && it != files.end(); ++it, ++j) {
    TagLib::FileRef f(QFile::encodeName(*it));
    if(f.isNull() || !f.tag()) {
      if((*it).endsWith(QString::fromLatin1("/.directory"))) {
        directoryFiles += *it;
      }
      continue;
    }

    TagLib::Tag* tag = f.tag();
    QString album = TStringToQString(tag->album()).stripWhiteSpace();
    if(album.isEmpty()) {
      // can't do anything
      continue;
    }
    bool various = false;
    bool exists = true;
    Data::EntryPtr entry = 0;
    if(!(entry = albumMap[album.lower()])) {
      entry = new Data::Entry(m_coll);
      albumMap.insert(album.lower(), entry);
      exists = false;
    }
    // album entries use the album name as the title
    entry->setField(title, album);
    QString a = TStringToQString(tag->artist()).stripWhiteSpace();
    if(!a.isEmpty()) {
      if(exists && entry->field(artist).lower() != a.lower()) {
        various = true;
        entry->setField(artist, i18n("(Various)"));
      } else {
        entry->setField(artist, a);
      }
    }
    if(tag->year() > 0) {
      entry->setField(year, QString::number(tag->year()));
    }
    if(!tag->genre().isEmpty()) {
      entry->setField(genre, TStringToQString(tag->genre()).stripWhiteSpace());
    }

    if(!tag->title().isEmpty()) {
      int trackNum = tag->track();
      if(trackNum <= 0) { // try to figure out track number from file name
        QFileInfo f(*it);
        QString fileName = f.baseName();
        QString numString;
        int i = 0;
        const int len = fileName.length();
        while(fileName[i].isNumber() && i < len) {
          i++;
        }
        if(i == 0) { // does not start with a number
          i = len - 1;
          while(i >= 0 && fileName[i].isNumber()) {
            i--;
          }
          // file name ends with a number
          if(i != len - 1) {
            numString = fileName.mid(i + 1);
          }
        } else {
          numString = fileName.mid(0, i);
        }
        bool ok;
        int number = numString.toInt(&ok);
        if(ok) {
          trackNum = number;
        }
      }
      if(trackNum > 0) {
        QString t = TStringToQString(tag->title()).stripWhiteSpace();
        t += "::" + a;
        const int len = f.audioProperties()->length();
        if(len > 0) {
          t += "::" + Tellico::minutes(len);
        }
        entry->setField(track, insertValue(entry->field(track), t, trackNum));
        if(addFile) {
          entry->setField(file, insertValue(entry->field(file), *it, trackNum));
        }
      } else {
        myDebug() << *it << " contains no track number and track number cannot be determined, so the track is not imported." << endl;
      }
    } else {
      myDebug() << *it << " has an empty title, so the track is not imported." << endl;
    }
    if(!tag->comment().stripWhiteSpace().isEmpty()) {
      QString c = entry->field(comments);
      if(!c.isEmpty()) {
        c += QString::fromLatin1("<br/>");
      }
      if(!tag->title().isEmpty()) {
        c += QString::fromLatin1("<em>") + TStringToQString(tag->title()).stripWhiteSpace() + QString::fromLatin1("</em> - ");
      }
      c += TStringToQString(tag->comment().stripWhiteSpace()).stripWhiteSpace();
      entry->setField(comments, c);
    }

    if(!exists) {
      m_coll->addEntry(entry);
    }
    if(j%stepSize == 0) {
      ProgressManager::self()->setTotalSteps(this, files.count() + directoryFiles.count());
      ProgressManager::self()->setProgress(this, j);
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

  if(m_cancelled) {
    m_coll = 0;
    return 0;
  }

  QTextStream ts;
  QRegExp iconRx(QString::fromLatin1("Icon\\s*=\\s*(.*)"));
  for(QStringList::ConstIterator it = directoryFiles.begin(); !m_cancelled && it != directoryFiles.end(); ++it, ++j) {
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
      Data::EntryPtr entry = albumMap[thisDir.dirName()];
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

    if(j%stepSize == 0) {
      ProgressManager::self()->setProgress(this, j);
      kapp->processEvents();
    }
  }

  if(m_cancelled) {
    m_coll = 0;
    return 0;
  }

  ProgressManager::self()->setDone(this);

  m_coll->updateDicts(m_coll->entries());
  return m_coll;
#endif
}

QWidget* AudioFileImporter::widget(QWidget* parent_, const char* name_) {
  if(m_widget) {
    return m_widget;
  }

  m_widget = new QWidget(parent_, name_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QVGroupBox* box = new QVGroupBox(i18n("Audio File Options"), m_widget);

  m_recursive = new QCheckBox(i18n("Recursive &folder search"), box);
  QWhatsThis::add(m_recursive, i18n("If checked, folders are recursively searched for audio files."));
  // by default, make it checked
  m_recursive->setChecked(true);

  m_filePath = new QCheckBox(i18n("&Include file location"), box);
  QWhatsThis::add(m_filePath, i18n("If checked, the file names for each track are added to the entries."));
  // by default, make it checked
  m_filePath->setChecked(false);

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

void AudioFileImporter::slotCancel() {
  m_cancelled = true;
}

#include "audiofileimporter.moc"
