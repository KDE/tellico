/***************************************************************************
    Copyright (C) 2004-2009 Robby Stephenson <robby@periapsis.org>
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

#include <config.h>

#include "audiofileimporter.h"
#include "../collections/musiccollection.h"
#include "../entry.h"
#include "../field.h"
#include "../fieldformat.h"
#include "../images/imagefactory.h"
#include "../tellico_utils.h"
#include "../tellico_kernel.h"
#include "../progressmanager.h"
#include "../tellico_debug.h"

#ifdef HAVE_TAGLIB
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/id3v2tag.h>
#include <taglib/mpegfile.h>
#include <taglib/vorbisfile.h>
#include <taglib/flacfile.h>
#include <taglib/audioproperties.h>
#endif

#include <klocale.h>
#include <kapplication.h>

#include <QLabel>
#include <QGroupBox>
#include <QCheckBox>
#include <QDir>
#include <QTextStream>
#include <QVBoxLayout>

using Tellico::Import::AudioFileImporter;

AudioFileImporter::AudioFileImporter(const KUrl& url_) : Tellico::Import::Importer(url_)
    , m_widget(0)
    , m_cancelled(false) {
}

bool AudioFileImporter::canImport(int type) const {
  return type == Data::Collection::Album;
}

Tellico::Data::CollPtr AudioFileImporter::collection() {
#ifndef HAVE_TAGLIB
  return Data::CollPtr();
#else

  if(m_coll) {
    return m_coll;
  }

  ProgressItem& item = ProgressManager::self()->newProgressItem(this, i18n("Scanning audio files..."), true);
  item.setTotalSteps(100);
  connect(&item, SIGNAL(signalCancelled(ProgressItem*)), SLOT(slotCancel()));
  ProgressItem::Done done(this);

  // TODO: allow remote audio file importing
  QStringList dirs;
  dirs += url().path();
  if(m_recursive->isChecked()) {
    dirs += Tellico::findAllSubDirs(dirs[0]);
  }

  if(m_cancelled) {
    return Data::CollPtr();
  }

  const bool showProgress = options() & ImportProgress;

  QStringList files;
  for(QStringList::ConstIterator it = dirs.constBegin(); !m_cancelled && it != dirs.constEnd(); ++it) {
    if((*it).isEmpty()) {
      continue;
    }

    QDir dir(*it);
    dir.setFilter(QDir::Files | QDir::Readable | QDir::Hidden); // hidden since I want directory files
    const QStringList list = dir.entryList();
    for(QStringList::ConstIterator it2 = list.begin(); it2 != list.end(); ++it2) {
      files += dir.absoluteFilePath(*it2);
    }
//    kapp->processEvents(); not needed ?
  }

  if(m_cancelled) {
    return Data::CollPtr();
  }
  item.setTotalSteps(files.count());

  const QString title    = QLatin1String("title");
  const QString artist   = QLatin1String("artist");
  const QString year     = QLatin1String("year");
  const QString genre    = QLatin1String("genre");
  const QString track    = QLatin1String("track");
  const QString comments = QLatin1String("comments");
  const QString file     = QLatin1String("file");

  m_coll = new Data::MusicCollection(true);

  const bool addFile = m_addFilePath->isChecked();
  const bool addBitrate = m_addBitrate->isChecked();

  Data::FieldPtr f;
  if(addFile) {
    f = m_coll->fieldByName(file);
    if(!f) {
      f = new Data::Field(file, i18n("Files"), Data::Field::Table);
      m_coll->addField(f);
    }
    f->setProperty(QLatin1String("column1"), i18n("Files"));
    if(addBitrate) {
      f->setProperty(QLatin1String("columns"), QLatin1String("2"));
      f->setProperty(QLatin1String("column2"), i18n("Bitrate"));
    } else {
      f->setProperty(QLatin1String("columns"), QLatin1String("1"));
    }
  }

  QHash<QString, Data::EntryPtr> albumMap;

  QStringList directoryFiles;
  const uint stepSize = qMax(1, files.count() / 100);

  bool changeTrackTitle = true;
  uint j = 0;
  for(QStringList::ConstIterator it = files.constBegin(); !m_cancelled && it != files.constEnd(); ++it, ++j) {
    TagLib::FileRef f(QFile::encodeName(*it).data());
    if(f.isNull() || !f.tag()) {
      if((*it).endsWith(QLatin1String("/.directory"))) {
        directoryFiles += *it;
      }
      continue;
    }

    TagLib::Tag* tag = f.tag();
    QString album = TStringToQString(tag->album()).trimmed();
    if(album.isEmpty()) {
      // can't do anything since tellico entries are by album
      myWarning() << "Skipping: no album listed for " << *it;
      continue;
    }
    int disc = discNumber(f);
    if(disc > 1 && !m_coll->hasField(QString::fromLatin1("track%1").arg(disc))) {
      Data::FieldPtr f2(new Data::Field(QString::fromLatin1("track%1").arg(disc),
                                        i18n("Tracks (Disc %1)", disc),
                                        Data::Field::Table));
      f2->setFormatType(FieldFormat::FormatTitle);
      f2->setProperty(QLatin1String("columns"), QLatin1String("3"));
      f2->setProperty(QLatin1String("column1"), i18n("Title"));
      f2->setProperty(QLatin1String("column2"), i18n("Artist"));
      f2->setProperty(QLatin1String("column3"), i18n("Length"));
      m_coll->addField(f2);
      if(changeTrackTitle) {
        Data::FieldPtr newTrack(new Data::Field(*m_coll->fieldByName(track)));
        newTrack->setTitle(i18n("Tracks (Disc %1)", 1));
        m_coll->modifyField(newTrack);
        changeTrackTitle = false;
      }
    }
    bool exists = true;
    Data::EntryPtr entry;
/*
    Let's assume an album already exists (has already been imported) if an
    album entry with same Album Title and Album Artist is found; indeed,
    multiple albums can have the same title (but from different artists),
    but this is very unlikely the same artist release multiple albums with
    the same title. Therefore, we propose to make an album entry ID as follows:
    "<album title>::<album artist>" if album artist info is available,
    "<album title>" if not.
*/
    QString albumKey = album.toLower();
