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

#include "collectionfieldsdialog.h"
#include "collection.h"
#include "collectionfactory.h"
#include "stringmapdialog.h"
#include "translators/tellico_xml.h"

#include <klocale.h>
#include <kcombobox.h>
#include <klineedit.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kpushbutton.h>

#include <qgroupbox.h>
#include <qlayout.h>
#include <qhbox.h>
#include <qvbox.h>
#include <qlabel.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qregexp.h>
#include <qwhatsthis.h>
#include <qpainter.h>
#include <qtimer.h>

using Tellico::ListBoxText;
using Tellico::CollectionFieldsDialog;

ListBoxText::ListBoxText(QListBox* listbox_, Data::Field* field_)
    : QListBoxText(listbox_, field_->title()), m_field(field_), m_colored(false) {
}

ListBoxText::ListBoxText(QListBox* listbox_, Data::Field* field_, QListBoxItem* after_)
    : QListBoxText(listbox_, field_->title(), after_), m_field(field_), m_colored(false) {
}

void ListBoxText::setColored(bool colored_) {
  if(m_colored != colored_) {
    m_colored = colored_;
    listBox()->triggerUpdate(false);
  }
}

void ListBoxText::setText(const QString& text_) {
  QListBoxText::setText(text_);
  listBox()->triggerUpdate(true);
}

// mostly copied from QListBoxText::paint() in Qt 3.1.1
void ListBoxText::paint(QPainter* painter_) {
  int itemHeight = height(listBox());
  QFontMetrics fm = painter_->fontMetrics();
  int yPos = ((itemHeight - fm.height()) / 2) + fm.ascent();
  if(m_colored) {
    QFont font = painter_->font();
    font.setBold(true);
    font.setItalic(true);
    painter_->setFont(font);
    if(!isSelected()) {
      painter_->setPen(listBox()->colorGroup().highlight());
    }
  }
  painter_->drawText(3, yPos, text());
}

