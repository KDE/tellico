/***************************************************************************
    copyright            : (C) 2003-2008 by Robby Stephenson
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
#include "csvparser.h"
#include "translators.h" // needed for ImportAction
#include "../collectionfieldsdialog.h"
#include "../document.h"
#include "../collection.h"
#include "../progressmanager.h"
#include "../tellico_debug.h"
#include "../collectionfactory.h"
#include "../gui/collectiontypecombo.h"
#include "../utils/stringset.h"

#include <klineedit.h>
#include <kcombobox.h>
#include <knuminput.h>
#include <kpushbutton.h>
#include <kapplication.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <kmessagebox.h>

#include <QGroupBox>
#include <QLabel>
#include <QCheckBox>
#include <QRadioButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QRegExp>
#include <QGridLayout>
#include <QByteArray>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QButtonGroup>

using Tellico::Import::CSVImporter;

CSVImporter::CSVImporter(const KUrl& url_) : Tellico::Import::TextImporter(url_),
    m_existingCollection(0),
    m_firstRowHeader(false),
    m_delimiter(QLatin1String(",")),
    m_cancelled(false),
    m_widget(0),
    m_table(0),
    m_hasAssignedFields(false),
    m_parser(new CSVParser(text())) {
  m_parser->setDelimiter(m_delimiter);
}

CSVImporter::~CSVImporter() {
  delete m_parser;
  m_parser = 0;
}

Tellico::Data::CollPtr CSVImporter::collection() {
  // don't just check if m_coll is non-null since the collection can be created elsewhere
  if(m_coll && m_coll->entryCount() > 0) {
    return m_coll;
  }

  if(!m_coll) {
    m_coll = CollectionFactory::collection(m_comboColl->currentType(), true);
  }

  const QStringList existingNames = m_coll->fieldNames();

  QList<int> cols;
  QStringList names;
  for(int col = 0; col < m_table->columnCount(); ++col) {
    QString t = m_table->horizontalHeaderItem(col)->text();
    if(m_existingCollection && m_existingCollection->fieldByTitle(t)) {
      // the collection might have the right field, but a different title, say for translations
      Data::FieldPtr f = m_existingCollection->fieldByTitle(t);
      if(m_coll->hasField(f->name())) {
        // might have different values settings
        m_coll->removeField(f->name(), true /* force */);
      }
      m_coll->addField(Data::FieldPtr(new Data::Field(*f)));
      cols << col;
      names << f->name();
    } else if(m_coll->fieldByTitle(t)) {
      cols << col;
      names << m_coll->fieldNameByTitle(t);
    }
  }

  if(names.isEmpty()) {
    myDebug() << "CSVImporter::collection() - no fields assigned" << endl;
    return Data::CollPtr();
  }

  m_parser->reset(text());

  // if the first row are headers, skip it
  if(m_firstRowHeader) {
    m_parser->skipLine();
  }

  const uint numLines = text().count(QLatin1Char('\n'));
  const uint stepSize = qMax(s_stepSize, numLines/100);
  const bool showProgress = options() & ImportProgress;

  ProgressItem& item = ProgressManager::self()->newProgressItem(this, progressLabel(), true);
  item.setTotalSteps(numLines);
  connect(&item, SIGNAL(signalCancelled(ProgressItem*)), SLOT(slotCancel()));
  ProgressItem::Done done(this);

  uint j = 0;
  while(!m_cancelled && m_parser->hasNext()) {
    bool empty = true;
    Data::EntryPtr entry(new Data::Entry(m_coll));
    QStringList values = m_parser->nextTokens();
    for(int i = 0; i < names.size(); ++i) {
//      QString value = values[cols[i]].simplified();
      QString value = values[cols[i]].trimmed();
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

    if(showProgress && j%stepSize == 0) {
      ProgressManager::self()->setProgress(this, j);
      kapp->processEvents();
    }
    ++j;
  }

  {
    KConfigGroup config(KGlobal::config(), QLatin1String("ImportOptions - CSV"));
    config.writeEntry("Delimiter", m_delimiter);
    config.writeEntry("First Row Titles", m_firstRowHeader);
  }

  return m_coll;
}