/*
    For MP3 files, get the Album Artist from the ID3v2 TPE2 frame.
    See http://www.id3.org/id3v2.4.0-frames for a description of this frame.
    Although this is not standard in ID3, using a specific frame for album
    artist is a solution to the problem of tagging albums that feature
    various artists but still have an identified Album Artist, such as
    Remix and DJ albums. Example:
    Album title: Some Title; Album artist: Some DJ;
                 Track 1: Some Track Title - Some Artist(s);
                 Track 2: Some Other Track Title - Some Other Artist(s), etc.
    We read the Album Artist from the TPE2 frame to be compatible with
    Amarok as the most popular music player for KDE, but also Apple (iTunes),
    Microsoft (Windows Media Player) and others which use this frame to
    read/write the album artist too.
    See Amarok source file src/collectionscanner/CollectionScanner.cpp,
    method AttributeHash CollectionScanner::readTags(...).
*/
    // TODO: find another way for non-MP3 files
    QString albumArtist;
/*  As mpeg implementation on TagLib uses a Tag class that's not defined on the headers,
    we have to cast the files, not the tags!
*/
    TagLib::MPEG::File* mpegFile = dynamic_cast<TagLib::MPEG::File*>(f.file());
    if(mpegFile && mpegFile->ID3v2Tag() && !mpegFile->ID3v2Tag()->frameListMap()["TPE2"].isEmpty()) {
      albumArtist = TStringToQString(mpegFile->ID3v2Tag()->frameListMap()["TPE2"].front()->toString()).trimmed();
      if(!albumArtist.isEmpty()) {
        albumKey += FieldFormat::columnDelimiterString() + albumArtist.toLower();
      }
    }

    entry = albumMap[albumKey];
    if(!entry) {
      entry = Data::EntryPtr(new Data::Entry(m_coll));
      albumMap.insert(albumKey, entry);
      exists = false;
    }
    // album entries use the album name as the title
    entry->setField(title, album);
    QString a = TStringToQString(tag->artist()).trimmed();
    // If no album artist identified, we use track artist as album artist, or  "(Various)" if tracks have various artists.
    if(!albumArtist.isEmpty()) {
      entry->setField(artist, albumArtist);
    } else if(!a.isEmpty()) {
      if(exists && entry->field(artist).toLower() != a.toLower()) {
        entry->setField(artist, i18n("(Various)"));
      } else {
        entry->setField(artist, a);
      }
    }
    if(tag->year() > 0) {
      entry->setField(year, QString::number(tag->year()));
    }
    if(!tag->genre().isEmpty()) {
      entry->setField(genre, TStringToQString(tag->genre()).trimmed());
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
        QString t = TStringToQString(tag->title()).trimmed();
        t += FieldFormat::columnDelimiterString() + a;
        const int len = f.audioProperties()->length();
        if(len > 0) {
          t += FieldFormat::columnDelimiterString() + Tellico::minutes(len);
        }
        QString realTrack = disc > 1 ? track + QString::number(disc) : track;
        entry->setField(realTrack, insertValue(entry->field(realTrack), t, trackNum));
        if(addFile) {
          QString fileValue = *it;
          if(addBitrate) {
            fileValue += FieldFormat::columnDelimiterString() + QString::number(f.audioProperties()->bitrate());
          }
          entry->setField(file, insertValue(entry->field(file), fileValue, trackNum));
        }
      } else {
        myDebug() << *it << " contains no track number and track number cannot be determined, so the track is not imported.";
      }
    } else {
      myDebug() << *it << " has an empty title, so the track is not imported.";
    }
    if(!tag->comment().stripWhiteSpace().isEmpty()) {
      QString c = entry->field(comments);
      if(!c.isEmpty()) {
        c += QLatin1String("<br/>");
      }
      if(!tag->title().isEmpty()) {
        c += QLatin1String("<em>") + TStringToQString(tag->title()).trimmed() + QLatin1String("</em> - ");
      }
      c += TStringToQString(tag->comment().stripWhiteSpace());
      entry->setField(comments, c);
    }

    if(!exists) {
      m_coll->addEntries(entry);
    }
    if(showProgress && j%stepSize == 0) {
      ProgressManager::self()->setTotalSteps(this, files.count() + directoryFiles.count());
      ProgressManager::self()->setProgress(this, j);
      kapp->processEvents();
    }