CollectionFieldsDialog::CollectionFieldsDialog(Data::Collection* coll_, QWidget* parent_, const char* name_/*=0*/)
    : KDialogBase(parent_, name_, false, i18n("Collection Fields"), Help|Default|Ok|Apply|Cancel, Ok, false),
      m_coll(coll_),
      m_defaultCollection(0),
      m_typeMap(Data::Field::typeMap()),
      m_currentField(0),
      m_modified(false),
      m_updatingValues(false),
      m_reordered(false),
      m_oldIndex(-1) {
  QWidget* page = new QWidget(this);
  setMainWidget(page);
  QHBoxLayout* topLayout = new QHBoxLayout(page, 0, KDialog::spacingHint());

  QGroupBox* fieldsGroup = new QGroupBox(1, Qt::Horizontal, i18n("Current Fields"), page);
  topLayout->addWidget(fieldsGroup, 1);
  m_fieldsBox = new QListBox(fieldsGroup);
  m_fieldsBox->setMinimumWidth(150);

  for(Data::FieldListIterator it(m_coll->fieldList()); it.current(); ++it) {
    // ignore ReadOnly
    if(it.current()->type() != Data::Field::ReadOnly) {
      (void) new ListBoxText(m_fieldsBox, it.current());
    }
  }
  connect(m_fieldsBox, SIGNAL(highlighted(int)), SLOT(slotHighlightedChanged(int)));

  QHBox* hb1 = new QHBox(fieldsGroup);
  hb1->setSpacing(4);
  m_btnNew = new KPushButton(i18n("New Field", "New"), hb1);
  m_btnNew->setIconSet(BarIcon(QString::fromLatin1("filenew"), KIcon::SizeSmall));
  QWhatsThis::add(m_btnNew, i18n("Add a new field to the collection"));
  m_btnDelete = new KPushButton(i18n("Delete Field", "Delete"), hb1);
  m_btnDelete->setIconSet(BarIcon(QString::fromLatin1("editdelete"), KIcon::SizeSmall));
  QWhatsThis::add(m_btnDelete, i18n("Remove a field from the collection"));

  connect(m_btnNew, SIGNAL(clicked()), SLOT(slotNew()) );
  connect(m_btnDelete, SIGNAL(clicked()), SLOT(slotDelete()));

  QHBox* hb2 = new QHBox(fieldsGroup);
  hb2->setSpacing(4);
  m_btnUp = new KPushButton(hb2);
  m_btnUp->setPixmap(BarIcon(QString::fromLatin1("up"), KIcon::SizeSmall));
  QWhatsThis::add(m_btnUp, i18n("Move this field up in the list. The list order is important "
                                "for the layout of the entry editor."));
  m_btnDown = new KPushButton(hb2);
  m_btnDown->setPixmap(BarIcon(QString::fromLatin1("down"), KIcon::SizeSmall));
  QWhatsThis::add(m_btnDown, i18n("Move this field down in the list. The list order is important "
                                  "for the layout of the entry editor."));

  connect(m_btnUp, SIGNAL(clicked()), SLOT(slotMoveUp()) );
  connect(m_btnDown, SIGNAL(clicked()), SLOT(slotMoveDown()));

  QVBox* vbox = new QVBox(page);
  vbox->setSpacing(KDialog::spacingHint());
  topLayout->addWidget(vbox, 2);

  QGroupBox* propGroup = new QGroupBox(1, Qt::Horizontal, i18n("Field Properties"), vbox);

  QWidget* grid = new QWidget(propGroup);
  // (parent, nrows, ncols, margin, spacing)
  QGridLayout* layout = new QGridLayout(grid, 4, 4, 0, KDialog::spacingHint());

  QLabel* label = new QLabel(i18n("Title:"), grid);
  layout->addWidget(label, 0, 0);
  m_titleEdit = new KLineEdit(grid);
  layout->addWidget(m_titleEdit, 0, 1);
  QString whats = i18n("The title of the field");
  QWhatsThis::add(label, whats);
  QWhatsThis::add(m_titleEdit, whats);
  connect(m_titleEdit, SIGNAL(textChanged(const QString&)), SLOT(slotModified()));

  label = new QLabel(i18n("Type:"), grid);
  layout->addWidget(label, 0, 2);
  m_typeCombo = new KComboBox(grid);
  layout->addWidget(m_typeCombo, 0, 3);
  whats = QString::fromLatin1("<qt>");
  whats += i18n("The type of the field determines what values may be used. ");
  whats += i18n("<i>Simple Text</i> is used for most fields. ");
  whats += i18n("<i>Paragraph</i> is for large text blocks. ");
  whats += i18n("<i>Choice</i> limits the field to certain values. ");
  whats += i18n("<i>Checkbox</i> is for a simple yes/no value. ");
  whats += i18n("<i>Number</i> indicates that the field contains a numerical value. ");
  whats += i18n("<i>URL</i> is for fields which refer to URLs, including references to other files. ");
  whats += i18n("<i>Table</i>s may be a single or double column of values. ");
  whats += i18n("An <i>Image</i> field holds a picture. ");
  whats += i18n("A <i>Date</i> field can be used for values with a day, month, and year. ");
  whats += i18n("A <i>Dependent</i> field depends on the values of other "
                "fields, and is formatted according to the field description. ");
  whats += i18n("A <i>Read Only</i> is for internal values, possibly useful for import and export. ");
  whats += QString::fromLatin1("</qt>");
  QWhatsThis::add(label, whats);
  QWhatsThis::add(m_typeCombo, whats);
  // the typeTitles match the typeMap().values() but in a better order
  m_typeCombo->insertStringList(Data::Field::typeTitles());
  connect(m_typeCombo, SIGNAL(activated(int)), SLOT(slotModified()));
  connect(m_typeCombo, SIGNAL(activated(const QString&)), SLOT(slotTypeChanged(const QString&)));

  label = new QLabel(i18n("Category:"), grid);
  layout->addWidget(label, 1, 0);
  m_catCombo = new KComboBox(true, grid);
  layout->addWidget(m_catCombo, 1, 1);
  whats = i18n("The field category determines where the field is placed in the editor.");
  QWhatsThis::add(label, whats);
  QWhatsThis::add(m_catCombo, whats);

  // I don't want to include the categories for singleCategory fields
  QStringList cats;
  const QStringList allCats = m_coll->fieldCategories();
  for(QStringList::ConstIterator it = allCats.begin(); it != allCats.end(); ++it) {
    Data::FieldList list = m_coll->fieldsByCategory(*it);
    if(!list.isEmpty() && !list.getFirst()->isSingleCategory()) {
      cats.append(*it);
    }
  }
  m_catCombo->insertStringList(cats);
  m_catCombo->setDuplicatesEnabled(false);
  connect(m_catCombo, SIGNAL(textChanged(const QString&)), SLOT(slotModified()));

  label = new QLabel(i18n("Allowed:"), grid);
  layout->addWidget(label, 2, 0);
  m_allowEdit = new KLineEdit(grid);
  layout->addMultiCellWidget(m_allowEdit, 2, 2, 1, 3);
  whats = i18n("<qt>For <i>Choice</i>-type fields, these are the only values allowed. They are "
               "placed in a combo box.</qt>");
  QWhatsThis::add(label, whats);
  QWhatsThis::add(m_allowEdit, whats);
  connect(m_allowEdit, SIGNAL(textChanged(const QString&)), SLOT(slotModified()));

  label = new QLabel(i18n("Description:"), grid);
  layout->addWidget(label, 3, 0);
  m_descEdit = new KLineEdit(grid);
  m_descEdit->setMinimumWidth(150);
  layout->addMultiCellWidget(m_descEdit, 3, 3, 1, 3);
  whats = i18n("The description is a useful reminder of what information is contained in the "
               "field. For <i>Dependent</i> fields, the description is a format string such as "
               "\"%{year} %{title}\" where the named fields get substituted in the string.");
  QWhatsThis::add(label, whats);
  QWhatsThis::add(m_descEdit, whats);
  connect(m_descEdit, SIGNAL(textChanged(const QString&)), SLOT(slotModified()));

  label = new QLabel(i18n("Extended Properties:"), grid);
  layout->addWidget(label, 4, 0);
  m_btnExtended = new KPushButton(i18n("Set..."), grid);
  layout->addMultiCellWidget(m_btnExtended, 4, 4, 1, 1);
  whats = i18n("Extended field properties are used to specify things such as the corresponding bibtex field.");
  QWhatsThis::add(label, whats);
  connect(m_btnExtended, SIGNAL(clicked()), SLOT(slotShowExtendedProperties()));

  QButtonGroup* bg = new QButtonGroup(1, Qt::Horizontal, i18n("Format Options"), vbox);
  m_formatNone = new QRadioButton(i18n("No formatting"), bg);
  QWhatsThis::add(m_formatNone, i18n("This option prevents the field from ever being "
                                     "automatically formatted or capitalized."));
  m_formatPlain = new QRadioButton(i18n("Allow auto-capitalization only"), bg);
  QWhatsThis::add(m_formatPlain, i18n("This option allows the field to be capitalized, but "
                                      "not specially formatted."));
  m_formatTitle = new QRadioButton(i18n("Format as a title"), bg);
  QWhatsThis::add(m_formatTitle, i18n("This option capitalizes and formats the field as a "
                                      "title, but only if those options are globally set."));
  m_formatName = new QRadioButton(i18n("Format as a name"), bg);
  QWhatsThis::add(m_formatName, i18n("This option capitalizes and formats the field as a "
                                     "name, but only if those options are globally set."));
  connect(bg, SIGNAL(clicked(int)), SLOT(slotModified()));

  QGroupBox* optionsGroup = new QGroupBox(1, Qt::Horizontal, i18n("Field Options"), vbox);
  m_complete = new QCheckBox(i18n("Enable auto-completion"), optionsGroup);
  QWhatsThis::add(m_complete, i18n("If checked, KDE auto-completion will be enabled in the "
                                   "text edit box for this field."));
  m_multiple = new QCheckBox(i18n("Allow multiple values"), optionsGroup);
  QWhatsThis::add(m_multiple, i18n("If checked, Tellico will parse the values in the field "
                                   "for multiple values, separated by a semi-colon."));
  m_grouped = new QCheckBox(i18n("Allow grouping"), optionsGroup);
  QWhatsThis::add(m_grouped, i18n("If checked, this field may be used to group the entries in "
                                  "the group view."));
  connect(m_complete, SIGNAL(clicked()), SLOT(slotModified()));
  connect(m_multiple, SIGNAL(clicked()), SLOT(slotModified()));
  connect(m_grouped, SIGNAL(clicked()), SLOT(slotModified()));

  // need to stretch at bottom
  vbox->setStretchFactor(new QWidget(vbox), 1);

  // keep a default collection
  m_defaultCollection = CollectionFactory::collection(m_coll->type(), true);

  QWhatsThis::add(actionButton(KDialogBase::Default),
                  i18n("Revert the selected field's properties to the default values."));

  enableButtonOK(false);
  enableButtonApply(false);

  setHelp(QString::fromLatin1("fields-dialog"));

  // initially the m_typeCombo is populated with all types, but as soon as something is
  // selected in the fields box, the combo box is cleared and filled with the allowable
  // new types. The problem is that when more types are added, the size of the combo box
  // doesn't change. So when everything is laid out, the combo box needs to have all the
  // items there.
  QTimer::singleShot(0, this, SLOT(slotSelectInitial()));
}

