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

#include "csvimporter.h"
#include "translators.h" // needed for ImportAction
#include "../collectionfieldsdialog.h"
#include "../document.h"
#include "../collection.h"
#include "../progressmanager.h"
#include "../tellico_debug.h"
#include "../collectionfactory.h"
#include "../gui/collectiontypecombo.h"
#include "../latin1literal.h"
#include "../stringset.h"

#include <klineedit.h>
#include <kcombobox.h>
#include <knuminput.h>
#include <kpushbutton.h>
#include <kapplication.h>
#include <kiconloader.h>
#include <kconfig.h>

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

using Tellico::Import::CSVImporter;

const QChar CSVImporter::s_quote('"');

CSVImporter::CSVImporter(const KURL& url_) : Tellico::Import::TextImporter(url_),
    m_coll(0),
    m_existingCollection(0),
    m_firstRowHeader(false),
    m_delimiter(QString::fromLatin1(",")),
    m_cancelled(false),
    m_widget(0),
    m_table(0) {
}

Tellico::Data::CollPtr CSVImporter::collection() {
  // don't just check if m_coll is non-null since the collection can be created elsewhere
  if(m_coll && m_coll->entryCount() > 0) {
    return m_coll;
  }

  if(!m_coll) {
    m_coll = CollectionFactory::collection(m_comboColl->currentType(), true);
  }

  QString str = text();
  QTextIStream t(&str);

  const QStringList existingNames = m_coll->fieldNames();

  QValueVector<int> cols;
  QStringList names;
  for(int col = 0; col < m_table->numCols(); ++col) {
    QString t = m_table->horizontalHeader()->label(col);
    if(m_existingCollection && m_existingCollection->fieldByTitle(t)) {
      // the collection might have the right field, but a different title, say for translations
      Data::FieldPtr f = m_existingCollection->fieldByTitle(t);
      if(m_coll->hasField(f->name())) {
        // might have different values settings
        m_coll->removeField(f->name(), true /* force */);
      }
      m_coll->addField(new Data::Field(*f));
      cols.push_back(col);
      names << f->name();
    } else if(m_coll->fieldByTitle(t)) {
      cols.push_back(col);
      names << m_coll->fieldNameByTitle(t);
    }
  }

  // we don't want to add fields from the default collection
  // that are not in the imported data
  for(QStringList::ConstIterator it = existingNames.begin(); it != existingNames.end(); ++it) {
    if(names.findIndex(*it) == -1) {
      m_coll->removeField(*it);
    }
  }

  // if the first row are headers, skip it
  if(m_firstRowHeader) {
    t.readLine();
  }

  const uint numLines = str.contains('\n');
  const uint stepSize = QMAX(s_stepSize, numLines/100);

  ProgressItem& item = ProgressManager::self()->newProgressItem(this, progressLabel(), true);
  item.setTotalSteps(numLines);
  connect(&item, SIGNAL(signalCancelled(ProgressItem*)), SLOT(slotCancel()));
  ProgressItem::Done done(this);

  uint j = 0;
  for(QString line = t.readLine(); !m_cancelled && !line.isNull(); line = t.readLine(), ++j) {
    bool empty = true;
    Data::EntryPtr entry = new Data::Entry(m_coll);
    QStringList values = splitLine(line);
    for(uint i = 0; i < names.size(); ++i) {
      QString value = values[cols[i]].simplifyWhiteSpace();
      bool success = entry->setField(names[i], value);
      // we might need to add a new allowed value
      // assume that if the user is importing the value, it should be allowed
      if(!success && m_coll->fieldByName(names[i])->type() == Data::Field::Choice) {
        Data::FieldPtr f = m_coll->fieldByName(names[i]);
        StringSet allow;
        allow.add(f->allowed());
        allow.add(value);
        f->setAllowed(allow.toList());
        m_coll->modifyField(f);
        success = entry->setField(names[i], value);
      }
      if(empty && success) {
        empty = false;
      }
    }
    if(!empty) {
      m_coll->addEntries(entry);
    }

    if(j%stepSize == 0) {
      ProgressManager::self()->setProgress(this, j);
      kapp->processEvents();
    }
  }

  {
    KConfigGroup config(KGlobal::config(), QString::fromLatin1("ImportOptions - CSV"));
    config.writeEntry("Delimiter", m_delimiter);
    config.writeEntry("First Row Titles", m_firstRowHeader);
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
  QLabel* lab = new QLabel(i18n("Collection &type:"), box);
  m_comboColl = new GUI::CollectionTypeCombo(box);
  lab->setBuddy(m_comboColl);
  QWhatsThis::add(m_comboColl, i18n("Select the type of collection being imported."));
  connect(m_comboColl, SIGNAL(activated(const QString&)), SLOT(slotTypeChanged(const QString&)));
  // need a spacer
  QWidget* w = new QWidget(box);
  box->setStretchFactor(w, 1);

  m_checkFirstRowHeader = new QCheckBox(i18n("&First row contains field titles"), group);
  QWhatsThis::add(m_checkFirstRowHeader, i18n("If checked, the first row is used as field titles."));
  connect(m_checkFirstRowHeader, SIGNAL(toggled(bool)), SLOT(slotFirstRowHeader(bool)));

  QHBox* hbox2 = new QHBox(group);
  m_delimiterGroup = new QButtonGroup(0, Qt::Vertical, i18n("Delimiter"), hbox2);
  QGridLayout* m_delimiterGroupLayout = new QGridLayout(m_delimiterGroup->layout(), 3, 3);
  m_delimiterGroupLayout->setAlignment(Qt::AlignTop);
  QWhatsThis::add(m_delimiterGroup, i18n("In addition to a comma, other characters may be used as "
                                         "a delimiter, separating each value in the file."));
  connect(m_delimiterGroup, SIGNAL(clicked(int)), SLOT(slotDelimiter()));

  m_radioComma = new QRadioButton(m_delimiterGroup);
  m_radioComma->setText(i18n("&Comma"));
  m_radioComma->setChecked(true);
  QWhatsThis::add(m_radioComma, i18n("Use a comma as the delimiter."));
  m_delimiterGroupLayout->addWidget(m_radioComma, 1, 0);

  m_radioSemicolon = new QRadioButton( m_delimiterGroup);
  m_radioSemicolon->setText(i18n("&Semicolon"));
  QWhatsThis::add(m_radioSemicolon, i18n("Use a semi-colon as the delimiter."));
  m_delimiterGroupLayout->addWidget(m_radioSemicolon, 1, 1);

  m_radioTab = new QRadioButton(m_delimiterGroup);
  m_radioTab->setText(i18n("Ta&b"));
  QWhatsThis::add(m_radioTab, i18n("Use a tab as the delimiter."));
  m_delimiterGroupLayout->addWidget(m_radioTab, 2, 0);

  m_radioOther = new QRadioButton(m_delimiterGroup);
  m_radioOther->setText(i18n("Ot&her:"));
  QWhatsThis::add(m_radioOther, i18n("Use a custom string as the delimiter."));
  m_delimiterGroupLayout->addWidget(m_radioOther, 2, 1);

  m_editOther = new KLineEdit(m_delimiterGroup);
  m_editOther->setEnabled(false);
  m_editOther->setFixedWidth(m_widget->fontMetrics().width('X') * 4);
  QWhatsThis::add(m_editOther, i18n("A custom string, such as a colon, may be used as a delimiter."));
  m_delimiterGroupLayout->addWidget(m_editOther, 2, 2);
  connect(m_radioOther, SIGNAL(toggled(bool)),
          m_editOther, SLOT(setEnabled(bool)));
  connect(m_editOther, SIGNAL(textChanged(const QString&)), SLOT(slotDelimiter()));

  w = new QWidget(hbox2);
  hbox2->setStretchFactor(w, 1);

  m_table = new QTable(5, 0, group);
  m_table->setSelectionMode(QTable::Single);
  m_table->setFocusStyle(QTable::FollowStyle);
  m_table->setLeftMargin(0);
  m_table->verticalHeader()->hide();
  m_table->horizontalHeader()->setClickEnabled(true);
  m_table->setReadOnly(true);
  m_table->setMinimumHeight(m_widget->fontMetrics().lineSpacing() * 8);
  QWhatsThis::add(m_table, i18n("The table shows up to the first five lines of the CSV file."));
  connect(m_table, SIGNAL(currentChanged(int, int)), SLOT(slotCurrentChanged(int, int)));
  connect(m_table->horizontalHeader(), SIGNAL(clicked(int)), SLOT(slotHeaderClicked(int)));

  QWidget* hbox = new QWidget(group);
  QHBoxLayout* hlay = new QHBoxLayout(hbox, 5);
  hlay->addStretch(10);
  QWhatsThis::add(hbox, i18n("<qt>Set each column to correspond to a field in the collection by choosing "
                             "a column, selecting the field, then clicking the <i>Assign Field</i> button.</qt>"));
  lab = new QLabel(i18n("Co&lumn:"), hbox);
  hlay->addWidget(lab);
  m_colSpinBox = new KIntSpinBox(hbox);
  hlay->addWidget(m_colSpinBox);
  m_colSpinBox->setMinValue(1);
  connect(m_colSpinBox, SIGNAL(valueChanged(int)), SLOT(slotSelectColumn(int)));
  lab->setBuddy(m_colSpinBox);
  hlay->addSpacing(10);

  lab = new QLabel(i18n("&Data field in this column:"), hbox);
  hlay->addWidget(lab);
  m_comboField = new KComboBox(hbox);
  hlay->addWidget(m_comboField);
  connect(m_comboField, SIGNAL(activated(int)), SLOT(slotFieldChanged(int)));
  lab->setBuddy(m_comboField);
  hlay->addSpacing(10);

  m_setColumnBtn = new KPushButton(i18n("&Assign Field"), hbox);
  hlay->addWidget(m_setColumnBtn);
  m_setColumnBtn->setIconSet(SmallIconSet(QString::fromLatin1("apply")));
  connect(m_setColumnBtn, SIGNAL(clicked()), SLOT(slotSetColumnTitle()));
  hlay->addStretch(10);

  l->addStretch(1);

  KConfigGroup config(KGlobal::config(), QString::fromLatin1("ImportOptions - CSV"));
  m_delimiter = config.readEntry("Delimiter", m_delimiter);
  m_firstRowHeader = config.readBoolEntry("First Row Titles", m_firstRowHeader);

  m_checkFirstRowHeader->setChecked(m_firstRowHeader);
  if(m_delimiter == Latin1Literal(",")) {
    m_radioComma->setChecked(true);
    slotDelimiter(); // since the comma box was already checked, the slot won't fire
  } else if(m_delimiter == Latin1Literal(";")) {
    m_radioSemicolon->setChecked(true);
  } else if(m_delimiter == Latin1Literal("\t")) {
    m_radioTab->setChecked(true);
  } else if(!m_delimiter.isEmpty()) {
    m_radioOther->setChecked(true);
    m_editOther->setEnabled(true);
    m_editOther->setText(m_delimiter);
  }

  return m_widget;
}

// nominally, everything should be separated by the delimiter
// or quotes could surround the cell value if either the delimiter or quotes are contained
// actual quotes may or may not be doubled
QStringList CSVImporter::splitLine(const QString& line_) {
  // double quote
  QString dq = QString(s_quote) + s_quote;
  // delimiter length
  uint dn = m_delimiter.length();
  // list of eventual field values
  QStringList values;
  if(dn == 0) {
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
  tmp = line_.mid(oldpos + dn);
  if(tmp.endsWith(s_quote)) {
    if(inquote) {
      tmp.truncate(tmp.length()-1);
      str += tmp;
    } else if(tmp.startsWith(s_quote)) {
      str = tmp.mid(1, tmp.length()-2);
    }
  } else {
    str += tmp;
  }

  str.replace(dq, s_quote);
  values << str;
  return values;
}

void CSVImporter::fillTable() {
  if(!m_table) {
    return;
  }

  QString str = text();
  QTextStream t(&str, IO_ReadOnly);

  int maxCols = 0;
  QString line;
  for(int row = 0; row < m_table->numRows(); ++row) {
    line = t.readLine();
    const QStringList values = splitLine(line);
    if(static_cast<int>(values.count()) > m_table->numCols()) {
      m_table->setNumCols(values.count());
      m_colSpinBox->setMaxValue(values.count());
    }
    int col = 0;
    for(QStringList::ConstIterator it = values.begin(); it != values.end(); ++it) {
      m_table->setText(row, col, *it);
      m_table->adjustColumn(col);
      ++col;
    }
    if(col > maxCols) {
      maxCols = col;
    }
  }
  m_table->setNumCols(maxCols);
}

void CSVImporter::slotTypeChanged(const QString&) {
  // iterate over the collection names until it matches the text of the combo box
  Data::Collection::Type type = static_cast<Data::Collection::Type>(m_comboColl->currentType());
  m_coll = CollectionFactory::collection(type, true);

  updateHeader(true);
  m_comboField->clear();
  m_comboField->insertStringList(m_existingCollection ? m_existingCollection->fieldTitles() : m_coll->fieldTitles());
  m_comboField->insertItem('<' + i18n("New Field") + '>');

  // hack to force a resize
  m_comboField->setFont(m_comboField->font());
  m_comboField->updateGeometry();
}

void CSVImporter::slotFirstRowHeader(bool b_) {
  m_firstRowHeader = b_;
  updateHeader(false);
}

void CSVImporter::slotDelimiter() {
  if(m_radioComma->isChecked()) {
    m_delimiter = ',';
  } else if(m_radioSemicolon->isChecked()) {
    m_delimiter = ';';
  } else if(m_radioTab->isChecked()) {
    m_delimiter = '\t';
  } else {
    m_editOther->setFocus();
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
  if(!m_table) {
    return;
  }
  if(!m_firstRowHeader && !force_) {
    return;
  }

  Data::CollPtr c = m_existingCollection ? m_existingCollection : m_coll;
  for(int col = 0; col < m_table->numCols(); ++col) {
    const QString s = m_table->text(0, col);
    if(m_firstRowHeader && !s.isEmpty() && c && c->fieldByTitle(s) != 0) {
      m_table->horizontalHeader()->setLabel(col, s);
    } else {
      m_table->horizontalHeader()->setLabel(col, QString::number(col+1));
    }
  }
}

void CSVImporter::slotFieldChanged(int idx_) {
  // only care if it's the last item -> add new field
  if(idx_ < m_comboField->count()-1) {
    return;
  }

  Data::CollPtr c = m_existingCollection ? m_existingCollection : m_coll;
  uint count = c->fieldTitles().count();
  CollectionFieldsDialog dlg(c, m_widget);
//  dlg.setModal(true);
  if(dlg.exec() == QDialog::Accepted) {
    m_comboField->clear();
    m_comboField->insertStringList(c->fieldTitles());
    m_comboField->insertItem('<' + i18n("New Field") + '>');
    if(count != c->fieldTitles().count()) {
      fillTable();
    }
    m_comboField->setCurrentItem(0);
  }
}

void CSVImporter::slotActionChanged(int action_) {
  Data::CollPtr currColl = Data::Document::self()->collection();
  if(!currColl) {
    m_existingCollection = 0;
    return;
  }

  switch(action_) {
    case Import::Replace:
      {
        int currType = m_comboColl->currentType();
        m_comboColl->reset();
        m_comboColl->setCurrentType(currType);
        m_existingCollection = 0;
      }
      break;

    case Import::Append:
    case Import::Merge:
     {
        m_comboColl->clear();
        QString name = CollectionFactory::nameMap()[currColl->type()];
        m_comboColl->insertItem(name, currColl->type());
        m_existingCollection = currColl;
     }
     break;
  }
  slotTypeChanged(m_comboColl->currentText());
}

void CSVImporter::slotCancel() {
  m_cancelled = true;
}

#include "csvimporter.moc"
