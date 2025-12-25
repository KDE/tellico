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

#include "csvimporter.h"
#include "csvparser.h"
#include "translators.h" // needed for ImportAction
#include "../collectionfieldsdialog.h"
#include "../collection.h"
#include "../tellico_debug.h"
#include "../collectionfactory.h"
#include "../gui/collectiontypecombo.h"
#include "../utils/stringset.h"
#include "../images/imagefactory.h"

#include <KComboBox>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KMessageBox>
#include <KLocalizedString>

#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
#include <QCheckBox>
#include <QRadioButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QGridLayout>
#include <QByteArray>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QButtonGroup>
#include <QApplication>

using Tellico::Import::CSVImporter;

CSVImporter::CSVImporter(const QUrl& url_) : Tellico::Import::TextImporter(url_),
    m_collType(-1),
    m_existingCollection(nullptr),
    m_firstRowHeader(false),
    m_delimiter(QStringLiteral(",")),
    m_cancelled(false),
    m_widget(nullptr),
    m_comboColl(nullptr),
    m_checkFirstRowHeader(nullptr),
    m_radioComma(nullptr),
    m_radioSemicolon(nullptr),
    m_radioTab(nullptr),
    m_radioOther(nullptr),
    m_editOther(nullptr),
    m_editColDelimiter(nullptr),
    m_editRowDelimiter(nullptr),
    m_table(nullptr),
    m_colSpinBox(nullptr),
    m_comboField(nullptr),
    m_setColumnBtn(nullptr),
    m_hasAssignedFields(false),
    m_isLibraryThing(false),
    m_imageLinksOnly(false),
    m_parser(new CSVParser(text())) {
  m_parser->setDelimiter(m_delimiter);
}

CSVImporter::~CSVImporter() {
  delete m_parser;
  m_parser = nullptr;
}