CollectionFieldsDialog::~CollectionFieldsDialog() {
  delete m_defaultCollection;
  m_defaultCollection = 0;
}

void CollectionFieldsDialog::slotSelectInitial() {
  m_fieldsBox->setSelected(0, true);
}

void CollectionFieldsDialog::slotOk() {
  if(!checkValues()) {
    return;
  }

  slotApply();
  accept();
}

void CollectionFieldsDialog::slotApply() {
  if(!checkValues()) {
    return;
  }

  updateField();

  for(Data::FieldListIterator it(m_copiedFields); it.current(); ++it) {
    // check for Choice fields with removed values to warn user
    if(it.current()->type() == Data::Field::Choice) {
      QStringList oldValues = m_coll->fieldByName(it.current()->name())->allowed();
      QStringList newValues = it.current()->allowed();
      for(QStringList::ConstIterator vIt = oldValues.begin(); vIt != oldValues.end(); ++vIt) {
        if(newValues.contains(*vIt)) {
          continue;
        }
        int ret = KMessageBox::warningContinueCancel(this,
                                                     i18n("<qt>Removing allowed values from the <i>%1</i> field which "
                                                          "currently exist in the collection may cause data corruption. "
                                                          "Do you want to keep your modified values or cancel and revert "
                                                          "to the current ones?</qt>").arg(it.current()->title()),
                                                     QString::null,
                                                     i18n("Keep modified values"));
        if(ret != KMessageBox::Continue) {
          it.current()->setAllowed(oldValues);
        }
        break;
      }
    }
    m_coll->modifyField(it.current());
  }


  for(Data::FieldListIterator it(m_newFields); it.current(); ++it) {
    m_coll->addField(it.current());
  }

  // set all text not to be colored, and get new list
  Data::FieldList list;
  for(QListBoxItem* item = m_fieldsBox->firstItem(); item; item = item->next()) {
    static_cast<ListBoxText*>(item)->setColored(false);
    if(m_reordered) {
      Data::Field* field = static_cast<ListBoxText*>(item)->field();
      if(field) {
        list.append(field);
      }
    }
  }

  if(list.count() > 0) {
    m_coll->reorderFields(list);
  }

  if(m_newFields.count() > 0 || m_copiedFields.count() > 0 || m_reordered) {
    emit signalCollectionModified();
  }

  // now clear copied fields, they all got added to the collection, so don't delete them
  m_copiedFields.clear();
  // clear new ones, too, do not delete them
  m_newFields.clear();

  // the field type might have changed, so need to update the type combo list with possible values
  m_typeCombo->clear();
  m_typeCombo->insertStringList(newTypesAllowed(m_currentField->type()));

  enableButtonApply(false);
}

