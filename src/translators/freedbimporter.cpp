/***************************************************************************
    copyright            : (C) 2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "freedbimporter.h"
#include "../collections/musiccollection.h"
#include "../latin1literal.h"

#if HAVE_KCDDB
#undef QT_NO_CAST_ASCII
#include <libkcddb/client.h>
#endif

#include <klocale.h>
#include <kdebug.h>
#include <kcombobox.h>
#include <kglobal.h>
#include <kconfig.h>

#include <qfile.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qwhatsthis.h>

using Tellico::Import::FreeDBImporter;

FreeDBImporter::FreeDBImporter() : Tellico::Import::Importer(), m_coll(0), m_widget(0) {
}

Tellico::Data::Collection* FreeDBImporter::collection() {
#if !HAVE_KCDDB
  return 0;
#else
  if(m_coll) {
    return m_coll;
  }

  QString drivePath = m_driveCombo->currentText();
  QCString drive = QFile::encodeName(drivePath);
  if(drivePath.isEmpty()) {
    setStatusMessage(i18n("<qt>Tellico was unable to access the CD-ROM device - <i>%1</i>.</qt>").arg(drivePath));
    kdDebug() << "FreeDBImporter::collection() - no drive!" << endl;
    return 0;
  }

  // now it's ok to add device to saved list
  m_driveCombo->insertItem(drivePath);
  KConfigGroup config(KGlobal::config(), QString::fromLatin1("ImportOptions - FreeDB"));
  QStringList drives;
  for(int i = 0; i < m_driveCombo->count(); ++i) {
    if(drives.findIndex(m_driveCombo->text(i)) == -1) {
      drives += m_driveCombo->text(i);
    }
  }
  config.writeEntry("CD-ROM Devices", drives);
  config.writeEntry("Last Device", drivePath);

  KCDDB::TrackOffsetList list;
#if 0
  // a1107d0a - Kruder & Dorfmeister - The K&D Sessions - Disc One.
  list
    << 150      // First track start.
    << 29462
    << 66983
    << 96785
    << 135628
    << 168676
    << 194147
    << 222158
    << 247076
    << 278203   // Last track start.
    << 10       // Disc start.
    << 316732;  // Disc end.
/*
  list
    << 150
    << 106965
    << 127220
    << 151925
    << 176085
    << 5
    << 234500;
*/
#else
  list = offsetList(drive);
#endif

  if(list.isEmpty()) {
    setStatusMessage(i18n("<qt>Tellico was unable to access the CD-ROM device - <i>%1</i>.</qt>").arg(drivePath));
    return 0;
  }
//  kdDebug() << KCDDB::CDDB::trackOffsetListToId(list) << endl;
//  for(KCDDB::TrackOffsetList::iterator it = list.begin(); it != list.end(); ++it) {
//    kdDebug() << *it << endl;
//  }

// crashes if I use the Config class
//  KCDDB::Config c;
//  config.setHostname(QString::fromLatin1("freedb.freedb.org"));
//  config.setPort(8880);

//  KCDDB::Client client(c);
  KCDDB::Client client;
  client.setBlockingMode(true);
  KCDDB::CDDB::Result r = client.lookup(list);
  if(r != KCDDB::CDDB::Success) {
    kdDebug() << "FreeDBImporter::collection() - no success! Return value = " << r << endl;
    switch(r) {
      case KCDDB::CDDB::NoRecordFound:
        setStatusMessage(i18n("<qt>No records were found to match the CD.</qt>"));
        break;

      case KCDDB::CDDB::MultipleRecordFound:
        setStatusMessage(i18n("<qt>Multiple records were found to match the CD.</qt>"));
        break;

      case KCDDB::CDDB::ServerError:
        kdDebug() << "Server Error" << endl;
      case KCDDB::CDDB::HostNotFound:
        kdDebug() << "Host Not Found" << endl;
      case KCDDB::CDDB::NoResponse:
        kdDebug() << "No Repsonse" << endl;
      case KCDDB::CDDB::UnknownError:
        kdDebug() << "Unknown Error" << endl;
      default:
        setStatusMessage(i18n("<qt>Tellico was unable to complete the CD lookup.</qt>"));
        break;

    }
    return 0;
  }

/*
  KCDDB::CDInfoList response = client.lookupResponse();
  kdDebug() << "Client::lookup returned : " << response.count() << " entries" << endl;

  for(KCDDB::CDInfoList::ConstIterator it = response.begin(); it != response.end(); ++it) {
    KCDDB::CDInfo info(*it);
    kdDebug() << "Disc title: " << info.title << endl;
    kdDebug() << "Total tracks: " << info.trackInfoList.count() << endl;
    kdDebug() << "Disc revision: '" << info.revision << "'" << endl;
  }
*/
  KCDDB::CDInfo info = client.bestLookupResponse();
//  kdDebug() << "Best CDInfo had title: " << info.title << endl;

  m_coll = new Data::MusicCollection(true);

  Data::Entry* entry = new Data::Entry(m_coll);
  // obviously a CD
  entry->setField(QString::fromLatin1("medium"), i18n("Compact Disc"));
  entry->setField(QString::fromLatin1("title"), info.title);
  entry->setField(QString::fromLatin1("artist"), info.artist);
  entry->setField(QString::fromLatin1("genre"), info.genre);
  if(info.year > 0) {
    entry->setField(QString::fromLatin1("year"), QString::number(info.year));
  }

  QStringList trackList;
  KCDDB::TrackInfoList t = info.trackInfoList;
  for(uint i = 0; i < t.count(); ++i) {
    trackList << t[i].title;
  }
  entry->setField(QString::fromLatin1("track"), trackList.join(QString::fromLatin1("; ")));

  m_coll->addEntry(entry);

  return m_coll;
#endif
}

QWidget* FreeDBImporter::widget(QWidget* parent_, const char* name_/*=0*/) {
  m_widget = new QWidget(parent_, name_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* box = new QGroupBox(2, Qt::Horizontal, i18n("Audio CD Options"), m_widget);

  (void) new QLabel(i18n("CD-ROM Device:"), box);
  m_driveCombo = new KComboBox(true, box);
  m_driveCombo->setDuplicatesEnabled(false);
  QWhatsThis::add(m_driveCombo, i18n("Select or input the CD-ROM device location."));

  KConfigGroup config(KGlobal::config(), QString::fromLatin1("ImportOptions - FreeDB"));
  QStringList devices = config.readListEntry("CD-ROM Devices");
  if(devices.isEmpty()) {
    devices += QString::fromLatin1("/dev/cdrom");
  }
  m_driveCombo->insertStringList(devices);
  QString device = config.readEntry("Last Device");
  if(!device.isEmpty()) {
    m_driveCombo->setCurrentText(device);
  }

  l->addWidget(box);
  l->addStretch(1);
  return m_widget;
}

#include "freedbimporter.moc"