Tellico::Data::CollPtr CSVImporter::collection() {
  // don't just check if m_coll is non-null since the collection can be created elsewhere
  if(m_coll && m_coll->entryCount() > 0) {
    return m_coll;
  }

  if(!m_coll) {
    createCollection();
  }

  if(m_fieldsToImport.isEmpty() && m_table) {
    for(int col = 0; col < m_table->columnCount(); ++col) {
      QString t = m_table->horizontalHeaderItem(col)->text();
      auto f = m_coll->fieldByTitle(t);
      if(f) {
        m_columnsToImport << col;
        m_fieldsToImport << f->name();
      }
    }
  }

  if(m_fieldsToImport.isEmpty()) {
    myDebug() << "no fields assigned";
    return Data::CollPtr();
  }

  m_parser->reset(text());

  // if the first row are headers, skip it
  if(m_firstRowHeader) {
    m_parser->skipLine();
  }

  const uint numChars = text().size();
  const uint stepSize = qMax(s_stepSize, numChars/100);
  const bool showProgress = options() & ImportProgress;

  // do we need to replace column or row delimiters
  const bool replaceColDelimiter = (!m_colDelimiter.isEmpty() && m_colDelimiter != FieldFormat::columnDelimiterString());
  const bool replaceRowDelimiter = (!m_rowDelimiter.isEmpty() && m_rowDelimiter != FieldFormat::rowDelimiterString());

  uint j = 0;
  while(!m_cancelled && m_parser->hasNext()) {
    bool empty = true;
    Data::EntryPtr entry(new Data::Entry(m_coll));
    QStringList values = m_parser->nextTokens();
    for(int i = 0; i < m_fieldsToImport.size(); ++i) {
      if(m_columnsToImport.at(i) >= values.size()) {
        break;
      }
      const QString currentFieldName = m_fieldsToImport.at(i);
      if(!m_coll->hasField(currentFieldName)) {
        myDebug() << "No field in collection named" << currentFieldName;
        continue;
      }
      QString value = values[m_columnsToImport.at(i)].trimmed();
      const auto fieldType = m_coll->fieldByName(currentFieldName)->type();
      // only replace delimiters for tables
      // see https://forum.kde.org/viewtopic.php?f=200&t=142712
      if(replaceColDelimiter && fieldType == Data::Field::Table) {
        value.replace(m_colDelimiter, FieldFormat::columnDelimiterString());
      }
      if(replaceRowDelimiter && fieldType == Data::Field::Table) {
        value.replace(m_rowDelimiter, FieldFormat::rowDelimiterString());
      }
      if(m_isLibraryThing) {
        // special cases for LibraryThing import
        if(currentFieldName == QLatin1String("isbn")) {
          // ISBN values are enclosed by brackets
          value.remove(QLatin1Char('[')).remove(QLatin1Char(']'));
        } else if(currentFieldName == QLatin1String("keyword")) {
          // LT values are comma-separated
          value.replace(QLatin1String(","), FieldFormat::delimiterString());
        } else if(currentFieldName == QLatin1String("cdate")) {
          // only want date, not time. 10 characters since it's zero-padded
          value.truncate(10);
        }
      }
      if(fieldType == Data::Field::Image) {
        // try to import as a absolute or relative link
        // follow same logic as in CollectionHandler::end(..)
        QUrl u(value);
        if(u.isRelative()) {
          if(url().isEmpty()) {
            // assume a local file, as fromUserInput() would do
            u = QUrl::fromLocalFile(value);
          } else {
            u = url().resolved(u);
          }
        }
        // TODO: data: urls?
        if(u.isValid() && (u.isLocalFile() || !u.host().isEmpty())) {
          value = ImageFactory::addImage(u,
                                         options() ^ ImportShowImageErrors /* quiet */,
                                         QUrl() /* referrer */,
                                         m_imageLinksOnly);
        }
      } else if(fieldType == Data::Field::Date ||
                m_coll->fieldByName(currentFieldName)->formatType() == FieldFormat::FormatDate) {
        // allow for '/' in addition to '-' for ISO date format
        value.replace(QLatin1Char('/'), QLatin1Char('-'));
      }
      bool success = entry->setField(currentFieldName, value);
      // we might need to add a new allowed value
      // assume that if the user is importing the value, it should be allowed
      if(!success && fieldType == Data::Field::Choice) {
        Data::FieldPtr f = m_coll->fieldByName(currentFieldName);
        StringSet allow;
        allow.add(f->allowed());
        allow.add(value);
        f->setAllowed(allow.values());
        m_coll->modifyField(f);
        success = entry->setField(currentFieldName, value);
      }
      if(empty && success) {
        empty = false;
      }
      j += value.size();
    }
    if(!empty) {
      m_coll->addEntries(entry);
    }

    if(showProgress && j%stepSize == 0) {
      Q_EMIT signalProgress(this, 100*j/numChars);
      qApp->processEvents();
    }
  }

  {
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("ImportOptions - CSV"));
    config.writeEntry("Delimiter", m_delimiter);
    config.writeEntry("ColumnDelimiter", m_colDelimiter);
    config.writeEntry("RowDelimiter", m_rowDelimiter);
    config.writeEntry("First Row Titles", m_firstRowHeader);
    config.writeEntry("Image Links", m_imageLinksOnly);
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

  QHBoxLayout* hlay = new QHBoxLayout();
  vlay->addLayout(hlay);
  QLabel* lab = new QLabel(i18n("Collection &type:"), groupBox);
  hlay->addWidget(lab);
  m_comboColl = new GUI::CollectionTypeCombo(groupBox);
  hlay->addWidget(m_comboColl);
  lab->setBuddy(m_comboColl);
  m_comboColl->setWhatsThis(i18n("Select the type of collection being imported."));
  void (QComboBox::* activatedInt)(int) = &QComboBox::activated;
  connect(m_comboColl, activatedInt, this, &CSVImporter::slotTypeChanged);
  hlay->addStretch(10);

  hlay = new QHBoxLayout();
  vlay->addLayout(hlay);
  m_checkFirstRowHeader = new QCheckBox(i18n("&First row contains field titles"), groupBox);
  m_checkFirstRowHeader->setWhatsThis(i18n("If checked, the first row is used as field titles."));
  connect(m_checkFirstRowHeader, &QAbstractButton::toggled, this, &CSVImporter::slotFirstRowHeader);
  hlay->addWidget(m_checkFirstRowHeader);

  m_checkImageLinks = new QCheckBox(i18n("Import images as links only"), groupBox);
  m_checkImageLinks->setWhatsThis(i18n("If checked, the image paths will be imported as links "
                                       "instead of being managed by Tellico directly."));
  connect(m_checkImageLinks, &QCheckBox::toggled, m_checkImageLinks, [this](bool checked){
    m_imageLinksOnly = checked;
  });
  hlay->addWidget(m_checkImageLinks);
  hlay->addStretch(10);

  // use a constant width for the edit boxes. They're 1 or 2 characters long.
  const int editWidth = 4 * m_widget->fontMetrics().horizontalAdvance(QLatin1Char('X'));

  QHBoxLayout* delimiterLayout = new QHBoxLayout();
  vlay->addLayout(delimiterLayout);

  lab = new QLabel(i18n("Delimiter:"), groupBox);
  lab->setWhatsThis(i18n("In addition to a comma, other characters may be used as "
                         "a delimiter, separating each value in the file."));
  delimiterLayout->addWidget(lab);

  m_radioComma = new QRadioButton(groupBox);
  m_radioComma->setText(i18n("&Comma"));
  m_radioComma->setChecked(true);
  m_radioComma->setWhatsThis(i18n("Use a comma as the delimiter."));
  delimiterLayout->addWidget(m_radioComma);

  m_radioSemicolon = new QRadioButton( groupBox);
  m_radioSemicolon->setText(i18n("&Semicolon"));
  m_radioSemicolon->setWhatsThis(i18n("Use a semi-colon as the delimiter."));
  delimiterLayout->addWidget(m_radioSemicolon);

  m_radioTab = new QRadioButton(groupBox);
  m_radioTab->setText(i18n("Ta&b"));
  m_radioTab->setWhatsThis(i18n("Use a tab as the delimiter."));
  delimiterLayout->addWidget(m_radioTab);

  m_radioOther = new QRadioButton(groupBox);
  m_radioOther->setText(i18n("Ot&her:"));
  m_radioOther->setWhatsThis(i18n("Use a custom string as the delimiter."));
  delimiterLayout->addWidget(m_radioOther);

  m_editOther = new QLineEdit(groupBox);
  m_editOther->setEnabled(false);
  m_editOther->setFixedWidth(editWidth);
  m_editOther->setMaxLength(1);
  m_editOther->setWhatsThis(i18n("A custom string, such as a colon, may be used as a delimiter."));
  m_editOther->setEnabled(false);
  delimiterLayout->addWidget(m_editOther);
  connect(m_radioOther, &QAbstractButton::toggled,
          m_editOther, &QWidget::setEnabled);
  connect(m_editOther, &QLineEdit::textChanged, this, &CSVImporter::slotDelimiter);
  delimiterLayout->addStretch(10);

  QButtonGroup* buttonGroup = new QButtonGroup(groupBox);
  buttonGroup->addButton(m_radioComma);
  buttonGroup->addButton(m_radioSemicolon);
  buttonGroup->addButton(m_radioTab);
  buttonGroup->addButton(m_radioOther);
  connect(buttonGroup, &QButtonGroup::idClicked, this, &CSVImporter::slotDelimiter);

  QHBoxLayout* delimiterLayout2 = new QHBoxLayout();
  vlay->addLayout(delimiterLayout2);

  QString w = i18n("The column delimiter separates values in each column of a <i>Table</i> field.");
  lab = new QLabel(i18n("Table column delimiter:"), groupBox);
  lab->setWhatsThis(w);
  delimiterLayout2->addWidget(lab);
  m_editColDelimiter = new QLineEdit(groupBox);
  m_editColDelimiter->setWhatsThis(w);
  m_editColDelimiter->setFixedWidth(editWidth);
  m_editColDelimiter->setMaxLength(1);
  delimiterLayout2->addWidget(m_editColDelimiter);
  connect(m_editColDelimiter, &QLineEdit::textChanged, this, &CSVImporter::slotDelimiter);

  w = i18n("The row delimiter separates values in each row of a <i>Table</i> field.");
  lab = new QLabel(i18n("Table row delimiter:"), groupBox);
  lab->setWhatsThis(w);
  delimiterLayout2->addWidget(lab);
  m_editRowDelimiter = new QLineEdit(groupBox);
  m_editRowDelimiter->setWhatsThis(w);
  m_editRowDelimiter->setFixedWidth(editWidth);
  m_editRowDelimiter->setMaxLength(1);
  delimiterLayout2->addWidget(m_editRowDelimiter);
  connect(m_editRowDelimiter, &QLineEdit::textChanged, this, &CSVImporter::slotDelimiter);

  delimiterLayout2->addStretch(10);

  m_table = new QTableWidget(5, 0, groupBox);
  vlay->addWidget(m_table);
  m_table->setSelectionMode(QAbstractItemView::SingleSelection);
  m_table->setSelectionBehavior(QAbstractItemView::SelectColumns);
  m_table->verticalHeader()->hide();
  m_table->horizontalHeader()->setSectionsClickable(true);
  m_table->setMinimumHeight(m_widget->fontMetrics().lineSpacing() * 8);
  m_table->setWhatsThis(i18n("The table shows up to the first five lines of the CSV file."));
  connect(m_table, &QTableWidget::currentCellChanged, this, &CSVImporter::slotCurrentChanged);
  connect(m_table->horizontalHeader(), &QHeaderView::sectionClicked, this, &CSVImporter::slotHeaderClicked);

  QHBoxLayout* hlay3 = new QHBoxLayout();
  vlay->addLayout(hlay3);

  QString what = i18n("<qt>Set each column to correspond to a field in the collection by choosing "
                      "a column, selecting the field, then clicking the <i>Assign Field</i> button.</qt>");
  lab = new QLabel(i18n("Co&lumn:"), groupBox);
  hlay3->addWidget(lab);
  lab->setWhatsThis(what);
  m_colSpinBox = new QSpinBox(groupBox);
  hlay3->addWidget(m_colSpinBox);
  m_colSpinBox->setWhatsThis(what);
  m_colSpinBox->setMinimum(1);
  void (QSpinBox::* valueChangedInt)(int) = &QSpinBox::valueChanged;
  connect(m_colSpinBox, valueChangedInt, this, &CSVImporter::slotSelectColumn);
  lab->setBuddy(m_colSpinBox);

  hlay3->addSpacing(10);

  lab = new QLabel(i18n("&Data field in this column:"), groupBox);
  hlay3->addWidget(lab);
  lab->setWhatsThis(what);
  m_comboField = new KComboBox(groupBox);
  hlay3->addWidget(m_comboField);
  m_comboField->setWhatsThis(what);
  // roughly 5 times the width of the edit box
  m_comboField->setFixedWidth(5 * editWidth);
//  m_comboField->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  connect(m_comboField, activatedInt, this, &CSVImporter::slotFieldChanged);
  lab->setBuddy(m_comboField);

  hlay3->addSpacing(10);

  m_setColumnBtn = new QPushButton(i18n("&Assign Field"), groupBox);
  hlay3->addWidget(m_setColumnBtn);
  m_setColumnBtn->setWhatsThis(what);
  m_setColumnBtn->setIcon(QIcon::fromTheme(QStringLiteral("dialog-ok-apply")));
  connect(m_setColumnBtn, &QAbstractButton::clicked, this, &CSVImporter::slotSetColumnTitle);
//  hlay3->addStretch(10);

  l->addWidget(groupBox);
  l->addStretch(1);

  KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("ImportOptions - CSV"));
  m_delimiter = config.readEntry("Delimiter", m_delimiter);
  m_colDelimiter = config.readEntry("ColumnDelimiter", m_colDelimiter);
  m_rowDelimiter = config.readEntry("RowDelimiter", m_rowDelimiter);
  m_firstRowHeader = config.readEntry("First Row Titles", m_firstRowHeader);
  m_imageLinksOnly = config.readEntry("Image Links", m_imageLinksOnly);

  m_checkFirstRowHeader->setChecked(m_firstRowHeader);
  m_checkImageLinks->setChecked(m_imageLinksOnly);
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
  m_editColDelimiter->setText(m_colDelimiter);
  m_editRowDelimiter->setText(m_rowDelimiter);

  slotDelimiter(); // initialize the parser and then load the text

  return m_widget;
}