void CollectionFieldsDialog::slotNew() {
  // first update the current one with all the values from the edit widgets
  updateField();

  // next check old values
  if(!checkValues()) {
    return;
  }

  QString name = QString::fromLatin1("custom") + QString::number(m_newFields.count()+1);
  int count = m_newFields.count() + 1;
  QString title = i18n("New Field") + QString::fromLatin1(" %1").arg(count);
  while(m_fieldsBox->findItem(title)) {
    ++count;
    title = i18n("New Field") + QString::fromLatin1(" %1").arg(count);
  }

  Data::Field* field = new Data::Field(name, title);
  m_newFields.append(field);
//  kdDebug() << "CollectionFieldsDialog::slotNew() - adding new field " << title << endl;

//  m_fieldsBox->insertItem(title);
  ListBoxText* box = new ListBoxText(m_fieldsBox, field);
  m_fieldsBox->setSelected(box, true);
  box->setColored(true);
  m_fieldsBox->ensureCurrentVisible();
  slotModified();
  m_titleEdit->setFocus();
  m_titleEdit->selectAll();
}

void CollectionFieldsDialog::slotDelete() {
  Data::Field* field = m_currentField;
  if(!field) {
    return;
  }

  if(m_newFields.containsRef(field)) {
    m_currentField = 0;
    m_fieldsBox->removeItem(m_fieldsBox->currentItem());
    m_newFields.removeRef(field);
    m_fieldsBox->setSelected(m_fieldsBox->currentItem(), true);
    m_fieldsBox->ensureCurrentVisible();
    delete field;
    return;
  }

  QString str = i18n("<qt><p>Do you really want to delete the <em>%1</em> field? "
                     "This action occurs immediately and can not be undone!</p></qt>").arg(field->title());
  QString dontAsk = QString::fromLatin1("DeleteField");
  int ret = KMessageBox::warningYesNo(this, str, i18n("Delete Field?"),
                                      KStdGuiItem::yes(), KStdGuiItem::no(), dontAsk);
  if(ret == KMessageBox::Yes) {
    bool success = m_coll->deleteField(field);
    if(success) {
      m_currentField = 0;
      emit signalCollectionModified();
      m_fieldsBox->removeItem(m_fieldsBox->currentItem());
      m_fieldsBox->setSelected(m_fieldsBox->currentItem(), true);
      m_fieldsBox->ensureCurrentVisible();
      enableButtonOK(true);
    }
  }
}