QWidget* CSVImporter::widget(QWidget* parent_) {
  if(m_widget && m_widget->parent() == parent_) {
    return m_widget;
  }

  m_widget = new QWidget(parent_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* groupBox = new QGroupBox(i18n("CSV Options"), m_widget);
  QVBoxLayout* vlay = new QVBoxLayout(groupBox);

  QHBoxLayout* hlay = new QHBoxLayout(groupBox);
  vlay->addLayout(hlay);
  QLabel* lab = new QLabel(i18n("Collection &type:"), groupBox);
  hlay->addWidget(lab);
  m_comboColl = new GUI::CollectionTypeCombo(groupBox);
  hlay->addWidget(m_comboColl);
  lab->setBuddy(m_comboColl);
  m_comboColl->setWhatsThis(i18n("Select the type of collection being imported."));
  connect(m_comboColl, SIGNAL(activated(int)), SLOT(slotTypeChanged()));

  m_checkFirstRowHeader = new QCheckBox(i18n("&First row contains field titles"), groupBox);
  m_checkFirstRowHeader->setWhatsThis(i18n("If checked, the first row is used as field titles."));
  connect(m_checkFirstRowHeader, SIGNAL(toggled(bool)), SLOT(slotFirstRowHeader(bool)));
  hlay->addWidget(m_checkFirstRowHeader);

  hlay->addStretch(1);

  QHBoxLayout* hlay2 = new QHBoxLayout(groupBox);
  vlay->addLayout(hlay2);

  m_delimiterGroup = new QGroupBox(i18n("Delimiter"), groupBox);
  hlay2->addWidget(m_delimiterGroup);
  QGridLayout* m_delimiterGroupLayout = new QGridLayout(m_delimiterGroup);
  m_delimiterGroupLayout->setAlignment(Qt::AlignTop);
  m_delimiterGroup->setWhatsThis(i18n("In addition to a comma, other characters may be used as "
                                      "a delimiter, separating each value in the file."));

  m_radioComma = new QRadioButton(m_delimiterGroup);
  m_radioComma->setText(i18n("&Comma"));
  m_radioComma->setChecked(true);
  m_radioComma->setWhatsThis(i18n("Use a comma as the delimiter."));
  m_delimiterGroupLayout->addWidget(m_radioComma, 1, 0);

  m_radioSemicolon = new QRadioButton( m_delimiterGroup);
  m_radioSemicolon->setText(i18n("&Semicolon"));
  m_radioSemicolon->setWhatsThis(i18n("Use a semi-colon as the delimiter."));
  m_delimiterGroupLayout->addWidget(m_radioSemicolon, 1, 1);

  m_radioTab = new QRadioButton(m_delimiterGroup);
  m_radioTab->setText(i18n("Ta&b"));
  m_radioTab->setWhatsThis(i18n("Use a tab as the delimiter."));
  m_delimiterGroupLayout->addWidget(m_radioTab, 2, 0);

  m_radioOther = new QRadioButton(m_delimiterGroup);
  m_radioOther->setText(i18n("Ot&her:"));
  m_radioOther->setWhatsThis(i18n("Use a custom string as the delimiter."));
  m_delimiterGroupLayout->addWidget(m_radioOther, 2, 1);

  m_editOther = new KLineEdit(m_delimiterGroup);
  m_editOther->setEnabled(false);
  m_editOther->setFixedWidth(m_widget->fontMetrics().width(QLatin1Char('X')) * 4);
  m_editOther->setMaxLength(1);
  m_editOther->setWhatsThis(i18n("A custom string, such as a colon, may be used as a delimiter."));
  m_editOther->setEnabled(false);
  m_delimiterGroupLayout->addWidget(m_editOther, 2, 2);
  connect(m_radioOther, SIGNAL(toggled(bool)),
          m_editOther, SLOT(setEnabled(bool)));
  connect(m_editOther, SIGNAL(textChanged(const QString&)), SLOT(slotDelimiter()));

  QButtonGroup* buttonGroup = new QButtonGroup(m_delimiterGroup);
  buttonGroup->addButton(m_radioComma);
  buttonGroup->addButton(m_radioSemicolon);
  buttonGroup->addButton(m_radioTab);
  buttonGroup->addButton(m_radioOther);
  connect(buttonGroup, SIGNAL(buttonClicked(int)), SLOT(slotDelimiter()));

  hlay2->addStretch(1);

  m_table = new QTableWidget(5, 0, groupBox);
  vlay->addWidget(m_table);
  m_table->setSelectionMode(QAbstractItemView::SingleSelection);
  m_table->verticalHeader()->hide();
  m_table->horizontalHeader()->setClickable(true);
  m_table->setMinimumHeight(m_widget->fontMetrics().lineSpacing() * 8);
  m_table->setWhatsThis(i18n("The table shows up to the first five lines of the CSV file."));
  connect(m_table, SIGNAL(currentCellChanged(int, int, int, int)), SLOT(slotCurrentChanged(int, int)));
  connect(m_table->horizontalHeader(), SIGNAL(sectionClicked(int)), SLOT(slotHeaderClicked(int)));

  QHBoxLayout* hlay3 = new QHBoxLayout(groupBox);
  vlay->addLayout(hlay3);
  hlay3->addStretch(1);
  QString what = i18n("<qt>Set each column to correspond to a field in the collection by choosing "
                      "a column, selecting the field, then clicking the <i>Assign Field</i> button.</qt>");
  lab = new QLabel(i18n("Co&lumn:"), groupBox);
  hlay3->addWidget(lab);
  lab->setWhatsThis(what);
  m_colSpinBox = new KIntSpinBox(groupBox);
  hlay3->addWidget(m_colSpinBox);
  m_colSpinBox->setWhatsThis(what);
  m_colSpinBox->setMinimum(1);
  connect(m_colSpinBox, SIGNAL(valueChanged(int)), SLOT(slotSelectColumn(int)));
  lab->setBuddy(m_colSpinBox);
  hlay3->addSpacing(10);

  lab = new QLabel(i18n("&Data field in this column:"), groupBox);
  hlay3->addWidget(lab);
  lab->setWhatsThis(what);
  m_comboField = new KComboBox(groupBox);
  hlay3->addWidget(m_comboField);
  m_comboField->setWhatsThis(what);
  connect(m_comboField, SIGNAL(activated(int)), SLOT(slotFieldChanged(int)));
  lab->setBuddy(m_comboField);
  hlay3->addSpacing(10);

  m_setColumnBtn = new KPushButton(i18n("&Assign Field"), groupBox);
  hlay3->addWidget(m_setColumnBtn);
  m_setColumnBtn->setWhatsThis(what);
  m_setColumnBtn->setIcon(KIcon(QLatin1String("dialog-ok-apply")));
  connect(m_setColumnBtn, SIGNAL(clicked()), SLOT(slotSetColumnTitle()));
  hlay3->addStretch(10);

  l->addWidget(groupBox);
  l->addStretch(1);

  KConfigGroup config(KGlobal::config(), QLatin1String("ImportOptions - CSV"));
  m_delimiter = config.readEntry("Delimiter", m_delimiter);
  m_firstRowHeader = config.readEntry("First Row Titles", m_firstRowHeader);

  m_checkFirstRowHeader->setChecked(m_firstRowHeader);
  if(m_delimiter == QLatin1String(",")) {
    m_radioComma->setChecked(true);
    slotDelimiter(); // since the comma box was already checked, the slot won't fire
  } else if(m_delimiter == QLatin1String(";")) {
    m_radioSemicolon->setChecked(true);
  } else if(m_delimiter == QLatin1String("\t")) {
    m_radioTab->setChecked(true);
  } else if(!m_delimiter.isEmpty()) {
    m_radioOther->setChecked(true);
    m_editOther->setEnabled(true);
    m_editOther->setText(m_delimiter);
  }

  return m_widget;
}

bool CSVImporter::validImport() const {
  // at least one column has to be defined
  if(!m_hasAssignedFields) {
    KMessageBox::sorry(m_widget, i18n("At least one column must be assigned to a field. "
                                      "Only assigned columns will be imported."));
  }
  return m_hasAssignedFields;
}

void CSVImporter::fillTable() {
  if(!m_table) {
    return;
  }

  m_parser->reset(text());
  // not skipping first row since the updateHeader() call depends on it

  int maxCols = 0;
  int row = 0;
  for( ; m_parser->hasNext() && row < m_table->rowCount(); ++row) {
    QStringList values = m_parser->nextTokens();
    if(static_cast<int>(values.count()) > m_table->columnCount()) {
      m_table->setColumnCount(values.count());
      m_colSpinBox->setMaximum(values.count());
    }
    int col = 0;
    foreach(const QString& value, values) {
      m_table->setItem(row, col, new QTableWidgetItem(value));
      m_table->resizeColumnToContents(col);
      ++col;
    }
    if(col > maxCols) {
      maxCols = col;
    }
  }
  for( ; row < m_table->rowCount(); ++row) {
    for(int col = 0; col < m_table->columnCount(); ++col) {
      delete m_table->takeItem(row, col);
    }
  }

  m_table->setColumnCount(maxCols);
}

void CSVImporter::slotTypeChanged() {
  // iterate over the collection names until it matches the text of the combo box
  Data::Collection::Type type = static_cast<Data::Collection::Type>(m_comboColl->currentType());
  m_coll = CollectionFactory::collection(type, true);

  updateHeader(true);
  m_comboField->clear();
  m_comboField->addItems(m_existingCollection ? m_existingCollection->fieldTitles() : m_coll->fieldTitles());
  m_comboField->addItem(QLatin1Char('<') + i18n("New Field") + QLatin1Char('>'));

  // hack to force a resize
  m_comboField->setFont(m_comboField->font());
  m_comboField->updateGeometry();
}

void CSVImporter::slotFirstRowHeader(bool b_) {
  m_firstRowHeader = b_;
  updateHeader(false);
  fillTable();
}

void CSVImporter::slotDelimiter() {
  if(m_radioComma->isChecked()) {
    m_delimiter = QLatin1String(",");
  } else if(m_radioSemicolon->isChecked()) {
    m_delimiter = QLatin1String(";");
  } else if(m_radioTab->isChecked()) {
    m_delimiter = QLatin1String("\t");
  } else {
    m_editOther->setFocus();
    m_delimiter = m_editOther->text();
  }
  if(!m_delimiter.isEmpty()) {
    m_parser->setDelimiter(m_delimiter);
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
  m_table->scrollToItem(m_table->item(0, col));
  m_comboField->setCurrentItem(m_table->horizontalHeaderItem(col)->text());
}

void CSVImporter::slotSetColumnTitle() {
  int col = m_colSpinBox->value()-1;
  const QString title = m_comboField->currentText();
  m_table->horizontalHeaderItem(col)->setText(title);
  m_hasAssignedFields = true;
  // make sure none of the other columns have this title
  bool found = false;
  for(int i = 0; i < col; ++i) {
    if(m_table->horizontalHeaderItem(i)->text() == title) {
      m_table->horizontalHeaderItem(i)->setText(QString::number(i+1));
      found = true;
      break;
    }
  }
  // if found, then we're done
  if(found) {
    return;
  }
  for(int i = col+1; i < m_table->columnCount(); ++i) {
    if(m_table->horizontalHeaderItem(i)->text() == title) {
      m_table->horizontalHeaderItem(i)->setText(QString::number(i+1));
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

  Data::CollPtr currColl = m_existingCollection ? m_existingCollection : m_coll;
  for(int col = 0; col < m_table->columnCount(); ++col) {
    QTableWidgetItem* item = m_table->item(0, col);
    Data::FieldPtr field;
    if(item && currColl) {
      QString itemValue = item->text();
      field = currColl->fieldByTitle(itemValue);
      if(!field) {
        field = currColl->fieldByName(itemValue);
      }
    }
    QTableWidgetItem* headerItem = m_table->horizontalHeaderItem(col);
    if(!headerItem) {
      headerItem = new QTableWidgetItem();
      m_table->setHorizontalHeaderItem(col, headerItem);
    }
    if(m_firstRowHeader && field) {
      headerItem->setText(field->title());
      m_hasAssignedFields = true;
    } else {
      headerItem->setText(QString::number(col+1));
    }
  }
}

void CSVImporter::slotFieldChanged(int idx_) {
  // only care if it's the last item -> add new field
  if(idx_ < m_comboField->count()-1) {
    return;
  }

  Data::CollPtr c = m_existingCollection ? m_existingCollection : m_coll;
  int count = c->fieldTitles().count();
  CollectionFieldsDialog dlg(c, m_widget);
//  dlg.setModal(true);
  if(dlg.exec() == QDialog::Accepted) {
    m_comboField->clear();
    m_comboField->addItems(c->fieldTitles());
    m_comboField->addItem(QLatin1Char('<') + i18n("New Field") + QLatin1Char('>'));
    if(count != c->fieldTitles().count()) {
      fillTable();
    }
    m_comboField->setCurrentIndex(0);
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
        QString name = CollectionFactory::nameHash().value(currColl->type());
        m_comboColl->addItem(name, currColl->type());
        m_existingCollection = currColl;
     }
     break;
  }
  slotTypeChanged();
}

void CSVImporter::slotCancel() {
  m_cancelled = true;
}

#include "csvimporter.moc"