bool CSVImporter::validImport() const {
  // at least one column has to be defined
  if(!m_hasAssignedFields) {
    KMessageBox::error(m_widget, i18n("At least one column must be assigned to a field. "
                                      "Only assigned columns will be imported."));
  }
  return m_hasAssignedFields;
}

void CSVImporter::setCollectionType(int collType_) {
  m_collType = collType_;
}

void CSVImporter::setImportColumns(const QList<int>& columns_, const QStringList& fieldNames_) {
  Q_ASSERT(columns_.size() == fieldNames_.size());
  m_columnsToImport = columns_;
  m_fieldsToImport = fieldNames_;
}

void CSVImporter::setDelimiter(const QString& delimiter_) {
  m_delimiter = delimiter_;
}

void CSVImporter::setColumnDelimiter(const QString& delimiter_) {
  m_colDelimiter = delimiter_;
}

void CSVImporter::setRowDelimiter(const QString& delimiter_) {
  m_rowDelimiter = delimiter_;
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
    // special case, check if the header row matches LibraryThing CSV export
    // assume LT export always uses identical header row and verify against first 7 columns
    if(row == 0 && values.count() > 7) {
      m_isLibraryThing = (values.at(0) == QLatin1String("'TITLE'") &&
                          values.at(1) == QLatin1String("'AUTHOR (first, last)'") &&
                          values.at(2) == QLatin1String("'AUTHOR (last, first)'") &&
                          values.at(3) == QLatin1String("'DATE'") &&
                          values.at(4) == QLatin1String("'LCC'") &&
                          values.at(5) == QLatin1String("'DDC'") &&
                          values.at(6) == QLatin1String("'ISBNs'"));
    }
  }
  for( ; row < m_table->rowCount(); ++row) {
    for(int col = 0; col < m_table->columnCount(); ++col) {
      delete m_table->takeItem(row, col);
    }
  }

  m_table->setColumnCount(maxCols);

  if(m_isLibraryThing) {
    // do not call slotFirstRowHeader since it will loop
    m_firstRowHeader = true;
    updateHeader();
  }
}