void CollectionFieldsDialog::slotTypeChanged(const QString& type_) {
  Data::Field::Type type = Data::Field::Undef;
  for(QMap<Data::Field::Type, QString>::ConstIterator it = m_typeMap.begin(); it != m_typeMap.end(); ++it) {
    if(it.data() == type_) {
      type = it.key();
      break;
    }
  }
  if(type == Data::Field::Undef) {
    kdWarning() << "CollectionFieldsDialog::slotTypeChanged() - type name not recognized: " << type_ << endl;
    type = Data::Field::Line;
  }

  // only lists gets allowed values
  m_allowEdit->setEnabled(type == Data::Field::Choice);

  // paragraphs and tables are their own category
  bool isCategory = (type == Data::Field::Para || type == Data::Field::Table ||
                     type == Data::Field::Table2 || type == Data::Field::Image);
  m_catCombo->setEnabled(!isCategory);

  // formatting is only applicable when the type is simple text or a table
  bool isText = (type == Data::Field::Line || type == Data::Field::Table ||
                 type == Data::Field::Table2);
  // formatNone is the default
  m_formatPlain->setEnabled(isText);
  m_formatName->setEnabled(isText);
  m_formatTitle->setEnabled(isText);

  // multiple is only applicable for simple text and number
  isText = (type == Data::Field::Line || type == Data::Field::Number);
  m_multiple->setEnabled(isText);

  // completion is only applicable for simple text, number, and URL
  isText = (isText || type == Data::Field::URL);
  m_complete->setEnabled(isText);

  // grouping is not possible with paragraphs or images
  m_grouped->setEnabled(type != Data::Field::Para && type != Data::Field::Image);
}

void CollectionFieldsDialog::slotHighlightedChanged(int index_) {
//  kdDebug() << "CollectionFieldsDialog::slotHighlightedChanged() - " << index_ << endl;

  // use this instead of blocking signals everywhere
  m_updatingValues = true;

  // first update the current one with all the values from the edit widgets
  updateField();

  // next check old values
  if(!checkValues()) {
    m_fieldsBox->blockSignals(true);
    m_fieldsBox->setSelected(m_oldIndex, true);
    m_fieldsBox->blockSignals(false);
    m_updatingValues = false;
    return;
  }
  m_oldIndex = index_;

  m_btnUp->setEnabled(index_ > 0);
  m_btnDown->setEnabled(index_ < static_cast<int>(m_fieldsBox->count())-1);

  ListBoxText* item = dynamic_cast<ListBoxText*>(m_fieldsBox->item(index_));
  if(!item) {
    return;
  }

  // need to get a pointer to the field with the new values to insert
  Data::Field* field = item->field();
  if(!field) {
    kdDebug() << "CollectionFieldsDialog::slotHighlightedChanged() - no field found!" << endl;
    return;
  }

  m_titleEdit->setText(field->title());

  // type is limited to certain types, unless it's a new field
  m_typeCombo->clear();
  if(m_newFields.containsRef(field)) {
    m_typeCombo->insertStringList(newTypesAllowed(Data::Field::Undef));
  } else {
    m_typeCombo->insertStringList(newTypesAllowed(field->type()));
  }
  // if the current name is not there, then this will change the list!
  m_typeCombo->setCurrentText(m_typeMap[field->type()]);
  slotTypeChanged(m_typeMap[field->type()]); // just setting the text doesn't emit the activated signal

  if(field->type() == Data::Field::Choice) {
    m_allowEdit->setText(field->allowed().join(QString::fromLatin1("; ")));
  } else {
    m_allowEdit->clear();
  }

  m_catCombo->setCurrentText(field->category()); // have to do this here
  m_descEdit->setText(field->description());

  switch(field->formatFlag()) {
    case Data::Field::FormatNone:
      m_formatNone->setChecked(true);
      break;

    case Data::Field::FormatPlain:
      m_formatPlain->setChecked(true);
      break;

    case Data::Field::FormatTitle:
      m_formatTitle->setChecked(true);
      break;

    case Data::Field::FormatName:
      m_formatName->setChecked(true);
      break;

    default:
      break;
  }

  int flags = field->flags();
  m_complete->setChecked(flags & Data::Field::AllowCompletion);
  m_multiple->setChecked(flags & Data::Field::AllowMultiple);
  m_grouped->setChecked(flags & Data::Field::AllowGrouped);

  m_btnDelete->setEnabled(!(flags & Data::Field::NoDelete));

  // default button is enabled only if default collection contains the field
  if(m_defaultCollection) {
    bool hasField = (m_defaultCollection->fieldByName(field->name()) != 0);
    actionButton(KDialogBase::Default)->setEnabled(hasField);
  }

  m_currentField = field;
  m_updatingValues = false;
}

