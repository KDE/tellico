/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
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
#include "../filehandler.h"

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
#include <qdatetime.h>

using Tellico::Export::PilotDBExporter;

PilotDBExporter::PilotDBExporter() : Tellico::Export::Exporter(),
      m_backup(true),
      m_widget(0),
      m_checkBackup(0) {
}

QString PilotDBExporter::formatString() const {
  return i18n("PilotDB");
}

QString PilotDBExporter::fileFilter() const {
  return i18n("*.pdb|Pilot Database Files (*.pdb)") + QChar('\n') + i18n("*|All Files");
}

bool PilotDBExporter::exec() {
  Data::CollPtr coll = collection();
  if(!coll) {
    return false;
  }

  // This is something of a hidden preference cause I don't want to put it in the GUI right now
  // Latin1 by default
  QTextCodec* codec = 0;
  {
    // Latin1 is default
    KConfigGroupSaver group(KGlobal::config(), QString::fromLatin1("ExportOptions - %1").arg(formatString()));
    codec = KGlobal::charsets()->codecForName(KGlobal::config()->readEntry("Charset"));
  }
  if(!codec) {
    kdWarning() << "PilotDBExporter::exec() - no QTextCodec!" << endl;
    return false;
#ifndef NDEBUG
  } else {
    kdDebug() << "PilotDBExporter::exec() - encoding with " << codec->name() << endl;
#endif
  }

  // DB 0.3.x format
  PalmLib::FlatFile::DB db;

  // set database title
  db.title(codec->fromUnicode(coll->title()).data());

  // set backup option
//  db.setOption("backup", (m_checkBackup && m_checkBackup->isChecked()) ? "true" : "false");

  // all fields are added
  // except that only one field of type NOTE
  bool hasNote = false;
  Data::FieldVec outputFields; // not all fields will be output
  Data::FieldVec fields = coll->fields();
  for(Data::FieldVec::Iterator fIt = fields.begin(); fIt != fields.end(); ++fIt) {
    switch(fIt->type()) {
      case Data::Field::Choice:
        // the charSeparator is actually defined in DB.h
        db.appendField(codec->fromUnicode(fIt->title()).data(), PalmLib::FlatFile::Field::LIST,
                       codec->fromUnicode(fIt->allowed().join(QChar('/'))).data());
        outputFields.append(fIt);
        break;

      case Data::Field::Number:
        // the DB only supports single values of integers
        if(fIt->flags() & Data::Field::AllowMultiple) {
          db.appendField(codec->fromUnicode(fIt->title()).data(), PalmLib::FlatFile::Field::STRING);
        } else {
          db.appendField(codec->fromUnicode(fIt->title()).data(), PalmLib::FlatFile::Field::INTEGER);
        }
        outputFields.append(fIt);
        break;

      case Data::Field::Bool:
        db.appendField(codec->fromUnicode(fIt->title()).data(), PalmLib::FlatFile::Field::BOOLEAN);
        outputFields.append(fIt);
        break;

      case Data::Field::Para:
        if(hasNote) { // only one is allowed, according to palm-db-tools documentation
          kdDebug() << "PilotDBExporter::data() - adding note as string" << endl;
          db.appendField(codec->fromUnicode(fIt->title()).data(), PalmLib::FlatFile::Field::STRING);
        } else {
          kdDebug() << "PilotDBExporter::data() - adding note" << endl;
          db.appendField(codec->fromUnicode(fIt->title()).data(), PalmLib::FlatFile::Field::NOTE);
          hasNote = true;
        }
        outputFields.append(fIt);
        break;

      case Data::Field::Date:
        db.appendField(codec->fromUnicode(fIt->title()).data(), PalmLib::FlatFile::Field::DATE);
        outputFields.append(fIt);
        break;

      case Data::Field::Image:
        // don't include images
        kdDebug() << "PilotDBExporter::data() - skipping " << fIt->title() << " image field" << endl;
        break;

      default:
        db.appendField(codec->fromUnicode(fIt->title()).data(), PalmLib::FlatFile::Field::STRING);
        outputFields.append(fIt);
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
  }
  db.doneWithSchema();

  Data::FieldVec::ConstIterator fIt, end = outputFields.constEnd();
  bool format = options() & Export::ExportFormatted;

  QRegExp br(QString::fromLatin1("<br/?>"), false /*case-sensitive*/);
  QRegExp tags(QString::fromLatin1("<.*>"));
  tags.setMinimal(true);

  QString value;
  for(Data::EntryVec::ConstIterator entryIt = entries().begin(); entryIt != entries().end(); ++entryIt) {
    PalmLib::FlatFile::Record record;
    unsigned i = 0;
    for(fIt = outputFields.constBegin(); fIt != end; ++fIt, ++i) {
      value = entryIt->field(fIt->name(), format);

      if(fIt->type() == Data::Field::Date) {
        QStringList s = QStringList::split('-', value, true);
        bool ok = true;
        int y = s.count() > 0 ? s[0].toInt(&ok) : QDate::currentDate().year();
        if(!ok) {
          y = QDate::currentDate().year();
        }
        int m = s.count() > 1 ? s[1].toInt(&ok) : 1;
        if(!ok) {
          m = 1;
        }
        int d = s.count() > 2 ? s[2].toInt(&ok) : 1;
        if(!ok) {
          d = 1;
        }
        QDate date(y, m, d);
        value = date.toString(QString::fromLatin1("yyyy/MM/dd"));
      } else if(fIt->type() == Data::Field::Para) {
        value.replace(br, QChar('\n'));
        value.replace(tags, QString::null);
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

  return FileHandler::writeDataURL(url(), pdb.data(), options() & Export::ExportForce);
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

#include "pilotdbexporter.moc"