void CSVImporter::slotTypeChanged() {
  createCollection();

  updateHeader();
  updateFieldCombo();

  // hack to force a resize
  m_comboField->setFont(m_comboField->font());
  m_comboField->updateGeometry();
}

void CSVImporter::slotFirstRowHeader(bool b_) {
  m_firstRowHeader = b_;
  updateHeader();
  fillTable();
}

void CSVImporter::slotDelimiter() {
  if(m_radioComma->isChecked()) {
    m_delimiter = QStringLiteral(",");
  } else if(m_radioSemicolon->isChecked()) {
    m_delimiter = QStringLiteral(";");
  } else if(m_radioTab->isChecked()) {
    m_delimiter = QStringLiteral("\t");
  } else {
    m_editOther->setFocus();
    m_delimiter = m_editOther->text();
  }
  m_colDelimiter = m_editColDelimiter->text();
  m_rowDelimiter = m_editRowDelimiter->text();
  if(!m_delimiter.isEmpty()) {
    m_parser->setDelimiter(m_delimiter);
    fillTable();
    updateHeader();
  }
}

void CSVImporter::slotCurrentChanged(int, int col_) {
  const int pos = col_+1;
  m_colSpinBox->setValue(pos); //slotSelectColumn() gets called because of the signal
}

void CSVImporter::slotHeaderClicked(int col_) {
  const int pos = col_+1;
  m_colSpinBox->setValue(pos); //slotSelectColumn() gets called because of the signal
}