void CollectionFieldsDialog::updateField() {
//  kdDebug() << "CollectionFieldsDialog::updateField()" << endl;
  Data::Field* field = m_currentField;
  if(!field || !m_modified) {
    return;
  }

  // only update name if it's one of the new ones
  if(m_newFields.containsRef(field)) {
    // name needs to be a valid XML element name
    QString name = XML::elementName(m_titleEdit->text().lower());
    if(name.isEmpty()) { // might end up with empty string
      name = QString::fromLatin1("custom") + QString::number(m_newFields.count()+1);
    }
    while(m_coll->fieldByName(name)) { // ensure name uniqueness
      name += QString::fromLatin1("-new");
    }
    field->setName(name);
  }

  QString title = m_titleEdit->text().simplifyWhiteSpace();
  slotUpdateTitle(title);

  QMap<Data::Field::Type, QString>::Iterator it;
  for(it = m_typeMap.begin(); it != m_typeMap.end(); ++it) {
    if(it.data() == m_typeCombo->currentText()) {
      field->setType(it.key());
      break;
    }
  }

  if(field->type() == Data::Field::Choice) {
    const QRegExp rx(QString::fromLatin1("\\s*;\\s*"));
    field->setAllowed(QStringList::split(rx, m_allowEdit->text()));
  }

  if(field->isSingleCategory()) {
    field->setCategory(field->title());
  } else {
    QString category = m_catCombo->currentText().simplifyWhiteSpace();
    field->setCategory(category);
    m_catCombo->setCurrentItem(category, true); // if it doesn't exist, it's added
  }

  field->setDescription(m_descEdit->text());

  if(m_formatTitle->isChecked()) {
    field->setFormatFlag(Data::Field::FormatTitle);
  } else if(m_formatName->isChecked()) {
    field->setFormatFlag(Data::Field::FormatName);
  } else if(m_formatPlain->isChecked()) {
    field->setFormatFlag(Data::Field::FormatPlain);
  } else {
    field->setFormatFlag(Data::Field::FormatNone);
  }

  int flags = 0;
  if(m_complete->isChecked()) {
    flags |= Data::Field::AllowCompletion;
  }
  if(m_grouped->isChecked()) {
    flags |= Data::Field::AllowGrouped;
  }
  if(m_multiple->isChecked()) {
    flags |= Data::Field::AllowMultiple;
  }
  field->setFlags(flags);

  m_modified = false;
}

// The purpose here is to first set the modified flag. Then, if the field being edited is one
// that exists in the collection already, a deep copy needs to be made.
void CollectionFieldsDialog::slotModified() {
//  kdDebug() << "CollectionFieldsDialog::slotModified()" << endl;
  // if I'm just updating the values, I don't care
  if(m_updatingValues) {
    return;
  }

  m_modified = true;

  enableButtonOK(true);
  enableButtonApply(true);

  if(!m_currentField) {
    kdDebug() << "CollectionFieldsDialog::slotModified() - no current field!" << endl;
    m_currentField = static_cast<ListBoxText*>(m_fieldsBox->selectedItem())->field();
  }

  // color the text
  static_cast<ListBoxText*>(m_fieldsBox->selectedItem())->setColored(true);

  // check if copy exists already
  if(m_copiedFields.containsRef(m_currentField)) {
    return;
  }

  // or, check if is a new field, in which case no copy is needed
  // check if copy exists already
  if(m_newFields.containsRef(m_currentField)) {
    return;
  }

  Data::Field* newField = m_currentField->clone();

  m_copiedFields.append(newField);
  m_currentField = newField;
  static_cast<ListBoxText*>(m_fieldsBox->selectedItem())->setField(newField);
}

void CollectionFieldsDialog::slotUpdateTitle(const QString& title_) {
//  kdDebug() << "CollectionFieldsDialog::slotUpdateTitle()" << endl;
  if(m_currentField && m_currentField->title() != title_) {
    m_fieldsBox->blockSignals(true);
    ListBoxText* oldItem = findItem(m_fieldsBox, m_currentField);
    if(!oldItem) {
      return;
    }
    oldItem->setText(title_);
    // will always be colored since it's new
    oldItem->setColored(true);
    m_fieldsBox->triggerUpdate(true);

    m_currentField->setTitle(title_);
    m_fieldsBox->blockSignals(false);
  }
}

