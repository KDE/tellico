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

extern "C" {
#include "libcsv.h"
}

#include <klineedit.h>
#include <kcombobox.h>
#include <knuminput.h>
#include <kpushbutton.h>
#include <kapplication.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <kmessagebox.h>

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

typedef int(*SpaceFunc)(char);

static void writeToken(char* buffer, size_t len, void* data);
static void writeRow(char buffer, void* data);
static int isSpace(char c);
static int isSpaceOrTab(char c);
static int isTab(char c);

class CSVImporter::Parser {
public:
  Parser(const QString& str) : stream(new QTextIStream(&str)) { csv_init(&parser, 0); }
  ~Parser() { csv_free(parser); delete stream; stream = 0; }

  void setDelimiter(const QString& s) {
    Q_ASSERT(s.length() == 1);
    csv_set_delim(parser, s[0].latin1());
    if(s[0] == '\t')     csv_set_space_func(parser, isSpace);
    else if(s[0] == ' ') csv_set_space_func(parser, isTab);
    else                 csv_set_space_func(parser, isSpaceOrTab);
  }
  void reset(const QString& str) { delete stream; stream = new QTextIStream(&str); };
  bool hasNext() { return !stream->atEnd(); }
  void skipLine() { stream->readLine(); }

  void addToken(const QString& t) { tokens += t; }
  void setRowDone(bool b) { done = b; }

  QStringList nextTokens() {
    tokens.clear();
    done = false;
    while(hasNext() && !done) {
      QCString line = stream->readLine().utf8() + '\n'; // need the eol char
      csv_parse(parser, line, line.length(), &writeToken, &writeRow, this);
    }
    csv_fini(parser, &writeToken, &writeRow, this);
    return tokens;
  }

private:
  struct csv_parser* parser;
  QTextIStream* stream;
  QStringList tokens;
  bool done;
};

static void writeToken(char* buffer, size_t len, void* data) {
  CSVImporter::Parser* p = static_cast<CSVImporter::Parser*>(data);
  p->addToken(QString::fromUtf8(buffer, len));
}

static void writeRow(char c, void* data) {
  Q_UNUSED(c);
  CSVImporter::Parser* p = static_cast<CSVImporter::Parser*>(data);
  p->setRowDone(true);
}

static int isSpace(char c) {
  if (c == CSV_SPACE) return 1;
  return 0;
}

static int isSpaceOrTab(char c) {
  if (c == CSV_SPACE || c == CSV_TAB) return 1;
  return 0;
}

static int isTab(char c) {
  if (c == CSV_TAB) return 1;
  return 0;
}

CSVImporter::CSVImporter(const KURL& url_) : Tellico::Import::TextImporter(url_),
    m_coll(0),
    m_existingCollection(0),
    m_firstRowHeader(false),
    m_delimiter(QString::fromLatin1(",")),
    m_cancelled(false),
    m_widget(0),
    m_table(0),
    m_hasAssignedFields(false),
    m_parser(new Parser(text())) {
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

  if(names.isEmpty()) {
    myDebug() << "CSVImporter::collection() - no fields assigned" << endl;
    return 0;
  }

  m_parser->reset(text());

  // if the first row are headers, skip it
  if(m_firstRowHeader) {
    m_parser->skipLine();
  }

  const uint numLines = text().contains('\n');
  const uint stepSize = QMAX(s_stepSize, numLines/100);
  const bool showProgress = options() & ImportProgress;

  ProgressItem& item = ProgressManager::self()->newProgressItem(this, progressLabel(), true);
  item.setTotalSteps(numLines);
  connect(&item, SIGNAL(signalCancelled(ProgressItem*)), SLOT(slotCancel()));
  ProgressItem::Done done(this);

  uint j = 0;
  while(!m_cancelled && m_parser->hasNext()) {
    bool empty = true;
    Data::EntryPtr entry = new Data::Entry(m_coll);
    QStringList values = m_parser->nextTokens();
    for(uint i = 0; i < names.size(); ++i) {
//      QString value = values[cols[i]].simplifyWhiteSpace();
      QString value = values[cols[i]].stripWhiteSpace();
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
  connect(m_comboColl, SIGNAL(activated(int)), SLOT(slotTypeChanged()));
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
  m_editOther->setMaxLength(1);
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
  for( ; m_parser->hasNext() && row < m_table->numRows(); ++row) {
    QStringList values = m_parser->nextTokens();
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
  for( ; row < m_table->numRows(); ++row) {
    for(int col = 0; col < m_table->numCols(); ++col) {
      m_table->clearCell(row, col);
    }
  }

  m_table->setNumCols(maxCols);
}

void CSVImporter::slotTypeChanged() {
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
  fillTable();
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
  m_table->ensureCellVisible(0, col);
  m_comboField->setCurrentItem(m_table->horizontalHeader()->label(col));
}

void CSVImporter::slotSetColumnTitle() {
  int col = m_colSpinBox->value()-1;
  const QString title = m_comboField->currentText();
  m_table->horizontalHeader()->setLabel(col, title);
  m_hasAssignedFields = true;
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
    QString s = m_table->text(0, col);
    Data::FieldPtr f;
    if(c) {
      c->fieldByTitle(s);
      if(!f) {
        f = c->fieldByName(s);
      }
    }
    if(m_firstRowHeader && !s.isEmpty() && c && f) {
      m_table->horizontalHeader()->setLabel(col, f->title());
      m_hasAssignedFields = true;
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
  slotTypeChanged();
}

void CSVImporter::slotCancel() {
  m_cancelled = true;
}

#include "csvimporter.moc"
