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

#include "csvimporter.h"
#include "../collectionfieldsdialog.h"

#include <kdebug.h>
#include <klocale.h>
#include <klineedit.h>
#include <kcombobox.h>
#include <knuminput.h>
#include <kpushbutton.h>

#include <qgroupbox.h>
#include <qlayout.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qwhatsthis.h>
#include <qtable.h>
#include <qvaluevector.h>
#include <qregexp.h>

using Bookcase::Import::CSVImporter;

const QChar CSVImporter::s_quote('"');

CSVImporter::CSVImporter(const KURL& url_) : Bookcase::Import::TextImporter(url_),
    m_coll(0),
    m_firstRowHeader(false),
    m_delimiter(QString::fromLatin1(",")),
    m_widget(0) {
}

Bookcase::Data::Collection* CSVImporter::collection() {
  // don't just check if m_coll is non-null since the collection can be created elsewhere
  if(!m_coll->entryList().isEmpty()) {
    return m_coll;
  }

  if(!m_coll) {
    // iterate over the collection names until it matches the text of the combo box
    for(CollectionNameMap::Iterator it = m_nameMap.begin(); it != m_nameMap.end(); ++it) {
      if(it.data() == m_comboType->currentText()) {
        m_coll = CollectionFactory::collection(it.key(), true);
        break;
      }
    }
  }

  QString str = text();
  QTextIStream t(&str);

  QValueVector<int> cols;
  QStringList names;
  for(int col = 0; col < m_table->numCols(); ++col) {
    if(m_coll->fieldTitles().findIndex(m_table->horizontalHeader()->label(col)) > -1) {
      cols.push_back(col);
      names << m_coll->fieldNameByTitle(m_table->horizontalHeader()->label(col));
    }
  }

  // if the first row are headers, skip it
  if(m_firstRowHeader) {
    t.readLine();
  }

  int numLines = str.contains(QString::fromLatin1("\n"));
  int j = 0;
  for(QString line = t.readLine(); !line.isNull(); line = t.readLine(), ++j) {
    Data::Entry* unit = new Data::Entry(m_coll);
    QStringList values = splitLine(line);
    for(unsigned i = 0; i < cols.size(); ++i) {
      unit->setField(names[i], values[cols[i]].simplifyWhiteSpace());
    }
    m_coll->addEntry(unit);

    if(j%s_stepSize == 0) {
      emit signalFractionDone(static_cast<float>(j)/static_cast<float>(numLines));
    }
  }

  return m_coll;
}