void CollectionFieldsDialog::slotDefault() {
  if(!m_currentField) {
    return;
  }

  Data::Field* defaultField = m_defaultCollection->fieldByName(m_currentField->name());
  if(!defaultField) {
    return;
  }

  QString caption = i18n("Revert Field Properties?");
  QString text = i18n("<qt><p>Do you really want to revert the properties for the <em>%1</em> "
                      "field back to their default values?</p></qt>").arg(m_currentField->title());
  QString dontAsk = QString::fromLatin1("RevertFieldProperties");
  int ret = KMessageBox::warningContinueCancel(this, text, caption, i18n("Revert"), dontAsk);
  if(ret != KMessageBox::Continue) {
    return;
  }

  // now update all values with default
  m_updatingValues = true;
  m_titleEdit->setText(defaultField->title());

  m_typeCombo->setCurrentText(m_typeMap[defaultField->type()]);
  slotTypeChanged(m_typeMap[defaultField->type()]); // just setting the text doesn't emit the activated signal

  if(defaultField->type() == Data::Field::Choice) {
    m_allowEdit->setText(defaultField->allowed().join(QString::fromLatin1("; ")));
  } else {
    m_allowEdit->clear();
  }

  m_catCombo->setCurrentText(defaultField->category()); // have to do this here
  m_descEdit->setText(defaultField->description());
//  m_bibtexEdit->setText(defaultField->property(QString::fromLatin1("bibtex")));

  switch(defaultField->formatFlag()) {
    case Data::Field::FormatNone:
      m_formatNone->setChecked(true);
      break;

    case Data::Field::FormatPlain:
      m_formatPlain->setChecked(true);
      break;

    case Data::Field::FormatTitle:
      m_formatTitle->setChecked(true);
      break;

    case Data::Field::FormatName:
      m_formatName->setChecked(true);
      break;

    default:
      break;
  }

  int flags = defaultField->flags();
  m_complete->setChecked(flags & Data::Field::AllowCompletion);
  m_multiple->setChecked(flags & Data::Field::AllowMultiple);
  m_grouped->setChecked(flags & Data::Field::AllowGrouped);

  m_btnDelete->setEnabled(!(defaultField->flags() & Data::Field::NoDelete));

  // type is only changeable for new attributes
  m_typeCombo->setEnabled(false);

//  m_titleEdit->setFocus();
//  m_titleEdit->selectAll();

  m_updatingValues = false;

  slotModified();
}

void CollectionFieldsDialog::slotMoveUp() {
  QListBoxItem* item;
  item = m_fieldsBox->selectedItem();
  if(item) {
    ListBoxText* prev = dynamic_cast<ListBoxText*>(item->prev()); // could be 0
    if(prev) {
      (void) new ListBoxText(m_fieldsBox, prev->field(), item);
      delete prev;
      m_fieldsBox->ensureCurrentVisible();
      // since the current one doesn't get re-highlighted, need to highlighted doesn't get emitted
      slotHighlightedChanged(m_fieldsBox->currentItem());
    }
  }
  m_reordered = true;
  // don't call slotModified() since that creates a deep copy.
  m_modified = true;

  enableButtonOK(true);
  enableButtonApply(true);
}

void CollectionFieldsDialog::slotMoveDown() {
  ListBoxText* item = dynamic_cast<ListBoxText*>(m_fieldsBox->selectedItem());
  if(item) {
    QListBoxItem* next = item->next(); // could be 0
    if(next) {
      QListBoxItem* newItem = new ListBoxText(m_fieldsBox, item->field(), next);
      delete item;
      m_fieldsBox->setSelected(newItem, true);
      m_fieldsBox->ensureCurrentVisible();
    }
  }
  m_reordered = true;
  // don't call slotModified() since that creates a deep copy.
  m_modified = true;

  enableButtonOK(true);
  enableButtonApply(true);
}

ListBoxText* CollectionFieldsDialog::findItem(const QListBox* box_, const Data::Field* field_) {
//  kdDebug() << "CollectionFieldsDialog::findItem()" << endl;
  for(QListBoxItem* item = box_->firstItem(); item; item = item->next()) {
    ListBoxText* textItem = static_cast<ListBoxText*>(item);
    if(textItem->field() == field_) {
      return textItem;
    }
  }
  return 0;
}

void CollectionFieldsDialog::slotShowExtendedProperties() {
  if(!m_currentField) {
    return;
  }

  StringMapDialog* dlg = new StringMapDialog(m_currentField->propertyList(), this, "ExtendedPropertiesDialog", true);
  dlg->setCaption(i18n("Extended Field Properties"));
  dlg->setLabels(i18n("Property"), i18n("Value"));
  if(dlg->exec() == QDialog::Accepted) {
    m_currentField->setPropertyList(dlg->stringMap());
    slotModified();
  }
  dlg->delayedDestruct();
}