/*    myDebug() << "-- TAG --";
    myDebug() << "title   - \"" << tag->title().to8Bit()   << "\"";
    myDebug() << "artist  - \"" << tag->artist().to8Bit()  << "\"";
    myDebug() << "album   - \"" << tag->album().to8Bit()   << "\"";
    myDebug() << "year    - \"" << tag->year()             << "\"";
    myDebug() << "comment - \"" << tag->comment().to8Bit() << "\"";
    myDebug() << "track   - \"" << tag->track()            << "\"";
    myDebug() << "genre   - \"" << tag->genre().to8Bit()   << "\"";*/
  }

  if(m_cancelled) {
    m_coll = Data::CollPtr();
    return m_coll;
  }

  QTextStream ts;
  QRegExp iconRx(QLatin1String("Icon\\s*=\\s*(.*)"));
  for(QStringList::ConstIterator it = directoryFiles.constBegin(); !m_cancelled && it != directoryFiles.constEnd(); ++it, ++j) {
    QFile file(*it);
    if(!file.open(QIODevice::ReadOnly)) {
      continue;
    }
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
      KUrl u;
      u.setPath(fi.absoluteFilePath());
      QString id = ImageFactory::addImage(u, true);
      if(!id.isEmpty()) {
        entry->setField(QLatin1String("cover"), id);
      }
      break;
    }

    if(showProgress && j%stepSize == 0) {
      ProgressManager::self()->setProgress(this, j);
      kapp->processEvents();
    }
  }

  if(m_cancelled) {
    m_coll = Data::CollPtr();
  }

  return m_coll;
#endif
}

