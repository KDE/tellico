/***************************************************************************
                          bccollectionpropdialog.cpp
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

#include "bccollectionpropdialog.h"
#include "bccollection.h"

#include <klocale.h>
#include <kcombobox.h>
#include <klineedit.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <klineeditdlg.h>

#include <qgroupbox.h>
#include <qlayout.h>
#include <qhbox.h>
#include <qvbox.h>
#include <qlabel.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qregexp.h>
#include <qwhatsthis.h>
#include <qpainter.h>

BCListBoxText::BCListBoxText(QListBox* listbox_, const QString& text_)
    : QListBoxText(listbox_, text_), m_colored(false) {
}

void BCListBoxText::setColored(bool colored_) {
  if(m_colored != colored_) {
    m_colored = colored_;
    listBox()->triggerUpdate(false);
  }
}

void BCListBoxText::setText(const QString& text_) {
  QListBoxItem::setText(text_);
  listBox()->triggerUpdate(true);
}

// mostly copied from QListBoxText::paint() in Qt 3.1.1
void BCListBoxText::paint(QPainter* painter_) {
  int itemHeight = height(listBox());
  QFontMetrics fm = painter_->fontMetrics();
  int yPos = ((itemHeight - fm.height()) / 2) + fm.ascent();
  if(!isSelected() && m_colored) {
    QFont font = painter_->font();
    font.setBold(true);
    font.setItalic(true);
    painter_->setFont(font);
    painter_->setPen(listBox()->colorGroup().highlight());
  }
  painter_->drawText(3, yPos, text());
}

BCCollectionPropDialog::BCCollectionPropDialog(BCCollection* coll_, QWidget* parent_, const char* name_/*=0*/)
    : KDialogBase(parent_, name_, false, i18n("Edit Collection Fields"), Ok|Apply|Cancel, Ok, false),
      m_coll(coll_), m_typeMap(BCAttribute::typeMap()), m_currentAttribute(0),
      m_modified(false), m_updatingValues(false) {
  QWidget* page = new QWidget(this);
  setMainWidget(page);
  QHBoxLayout* topLayout = new QHBoxLayout(page, 0, KDialog::spacingHint());

  QGroupBox* fieldsGroup = new QGroupBox(1, Qt::Horizontal, i18n("Current Fields"), page);
  topLayout->addWidget(fieldsGroup, 1);
  m_fieldsBox = new QListBox(fieldsGroup);
  m_fieldsBox->setMinimumWidth(150);
  BCAttributeListIterator it(m_coll->attributeList());
  for( ; it.current(); ++it) {
    if(it.current()->type() != BCAttribute::ReadOnly) {
      (void) new BCListBoxText(m_fieldsBox, it.current()->title());
    }
  }
//  m_fieldsBox->insertStringList(m_coll->attributeTitles()); // this will include ReadOnly types! TODO
  connect(m_fieldsBox, SIGNAL(highlighted(const QString&)), SLOT(slotUpdateValues(const QString&)));

  QHBox* hb = new QHBox(fieldsGroup);
  hb->setSpacing(4);
  m_btnNew = new QPushButton(i18n("New Field", "New"), hb);
  m_btnNew->setIconSet(BarIcon(QString::fromLatin1("filenew"), KIcon::SizeSmall));
  m_btnDelete = new QPushButton(i18n("Delete Field", "Delete"), hb);
  m_btnDelete->setIconSet(BarIcon(QString::fromLatin1("editdelete"), KIcon::SizeSmall));

  connect(m_btnNew, SIGNAL(clicked()), SLOT(slotNew()) );
  connect(m_btnDelete, SIGNAL(clicked()), SLOT(slotDelete()));

  QVBox* vbox = new QVBox(page);
  vbox->setSpacing(KDialog::spacingHint());
  topLayout->addWidget(vbox, 2);

  QGroupBox* propGroup = new QGroupBox(1, Qt::Horizontal, i18n("Field Properties"), vbox);

  QWidget* grid = new QWidget(propGroup);
  // (parent, nrows, ncols, margin, spacing)
  QGridLayout* layout = new QGridLayout(grid, 3, 4, 0, KDialog::spacingHint());

  layout->addWidget(new QLabel(i18n("Title:"), grid), 0, 0);
  m_titleEdit = new KLineEdit(grid);
  layout->addWidget(m_titleEdit, 0, 1);
  connect(m_titleEdit, SIGNAL(textChanged(const QString&)), SLOT(slotModified()));

  layout->addWidget(new QLabel(i18n("Type:"), grid), 0, 2);
  m_typeCombo = new KComboBox(grid);
  layout->addWidget(m_typeCombo, 0, 3);
  m_typeCombo->insertStringList(m_typeMap.values());
  connect(m_typeCombo, SIGNAL(activated(int)), SLOT(slotModified()));
  connect(m_typeCombo, SIGNAL(activated(const QString&)), SLOT(slotTypeChanged(const QString&)));

  layout->addWidget(new QLabel(i18n("Category:"), grid), 1, 0);
  m_catCombo = new KComboBox(true, grid);
  layout->addWidget(m_catCombo, 1, 1);
  m_catCombo->insertStringList(m_coll->attributeCategories());
  m_catCombo->setDuplicatesEnabled(false);
  connect(m_catCombo, SIGNAL(textChanged(const QString&)), SLOT(slotModified()));
  QWhatsThis::add(m_titleEdit, i18n("The fields are grouped by common categories for editing."));

  layout->addWidget(new QLabel(i18n("Allowed:"), grid), 2, 0);
  m_allowEdit = new KLineEdit(grid);
  layout->addMultiCellWidget(m_allowEdit, 2, 2, 1, 3);
  connect(m_allowEdit, SIGNAL(textChanged(const QString&)), SLOT(slotModified()));
  QWhatsThis::add(m_titleEdit, i18n("Allowed values for this field are separated by a semi-colon."));

  layout->addWidget(new QLabel(i18n("Description:"), grid), 3, 0);
  m_descEdit = new KLineEdit(grid);
  m_descEdit->setMinimumWidth(150);
  layout->addMultiCellWidget(m_descEdit, 3, 3, 1, 3);
  connect(m_descEdit, SIGNAL(textChanged(const QString&)), SLOT(slotModified()));

  QButtonGroup* bg = new QButtonGroup(1, Qt::Horizontal, i18n("Format Options"), vbox);
  m_formatPlain = new QRadioButton(i18n("No formatting"), bg);
  m_formatTitle = new QRadioButton(i18n("Format as a title"), bg);
  m_formatName = new QRadioButton(i18n("Format as a name"), bg);
  connect(bg, SIGNAL(clicked(int)), SLOT(slotModified()));

  QGroupBox* optionsGroup = new QGroupBox(1, Qt::Horizontal, i18n("Field Options"), vbox);
  m_complete = new QCheckBox(i18n("Enable auto-completion"), optionsGroup);
  m_multiple = new QCheckBox(i18n("Allow multiple values"), optionsGroup);
  m_grouped = new QCheckBox(i18n("Allow grouping"), optionsGroup);
  connect(m_complete, SIGNAL(clicked()), SLOT(slotModified()));
  connect(m_multiple, SIGNAL(clicked()), SLOT(slotModified()));
  connect(m_grouped, SIGNAL(clicked()), SLOT(slotModified()));

  m_fieldsBox->setSelected(0, true);
  // need to call this since the text isn't actually changed when it's set initially
  slotTypeChanged(m_typeCombo->currentText());

  enableButtonOK(false);
  enableButtonApply(false);
}