QWidget* CSVImporter::widget(QWidget* parent_, const char* name_) {
  if(m_widget && m_widget->parent() == parent_) {
    return m_widget;
  }

  m_widget = new QWidget(parent_, name_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* group = new QGroupBox(1, Qt::Horizontal, i18n("CSV Options"), m_widget);
  l->addWidget(group);

  QHBox* box = new QHBox(group);
  box->setSpacing(5);
  (void) new QLabel(i18n("Data Type:"), box);
  m_comboType = new KComboBox(box);
  QWhatsThis::add(m_comboType, i18n("Select the type of collection being imported."));
  m_nameMap = CollectionFactory::nameMap();
  m_comboType->insertStringList(m_nameMap.values());
  connect(m_comboType, SIGNAL(activated(const QString&)), SLOT(slotTypeChanged(const QString&)));
  // need a spacer
  QWidget*w = new QWidget(box);
  box->setStretchFactor(w, 1);

  m_checkFirstRowHeader = new QCheckBox(i18n("First row contains field titles"), group);
  m_checkFirstRowHeader->setChecked(m_firstRowHeader);
  QWhatsThis::add(m_checkFirstRowHeader, i18n("If checked, the first row is used as field titles."));
  connect(m_checkFirstRowHeader, SIGNAL(toggled(bool)), SLOT(slotFirstRowHeader(bool)));

  m_delimiterGroup = new QButtonGroup(0, Qt::Vertical, i18n("Delimiter"), group);
  QGridLayout* m_delimiterGroupLayout = new QGridLayout(m_delimiterGroup->layout(), 3, 3);
  m_delimiterGroupLayout->setAlignment(Qt::AlignTop);
  QWhatsThis::add(m_delimiterGroup, i18n("In addition to a comma, other characters may be used as "
                                         "a delimiter, separating each value in the file."));
  connect(m_delimiterGroup, SIGNAL(clicked(int)), SLOT(slotDelimiter()));

  m_radioComma = new QRadioButton(m_delimiterGroup);
  m_radioComma->setText(i18n("Comma"));
  m_radioComma->setChecked(true);
  QWhatsThis::add(m_radioComma, i18n("Use a comma as the delimiter."));
  m_delimiterGroupLayout->addWidget(m_radioComma, 1, 0);

  m_radioSemicolon = new QRadioButton( m_delimiterGroup);
  m_radioSemicolon->setText(i18n("Semicolon"));
  QWhatsThis::add(m_radioSemicolon, i18n("Use a semi-colon as the delimiter."));
  m_delimiterGroupLayout->addWidget(m_radioSemicolon, 1, 1);

  m_radioTab = new QRadioButton(m_delimiterGroup);
  m_radioTab->setText(i18n("Tab"));
  QWhatsThis::add(m_radioTab, i18n("Use a tab as the delimiter."));
  m_delimiterGroupLayout->addWidget(m_radioTab, 2, 0);

  m_radioOther = new QRadioButton(m_delimiterGroup);
  m_radioOther->setText(i18n("Other"));
  QWhatsThis::add(m_radioOther, i18n("Use a custom string as the delimiter."));
  m_delimiterGroupLayout->addWidget(m_radioOther, 2, 1);

  m_editOther = new KLineEdit(m_delimiterGroup);
  m_editOther->setEnabled(m_radioOther->isChecked());
  QWhatsThis::add(m_editOther, i18n("A custom string, such as a colon, may be used as a delimiter."));
  m_delimiterGroupLayout->addWidget(m_editOther, 2, 2);
  connect(m_radioOther, SIGNAL(toggled(bool)),
          m_editOther, SLOT(setEnabled(bool)));
  connect(m_editOther, SIGNAL(textChanged(const QString&)), SLOT(slotDelimiter()));

  if(m_delimiter == QString::fromLatin1(",")) {
    m_radioComma->setChecked(true);
  } else if(m_delimiter == QString::fromLatin1(";")) {
    m_radioSemicolon->setChecked(true);
  } else if(m_delimiter == QString::fromLatin1("\t")) {
    m_radioTab->setChecked(true);
  } else {
    m_radioOther->setChecked(true);
    m_editOther->setEnabled(true);
    m_editOther->setText(m_delimiter);
  }

  m_table = new QTable(5, 0, group);
  m_table->setSelectionMode(QTable::Single);
  m_table->setFocusStyle(QTable::FollowStyle);
  m_table->setLeftMargin(0);
  m_table->verticalHeader()->hide();
  m_table->horizontalHeader()->setClickEnabled(true);
  m_table->setReadOnly(true);
  QWhatsThis::add(m_table, i18n("The table shows up to the first five lines of the CSV file."));
  connect(m_table, SIGNAL(currentChanged(int, int)), SLOT(slotCurrentChanged(int, int)));
  connect(m_table->horizontalHeader(), SIGNAL(clicked(int)), SLOT(slotHeaderClicked(int)));

  QHBox* hbox = new QHBox(group);
  hbox->setSpacing(5);
  QWhatsThis::add(hbox, i18n("<qt>Set each column to correspond to a field in the collection by choosing "
                             "a column, selecting the field, then clicking the <i>Set</i> button.</qt>"));
  (void) new QLabel(i18n("Column:"), hbox);
  m_colSpinBox = new KIntSpinBox(hbox);
  m_colSpinBox->setMinValue(1);
  connect(m_colSpinBox, SIGNAL(valueChanged(int)), SLOT(slotSelectColumn(int)));
  (void) new QLabel(i18n("Data field in this column:"), hbox);
  m_comboField = new KComboBox(hbox);
  connect(m_comboField, SIGNAL(activated(int)), SLOT(slotFieldChanged(int)));
  m_setColumnBtn = new KPushButton(i18n("Set"), hbox);
  connect(m_setColumnBtn, SIGNAL(clicked()), SLOT(slotSetColumnTitle()));

  slotTypeChanged(m_comboType->currentText());
  fillTable();

  l->addStretch(1);
  return m_widget;
}

// nominally, everything should be separated by the delimiter
// or quotes could surround the cell value if either the delimiter or quotes are contained
// actual quotes may or may not be doubled
QStringList CSVImporter::splitLine(const QString& line_) {
  // cache double quote
  static QString dq = QString(s_quote)+QString(s_quote);
  // delimiter length
  unsigned dn = m_delimiter.length();
  // list of eventual field values
  QStringList values;
  if(dn==0) {
    return values;
  }
  // temporary string to hold splits, and cumulative string for quoted fields
  QString tmp, str;
  // are we inside a quoted field or not
  bool inquote = false;
  // old position
  int oldpos = -1;
  for(int newpos = line_.find(m_delimiter); newpos != -1; newpos = line_.find(m_delimiter, oldpos+dn)) {
    // grab temporary field value
    tmp = line_.mid(oldpos+dn, newpos-oldpos-dn);

    // if not inside a quote, and the string starts with a quote but doesn't end with one
    if(!inquote && tmp.startsWith(s_quote) && !tmp.endsWith(s_quote)) {
      // skip first quote character and add delimiter
      str += tmp.mid(1) + m_delimiter;
      // now we're inside
      inquote = true;
      oldpos = newpos;
      continue;
    }

    // if inside a quoted field, and the new string doesn't end with a quote
    if(inquote && !tmp.endsWith(s_quote)) {
      //we're still in the field, so add the delimiter
      str += tmp + m_delimiter;
      oldpos = newpos;
      continue;
    }

    // finally, now we're in a quote, but the new string ends the quote
    if(inquote) {
      // remove final quote character
      tmp.truncate(tmp.length()-1);
      // no longer in quoted field
      inquote = false;
    }

    str += tmp;
    // special case for fields that start and end with quote
    if(str.startsWith(s_quote) && str.endsWith(s_quote)) {
      str = str.mid(1, tmp.length()-2);
    }

    // append the temporary field value to the cumulative
    // replace any double-quotes with single ones
    str.replace(dq, s_quote);
    // add the accumulated string to the list
    values << str;
    // clear the string now, since we've added it
    str.truncate(0);
    oldpos = newpos;
  }

  // get last word, too
  tmp = line_.mid(oldpos+dn);
  if(inquote && tmp.endsWith(s_quote)) {
    tmp.truncate(tmp.length()-1);
  }
  str += tmp;
  str.replace(dq, s_quote);
  values << str;
  return values;
}

void CSVImporter::fillTable() {
  QString str = text();
  QTextStream t(&str, IO_ReadOnly);

  // want first two lines
  for(int row = 0; row < m_table->numRows(); ++row) {
    QString line = t.readLine();
//    kdDebug() << "CSVImporter::fillTable() - line is\n" << line << endl;
    QStringList values = splitLine(line);
    if(static_cast<int>(values.count()) > m_table->numCols()) {
      m_table->setNumCols(values.count());
      m_colSpinBox->setMaxValue(values.count());
    }
    int col = 0;
    for(QStringList::Iterator it = values.begin(); it != values.end(); ++it) {
      m_table->setText(row, col, *it);
//      m_table->adjustColumn(col);
      ++col;
    }
    m_table->setNumCols(col);
//    for( ; col < m_table->numCols(); ++col) {
//      m_table->clearCell(row, col);
//    }
  }
}

void CSVImporter::slotTypeChanged(const QString& name_) {
  // iterate over the collection names until it matches the text of the combo box
  for(CollectionNameMap::Iterator it = m_nameMap.begin(); it != m_nameMap.end(); ++it) {
    if(it.data() == name_) {
    // delete the old collection
      delete m_coll;
      m_coll = CollectionFactory::collection(it.key(), true);
      break;
    }
  }

  updateHeader(true);
  m_comboField->clear();
  m_comboField->insertStringList(m_coll->fieldTitles());
  m_comboField->insertItem('<' + i18n("New Field") + '>');
}

void CSVImporter::slotFirstRowHeader(bool b_) {
  m_firstRowHeader = b_;
  updateHeader(false);
}

void CSVImporter::slotDelimiter() {
  if(m_radioComma->isChecked()) {
    m_delimiter = QString::fromLatin1(",");
  } else if(m_radioSemicolon->isChecked()) {
    m_delimiter = QString::fromLatin1(";");
  } else if(m_radioTab->isChecked()) {
    m_delimiter = QString::fromLatin1("\t");
  } else {
    m_delimiter = m_editOther->text();
  }
  if(!m_delimiter.isEmpty()) {
    fillTable();
    updateHeader(false);
  }
}

void CSVImporter::slotCurrentChanged(int, int col_) {
  int pos = col_+1;
  m_colSpinBox->setValue(pos); //slotSelectColumn() gets called because of the signal
}

void CSVImporter::slotHeaderClicked(int col_) {
  int pos = col_+1;
  m_colSpinBox->setValue(pos); //slotSelectColumn() gets called because of the signal
}

void CSVImporter::slotSelectColumn(int pos_) {
  // pos is really the number of the position of the column
  int col = pos_ - 1;
  m_table->ensureCellVisible(0, col);
  m_comboField->setCurrentItem(m_table->horizontalHeader()->label(col));
}

void CSVImporter::slotSetColumnTitle() {
  int col = m_colSpinBox->value()-1;
  QString title = m_comboField->currentText();
  m_table->horizontalHeader()->setLabel(col, title);
  // make sure none of the other columns have this title
  bool found = false;
  for(int i = 0; i < col; ++i) {
    if(m_table->horizontalHeader()->label(i) == title) {
      m_table->horizontalHeader()->setLabel(i, QString::number(i+1));
      found = true;
      break;
    }
  }
  // if found, then we're done
  if(found) {
    return;
  }
  for(int i = col+1; i < m_table->numCols(); ++i) {
    if(m_table->horizontalHeader()->label(i) == title) {
      m_table->horizontalHeader()->setLabel(i, QString::number(i+1));
      break;
    }
  }
}

void CSVImporter::updateHeader(bool force_) {
  if(m_firstRowHeader || force_) {
    for(int col = 0; col < m_table->numCols(); ++col) {
      if(m_firstRowHeader && m_firstRowHeader
         && m_coll->fieldByTitle(m_table->text(0, col)) != 0) {
        m_table->horizontalHeader()->setLabel(col, m_table->text(0, col));
      } else {
        m_table->horizontalHeader()->setLabel(col, QString::number(col+1));
      }
    }
  }
}

void CSVImporter::slotFieldChanged(int idx_) {
  // only care if it's not the last item
  if(idx_ < m_comboField->count()-1) {
    return;
  }
  CollectionFieldsDialog dlg(m_coll, m_widget);
//  dlg.setModal(true);
  if(dlg.exec() == QDialog::Accepted) {
    m_comboField->clear();
    m_comboField->insertStringList(m_coll->fieldTitles());
    m_comboField->insertItem('<' + i18n("New Field") + '>');
  }
  m_comboField->setCurrentItem(0);
}