bool CollectionFieldsDialog::checkValues() {
  if(!m_currentField) {
    return true;
  }

  const QString& title = m_currentField->title();
  if(m_coll->fieldByTitle(title) && m_coll->fieldNameByTitle(title) != m_currentField->name()) {
    // already have a field with this title
    KMessageBox::sorry(this, i18n("A field with this title already exists. Please enter a different title."));
    m_titleEdit->selectAll();
    return false;
  }

  const QString& category = m_currentField->category();
  Data::FieldList list = m_coll->fieldsByCategory(category);
  if(!list.isEmpty() && list.getFirst()->isSingleCategory() && list.getFirst()->name() != m_currentField->name()) {
    // can't have this category, cause it conflicts with a single-category field
    KMessageBox::sorry(this, i18n("<qt>A field may not be in the same category as a <em>Paragraph</em>, "
                                  "<em>Table</em> or <em>Image</em> field. Please enter a different category.</qt>"));
    m_catCombo->lineEdit()->selectAll();
    return false;
  }

  // the combobox is disabled for single-category fields
  if(!m_catCombo->isEnabled() && m_coll->fieldByTitle(title) && m_coll->fieldNameByTitle(title) != m_currentField->name()) {
    KMessageBox::sorry(this, i18n("A field's title may not be the same as an existing category. "
                                  "Please enter a different title."));
    m_titleEdit->selectAll();
    return false;
  }

  return true;
}

// only certain type changes are allowed
QStringList CollectionFieldsDialog::newTypesAllowed(Data::Field::Type type_ /*=0*/) {
  // Undef means return all
  if(type_ == Data::Field::Undef) {
    return Data::Field::typeTitles();
  }

  QStringList newTypes;
  switch(type_) {
    case Data::Field::Line: // might not work if converted to a number or URL, but ok
    case Data::Field::Number:
    case Data::Field::URL:
      newTypes += m_typeMap[Data::Field::Line];
      newTypes += m_typeMap[Data::Field::Para];
      newTypes += m_typeMap[Data::Field::Number];
      newTypes += m_typeMap[Data::Field::URL];
      newTypes += m_typeMap[Data::Field::Table];
      newTypes += m_typeMap[Data::Field::Table2];
      break;

    case Data::Field::Date:
      newTypes += m_typeMap[Data::Field::Line];
      newTypes += m_typeMap[Data::Field::Date];
      break;

    case Data::Field::Bool: // doesn't really make sense, but can't hurt
      newTypes += m_typeMap[Data::Field::Line];
      newTypes += m_typeMap[Data::Field::Para];
      newTypes += m_typeMap[Data::Field::Bool];
      newTypes += m_typeMap[Data::Field::Number];
      newTypes += m_typeMap[Data::Field::URL];
      newTypes += m_typeMap[Data::Field::Table];
      newTypes += m_typeMap[Data::Field::Table2];
      break;

    case Data::Field::Choice:
      newTypes += m_typeMap[Data::Field::Line];
      newTypes += m_typeMap[Data::Field::Para];
      newTypes += m_typeMap[Data::Field::Choice];
      newTypes += m_typeMap[Data::Field::Number];
      newTypes += m_typeMap[Data::Field::URL];
      newTypes += m_typeMap[Data::Field::Table];
      newTypes += m_typeMap[Data::Field::Table2];
      break;

    case Data::Field::Table:
      newTypes += m_typeMap[Data::Field::Line];
      newTypes += m_typeMap[Data::Field::Number];
      newTypes += m_typeMap[Data::Field::Table];
      newTypes += m_typeMap[Data::Field::Table2];
      break;

    case Data::Field::Table2: // not really a good idea since the "::" will be exposed, but allow it
      newTypes += m_typeMap[Data::Field::Line];
      newTypes += m_typeMap[Data::Field::Number];
      newTypes += m_typeMap[Data::Field::Table];
      newTypes += m_typeMap[Data::Field::Table2];
      break;

    case Data::Field::Para:
      newTypes += m_typeMap[Data::Field::Line];
      newTypes += m_typeMap[Data::Field::Para];
      break;

    // these can never be changed
    case Data::Field::Image:
    case Data::Field::Dependent:
      newTypes = m_typeMap[static_cast<Data::Field::Type>(type_)];
      break;

    default:
      kdDebug() << "CollectionFieldsDialog::newTypesAllowed() - no match for " << type_ << endl;
      newTypes = Data::Field::typeTitles();
      break;
  }
  return newTypes;
}

#include "collectionfieldsdialog.moc"