void BCCollectionPropDialog::slotOk() {
  slotApply();
  accept();
}

void BCCollectionPropDialog::slotApply() {
  updateAttribute();

  QDictIterator<BCAttribute> it1(m_copiedAttributes);
  for( ; it1.current(); ++it1) {
    m_coll->modifyAttribute(it1.current());
  }

  BCAttributeListIterator it2(m_newAttributes);
  for( ; it2.current(); ++it2) {
    m_coll->addAttribute(it2.current());
  }

  // set all text not to be colored
  QListBoxItem* item;
  for(item = m_fieldsBox->firstItem(); item; item = item->next()) {
    static_cast<BCListBoxText*>(item)->setColored(false);
  }
  
  if(m_newAttributes.count() > 0 || m_copiedAttributes.count() > 0) {
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

void BCCollectionPropDialog::slotNew() {
  updateAttribute();

  QString name = QString::fromLatin1("custom") + QString::number(m_newAttributes.count()+1);
  int count = m_newAttributes.count() + 1;
  QString title = i18n("New Field") + QString::fromLatin1(" %1").arg(count);
  while(m_fieldsBox->findItem(title)) {
    ++count;
    title = i18n("New Field") + QString::fromLatin1(" %1").arg(count);
  }

  BCAttribute* att = new BCAttribute(name, title);
  m_newAttributes.append(att);
//  kdDebug() << "BCCollectionPropDialog::slotNew() - adding new attribute " << title << endl;
  
//  m_fieldsBox->insertItem(title);
  BCListBoxText* box = new BCListBoxText(m_fieldsBox, title);
  m_fieldsBox->setSelected(box, true);
  box->setColored(true);
  m_fieldsBox->ensureCurrentVisible();
  slotModified();
  m_titleEdit->setFocus();
  m_titleEdit->selectAll();
}

void BCCollectionPropDialog::slotDelete() {
  BCAttribute* att = m_currentAttribute;
  if(!att) {
    return;
  }
  
  if(m_newAttributes.containsRef(att)) {
    m_fieldsBox->removeItem(m_fieldsBox->currentItem());
    m_newAttributes.removeRef(att);
    m_fieldsBox->setSelected(m_fieldsBox->currentItem(), true);
    m_fieldsBox->ensureCurrentVisible();
    return;
  }
  
  QString str = i18n("Do you really want to delete the %1 field? "
                     "This action occurs immediately and can not be undone!").arg(att->title());
  QString dontAsk = QString::fromLatin1("DeleteField");
  int ret = KMessageBox::questionYesNo(this, str, i18n("Delete Field?"),
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

void BCCollectionPropDialog::slotTypeChanged(const QString& type_) {
  m_allowEdit->setEnabled(type_ == i18n("List"));
  m_catCombo->setEnabled(!(type_ == i18n("Paragraph"))); // paragraphs are their own category
  // formatting is only applicable when the type is simple text
  bool isText = (type_ == i18n("Simple Text"));
  m_formatPlain->setEnabled(isText);
  m_formatName->setEnabled(isText);
  m_formatTitle->setEnabled(isText);
  // multiple is only applicable for simple text and year
  isText = isText || (type_ == i18n("Year"));
  m_multiple->setEnabled(isText);
  // completion is only applicable for simple text, year, and URL
  isText = isText || (type_ == i18n("URL"));
  m_complete->setEnabled(isText);
  // grouping is not possible with Paragraphs
  m_grouped->setEnabled(type_ != i18n("Paragraph"));
}

void BCCollectionPropDialog::slotUpdateValues(const QString& title_) {
//  kdDebug() << "BCCollectionPropDialog::slotUpdateValues() - " << title << endl;

  // use this instead of blocking signals everywhere
  m_updatingValues = true;

  // first update the current one with all the values from the edit widgets
  updateAttribute();

  bool isNew = false;
  // need to get a pointer to the attribute with the new values to insert
  // first check in new list
  BCAttribute* att = 0;
  BCAttributeListIterator it(m_newAttributes);
  for( ; it.current(); ++it) {
    if(it.current()->title() == title_) {
      att = it.current();
      isNew = true;
//      kdDebug() << "BCCollectionPropDialog::slotUpdateValues() - found new attribute " << endl;
      break;
    }
  }

  // if none found, check copied list
  if(!att) {
    att = m_copiedAttributes[title_];
  }
  // if none found, check attributes in collection
  if(!att) {
    att = m_coll->attributeByTitle(title_);
  }
  // now we're in trouble if none found
  if(!att) {
    kdDebug() << "BCCollectionPropDialog::slotUpdateValues() - no attribute named " << title_ << endl;
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

  switch(att->formatFlag()) {
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
  m_grouped->setChecked(flags & BCAttribute::AllowGrouped);

  // can't delete the title attribute, or have multiple titles
  m_btnDelete->setEnabled(att->name() != QString::fromLatin1("title"));
  m_multiple->setEnabled(att->name() != QString::fromLatin1("title"));

  // type is only changeable for new attributes
  m_typeCombo->setEnabled(isNew);

  m_currentAttribute = att;
  m_updatingValues = false;
}

void BCCollectionPropDialog::updateAttribute() {
//  kdDebug() << "BCCollectionPropDialog::updateAttribute()" << endl;
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
  
  QString title = m_titleEdit->text();
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
  
  // Paragraphs are their own category
  QString category;
  if(att->type() == BCAttribute::Para) {
    category = att->title();
  } else {
    category = m_catCombo->currentText();
  }
  att->setCategory(category);
  m_catCombo->setCurrentItem(category, true); // if it doesn't exist, it's added

  att->setDescription(m_descEdit->text());

  if(m_formatTitle->isChecked()) {
    att->setFormatFlag(BCAttribute::FormatTitle);
  } else if(m_formatName->isChecked()) {
    att->setFormatFlag(BCAttribute::FormatName);
  } else {
    att->setFormatFlag(BCAttribute::FormatPlain);
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
void BCCollectionPropDialog::slotModified() {
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

  QString title = m_fieldsBox->currentText();

  // check if copy exists already
  if(m_copiedAttributes[title]) {
    return;
  }
  
  // or, check if is a new attribute, in which case no copy is needed
  BCAttributeListIterator it(m_newAttributes);
  for( ; it.current(); ++it) {
    // need to enforce unique in titles!
    if(it.current()->title() == title) {
     return;
    }
  }

  // so now need to create a deep copy of the attribute
  BCAttribute* att = m_coll->attributeByTitle(title);
  if(!att) {
    kdWarning() << "BCCollectionPropDialog::slotModified() - no attribute named " << title << endl;
    return;
  }

  BCAttribute* newAtt = new BCAttribute(*att);
  m_copiedAttributes.insert(title, newAtt);
  m_currentAttribute = newAtt; 
}

void BCCollectionPropDialog::slotUpdateTitle(const QString& title_) {
//  kdDebug() << "BCCollectionPropDialog::slotUpdateTitle()" << endl;

  if(m_currentAttribute && m_currentAttribute->title() != title_) {
    m_fieldsBox->blockSignals(true);
    BCListBoxText* oldItem = static_cast<BCListBoxText*>(m_fieldsBox->findItem(m_currentAttribute->title()));
    oldItem->setText(title_);
    // will always be colred since it's new
    oldItem->setColored(true);

    if(m_copiedAttributes[m_currentAttribute->title()]) {
      m_copiedAttributes.remove(m_currentAttribute->title());
      m_copiedAttributes.insert(title_, m_currentAttribute);
    }
    
    m_currentAttribute->setTitle(title_);
    m_fieldsBox->blockSignals(false);
  }
}