QWidget* AudioFileImporter::widget(QWidget* parent_) {
  if(m_widget) {
    return m_widget;
  }

  m_widget = new QWidget(parent_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* gbox = new QGroupBox(i18n("Audio File Options"), m_widget);
  QVBoxLayout* vlay = new QVBoxLayout(gbox);

  m_recursive = new QCheckBox(i18n("Recursive &folder search"), gbox);
  m_recursive->setWhatsThis(i18n("If checked, folders are recursively searched for audio files."));
  // by default, make it checked
  m_recursive->setChecked(true);

  m_addFilePath = new QCheckBox(i18n("Include file &location"), gbox);
  m_addFilePath->setWhatsThis(i18n("If checked, the file names for each track are added to the entries."));
  m_addFilePath->setChecked(false);
  connect(m_addFilePath, SIGNAL(toggled(bool)), SLOT(slotAddFileToggled(bool)));

  m_addBitrate = new QCheckBox(i18n("Include &bitrate"), gbox);
  m_addBitrate->setWhatsThis(i18n("If checked, the bitrate for each track is added to the entries."));
  m_addBitrate->setChecked(false);
  m_addBitrate->setEnabled(false);

  vlay->addWidget(m_recursive);
  vlay->addWidget(m_addFilePath);
  vlay->addWidget(m_addBitrate);

  l->addWidget(gbox);
  l->addStretch(1);
  return m_widget;
}

// pos_ is NOT zero-indexed!
QString AudioFileImporter::insertValue(const QString& str_, const QString& value_, int pos_) {
  QStringList list = FieldFormat::splitTable(str_);
  for(int i = list.count(); i < pos_; ++i) {
    list.append(QString());
  }
  if(!list.at(pos_-1).isEmpty()) {
    myDebug() << "overwriting track " << pos_;
    myDebug() << "*** Old value: " << list[pos_-1];
    myDebug() << "*** New value: " << value_;
  }
  list[pos_-1] = value_;
  return list.join(FieldFormat::rowDelimiterString());
}

void AudioFileImporter::slotCancel() {
  m_cancelled = true;
}

void AudioFileImporter::slotAddFileToggled(bool on_) {
  m_addBitrate->setEnabled(on_);
  if(!on_) {
    m_addBitrate->setChecked(false);
  }
}

int AudioFileImporter::discNumber(const TagLib::FileRef& ref_) const {
  // default to 1 unless otherwise
  int num = 1;
#ifdef HAVE_TAGLIB
  QString disc;
  if(TagLib::MPEG::File* file = dynamic_cast<TagLib::MPEG::File*>(ref_.file())) {
    if(file->ID3v2Tag() && !file->ID3v2Tag()->frameListMap()["TPOS"].isEmpty()) {
      disc = TStringToQString(file->ID3v2Tag()->frameListMap()["TPOS"].front()->toString()).trimmed();
    }
  } else if(TagLib::Ogg::Vorbis::File* file = dynamic_cast<TagLib::Ogg::Vorbis::File*>(ref_.file())) {
    if(file->tag() && !file->tag()->fieldListMap()["DISCNUMBER"].isEmpty()) {
      disc = TStringToQString(file->tag()->fieldListMap()["DISCNUMBER"].front()).trimmed();
    }
  } else if(TagLib::FLAC::File* file = dynamic_cast<TagLib::FLAC::File*>(ref_.file())) {
    if(file->xiphComment() && !file->xiphComment()->fieldListMap()["DISCNUMBER"].isEmpty()) {
      disc = TStringToQString(file->xiphComment()->fieldListMap()["DISCNUMBER"].front()).trimmed();
    }
  }

  if(!disc.isEmpty()) {
    int pos = disc.indexOf(QLatin1Char('/'));
    int n;
    bool ok;
    if(pos == -1) {
      n = disc.toInt(&ok);
    } else {
      n = disc.left(pos).toInt(&ok);
    }
    if(ok && n > 0) {
      num = n;
    }
  }
#else
  Q_UNUSED(ref_);
#endif
  return num;
}

#include "audiofileimporter.moc"