void CSVImporter::slotSelectColumn(int pos_) {
  // pos is really the number of the position of the column
  const int col = pos_ - 1;
  m_table->scrollToItem(m_table->item(0, col));
  m_table->selectColumn(col);
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

void CSVImporter::updateHeader() {
  if(!m_table) {
    return;
  }

  for(int col = 0; col < m_table->columnCount(); ++col) {
    QTableWidgetItem* headerItem = m_table->horizontalHeaderItem(col);
    if(!headerItem) {
      headerItem = new QTableWidgetItem();
      m_table->setHorizontalHeaderItem(col, headerItem);
    }

    QTableWidgetItem* item = m_table->item(0, col);
    Data::FieldPtr field;
    if(item && m_coll) {
      QString itemValue = item->text();
      // check against LibraryThing import
      if(m_isLibraryThing && m_coll->type() == Data::Collection::Book) {
        static QHash<QString, QString> ltFields;
        if(ltFields.isEmpty()) {
          ltFields[QStringLiteral("TITLE")]                = QStringLiteral("title");
          ltFields[QStringLiteral("AUTHOR (first, last)")] = QStringLiteral("author");
          ltFields[QStringLiteral("DATE")]                 = QStringLiteral("pub_year");
          ltFields[QStringLiteral("ISBNs")]                = QStringLiteral("isbn");
          ltFields[QStringLiteral("RATINGS")]              = QStringLiteral("rating");
          ltFields[QStringLiteral("ENTRY DATE")]           = QStringLiteral("cdate");
          ltFields[QStringLiteral("TAGS")]                 = QStringLiteral("keyword");
          ltFields[QStringLiteral("COMMENT")]              = QStringLiteral("comments");
          ltFields[QStringLiteral("REVIEWS")]              = QStringLiteral("review");
        }
        // strip leading and trailing single quotes
        itemValue.remove(0,1).chop(1);
        itemValue = ltFields.value(itemValue);

        // review is a new field, we're going to add it by default
        if(itemValue == QLatin1String("review") && !m_coll->hasField(itemValue)) {
          Data::FieldPtr field(new Data::Field(QStringLiteral("review"), i18n("Review"), Data::Field::Para));
          m_coll->addField(field);
          updateFieldCombo();
          m_comboField->setCurrentIndex(m_comboField->count()-2);
        }
      }
      field = m_coll->fieldByTitle(itemValue);
      if(!field) {
        field = m_coll->fieldByName(itemValue);
      }
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

  CollectionFieldsDialog dlg(m_coll, m_widget);
  dlg.setNotifyKernel(false);

  if(dlg.exec() == QDialog::Accepted) {
    updateFieldCombo();
    fillTable();
  }

  // set the combo to the item before last
  m_comboField->setCurrentIndex(m_comboField->count()-2);
}

void CSVImporter::slotActionChanged(int action_) {
  Data::CollPtr currColl = currentCollection();
  if(!currColl) {
    m_existingCollection = nullptr;
    return;
  }

  switch(action_) {
    case Import::Replace:
      {
        int currType = m_comboColl->currentType();
        m_comboColl->reset();
        m_comboColl->setCurrentType(currType);
        m_existingCollection = nullptr;
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

void CSVImporter::createCollection() {
  Q_ASSERT(m_collType > -1 || m_comboColl);
  Data::Collection::Type type = static_cast<Data::Collection::Type>(m_collType > -1 ? m_collType : m_comboColl->currentType());
  m_coll = CollectionFactory::collection(type, true);
  if(m_existingCollection) {
    // if we're using the existing collection, then we
    // want the newly created collection to have the same fields
    foreach(Data::FieldPtr field, m_coll->fields()) {
      m_coll->removeField(field, true /* force */);
    }
    foreach(Data::FieldPtr field, m_existingCollection->fields()) {
      m_coll->addField(Data::FieldPtr(new Data::Field(*field)));
    }
  }
}

void CSVImporter::updateFieldCombo() {
  m_comboField->clear();
  foreach(Data::FieldPtr field, m_coll->fields()) {
    m_comboField->addItem(field->title());
  }
  m_comboField->addItem(i18n("<New Field>"));
}
