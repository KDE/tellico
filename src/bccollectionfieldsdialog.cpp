/***************************************************************************
                         bccollectionfieldsdialog.cpp
                             -------------------
    begin                : Thu Apr 3 2003
    copyright            : (C) 2003 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "bccollectionfieldsdialog.h"
#include "bccollection.h"
#include "bccollectionfactory.h"
#include "collections/bibtexattribute.h"

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

BCListBoxText::BCListBoxText(QListBox* listbox_, BCAttribute* att_)
    : QListBoxText(listbox_, att_->title()), m_attribute(att_), m_colored(false) {
}

BCListBoxText::BCListBoxText(QListBox* listbox_, BCAttribute* att_, QListBoxItem* after_)
    : QListBoxText(listbox_, att_->title(), after_), m_attribute(att_), m_colored(false) {
}

void BCListBoxText::setColored(bool colored_) {
  if(m_colored != colored_) {
    m_colored = colored_;
    listBox()->triggerUpdate(false);
  }
}

void BCListBoxText::setText(const QString& text_) {
  QListBoxText::setText(text_);
  listBox()->triggerUpdate(true);
}

// mostly copied from QListBoxText::paint() in Qt 3.1.1
void BCListBoxText::paint(QPainter* painter_) {
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

BCCollectionFieldsDialog::BCCollectionFieldsDialog(BCCollection* coll_, QWidget* parent_, const char* name_/*=0*/)
    : KDialogBase(parent_, name_, false, i18n("Collection Fields"), Default|Ok|Apply|Cancel, Ok, false),
      m_coll(coll_),
      m_defaultCollection(0),
      m_typeMap(BCAttribute::typeMap()),
      m_currentAttribute(0),
      m_modified(false),
      m_updatingValues(false),
      m_reordered(false) {
  QWidget* page = new QWidget(this);
  setMainWidget(page);
  QHBoxLayout* topLayout = new QHBoxLayout(page, 0, KDialog::spacingHint());

  QGroupBox* fieldsGroup = new QGroupBox(1, Qt::Horizontal, i18n("Current Fields"), page);
  topLayout->addWidget(fieldsGroup, 1);
  m_fieldsBox = new QListBox(fieldsGroup);
  m_fieldsBox->setMinimumWidth(150);
  BCAttributeListIterator it(m_coll->attributeList());
  for( ; it.current(); ++it) {
    // ignore ReadOnly
    if(it.current()->type() != BCAttribute::ReadOnly) {
      (void) new BCListBoxText(m_fieldsBox, it.current());
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
  whats = i18n("<qt>The type of the field determines what values may be used. <i>Simple Text</i> "
               "is used for most fields. <i>Paragraph</i> is for large text blocks. "
               "<i>List</i> limits the field to certain values. <i>Checkbox</i> is for "
               "a simple yes/no value. <i>Number</i> indicates that the field contains a "
               "numerical value. <i>URL</i> is for fields which refer to URLs, including "
               "references to other files. <i>Table</i>s may be a single or double column of values, "
               "while <i>Read Only</i> is for internal values, possibly useful for import and export.</qt>");
  QWhatsThis::add(label, whats);
  QWhatsThis::add(m_typeCombo, whats);
  m_typeCombo->insertStringList(m_typeMap.values());
  connect(m_typeCombo, SIGNAL(activated(int)), SLOT(slotModified()));
  connect(m_typeCombo, SIGNAL(activated(const QString&)), SLOT(slotTypeChanged(const QString&)));

  label = new QLabel(i18n("Category:"), grid);
  layout->addWidget(label, 1, 0);
  m_catCombo = new KComboBox(true, grid);
  layout->addWidget(m_catCombo, 1, 1);
  whats = i18n("The field category determines where the field is placed in the editor.");
  QWhatsThis::add(label, whats);
  QWhatsThis::add(m_catCombo, whats);
  m_catCombo->insertStringList(m_coll->attributeCategories());
  m_catCombo->setDuplicatesEnabled(false);
  connect(m_catCombo, SIGNAL(textChanged(const QString&)), SLOT(slotModified()));

  label = new QLabel(i18n("Allowed:"), grid);
  layout->addWidget(label, 2, 0);
  m_allowEdit = new KLineEdit(grid);
  layout->addMultiCellWidget(m_allowEdit, 2, 2, 1, 3);
  whats = i18n("<qt>For <i>List</i>-type fields, these are the only values allowed. They are "
               "placed in a combo box.</qt>");
  QWhatsThis::add(label, whats);
  QWhatsThis::add(m_allowEdit, whats);
  connect(m_allowEdit, SIGNAL(textChanged(const QString&)), SLOT(slotModified()));

  label = new QLabel(i18n("Description:"), grid);
  layout->addWidget(label, 3, 0);
  m_descEdit = new KLineEdit(grid);
  m_descEdit->setMinimumWidth(150);
  layout->addMultiCellWidget(m_descEdit, 3, 3, 1, 3);
  whats = i18n("The description is useful reminder of what information is contained in the field.");
  QWhatsThis::add(label, whats);
  QWhatsThis::add(m_descEdit, whats);
  connect(m_descEdit, SIGNAL(textChanged(const QString&)), SLOT(slotModified()));

  label = new QLabel(i18n("Bibtex Field Name:"), grid);
  layout->addWidget(label, 4, 0);
  m_bibtexEdit = new KLineEdit(grid);
  layout->addMultiCellWidget(m_bibtexEdit, 4, 4, 1, 3);
  whats = i18n("The Bibtex field name determines the entry name for exporting to "
               "bibliogrgaphy files.");
  QWhatsThis::add(label, whats);
  QWhatsThis::add(m_bibtexEdit, whats);
  if(m_coll->collectionType() == BCCollection::Bibtex) {
    connect(m_bibtexEdit, SIGNAL(textChanged(const QString&)), SLOT(slotModified()));
  } else {
    label->hide();
    m_bibtexEdit->hide();
  }

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
  QWhatsThis::add(m_multiple, i18n("If checked, Bookcase will parse the values in the field "
                                   "for multiple values, separated by a semi-colon."));
  m_grouped = new QCheckBox(i18n("Allow grouping"), optionsGroup);
  QWhatsThis::add(m_grouped, i18n("If checked, this field may be used to group the entries in "
                                  "the group view."));
  connect(m_complete, SIGNAL(clicked()), SLOT(slotModified()));
  connect(m_multiple, SIGNAL(clicked()), SLOT(slotModified()));
  connect(m_grouped, SIGNAL(clicked()), SLOT(slotModified()));

  // need to stretch at bottom
  QWidget* dummy = new QWidget(vbox);
  vbox->setStretchFactor(dummy, 1);

  // keep a default collection
  m_defaultCollection = BCCollectionFactory::collection(m_coll->collectionType(), true);

  QWhatsThis::add(actionButton(KDialogBase::Default),
                  i18n("Revert the selected field's properties to the default values."));

  enableButtonOK(false);
  enableButtonApply(false);

  m_fieldsBox->setSelected(0, true);
  // need to call this since the text isn't actually changed when it's set initially
  slotTypeChanged(m_typeCombo->currentText());
}

BCCollectionFieldsDialog::~BCCollectionFieldsDialog() {
  delete m_defaultCollection;
  m_defaultCollection = 0;
}

void BCCollectionFieldsDialog::slotOk() {
  slotApply();
  accept();
}

void BCCollectionFieldsDialog::slotApply() {
  updateAttribute();

  BCAttributeListIterator it1(m_copiedAttributes);
  for( ; it1.current(); ++it1) {
    m_coll->modifyAttribute(it1.current());
    // also must reset attribute pointers in fieldbox items
    for(QListBoxItem* item = m_fieldsBox->firstItem(); item; item = item->next()) {
      BCAttribute* att = static_cast<BCListBoxText*>(item)->attribute();
      if(att && att == it1.current()) {
        static_cast<BCListBoxText*>(item)->setAttribute(m_coll->attributeByName(att->name()));
        break;
      }
    }
  }

  BCAttributeListIterator it2(m_newAttributes);
  for( ; it2.current(); ++it2) {
    m_coll->addAttribute(it2.current());
  }

  // set all text not to be colored, and get new list
  BCAttributeList list;
  for(QListBoxItem* item = m_fieldsBox->firstItem(); item; item = item->next()) {
    static_cast<BCListBoxText*>(item)->setColored(false);
    if(m_reordered) {
      BCAttribute* att = static_cast<BCListBoxText*>(item)->attribute();
      if(att) {
        list.append(att);
      }
    }
  }

  if(list.count() > 0) {
    m_coll->reorderAttributes(list);
  }

  if(m_newAttributes.count() > 0 || m_copiedAttributes.count() > 0 || m_reordered) {
    emit signalCollectionModified();
  }

  // now clear copied attributes
  m_copiedAttributes.setAutoDelete(true);
  m_copiedAttributes.clear();
  m_copiedAttributes.setAutoDelete(false);
  // clear new ones, too
  m_newAttributes.clear();

  enableButtonApply(false);
}

void BCCollectionFieldsDialog::slotNew() {
  updateAttribute();

  QString name = QString::fromLatin1("custom") + QString::number(m_newAttributes.count()+1);
  int count = m_newAttributes.count() + 1;
  QString title = i18n("New Field") + QString::fromLatin1(" %1").arg(count);
  while(m_fieldsBox->findItem(title)) {
    ++count;
    title = i18n("New Field") + QString::fromLatin1(" %1").arg(count);
  }

  BCAttribute* att;
  if(m_coll->collectionType() == BCCollection::Bibtex) {
    att = new BibtexAttribute(name, title);
  } else {
    att = new BCAttribute(name, title);
  }
  m_newAttributes.append(att);
//  kdDebug() << "BCCollectionFieldsDialog::slotNew() - adding new attribute " << title << endl;

//  m_fieldsBox->insertItem(title);
  BCListBoxText* box = new BCListBoxText(m_fieldsBox, att);
  m_fieldsBox->setSelected(box, true);
  box->setColored(true);
  m_fieldsBox->ensureCurrentVisible();
  slotModified();
  m_titleEdit->setFocus();
  m_titleEdit->selectAll();
}

void BCCollectionFieldsDialog::slotDelete() {
  BCAttribute* att = m_currentAttribute;
  if(!att) {
    return;
  }

  if(m_newAttributes.containsRef(att)) {
    m_currentAttribute = 0;
    m_fieldsBox->removeItem(m_fieldsBox->currentItem());
    m_newAttributes.removeRef(att);
    m_fieldsBox->setSelected(m_fieldsBox->currentItem(), true);
    m_fieldsBox->ensureCurrentVisible();
    return;
  }

  QString str = i18n("<qt><p>Do you really want to delete the <em>%1</em> field? "
                     "This action occurs immediately and can not be undone!</p></qt>").arg(att->title());
  QString dontAsk = QString::fromLatin1("DeleteField");
  int ret = KMessageBox::warningYesNo(this, str, i18n("Delete Field?"),
                                      KStdGuiItem::yes(), KStdGuiItem::no(), dontAsk);
  if(ret != KMessageBox::Yes) {
    return;
  } else {
    m_coll->deleteAttribute(att);
    emit signalCollectionModified();
    m_fieldsBox->removeItem(m_fieldsBox->currentItem());
    m_fieldsBox->setSelected(m_fieldsBox->currentItem(), true);
    m_fieldsBox->ensureCurrentVisible();
    enableButtonOK(true);
  }
}

void BCCollectionFieldsDialog::slotTypeChanged(const QString& type_) {
  m_allowEdit->setEnabled(type_ == i18n("List"));
  // paragraphs and tables are their own category
  m_catCombo->setEnabled(type_ != i18n("Paragraph") &&
                         type_ != i18n("Table") &&
                         type_ != i18n("Table (2 Columns)"));
  // formatting is only applicable when the type is simple text or a table
  bool isText = (type_ == i18n("Simple Text") ||
                 type_ == i18n("Table") ||
                 type_ == i18n("Table (2 Columns)"));
  // formatNone is the default
  m_formatPlain->setEnabled(isText);
  m_formatName->setEnabled(isText);
  m_formatTitle->setEnabled(isText);
  // multiple is only applicable for simple text and number
  isText = (isText || type_ == i18n("Number"));
  m_multiple->setEnabled(isText && type_ != i18n("Table") && type_ != i18n("Table (2 Columns)"));
  // completion is only applicable for simple text, year, and URL
  isText = isText || (type_ == i18n("URL"));
  m_complete->setEnabled(isText);
  // grouping is not possible with paragraphs
  m_grouped->setEnabled(type_ != i18n("Paragraph"));
}

void BCCollectionFieldsDialog::slotHighlightedChanged(int index_) {
//  kdDebug() << "BCCollectionFieldsDialog::slotHighlightedChanged() - " << index_ << endl;

  m_btnUp->setEnabled(index_ > 0);
  m_btnDown->setEnabled(index_ < static_cast<int>(m_fieldsBox->count())-1);

  // use this instead of blocking signals everywhere
  m_updatingValues = true;

  // first update the current one with all the values from the edit widgets
  updateAttribute();

  BCListBoxText* item;
#if QT_VERSION >= 0x030100
  item = dynamic_cast<BCListBoxText*>(m_fieldsBox->selectedItem());
#else
  item = dynamic_cast<BCListBoxText*>(m_fieldsBox->item(m_fieldsBox->currentItem()));
#endif

  if(!item) {
    return;
  }

  // need to get a pointer to the attribute with the new values to insert
  BCAttribute* att = item->attribute();

  if(!att) {
    kdDebug() << "BCCollectionFieldsDialog::slotHighlightedChanged() - no attribute found!" << endl;
    return;
  }

  m_titleEdit->setText(att->title());

  m_typeCombo->setCurrentText(m_typeMap[att->type()]);
  slotTypeChanged(m_typeMap[att->type()]); // just setting the text doesn't emit the activated signal

  if(att->type() == BCAttribute::Choice) {
    m_allowEdit->setText(att->allowed().join(QString::fromLatin1("; ")));
  } else {
    m_allowEdit->clear();
  }

  m_catCombo->setCurrentText(att->category()); // have to do this here
  m_descEdit->setText(att->description());

  if(att->isBibtexAttribute() && dynamic_cast<BibtexAttribute*>(att)) {
    BibtexAttribute* bAtt = dynamic_cast<BibtexAttribute*>(att);
    m_bibtexEdit->setText(bAtt->bibtexFieldName());
  } else {
    m_bibtexEdit->clear();
  }

  switch(att->formatFlag()) {
    case BCAttribute::FormatNone:
      m_formatNone->setChecked(true);
      break;

    case BCAttribute::FormatPlain:
      m_formatPlain->setChecked(true);
      break;

    case BCAttribute::FormatTitle:
      m_formatTitle->setChecked(true);
      break;

    case BCAttribute::FormatName:
      m_formatName->setChecked(true);
      break;

    default:
      break;
  }

  int flags = att->flags();
  m_complete->setChecked(flags & BCAttribute::AllowCompletion);

  m_multiple->setChecked(flags & BCAttribute::AllowMultiple);
  m_multiple->setEnabled(att->name() != QString::fromLatin1("title"));

  m_grouped->setChecked(flags & BCAttribute::AllowGrouped);

  m_btnDelete->setEnabled(!(att->flags() & BCAttribute::NoDelete));

  // default button is enabled only if default collection contains the attribute
  if(m_defaultCollection) {
    bool hasAttribute = (m_defaultCollection->attributeByName(att->name()) != 0);
    actionButton(KDialogBase::Default)->setEnabled(hasAttribute);
  }

  // type is only changeable for new attributes
  m_typeCombo->setEnabled(m_newAttributes.containsRef(att));

  m_currentAttribute = att;
  m_updatingValues = false;
}

void BCCollectionFieldsDialog::updateAttribute() {
//  kdDebug() << "BCCollectionFieldsDialog::updateAttribute()" << endl;
  BCAttribute* att = m_currentAttribute;
  if(!att || !m_modified) {
    return;
  }

  // only update name if it's one of the new ones
  if(m_newAttributes.containsRef(att)) {
    // name needs to be a valid XML element name
    QString name = m_titleEdit->text().lower();
    // remove white space
    name.replace(QRegExp(QString::fromLatin1("\\s+")), QString::fromLatin1("-"));
    // replace non word characters, but allow dashes
    name.replace(QRegExp(QString::fromLatin1("[^\\w-]")), QString::null);
    while(name.at(0).isDigit() || name.at(0) == '-') {
      name = name.mid(1);
    }
    if(name.isEmpty()) { // might end up with empty string
      name = QString::fromLatin1("custom") + QString::number(m_newAttributes.count()+1);
    }
    while(m_coll->attributeByName(name)) { // ensure name uniqueness
      name += QString::fromLatin1("-new");
    }
    att->setName(name);
  }

  QString title = m_titleEdit->text().simplifyWhiteSpace();
  slotUpdateTitle(title);

  QMap<BCAttribute::AttributeType, QString>::Iterator it;
  for(it = m_typeMap.begin(); it != m_typeMap.end(); ++it) {
    if(it.data() == m_typeCombo->currentText()) {
      att->setType(it.key());
    }
  }

  if(att->type() == BCAttribute::Choice) {
    QRegExp rx(QString::fromLatin1("\\s*;\\s*"));
    att->setAllowed(QStringList::split(rx, m_allowEdit->text()));
  }

  if(!att->isSingleCategory()) {
    QString category = m_catCombo->currentText();
    att->setCategory(category);
    m_catCombo->setCurrentItem(category, true); // if it doesn't exist, it's added
  }

  att->setDescription(m_descEdit->text());

  if(att->isBibtexAttribute()) {
    BibtexAttribute* bAtt = dynamic_cast<BibtexAttribute*>(att);
    if(bAtt) {
      bAtt->setBibtexFieldName(m_bibtexEdit->text());
    }
  }

  if(m_formatTitle->isChecked()) {
    att->setFormatFlag(BCAttribute::FormatTitle);
  } else if(m_formatName->isChecked()) {
    att->setFormatFlag(BCAttribute::FormatName);
  } else if(m_formatPlain->isChecked()) {
    att->setFormatFlag(BCAttribute::FormatPlain);
  } else {
    att->setFormatFlag(BCAttribute::FormatNone);
  }

  int flags = 0;
  if(m_complete->isChecked()) {
    flags |= BCAttribute::AllowCompletion;
  }
  if(m_grouped->isChecked()) {
    flags |= BCAttribute::AllowGrouped;
  }
  if(m_multiple->isChecked()) {
    flags |= BCAttribute::AllowMultiple;
  }
  att->setFlags(flags);

  m_modified = false;
}

// The purpose here is to first set the modified flag. Then, if the attribute being edited is one
// that exists in the collection already, a deep copy needs to be made.
void BCCollectionFieldsDialog::slotModified() {
//  kdDebug() << "BCCollectionFieldsDialog::slotModified()" << endl;
  // if I'm just updating the values, I don't care
  if(m_updatingValues) {
    return;
  }

  m_modified = true;

  enableButtonOK(true);
  enableButtonApply(true);

  // color the text
#if QT_VERSION >= 0x030100
  static_cast<BCListBoxText*>(m_fieldsBox->selectedItem())->setColored(true);
#else
  static_cast<BCListBoxText*>(m_fieldsBox->item(m_fieldsBox->currentItem()))->setColored(true);
#endif

  // check if copy exists already
  if(m_copiedAttributes.containsRef(m_currentAttribute)) {
    return;
  }

  // or, check if is a new attribute, in which case no copy is needed
  // check if copy exists already
  if(m_newAttributes.containsRef(m_currentAttribute)) {
    return;
  }

  BCAttribute* newAtt = m_currentAttribute->clone();

  m_copiedAttributes.append(newAtt);
  m_currentAttribute = newAtt;
#if QT_VERSION >= 0x030100
  static_cast<BCListBoxText*>(m_fieldsBox->selectedItem())->setAttribute(newAtt);
#else
  static_cast<BCListBoxText*>(m_fieldsBox->item(m_fieldsBox->currentItem()))->setAttribute(newAtt);
#endif
}

void BCCollectionFieldsDialog::slotUpdateTitle(const QString& title_) {
//  kdDebug() << "BCCollectionFieldsDialog::slotUpdateTitle()" << endl;
  if(m_currentAttribute && m_currentAttribute->title() != title_) {
    m_fieldsBox->blockSignals(true);
    BCListBoxText* oldItem = findItem(m_fieldsBox, m_currentAttribute);
    if(!oldItem) {
      return;
    }
    oldItem->setText(title_);
    // will always be colored since it's new
    oldItem->setColored(true);
    m_fieldsBox->triggerUpdate(true);

    m_currentAttribute->setTitle(title_);
    m_fieldsBox->blockSignals(false);
  }
}

void BCCollectionFieldsDialog::slotDefault() {
  if(!m_currentAttribute) {
    return;
  }

  BCAttribute* defaultAttribute = m_defaultCollection->attributeByName(m_currentAttribute->name());
  if(!defaultAttribute) {
    return;
  }

  QString caption = i18n("Revert Field Properties?");
  QString text = i18n("<qt><p>Do you really want to revert the properties for the <em>%1</em> "
                      "field back to their default values?</p></qt>").arg(m_currentAttribute->title());
  QString dontAsk = QString::fromLatin1("RevertFieldProperties");
  int ret = KMessageBox::warningContinueCancel(this, text, caption, i18n("Revert"), dontAsk);
  if(ret != KMessageBox::Continue) {
    return;
  }

  // now update all values with default
  m_updatingValues = true;
  m_titleEdit->setText(defaultAttribute->title());

  m_typeCombo->setCurrentText(m_typeMap[defaultAttribute->type()]);
  slotTypeChanged(m_typeMap[defaultAttribute->type()]); // just setting the text doesn't emit the activated signal

  if(defaultAttribute->type() == BCAttribute::Choice) {
    m_allowEdit->setText(defaultAttribute->allowed().join(QString::fromLatin1("; ")));
  } else {
    m_allowEdit->clear();
  }

  m_catCombo->setCurrentText(defaultAttribute->category()); // have to do this here
  m_descEdit->setText(defaultAttribute->description());

  if(defaultAttribute->isBibtexAttribute()) {
    BibtexAttribute* batt = dynamic_cast<BibtexAttribute*>(defaultAttribute);
    if(batt) {
      m_bibtexEdit->setText(batt->bibtexFieldName());
    }
  }

  switch(defaultAttribute->formatFlag()) {
    case BCAttribute::FormatNone:
      m_formatNone->setChecked(true);
      break;

    case BCAttribute::FormatPlain:
      m_formatPlain->setChecked(true);
      break;

    case BCAttribute::FormatTitle:
      m_formatTitle->setChecked(true);
      break;

    case BCAttribute::FormatName:
      m_formatName->setChecked(true);
      break;

    default:
      break;
  }

  int flags = defaultAttribute->flags();
  m_complete->setChecked(flags & BCAttribute::AllowCompletion);
  m_multiple->setChecked(flags & BCAttribute::AllowMultiple);
  m_grouped->setChecked(flags & BCAttribute::AllowGrouped);

  m_btnDelete->setEnabled(!(defaultAttribute->flags() & BCAttribute::NoDelete));

  // type is only changeable for new attributes
  m_typeCombo->setEnabled(false);

//  m_titleEdit->setFocus();
//  m_titleEdit->selectAll();

  m_updatingValues = false;

  slotModified();
}

void BCCollectionFieldsDialog::slotMoveUp() {
  QListBoxItem* item;
#if QT_VERSION >= 0x030100
  item = m_fieldsBox->selectedItem();
#else
  item = m_fieldsBox->item(m_fieldsBox->currentItem());
#endif
  if(item) {
    BCListBoxText* prev = dynamic_cast<BCListBoxText*>(item->prev()); // could be 0
    if(prev) {
      (void) new BCListBoxText(m_fieldsBox, prev->attribute(), item);
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

void BCCollectionFieldsDialog::slotMoveDown() {
  BCListBoxText* item;
#if QT_VERSION >= 0x030100
  item = dynamic_cast<BCListBoxText*>(m_fieldsBox->selectedItem());
#else
  item = dynamic_cast<BCListBoxText*>(m_fieldsBox->item(m_fieldsBox->currentItem()));
#endif
  if(item) {
    QListBoxItem* next = item->next(); // could be 0
    if(next) {
      QListBoxItem* newItem = new BCListBoxText(m_fieldsBox, item->attribute(), next);
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

BCListBoxText* BCCollectionFieldsDialog::findItem(const QListBox* box_, const BCAttribute* att_) {
//  kdDebug() << "BCCollectionFieldsDialog::findItem()" << endl;
  for(QListBoxItem* item = box_->firstItem(); item; item = item->next()) {
    BCListBoxText* textItem = static_cast<BCListBoxText*>(item);
    if(textItem->attribute() == att_) {
      return textItem;
    }
  }
  return 0;
}
