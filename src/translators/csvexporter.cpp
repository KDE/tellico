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

#include "csvexporter.h"
#include "../document.h"
#include "../collection.h"
#include "../filehandler.h"

#include <klocale.h>
#include <kdebug.h>
#include <klineedit.h>
#include <kconfig.h>

#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qwhatsthis.h>

using Tellico::Export::CSVExporter;

CSVExporter::CSVExporter() : Tellico::Export::Exporter(),
    m_includeTitles(true),
    m_delimiter(QChar(',')),
    m_widget(0) {
}

QString CSVExporter::formatString() const {
  return i18n("CSV");
}

QString CSVExporter::fileFilter() const {
  return i18n("*.csv|CSV Files (*.csv)") + QChar('\n') + i18n("*|All Files");
}

QString& CSVExporter::escapeText(QString& text_) {
  bool quotes = false;
  if(text_.find('"') != -1) {
    quotes = true;
    // quotation marks will be escaped by using a double pair
    text_.replace('"', QString::fromLatin1("\"\""));
  }
  // if the text contains quotes or the delimiter, it needs to be surrounded by quotes
  if(quotes || text_.find(m_delimiter) != -1) {
    text_.prepend('"');
    text_.append('"');
  }
  return text_;
}

bool CSVExporter::exec() {
  if(!collection()) {
    return false;
  }

  QString text;

  Data::FieldVec fields = collection()->fields();
  Data::FieldVec::Iterator fIt;

  if(m_includeTitles) {
    for(fIt = fields.begin(); fIt != fields.end(); ++fIt) {
      QString title = fIt->title();
      text += escapeText(title);
      if(!fIt.nextEnd()) {
        text += m_delimiter;
      }
    }
    text += '\n';
  }

  bool format = options() & Export::ExportFormatted;

  QString tmp;
  for(Data::EntryVec::ConstIterator entryIt = entries().begin(); entryIt != entries().end(); ++entryIt) {
    for(fIt = fields.begin(); fIt != fields.end(); ++fIt) {
      tmp = entryIt->field(fIt->name(), format);
      text += escapeText(tmp);
      if(!fIt.nextEnd()) {
        text += m_delimiter;
      }
    }
    fIt = fields.begin();
    text += '\n';
  }

  return FileHandler::writeTextURL(url(), text, options() & ExportUTF8, options() & Export::ExportForce);
}

QWidget* CSVExporter::widget(QWidget* parent_, const char* name_/*=0*/) {
  if(m_widget && m_widget->parent() == parent_) {
    return m_widget;
  }

  m_widget = new QWidget(parent_, name_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* box = new QGroupBox(1, Qt::Horizontal, i18n("CSV Options"), m_widget);
  l->addWidget(box);

  m_checkIncludeTitles = new QCheckBox(i18n("Include field titles as column headers"), box);
  m_checkIncludeTitles->setChecked(m_includeTitles);
  QWhatsThis::add(m_checkIncludeTitles, i18n("If checked, a header row will be added with the "
                                             "field titles."));

  QButtonGroup* delimiterGroup = new QButtonGroup(0, Qt::Vertical, i18n("Delimiter"), box);
  QGridLayout* m_delimiterGroupLayout = new QGridLayout(delimiterGroup->layout());
  m_delimiterGroupLayout->setAlignment(Qt::AlignTop);
  QWhatsThis::add(delimiterGroup, i18n("In addition to a comma, other characters may be used as "
                                       "a delimiter, separating each value in the file."));

  m_radioComma = new QRadioButton(delimiterGroup);
  m_radioComma->setText(i18n("Comma"));
  m_radioComma->setChecked(true);
  QWhatsThis::add(m_radioComma, i18n("Use a comma as the delimiter."));
  m_delimiterGroupLayout->addWidget(m_radioComma, 0, 0);

  m_radioSemicolon = new QRadioButton( delimiterGroup);
  m_radioSemicolon->setText(i18n("Semicolon"));
  QWhatsThis::add(m_radioSemicolon, i18n("Use a semi-colon as the delimiter."));
  m_delimiterGroupLayout->addWidget(m_radioSemicolon, 0, 1);

  m_radioTab = new QRadioButton(delimiterGroup);
  m_radioTab->setText(i18n("Tab"));
  QWhatsThis::add(m_radioTab, i18n("Use a tab as the delimiter."));
  m_delimiterGroupLayout->addWidget(m_radioTab, 1, 0);

  m_radioOther = new QRadioButton(delimiterGroup);
  m_radioOther->setText(i18n("Other"));
  QWhatsThis::add(m_radioOther, i18n("Use a custom string as the delimiter."));
  m_delimiterGroupLayout->addWidget(m_radioOther, 1, 1);

  m_editOther = new KLineEdit(delimiterGroup);
  m_editOther->setEnabled(m_radioOther->isChecked());
  QWhatsThis::add(m_editOther, i18n("A custom string, such as a colon, may be used as a delimiter."));
  m_delimiterGroupLayout->addWidget(m_editOther, 1, 2);
  QObject::connect(m_radioOther, SIGNAL(toggled(bool)),
                   m_editOther, SLOT(setEnabled(bool)));

  if(m_delimiter == ',') {
    m_radioComma->setChecked(true);
  } else if(m_delimiter == ';') {
    m_radioSemicolon->setChecked(true);
  } else if(m_delimiter == '\t') {
    m_radioTab->setChecked(true);
  } else if(!m_delimiter.isEmpty()) {
    m_radioOther->setChecked(true);
    m_editOther->setEnabled(true);
    m_editOther->setText(m_delimiter);
  }

  l->addStretch(1);
  return m_widget;
}

void CSVExporter::readOptions(KConfig* config_) {
  KConfigGroupSaver group(config_, QString::fromLatin1("ExportOptions - %1").arg(formatString()));
  m_includeTitles = config_->readBoolEntry("Include Titles", m_includeTitles);
  m_delimiter = config_->readEntry("Delimiter", m_delimiter);
}

void CSVExporter::saveOptions(KConfig* config_) {
  m_includeTitles = m_checkIncludeTitles->isChecked();
  if(m_radioComma->isChecked()) {
    m_delimiter = ',';
  } else if(m_radioSemicolon->isChecked()) {
    m_delimiter = ';';
  } else if(m_radioTab->isChecked()) {
    m_delimiter = '\t';
  } else {
    m_delimiter = m_editOther->text();
  }

  KConfigGroupSaver group(config_, QString::fromLatin1("ExportOptions - %1").arg(formatString()));
  config_->writeEntry("Include Titles", m_includeTitles);
  config_->writeEntry("Delimiter", m_delimiter);
}

#include "csvexporter.moc"
