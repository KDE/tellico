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

#include "pilotdbexporter.h"
#include "pilotdb/pilotdb.h"
#include "pilotdb/libflatfile/DB.h"

#include "../collection.h"

#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kcharsets.h>

#include <qlayout.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>
#include <qtextcodec.h>

using Tellico::Export::PilotDBExporter;

PilotDBExporter::PilotDBExporter(const Data::Collection* coll_) : Tellico::Export::DataExporter(coll_),
      m_backup(true),
      m_widget(0),
      m_checkBackup(0) {
}

QString PilotDBExporter::formatString() const {
  return i18n("PilotDB");
}

QString PilotDBExporter::fileFilter() const {
  return i18n("*.pdb|Pilot Database files(*.pdb)") + QChar('\n') + i18n("*|All files");
}

QByteArray PilotDBExporter::data(bool formatFields_) {
  const Data::Collection* coll = collection();

  // This is something of a hidden preference cause I don't want to put it in the GUI right now
  // Latin1 by default
  QTextCodec* codec = 0;
  {
    // Latin1 is default
    KConfigGroupSaver group(KGlobal::config(), QString::fromLatin1("ExportOptions - %1").arg(formatString()));
    codec = KGlobal::charsets()->codecForName(KGlobal::config()->readEntry("Charset"));
  }
#ifndef NDEBUG
  if(!codec) {
    kdWarning() << "PilotDBExporter::data() - no QTextCodec!" << endl;
    return QByteArray();
  } else {
    kdDebug() << "PilotDBExporter::data() - encoding with " << codec->name() << endl;
  }
#endif

  // DB 0.3.x format
  PalmLib::FlatFile::DB db;

  // set database title
  db.title(codec->fromUnicode(coll->title()).data());

  // set backup option
  db.setOption("backup", (m_checkBackup && m_checkBackup->isChecked()) ? "true" : "false");

  // all fields are added
  // except that only one field of type NOTE
  bool hasNote = false;
  Data::FieldList outputFields; // not all fields will be output
  for(Data::FieldListIterator fIt(coll->fieldList()); fIt.current(); ++fIt) {
    switch(fIt.current()->type()) {
      case Data::Field::Choice:
        // the charSeparator is actually defined in DB.h
        db.appendField(codec->fromUnicode(fIt.current()->title()).data(), PalmLib::FlatFile::Field::LIST,
                       codec->fromUnicode(fIt.current()->allowed().join(QChar('/'))).data());
        outputFields.append(fIt.current());
        break;

      case Data::Field::Number:
        // the DB only supports single values of integers
        if(fIt.current()->flags() & Data::Field::AllowMultiple) {
          db.appendField(codec->fromUnicode(fIt.current()->title()).data(), PalmLib::FlatFile::Field::STRING);
        } else {
          db.appendField(codec->fromUnicode(fIt.current()->title()).data(), PalmLib::FlatFile::Field::INTEGER);
        }
        outputFields.append(fIt.current());
        break;

      case Data::Field::Bool:
        db.appendField(codec->fromUnicode(fIt.current()->title()).data(), PalmLib::FlatFile::Field::BOOLEAN);
        outputFields.append(fIt.current());
        break;

      case Data::Field::Para:
        if(hasNote) { // only one is allowed, according to palm-db-tools documentation
          kdDebug() << "adding note as string" << endl;
          db.appendField(codec->fromUnicode(fIt.current()->title()).data(), PalmLib::FlatFile::Field::STRING);
        } else {
          kdDebug() << "adding note" << endl;
          db.appendField(codec->fromUnicode(fIt.current()->title()).data(), PalmLib::FlatFile::Field::NOTE);
          hasNote = true;
        }
        outputFields.append(fIt.current());
        break;

      case Data::Field::Image:
        // don't include images
        kdDebug() << "PilotDBExporter::data() - skipping " << fIt.current()->title() << " image field" << endl;
        break;

      default:
        db.appendField(codec->fromUnicode(fIt.current()->title()).data(), PalmLib::FlatFile::Field::STRING);
        outputFields.append(fIt.current());
        break;
    }
  }

  // add view with visible fields
  if(m_columns.count() > 0) {
    PalmLib::FlatFile::ListView lv;
    lv.name = codec->fromUnicode(i18n("View Columns")).data();
    for(QStringList::ConstIterator it = m_columns.begin(); it != m_columns.end(); ++it) {
      PalmLib::FlatFile::ListViewColumn col;
      col.field = coll->fieldTitles().findIndex(*it);
      lv.push_back(col);
    }
    db.appendListView(lv);
//  } else {
//    // add view with all fields
//    db.appendListView(PalmLib::FlatFile::ListView());
  }

  QString value;
  for(Data::EntryListIterator entryIt(entryList()); entryIt.current(); ++entryIt) {
    PalmLib::FlatFile::Record record;
    unsigned i = 0;
    for(Data::FieldListIterator fIt(outputFields); fIt.current(); ++fIt, ++i) {
      if(formatFields_) {
        value = entryIt.current()->formattedField(fIt.current()->name());
      } else {
        value = entryIt.current()->field(fIt.current()->name());
      }
      // the number of fields in the record must match the number of fields in the database
      record.appendField(PilotDB::string2field(db.field_type(i),
                         value.isEmpty() ? std::string() : codec->fromUnicode(value).data()));
    }
    // Add the record to the database.
    db.appendRecord(record);
  }

  PilotDB pdb;
  db.outputPDB(pdb);

  return pdb.data();
}

QWidget* PilotDBExporter::widget(QWidget* parent_, const char* name_/*=0*/) {
  if(m_widget && m_widget->parent() == parent_) {
    return m_widget;
  }

  m_widget = new QWidget(parent_, name_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* box = new QGroupBox(1, Qt::Horizontal, i18n("PilotDB Options"), m_widget);
  l->addWidget(box);

  m_checkBackup = new QCheckBox(i18n("Set PDA backup flag for database"), box);
  m_checkBackup->setChecked(m_backup);
  QWhatsThis::add(m_checkBackup, i18n("Set PDA backup flag for database"));

  l->addStretch(1);
  return m_widget;
}

void PilotDBExporter::readOptions(KConfig* config_) {
  KConfigGroupSaver group(config_, QString::fromLatin1("ExportOptions - %1").arg(formatString()));
  m_backup = config_->readBoolEntry("Backup", m_backup);
}

void PilotDBExporter::saveOptions(KConfig* config_) {
  KConfigGroupSaver group(config_, QString::fromLatin1("ExportOptions - %1").arg(formatString()));
  m_backup = m_checkBackup->isChecked();
  config_->writeEntry("Backup", m_backup);
}
