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

#include "csvexporter.h"
#include "../document.h"
#include "../collection.h"
#include "../core/filehandler.h"

#include <klocale.h>
#include <kdebug.h>
#include <klineedit.h>
#include <kconfig.h>
#include <KConfigGroup>

#include <QGroupBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QGridLayout>
#include <QVBoxLayout>

using Tellico::Export::CSVExporter;

CSVExporter::CSVExporter(Data::CollPtr coll_) : Tellico::Export::Exporter(coll_),
    m_includeTitles(true),
    m_delimiter(QLatin1String(",")),
    m_widget(0) {
}

QString CSVExporter::formatString() const {
  return i18n("CSV");
}

QString CSVExporter::fileFilter() const {
  return i18n("*.csv|CSV Files (*.csv)") + QLatin1Char('\n') + i18n("*|All Files");
}

QString& CSVExporter::escapeText(QString& text_) {
  bool quotes = false;
  if(text_.indexOf(QLatin1Char('"')) != -1) {
    quotes = true;
    // quotation marks will be escaped by using a double pair
    text_.replace(QLatin1Char('"'), QLatin1String("\"\""));
  }
  // if the text contains quotes or the delimiter, it needs to be surrounded by quotes
  if(quotes || text_.indexOf(m_delimiter)!= -1) {
    text_.prepend(QLatin1Char('"'));
    text_.append(QLatin1Char('"'));
  }
  return text_;
}

bool CSVExporter::exec() {
  if(!collection()) {
    return false;
  }

  QString text;

  Data::FieldList fields = collection()->fields();

  if(m_includeTitles) {
    foreach(Data::FieldPtr fIt, fields) {
      QString title = fIt->title();
      text += escapeText(title) + m_delimiter;
    }
    // remove last delimiter
    text.truncate(text.length() - m_delimiter.length());
    text += QLatin1Char('\n');
  }

  FieldFormat::Request format = (options() & Export::ExportFormatted ?
                                                FieldFormat::ForceFormat :
                                                FieldFormat::AsIsFormat);

  QString tmp;
  Data::EntryList entries = this->entries();
  foreach(Data::EntryPtr entryIt, entries) {
    foreach(Data::FieldPtr fIt, fields) {
      tmp = entryIt->formattedField(fIt->name(), format);
      text += escapeText(tmp) + m_delimiter;
    }
    // remove last delimiter
    text.truncate(text.length() - m_delimiter.length());
    text += QLatin1Char('\n');
  }

  return FileHandler::writeTextURL(url(), text, options() & ExportUTF8, options() & Export::ExportForce);
}

QWidget* CSVExporter::widget(QWidget* parent_) {
  if(m_widget && m_widget->parent() == parent_) {
    return m_widget;
  }

  m_widget = new QWidget(parent_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* gbox = new QGroupBox(i18n("CSV Options"), m_widget);
  QVBoxLayout* vlay = new QVBoxLayout(gbox);

  m_checkIncludeTitles = new QCheckBox(i18n("Include field titles as column headers"), gbox);
  m_checkIncludeTitles->setChecked(m_includeTitles);
  m_checkIncludeTitles->setWhatsThis(i18n("If checked, a header row will be added with the "
                                          "field titles."));

  QGroupBox* delimiterGroup = new QGroupBox(i18n("Delimiter"), gbox);
  QGridLayout* m_delimiterGroupLayout = new QGridLayout(delimiterGroup);
  m_delimiterGroupLayout->setAlignment(Qt::AlignTop);
  delimiterGroup->setWhatsThis(i18n("In addition to a comma, other characters may be used as "
                                    "a delimiter, separating each value in the file."));

  m_radioComma = new QRadioButton(delimiterGroup);
  m_radioComma->setText(i18n("Comma"));
  m_radioComma->setChecked(true);
  m_radioComma->setWhatsThis(i18n("Use a comma as the delimiter."));
  m_delimiterGroupLayout->addWidget(m_radioComma, 0, 0);

  m_radioSemicolon = new QRadioButton( delimiterGroup);
  m_radioSemicolon->setText(i18n("Semicolon"));
  m_radioSemicolon->setWhatsThis(i18n("Use a semi-colon as the delimiter."));
  m_delimiterGroupLayout->addWidget(m_radioSemicolon, 0, 1);

  m_radioTab = new QRadioButton(delimiterGroup);
  m_radioTab->setText(i18n("Tab"));
  m_radioTab->setWhatsThis(i18n("Use a tab as the delimiter."));
  m_delimiterGroupLayout->addWidget(m_radioTab, 1, 0);

  m_radioOther = new QRadioButton(delimiterGroup);
  m_radioOther->setText(i18n("Other"));
  m_radioOther->setWhatsThis(i18n("Use a custom string as the delimiter."));
  m_delimiterGroupLayout->addWidget(m_radioOther, 1, 1);

  m_editOther = new KLineEdit(delimiterGroup);
  m_editOther->setEnabled(m_radioOther->isChecked());
  m_editOther->setWhatsThis(i18n("A custom string, such as a colon, may be used as a delimiter."));
  m_delimiterGroupLayout->addWidget(m_editOther, 1, 2);
  QObject::connect(m_radioOther, SIGNAL(toggled(bool)),
                   m_editOther, SLOT(setEnabled(bool)));

  if(m_delimiter == QLatin1String(",")) {
    m_radioComma->setChecked(true);
  } else if(m_delimiter == QLatin1String(";")) {
    m_radioSemicolon->setChecked(true);
  } else if(m_delimiter == QLatin1String("\t")) {
    m_radioTab->setChecked(true);
  } else if(!m_delimiter.isEmpty()) {
    m_radioOther->setChecked(true);
    m_editOther->setEnabled(true);
    m_editOther->setText(m_delimiter);
  }

  vlay->addWidget(m_checkIncludeTitles);
  vlay->addWidget(delimiterGroup);

  l->addWidget(gbox);
  l->addStretch(1);
  return m_widget;
}

void CSVExporter::readOptions(KSharedConfigPtr config_) {
  KConfigGroup group(config_, QString::fromLatin1("ExportOptions - %1").arg(formatString()));
  m_includeTitles = group.readEntry("Include Titles", m_includeTitles);
  m_delimiter = group.readEntry("Delimiter", m_delimiter);
}

void CSVExporter::saveOptions(KSharedConfigPtr config_) {
  m_includeTitles = m_checkIncludeTitles->isChecked();
  if(m_radioComma->isChecked()) {
    m_delimiter = QLatin1Char(',');
  } else if(m_radioSemicolon->isChecked()) {
    m_delimiter = QLatin1Char(';');
  } else if(m_radioTab->isChecked()) {
    m_delimiter = QLatin1Char('\t');
  } else {
    m_delimiter = m_editOther->text();
  }

  KConfigGroup group(config_, QString::fromLatin1("ExportOptions - %1").arg(formatString()));
  group.writeEntry("Include Titles", m_includeTitles);
  group.writeEntry("Delimiter", m_delimiter);
}

#include "csvexporter.moc"
