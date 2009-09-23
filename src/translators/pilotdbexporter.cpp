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

#include "pilotdbexporter.h"
#include "pilotdb/pilotdb.h"
#include "pilotdb/libflatfile/DB.h"

#include "../collection.h"
#include "../core/filehandler.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <KConfigGroup>
#include <kglobal.h>
#include <kcharsets.h>

#include <QGroupBox>
#include <QCheckBox>
#include <QTextCodec>
#include <QDateTime>
#include <QVBoxLayout>

using Tellico::Export::PilotDBExporter;

PilotDBExporter::PilotDBExporter(Data::CollPtr coll_) : Tellico::Export::Exporter(coll_),
      m_backup(true),
      m_widget(0),
      m_checkBackup(0) {
}

QString PilotDBExporter::formatString() const {
  return i18n("PilotDB");
}

QString PilotDBExporter::fileFilter() const {
  return i18n("*.pdb|Pilot Database Files (*.pdb)") + QLatin1Char('\n') + i18n("*|All Files");
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
    KConfigGroup group(KGlobal::config(), QString::fromLatin1("ExportOptions - %1").arg(formatString()));
    codec = KGlobal::charsets()->codecForName(group.readEntry("Charset"));
  }
  if(!codec) {
    myWarning() << "no QTextCodec!";
    return false;
#ifndef NDEBUG
  } else {
    myDebug() << "encoding with " << codec->name();
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
  Data::FieldList outputFields; // not all fields will be output
  foreach(Data::FieldPtr fIt, coll->fields()) {
    switch(fIt->type()) {
      case Data::Field::Choice:
        // the charSeparator is actually defined in DB.h
        db.appendField(codec->fromUnicode(fIt->title()).data(), PalmLib::FlatFile::Field::LIST,
                       codec->fromUnicode(fIt->allowed().join(QLatin1String("/"))).data());
        outputFields.append(fIt);
        break;

      case Data::Field::Number:
        // the DB only supports single values of integers
        if(fIt->hasFlag(Data::Field::AllowMultiple)) {
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
          myDebug() << "adding note as string";
          db.appendField(codec->fromUnicode(fIt->title()).data(), PalmLib::FlatFile::Field::STRING);
        } else {
          myDebug() << "adding note";
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
        myDebug() << "skipping " << fIt->title() << "image field";
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
    foreach(const QString& column, m_columns) {
      PalmLib::FlatFile::ListViewColumn col;
      col.field = coll->fieldTitles().indexOf(column);
      lv.push_back(col);
    }
    db.appendListView(lv);
  }
  db.doneWithSchema();

  Data::FieldList::ConstIterator fIt, end = outputFields.constEnd();
  bool format = options() & Export::ExportFormatted;

  QRegExp br(QLatin1String("<br/?>"));
  QRegExp tags(QLatin1String("<.*>"));
  tags.setMinimal(true);

  QString value;
  foreach(Data::EntryPtr entryIt, entries()) {
    PalmLib::FlatFile::Record record;
    unsigned i = 0;
    foreach(Data::FieldPtr fIt, outputFields) {
      value = entryIt->field(fIt->name(), format);

      if(fIt->type() == Data::Field::Date) {
        QStringList s = value.split(QLatin1Char('-'), QString::KeepEmptyParts);
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
        value = date.toString(QLatin1String("yyyy/MM/dd"));
      } else if(fIt->type() == Data::Field::Para) {
        value.replace(br, QLatin1String("\n"));
        value.remove(tags);
      }
      // the number of fields in the record must match the number of fields in the database
      record.appendField(PilotDB::string2field(db.field_type(i),
                         value.isEmpty() ? std::string() : codec->fromUnicode(value).data()));
      ++i;
    }
    // Add the record to the database.
    db.appendRecord(record);
  }

  PilotDB pdb;
  db.outputPDB(pdb);

  return FileHandler::writeDataURL(url(), pdb.data(), options() & Export::ExportForce);
}

QWidget* PilotDBExporter::widget(QWidget* parent_) {
  if(m_widget) {
    return m_widget;
  }

  m_widget = new QWidget(parent_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* gbox = new QGroupBox(i18n("PilotDB Options"), m_widget);
  QVBoxLayout* vlay = new QVBoxLayout(gbox);

  m_checkBackup = new QCheckBox(i18n("Set PDA backup flag for database"), gbox);
  m_checkBackup->setChecked(m_backup);
  m_checkBackup->setWhatsThis(i18n("Set PDA backup flag for database"));

  vlay->addWidget(m_checkBackup);

  l->addWidget(gbox);
  l->addStretch(1);
  return m_widget;
}

void PilotDBExporter::readOptions(KSharedConfigPtr config_) {
  KConfigGroup group(config_, QString::fromLatin1("ExportOptions - %1").arg(formatString()));
  m_backup = group.readEntry("Backup", m_backup);
}

void PilotDBExporter::saveOptions(KSharedConfigPtr config_) {
  KConfigGroup group(config_, QString::fromLatin1("ExportOptions - %1").arg(formatString()));
  m_backup = m_checkBackup->isChecked();
  group.writeEntry("Backup", m_backup);
}

#include "pilotdbexporter.moc"
