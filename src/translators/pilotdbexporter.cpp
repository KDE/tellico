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

#include <qlayout.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>

using Bookcase::Export::PilotDBExporter;

PilotDBExporter::PilotDBExporter(const Data::Collection* coll_, const Data::EntryList& list_)
    : Bookcase::Export::DataExporter(coll_, list_),
      m_backup(true),
      m_widget(0),
      m_checkBackup(0) {
}

QString PilotDBExporter::formatString() const {
  return i18n("PilotDB");
}

QString PilotDBExporter::fileFilter() const {
  return i18n("*.pdb|Pilot Database files(*.pdb)") + QString::fromLatin1("\n") + i18n("*|All files");
}

QByteArray PilotDBExporter::data(bool formatFields_) {
  const Data::Collection* coll = collection();

  // DB 0.3.x format
  PalmLib::FlatFile::DB* db = new PalmLib::FlatFile::DB();

  // set database title
  db->title(coll->title().ascii());

  // set backup option
  db->setOption("backup", (m_checkBackup && m_checkBackup->isChecked()) ? "true" : "false");

  // all fields are added
  // except that only one field of type NOTE
  bool hasNote = false;
  for(Data::FieldListIterator fIt(coll->fieldList()); fIt.current(); ++fIt) {
    switch(fIt.current()->type()) {
      case Data::Field::Choice:
        // the charSeparator is actually defined in DB.h
        db->appendField(fIt.current()->title().ascii(), PalmLib::FlatFile::Field::LIST,
                        fIt.current()->allowed().join(QChar('/')).ascii());
        break;
      case Data::Field::Number:
        // the DB only supports single values of integers
        if(fIt.current()->flags() & Data::Field::AllowMultiple) {
          db->appendField(fIt.current()->title().ascii(), PalmLib::FlatFile::Field::STRING);
        } else {
          db->appendField(fIt.current()->title().ascii(), PalmLib::FlatFile::Field::INTEGER);
        }
        break;
      case Data::Field::Bool:
        db->appendField(fIt.current()->title().ascii(), PalmLib::FlatFile::Field::BOOLEAN);
        break;
      case Data::Field::Para:
        if(hasNote) { // only one is allowed, according to palm-db-tools documentation
          db->appendField(fIt.current()->title().ascii(), PalmLib::FlatFile::Field::STRING);
        } else {
          db->appendField(fIt.current()->title().ascii(), PalmLib::FlatFile::Field::NOTE);
          hasNote = true;
        }
        break;
      default:
        db->appendField(fIt.current()->title().ascii(), PalmLib::FlatFile::Field::STRING);
        break;
    }
  }

  // add view with visible fields
  if(m_columns.count() > 0) {
    PalmLib::FlatFile::ListView lv;
    lv.name = i18n("View Columns").ascii();
    for(QStringList::ConstIterator it = m_columns.begin(); it != m_columns.end(); ++it) {
      PalmLib::FlatFile::ListViewColumn col;
      col.field = coll->fieldTitles().findIndex(*it);
      lv.push_back(col);
    }
    db->appendListView(lv);
  } else {
    // add view with all fields
    db->appendListView(PalmLib::FlatFile::ListView());
  }

  QString value;
  for(Data::EntryListIterator entryIt(entryList()); entryIt.current(); ++entryIt) {
    PalmLib::FlatFile::Record record;
    unsigned i = 0;
    for(Data::FieldListIterator fIt(coll->fieldList()); fIt.current(); ++fIt, ++i) {
      if(formatFields_) {
        value = entryIt.current()->formattedField(fIt.current()->name());
      } else {
        value = entryIt.current()->field(fIt.current()->name());
      }
      record.appendField(PilotDB::string2field(db->field_type(i), value.ascii()));
    }
    // Add the record to the database.
    db->appendRecord(record);
  }

  PilotDB pdb;
  db->outputPDB(pdb);

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
  config_->setGroup(QString::fromLatin1("ExportOptions - %1").arg(formatString()));
  m_backup = config_->readBoolEntry("Backup", m_backup);
}

void PilotDBExporter::saveOptions(KConfig* config_) {
  config_->setGroup(QString::fromLatin1("ExportOptions - %1").arg(formatString()));
  m_backup = m_checkBackup->isChecked();
  config_->writeEntry("Backup", m_backup);
}
