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

#include "collectionfieldsdialog.h"
#include "collection.h"
#include "field.h"
#include "fieldformat.h"
#include "collectionfactory.h"
#include "gui/listwidgetitem.h"
#include "gui/stringmapdialog.h"
#include "gui/combobox.h"
#include "translators/tellico_xml.h"
#include "utils/string_utils.h"
#include "tellico_debug.h"

#include <KLocalizedString>
#include <KMessageBox>
#include <KAcceleratorManager>
#include <KHelpClient>

#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QRadioButton>
#include <QGroupBox>
#include <QCheckBox>
#include <QRegularExpression>
#include <QTimer>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QListWidget>
#include <QDialogButtonBox>

using namespace Tellico;
using Tellico::FieldListItem;
using Tellico::CollectionFieldsDialog;

class Tellico::FieldListItem : public Tellico::GUI::ListWidgetItem {
public:
  FieldListItem(QListWidget* parent_, Data::FieldPtr field_) : GUI::ListWidgetItem(field_->title(), parent_), m_field(field_) {}

  Data::FieldPtr field() const { return m_field; }
  void setField(Data::FieldPtr field) { m_field = field; }

private:
  Data::FieldPtr m_field;
};

CollectionFieldsDialog::CollectionFieldsDialog(Tellico::Data::CollPtr coll_, QWidget* parent_)
    : QDialog(parent_),
      m_coll(coll_),
      m_defaultCollection(nullptr),
      m_currentField(nullptr),
      m_modified(false),
      m_updatingValues(false),
      m_reordered(false),
      m_oldIndex(-1),
      m_notifyMode(NotifyKernel) {
  setModal(false);
  setWindowTitle(i18n("Collection Fields"));

  QVBoxLayout* mainLayout = new QVBoxLayout();
  setLayout(mainLayout);

  QWidget* page = new QWidget(this);
  mainLayout->addWidget(page);
  QBoxLayout* topLayout = new QHBoxLayout(page);
  page->setLayout(topLayout);

  QGroupBox* fieldsGroup = new QGroupBox(i18n("Current Fields"), page);
  QBoxLayout* fieldsLayout = new QVBoxLayout(fieldsGroup);
  topLayout->addWidget(fieldsGroup, 1);

  m_fieldsWidget = new QListWidget(fieldsGroup);
  m_fieldsWidget->setMinimumWidth(150);
  fieldsLayout->addWidget(m_fieldsWidget);

  Data::FieldList fields = m_coll->fields();
  foreach(Data::FieldPtr field, fields) {
    // ignore fields which are not user-editable
    if(!field->hasFlag(Data::Field::NoEdit)) {
      (void) new FieldListItem(m_fieldsWidget, field);
    }
  }
  connect(m_fieldsWidget, &QListWidget::currentRowChanged, this, &CollectionFieldsDialog::slotHighlightedChanged);

  QWidget* hb1 = new QWidget(fieldsGroup);
  QHBoxLayout* hb1HBoxLayout = new QHBoxLayout(hb1);
  hb1HBoxLayout->setMargin(0);
  fieldsLayout->addWidget(hb1);
  m_btnNew = new QPushButton(i18nc("New Field", "&New"), hb1);
  hb1HBoxLayout->addWidget(m_btnNew);
  m_btnNew->setIcon(QIcon::fromTheme(QStringLiteral("document-new")));
  m_btnNew->setWhatsThis(i18n("Add a new field to the collection"));
  m_btnDelete = new QPushButton(i18nc("Delete Field", "Delete"), hb1);
  hb1HBoxLayout->addWidget(m_btnDelete);
  m_btnDelete->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));
  m_btnDelete->setWhatsThis(i18n("Remove a field from the collection"));

  connect(m_btnNew, &QAbstractButton::clicked, this, &CollectionFieldsDialog::slotNew);
  connect(m_btnDelete, &QAbstractButton::clicked, this, &CollectionFieldsDialog::slotDelete);

  QWidget* hb2 = new QWidget(fieldsGroup);
  QHBoxLayout* hb2HBoxLayout = new QHBoxLayout(hb2);
  hb2HBoxLayout->setMargin(0);
  fieldsLayout->addWidget(hb2);
  m_btnUp = new QPushButton(hb2);
  hb2HBoxLayout->addWidget(m_btnUp);
  m_btnUp->setIcon(QIcon::fromTheme(QStringLiteral("go-up")));
  m_btnUp->setWhatsThis(i18n("Move this field up in the list. The list order is important "
                             "for the layout of the entry editor."));
  m_btnDown = new QPushButton(hb2);
  hb2HBoxLayout->addWidget(m_btnDown);
  m_btnDown->setIcon(QIcon::fromTheme(QStringLiteral("go-down")));
  m_btnDown->setWhatsThis(i18n("Move this field down in the list. The list order is important "
                               "for the layout of the entry editor."));

  connect(m_btnUp, &QAbstractButton::clicked, this, &CollectionFieldsDialog::slotMoveUp);
  connect(m_btnDown, &QAbstractButton::clicked, this, &CollectionFieldsDialog::slotMoveDown);

  QWidget* vbox = new QWidget(page);
  QVBoxLayout* vboxVBoxLayout = new QVBoxLayout(vbox);
  vboxVBoxLayout->setMargin(0);
  topLayout->addWidget(vbox, 2);

  QGroupBox* propGroup = new QGroupBox(i18n("Field Properties"), vbox);
  vboxVBoxLayout->addWidget(propGroup);
  QBoxLayout* propLayout = new QVBoxLayout(propGroup);

  QWidget* grid = new QWidget(propGroup);
  QGridLayout* layout = new QGridLayout(grid);
  propLayout->addWidget(grid);

  int row = -1;
  QLabel* label = new QLabel(i18n("&Title:"), grid);
  layout->addWidget(label, ++row, 0);
  m_titleEdit = new QLineEdit(grid);
  layout->addWidget(m_titleEdit, row, 1);
  label->setBuddy(m_titleEdit);
  QString whats = i18n("The title of the field");
  label->setWhatsThis(whats);
  m_titleEdit->setWhatsThis(whats);
  connect(m_titleEdit, &QLineEdit::textChanged, this, &CollectionFieldsDialog::slotModified);

  label = new QLabel(i18n("T&ype:"), grid);
  layout->addWidget(label, row, 2);
  m_typeCombo = new KComboBox(grid);
  layout->addWidget(m_typeCombo, row, 3);
  label->setBuddy(m_typeCombo);
  whats = QStringLiteral("<qt>");
  whats += i18n("The type of the field determines what values may be used. ");
  whats += i18n("<i>Simple Text</i> is used for most fields. ");
  whats += i18n("<i>Paragraph</i> is for large text blocks. ");
  whats += i18n("<i>Choice</i> limits the field to certain values. ");
  whats += i18n("<i>Checkbox</i> is for a simple yes/no value. ");
  whats += i18n("<i>Number</i> indicates that the field contains a numerical value. ");
  whats += i18n("<i>URL</i> is for fields which refer to URLs, including references to other files. ");
  whats += i18n("A <i>Table</i> may hold one or more columns of values. ");
  whats += i18n("An <i>Image</i> field holds a picture. ");
  whats += i18n("A <i>Date</i> field can be used for values with a day, month, and year. ");
  whats += i18n("A <i>Rating</i> field uses stars to show a rating number. ");
  whats += QLatin1String("</qt>");
  label->setWhatsThis(whats);
  m_typeCombo->setWhatsThis(whats);
  // the typeTitles match the fieldMap().values() but in a better order
  m_typeCombo->addItems(Data::Field::typeTitles());
  void (QComboBox::* activatedInt)(int) = &QComboBox::activated;
  connect(m_typeCombo, activatedInt, this, &CollectionFieldsDialog::slotModified);
#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
  void (QComboBox::* activatedString)(const QString&) = &QComboBox::activated;
  connect(m_typeCombo, activatedString, this, &CollectionFieldsDialog::slotTypeChanged);
#else
  connect(m_typeCombo, &QComboBox::textActivated, this, &CollectionFieldsDialog::slotTypeChanged);
#endif

  label = new QLabel(i18n("Cate&gory:"), grid);
  layout->addWidget(label, ++row, 0);
  m_catCombo = new KComboBox(true, grid);
  layout->addWidget(m_catCombo, row, 1);
  label->setBuddy(m_catCombo);
  whats = i18n("The field category determines where the field is placed in the editor.");
  label->setWhatsThis(whats);
  m_catCombo->setWhatsThis(whats);

  // I don't want to include the categories for singleCategory fields
  QStringList cats;
  const QStringList allCats = m_coll->fieldCategories();
  foreach(const QString& cat, allCats) {
    Data::FieldList fields = m_coll->fieldsByCategory(cat);
    if(!fields.isEmpty() && !fields.at(0)->isSingleCategory()) {
      cats.append(cat);
    }
  }
  m_catCombo->addItems(cats);
  m_catCombo->setDuplicatesEnabled(false);
  connect(m_catCombo, &QComboBox::currentTextChanged, this, &CollectionFieldsDialog::slotModified);

  m_btnExtended = new QPushButton(i18n("Set &properties..."), grid);
  m_btnExtended->setIcon(QIcon::fromTheme(QStringLiteral("bookmarks")));
  layout->addWidget(m_btnExtended, row, 2, 1, 2);
  label->setBuddy(m_btnExtended);
  whats = i18n("Extended field properties are used to specify things such as the corresponding bibtex field.");
  label->setWhatsThis(whats);
  m_btnExtended->setWhatsThis(whats);
  connect(m_btnExtended, &QAbstractButton::clicked, this, &CollectionFieldsDialog::slotShowExtendedProperties);

  label = new QLabel(i18n("Description:"), grid);
  layout->addWidget(label, ++row, 0);
  m_descEdit = new QLineEdit(grid);
  m_descEdit->setMinimumWidth(150);
  layout->addWidget(m_descEdit, row, 1, 1, 3);
  label->setBuddy(m_descEdit);

  whats = i18n("The description is a useful reminder of what information is contained in the field.");
  label->setWhatsThis(whats);
  m_descEdit->setWhatsThis(whats);
  connect(m_descEdit, &QLineEdit::textChanged, this, &CollectionFieldsDialog::slotModified);

  QGroupBox* valueGroup = new QGroupBox(i18n("Value Options"), vbox);
  vboxVBoxLayout->addWidget(valueGroup);
  QGridLayout* valueLayout = new QGridLayout(valueGroup);
  int valueRow = -1;

  label = new QLabel(i18n("Default value:"), valueGroup);
  valueLayout->addWidget(label, ++valueRow, 0);
  m_defaultEdit = new QLineEdit(valueGroup);
  valueLayout->addWidget(m_defaultEdit, valueRow, 1, 1, 3);
  label->setBuddy(m_defaultEdit);
  whats = i18n("<qt>A default value can be set for new entries.</qt>");
  label->setWhatsThis(whats);
  m_defaultEdit->setWhatsThis(whats);
  connect(m_defaultEdit, &QLineEdit::textChanged, this, &CollectionFieldsDialog::slotModified);

  label = new QLabel(i18n("Value template:"), valueGroup);
  valueLayout->addWidget(label, ++valueRow, 0);
  m_derivedEdit = new QLineEdit(valueGroup);
  m_derivedEdit->setMinimumWidth(150);
  valueLayout->addWidget(m_derivedEdit, valueRow, 1);
  label->setBuddy(m_derivedEdit);

  /* TRANSLATORS: Do not translate %{year} and %{title}. */
  whats = i18n("Derived values are formed from the values of other fields according to the value template. "
               "Named fields, such as \"%{year} %{title}\", get substituted in the value.");
  label->setWhatsThis(whats);
  m_derivedEdit->setWhatsThis(whats);
  connect(m_derivedEdit, &QLineEdit::textChanged, this, &CollectionFieldsDialog::slotModified);

  m_derived = new QCheckBox(i18n("Use derived value"), valueGroup);
  m_derived->setWhatsThis(whats);
  valueLayout->addWidget(m_derived, valueRow, 2, 1, 2);
  connect(m_derived, &QAbstractButton::clicked, this, &CollectionFieldsDialog::slotDerivedChecked);
  connect(m_derived, &QAbstractButton::clicked, this, &CollectionFieldsDialog::slotModified);

  label = new QLabel(i18n("A&llowed values:"), valueGroup);
  valueLayout->addWidget(label, ++valueRow, 0);
  m_allowEdit = new QLineEdit(valueGroup);
  valueLayout->addWidget(m_allowEdit, valueRow, 1, 1, 3);
  label->setBuddy(m_allowEdit);
  whats = i18n("<qt>For <i>Choice</i>-type fields, these are the only values allowed. They are "
               "placed in a combo box. The possible values have to be separated by a semi-colon, "
               "for example: \"dog; cat; mouse\"</qt>");
  label->setWhatsThis(whats);
  m_allowEdit->setWhatsThis(whats);
  connect(m_allowEdit, &QLineEdit::textChanged, this, &CollectionFieldsDialog::slotModified);

  label = new QLabel(i18n("Format options:"), valueGroup);
  valueLayout->addWidget(label, ++valueRow, 0);
  m_formatCombo = new GUI::ComboBox(valueGroup);
  valueLayout->addWidget(m_formatCombo, valueRow, 1, 1, 3);
  label->setBuddy(m_formatCombo);

  m_formatCombo->addItem(i18n("No formatting"), FieldFormat::FormatNone);
  m_formatCombo->addItem(i18n("Allow auto-capitalization only"), FieldFormat::FormatPlain);
  m_formatCombo->addItem(i18n("Format as a title"), FieldFormat::FormatTitle);
  m_formatCombo->addItem(i18n("Format as a name"), FieldFormat::FormatName);
  void (QComboBox::* currentIndexChanged)(int) = &QComboBox::currentIndexChanged;
  connect(m_formatCombo, currentIndexChanged, this, &CollectionFieldsDialog::slotModified);

  QGroupBox* optionsGroup = new QGroupBox(i18n("Field Options"), vbox);
  vboxVBoxLayout->addWidget(optionsGroup);
  QBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);
  m_complete = new QCheckBox(i18n("Enable auto-completion"), optionsGroup);
  m_complete->setWhatsThis(i18n("If checked, KDE auto-completion will be enabled in the "
                                "text edit box for this field."));
  m_multiple = new QCheckBox(i18n("Allow multiple values"), optionsGroup);
  m_multiple->setWhatsThis(i18n("If checked, Tellico will parse the values in the field "
                                "for multiple values, separated by a semi-colon."));
  m_grouped = new QCheckBox(i18n("Allow grouping"), optionsGroup);
  m_grouped->setWhatsThis(i18n("If checked, this field may be used to group the entries in "
                               "the group view."));
  optionsLayout->addWidget(m_complete);
  optionsLayout->addWidget(m_multiple);
  optionsLayout->addWidget(m_grouped);

  connect(m_complete, &QAbstractButton::clicked, this, &CollectionFieldsDialog::slotModified);
  connect(m_multiple, &QAbstractButton::clicked, this, &CollectionFieldsDialog::slotModified);
  connect(m_grouped, &QAbstractButton::clicked, this, &CollectionFieldsDialog::slotModified);

  // need to stretch at bottom
  vboxVBoxLayout->addStretch(1);

  // keep a default collection
  m_defaultCollection = CollectionFactory::collection(m_coll->type(), true);

  m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|
                                     QDialogButtonBox::Cancel|
                                     QDialogButtonBox::Help|
                                     QDialogButtonBox::RestoreDefaults|
                                     QDialogButtonBox::Apply);
  mainLayout->addWidget(m_buttonBox);

  QPushButton* okButton = m_buttonBox->button(QDialogButtonBox::Ok);
  okButton->setDefault(true);
  okButton->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return));
  connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  connect(m_buttonBox, &QDialogButtonBox::helpRequested, this, &CollectionFieldsDialog::slotHelp);

  m_buttonBox->button(QDialogButtonBox::RestoreDefaults)->setWhatsThis(i18n("Revert the selected field's properties to the default values."));

  connect(okButton, &QAbstractButton::clicked, this, &CollectionFieldsDialog::slotOk);
  connect(m_buttonBox->button(QDialogButtonBox::Apply), &QAbstractButton::clicked, this, &CollectionFieldsDialog::slotApply);
  connect(m_buttonBox->button(QDialogButtonBox::RestoreDefaults), &QAbstractButton::clicked, this, &CollectionFieldsDialog::slotDefault);

  okButton->setEnabled(false);
  m_buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
  m_buttonBox->button(QDialogButtonBox::RestoreDefaults)->setEnabled(false);

  // initially the m_typeCombo is populated with all types, but as soon as something is
  // selected in the fields box, the combo box is cleared and filled with the allowable
  // new types. The problem is that when more types are added, the size of the combo box
  // doesn't change. So when everything is laid out, the combo box needs to have all the
  // items there.
  QTimer::singleShot(0, this, &CollectionFieldsDialog::slotSelectInitial);
}

CollectionFieldsDialog::~CollectionFieldsDialog() {
}

void CollectionFieldsDialog::setNotifyKernel(bool notify_) {
  if(notify_) {
    m_notifyMode = NotifyKernel;
  } else {
    m_notifyMode = NoNotification;
  }
}

void CollectionFieldsDialog::slotSelectInitial() {
  // the accel management is here so that it doesn't cause conflicts with the
  // ones explicitly set in the constructor
  KAcceleratorManager::manage(this);
  m_fieldsWidget->setCurrentRow(0);
}

void CollectionFieldsDialog::slotHelp() {
  KHelpClient::invokeHelp(QStringLiteral("fields-dialog"));
}

void CollectionFieldsDialog::slotOk() {
  slotApply();
  accept();
}

void CollectionFieldsDialog::slotApply() {
  updateField();
  if(!checkValues()) {
    return;
  }

  applyChanges();
}

void CollectionFieldsDialog::applyChanges() {
  // start a command group, "Modify" is a generic term here since the commands could be add, modify, or delete
  if(m_notifyMode == NotifyKernel) {
    emit beginCommandGroup(i18n("Modify Fields"));
  }

  foreach(Data::FieldPtr field, m_copiedFields) {
    // check for Choice fields with removed values to warn user
    if(field->type() == Data::Field::Choice || field->type() == Data::Field::Rating) {
      QStringList oldValues = m_coll->fieldByName(field->name())->allowed();
      QStringList newValues = field->allowed();
      for(QStringList::ConstIterator vIt = oldValues.constBegin(); vIt != oldValues.constEnd(); ++vIt) {
        if(newValues.contains(*vIt)) {
          continue;
        }
        int ret = KMessageBox::warningContinueCancel(this,
                                                     i18n("<qt>Removing allowed values from the <i>%1</i> field which "
                                                          "currently exist in the collection may cause data corruption. "
                                                          "Do you want to keep your modified values or cancel and revert "
                                                          "to the current ones?</qt>", field->title()),
                                                     QString(),
                                                     KGuiItem(i18n("Keep modified values")));
        if(ret != KMessageBox::Continue) {
          if(field->type() == Data::Field::Choice) {
            field->setAllowed(oldValues);
          } else { // rating field
            Data::FieldPtr oldField = m_coll->fieldByName(field->name());
            field->setProperty(QStringLiteral("minimum"), oldField->property(QStringLiteral("minimum")));
            field->setProperty(QStringLiteral("maximum"), oldField->property(QStringLiteral("maximum")));
          }
        }
        break;
      }
    }
    if(m_notifyMode == NotifyKernel) {
      emit modifyField(field);
    } else {
      m_coll->modifyField(field);
    }
  }

  foreach(Data::FieldPtr field, m_newFields) {
    if(m_notifyMode == NotifyKernel) {
      emit addField(field);
    } else {
      m_coll->addField(field);
    }
  }

  // set all text not to be colored, and get new list
  Data::FieldList fields;
  fields.reserve(m_fieldsWidget->count());
  for(int i = 0; i < m_fieldsWidget->count(); ++i) {
    FieldListItem* item = static_cast<FieldListItem*>(m_fieldsWidget->item(i));
    item->setColored(false);
    if(m_reordered) {
      Data::FieldPtr field = item->field();
      if(field) {
        fields.append(field);
      }
    }
  }

  // if reordering fields, need to add ReadOnly fields since they were not shown
  if(m_reordered) {
    foreach(Data::FieldPtr field, m_coll->fields()) {
      if(field->hasFlag(Data::Field::NoEdit)) {
        fields.append(field);
      }
    }
  }

  if(fields.count() > 0) {
    if(m_notifyMode == NotifyKernel) {
      emit reorderFields(fields);
    } else {
      m_coll->reorderFields(fields);
    }
  }

  // commit command group
  if(m_notifyMode == NotifyKernel) {
    emit endCommandGroup();
  }

  // now clear copied fields
  m_copiedFields.clear();
  // clear new ones, too
  m_newFields.clear();

  m_currentField = static_cast<FieldListItem*>(m_fieldsWidget->currentItem())->field();

  // the field type might have changed, so need to update the type combo list with possible values
  if(m_currentField) {
    // set the updating flag since the values are changing and slots are firing
    // but we don't care about UI indications of changes
    bool wasUpdating = m_updatingValues;
    m_updatingValues = true;
    QString currType = m_typeCombo->currentText();
    m_typeCombo->clear();
    m_typeCombo->addItems(newTypesAllowed(m_currentField->type()));
    m_typeCombo->setCurrentItem(currType);
    // template might have been changed for dependent fields
    m_derivedEdit->setText(m_currentField->property(QStringLiteral("template")));
    m_updatingValues = wasUpdating;
  }
  m_buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
}

void CollectionFieldsDialog::slotNew() {
  // first update the current one with all the values from the edit widgets
  updateField();

  // next check old values
  if(!checkValues()) {
    return;
  }

  QString name = QLatin1String("custom") + QString::number(m_newFields.count()+1);
  int count = m_newFields.count() + 1;
  QString title = i18n("New Field %1", count);
  while(!m_fieldsWidget->findItems(title, Qt::MatchExactly).isEmpty()) {
    ++count;
    title = i18n("New Field %1", count);
  }

  Data::FieldPtr field(new Data::Field(name, title));
  m_newFields.append(field);
//  myDebug() << "adding new field " << title;

  m_currentField = field;

  FieldListItem* item = new FieldListItem(m_fieldsWidget, field);
  item->setColored(true);
  m_fieldsWidget->setCurrentItem(item);
  m_fieldsWidget->scrollToItem(item);
  slotModified();
  m_titleEdit->setFocus();
  m_titleEdit->selectAll();
}

void CollectionFieldsDialog::slotDelete() {
  if(!m_currentField) {
    return;
  }

  if(m_newFields.contains(m_currentField)) {
    // remove field from vector before deleting item containing field
    m_newFields.removeAll(m_currentField);
  } else {
    if(m_notifyMode == NotifyKernel) {
      emit removeField(m_currentField);
    } else {
      m_coll->removeField(m_currentField);
    }
    emit signalCollectionModified();
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
  }
  int currentRow = m_fieldsWidget->currentRow();
  delete m_fieldsWidget->takeItem(currentRow);
  m_fieldsWidget->setCurrentRow(qMin(currentRow, m_fieldsWidget->count()-1));
  m_fieldsWidget->scrollToItem(m_fieldsWidget->currentItem());
  m_currentField = static_cast<FieldListItem*>(m_fieldsWidget->currentItem())->field(); // QSharedData gets auto-deleted
}

void CollectionFieldsDialog::slotTypeChanged(const QString& type_) {
  Data::Field::Type type = Data::Field::Undef;
  const Data::Field::FieldMap fieldMap = Data::Field::typeMap();
  for(Data::Field::FieldMap::ConstIterator it = fieldMap.begin(); it != fieldMap.end(); ++it) {
    if(it.value() == type_) {
      type = it.key();
      break;
    }
  }
  if(type == Data::Field::Undef) {
    myWarning() << "type name not recognized:" << type_;
    type = Data::Field::Line;
  }

  // only choice types gets allowed values
  m_allowEdit->setEnabled(type == Data::Field::Choice);

  // paragraphs, tables, and images are their own category
  bool isCategory = (type == Data::Field::Para || type == Data::Field::Table ||
                     type == Data::Field::Image);
  m_catCombo->setEnabled(!isCategory);

  // formatting is only applicable when the type is simple text or a table
  bool isText = (type == Data::Field::Line || type == Data::Field::Table);
  // formatNone is the default
  m_formatCombo->setEnabled(isText);

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
  if(index_ == m_oldIndex) {
    return; // state when checkValues() returns false
  }

  // use this instead of blocking signals everywhere
  m_updatingValues = true;

  // first update the current one with all the values from the edit widgets
  updateField();

  // next check old values
  if(!checkValues()) {
    // Other functions get called and change selection after this one. Use a SingleShot to revert
    QTimer::singleShot(0, this, &CollectionFieldsDialog::resetToCurrent);
    m_updatingValues = false;
    return;
  }
  m_oldIndex = index_;

  m_btnUp->setEnabled(index_ > 0);
  m_btnDown->setEnabled(index_ < static_cast<int>(m_fieldsWidget->count())-1);

  FieldListItem* item = dynamic_cast<FieldListItem*>(m_fieldsWidget->item(index_));
  if(!item) {
    return;
  }

  // need to get a pointer to the field with the new values to insert
  Data::FieldPtr field = item->field();
  if(!field) {
    myDebug() << "no field found!";
    return;
  }

  // type is limited to certain types, unless it's a new field
  m_typeCombo->clear();
  if(m_newFields.contains(field)) {
    m_typeCombo->addItems(newTypesAllowed(Data::Field::Undef));
  } else {
    m_typeCombo->addItems(newTypesAllowed(field->type()));
  }
  populate(field);

  // default button is enabled only if default collection contains the field
  if(m_defaultCollection) {
    const bool hasField = m_defaultCollection->hasField(field->name());
    m_buttonBox->button(QDialogButtonBox::RestoreDefaults)->setEnabled(hasField);
  }

  m_currentField = field;
  m_updatingValues = false;
}

void CollectionFieldsDialog::updateField() {
  Data::FieldPtr field = m_currentField;
  if(!field || !m_modified) {
    return;
  }

  // only update name if it's one of the new ones
  if(m_newFields.contains(field)) {
    // name needs to be a valid XML element name
    QString name = XML::elementName(m_titleEdit->text().toLower());
    if(name.isEmpty()) { // might end up with empty string
      name = QLatin1String("custom") + QString::number(m_newFields.count()+1);
    }
    while(m_coll->hasField(name)) { // ensure name uniqueness
      name += QLatin1String("-new");
    }
    field->setName(name);
  }

  const QString title = m_titleEdit->text().simplified();
  updateTitle(title);

  const Data::Field::FieldMap& fieldMap = Data::Field::typeMap();
  for(Data::Field::FieldMap::ConstIterator it = fieldMap.begin(); it != fieldMap.end(); ++it) {
    if(it.value() == m_typeCombo->currentText()) {
      field->setType(it.key());
      break;
    }
  }

  if(field->type() == Data::Field::Choice) {
    static const QRegularExpression rx(QLatin1String("\\s*;\\s*"));
#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
    field->setAllowed(m_allowEdit->text().split(rx, QString::SkipEmptyParts));
#else
    field->setAllowed(m_allowEdit->text().split(rx, Qt::SkipEmptyParts));
#endif
    field->setProperty(QStringLiteral("minimum"), QString());
    field->setProperty(QStringLiteral("maximum"), QString());
  } else if(field->type() == Data::Field::Rating) {
    QString v = field->property(QStringLiteral("minimum"));
    if(v.isEmpty()) {
      field->setProperty(QStringLiteral("minimum"), QString::number(1));
    }
    v = field->property(QStringLiteral("maximum"));
    if(v.isEmpty()) {
      field->setProperty(QStringLiteral("maximum"), QString::number(5));
    }
  }

  if(field->isSingleCategory()) {
    field->setCategory(field->title());
  } else {
    QString category = m_catCombo->currentText().simplified();
    field->setCategory(category);
    m_catCombo->setCurrentItem(category, true); // if it doesn't exist, it's added
  }

  if(m_derived->isChecked()) {
    field->setProperty(QStringLiteral("template"), m_derivedEdit->text());
  }
  field->setDescription(m_descEdit->text());
  field->setDefaultValue(m_defaultEdit->text());

  if(m_formatCombo->isEnabled()) {
    field->setFormatType(static_cast<FieldFormat::Type>(m_formatCombo->currentData().toInt()));
  } else {
    field->setFormatType(FieldFormat::FormatNone);
  }

  int flags = 0;
  if(m_derived->isChecked()) {
    flags |= Data::Field::Derived;
  }
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
  // if I'm just updating the values, I don't care
  if(m_updatingValues) {
    return;
  }

  m_modified = true;

  m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
  m_buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);

  if(!m_currentField) {
    myDebug() << "no current field!";
    m_currentField = static_cast<FieldListItem*>(m_fieldsWidget->currentItem())->field();
  }

  // color the text
  static_cast<FieldListItem*>(m_fieldsWidget->currentItem())->setColored(true);

  // check if copy exists already
  if(m_copiedFields.contains(m_currentField)) {
    return;
  }

  // or, check if is a new field, in which case no copy is needed
  // check if copy exists already
  if(m_newFields.contains(m_currentField)) {
    return;
  }

  m_currentField = new Data::Field(*m_currentField);
  m_copiedFields.append(m_currentField);
  static_cast<FieldListItem*>(m_fieldsWidget->currentItem())->setField(m_currentField);
}

void CollectionFieldsDialog::updateTitle(const QString& title_) {
  if(m_currentField && m_currentField->title() != title_) {
    m_fieldsWidget->blockSignals(true);
    FieldListItem* oldItem = findItem(m_currentField);
    if(!oldItem) {
      return;
    }
    oldItem->setText(title_);
    // will always be colored since it's new
    oldItem->setColored(true);

    m_currentField->setTitle(title_);
    m_fieldsWidget->blockSignals(false);
  }
}

void CollectionFieldsDialog::slotDefault() {
  if(!m_currentField) {
    return;
  }

  Data::FieldPtr defaultField = m_defaultCollection->fieldByName(m_currentField->name());
  if(!defaultField) {
    return;
  }

  QString caption = i18n("Revert Field Properties");
  QString text = i18n("<qt><p>Do you really want to revert the properties for the <em>%1</em> "
                      "field back to their default values?</p></qt>", m_currentField->title());
  QString dontAsk = QStringLiteral("RevertFieldProperties");
  int ret = KMessageBox::warningContinueCancel(this, text, caption, KGuiItem(i18n("Revert")), KStandardGuiItem::cancel(), dontAsk);
  if(ret != KMessageBox::Continue) {
    return;
  }

  // now update all values with default
  m_updatingValues = true;
  populate(defaultField);
  m_updatingValues = false;
  slotModified();
}

void CollectionFieldsDialog::slotMoveUp() {
  int idx = m_fieldsWidget->currentRow();
  if(idx < 1) {
    return;
  }
  // takeItem ends up signalling that the current index changed
  // need to revert m_oldIndex after taking the item
  QListWidgetItem* item = m_fieldsWidget->takeItem(idx);
  m_oldIndex++;
  m_fieldsWidget->insertItem(idx-1, item);
  m_fieldsWidget->setCurrentItem(item);
  m_reordered = true;
  // don't call slotModified() since that creates a deep copy.
  m_modified = true;

  m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
  m_buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
}

void CollectionFieldsDialog::slotMoveDown() {
  int idx = m_fieldsWidget->currentRow();
  if(idx > m_fieldsWidget->count()-1) {
    return;
  }
  // takeItem ends up signalling that the current index changed
  // need to revert m_oldIndex after taking the item
  QListWidgetItem* item = m_fieldsWidget->takeItem(idx);
  m_oldIndex--;
  m_fieldsWidget->insertItem(idx+1, item);
  m_fieldsWidget->setCurrentItem(item);
  m_reordered = true;
  // don't call slotModified() since that creates a deep copy.
  m_modified = true;

  m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
  m_buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
}

Tellico::FieldListItem* CollectionFieldsDialog::findItem(Tellico::Data::FieldPtr field_) {
  for(int i = 0; i < m_fieldsWidget->count(); ++i) {
    FieldListItem* textItem = static_cast<FieldListItem*>(m_fieldsWidget->item(i));
    if(textItem->field() == field_) {
      return textItem;
    }
  }
  return nullptr;
}

bool CollectionFieldsDialog::slotShowExtendedProperties() {
  if(!m_currentField) {
    return false;
  }

  // the default value is included in properties, but it has a
  // separate edit box
  QString dv = m_currentField->defaultValue();
  QString dt = m_currentField->property(QStringLiteral("template"));
  StringMap props = m_currentField->propertyList();
  props.remove(QStringLiteral("default"));
  props.remove(QStringLiteral("template"));

  StringMapDialog dlg(props, this, true);
  dlg.setWindowTitle(i18n("Extended Field Properties"));
  dlg.setLabels(i18n("Property"), i18n("Value"));
  if(dlg.exec() == QDialog::Accepted) {
    props = dlg.stringMap();
    if(!dv.isEmpty()) {
      props.insert(QStringLiteral("default"), dv);
    }
    if(!dt.isEmpty()) {
      props.insert(QStringLiteral("template"), dt);
    }
    m_currentField->setPropertyList(props);
    slotModified();
    return true;
  }
  return false;
}

void CollectionFieldsDialog::slotDerivedChecked(bool checked_) {
  m_defaultEdit->setEnabled(!checked_);
  m_derivedEdit->setEnabled(checked_);
}

void CollectionFieldsDialog::resetToCurrent() {
  m_fieldsWidget->setCurrentRow(m_oldIndex);
}

bool CollectionFieldsDialog::checkValues() {
  if(!m_currentField) {
    return true;
  }

  const QString title = m_currentField->title();
  // find total number of boxes with this title in case multiple new ones with same title were added
  int titleCount = m_fieldsWidget->findItems(title, Qt::MatchExactly).count();
  if((m_coll->fieldByTitle(title) && m_coll->fieldNameByTitle(title) != m_currentField->name()) ||
     titleCount > 1) {
    // already have a field with this title
    KMessageBox::error(this, i18n("A field with this title already exists. Please enter a different title."));
    m_titleEdit->selectAll();
    return false;
  }

  const QString category = m_currentField->category();
  if(category.isEmpty()) {
    KMessageBox::error(this, i18n("<qt>The category may not be empty. Please enter a category.</qt>"));
    m_catCombo->lineEdit()->selectAll();
    return false;
  }

  Data::FieldList fields = m_coll->fieldsByCategory(category);
  if(!fields.isEmpty() && fields[0]->isSingleCategory() && fields[0]->name() != m_currentField->name()) {
    // can't have this category, cause it conflicts with a single-category field
    KMessageBox::error(this, i18n("<qt>A field may not be in the same category as a <em>Paragraph</em>, "
                                  "<em>Table</em> or <em>Image</em> field. Please enter a different category.</qt>"));
    m_catCombo->lineEdit()->selectAll();
    return false;
  }

  // the combobox is disabled for single-category fields
  if(!m_catCombo->isEnabled() && m_coll->fieldByTitle(title) && m_coll->fieldNameByTitle(title) != m_currentField->name()) {
    KMessageBox::error(this, i18n("A field's title may not be the same as an existing category. "
                                  "Please enter a different title."));
    m_titleEdit->selectAll();
    return false;
  }

  // check for rating values outside bounds
  if(m_currentField->type() == Data::Field::Rating) {
    bool ok; // ok to ignore this here
    int low = Tellico::toUInt(m_currentField->property(QStringLiteral("minimum")), &ok);
    int high = Tellico::toUInt(m_currentField->property(QStringLiteral("maximum")), &ok);
    while(low < 1 || low > 9 || high < 1 || high > 10 || low >= high) {
      KMessageBox::error(this, i18n("The range for a rating field must be between 1 and 10, "
                                    "and the lower bound must be less than the higher bound. "
                                    "Please enter different low and high properties."));
      if(slotShowExtendedProperties()) {
        low = Tellico::toUInt(m_currentField->property(QStringLiteral("minimum")), &ok);
        high = Tellico::toUInt(m_currentField->property(QStringLiteral("maximum")), &ok);
      } else {
        return false;
      }
    }
  } else if(m_currentField->type() == Data::Field::Table) {
    bool ok; // ok to ignore this here
    int ncols = Tellico::toUInt(m_currentField->property(QStringLiteral("columns")), &ok);
    // also enforced in GUI::TableFieldWidget
    if(ncols > 10) {
      KMessageBox::error(this, i18n("Tables are limited to a maximum of ten columns."));
      m_currentField->setProperty(QStringLiteral("columns"), QStringLiteral("10"));
    }
  }

  if(m_derived->isChecked() && !m_derivedEdit->text().contains(QLatin1Char('%'))) {
    KMessageBox::error(this, i18n("A field with a derived value must have a value template."));
    m_derivedEdit->setFocus();
    m_derivedEdit->selectAll();
    return false;
  }

  return true;
}

void CollectionFieldsDialog::populate(Data::FieldPtr field_) {
  m_titleEdit->setText(field_->title());

  // if the current name is not there, then this will change the list!
  const Data::Field::FieldMap& fieldMap = Data::Field::typeMap();
  int idx = m_typeCombo->findText(fieldMap[field_->type()]);
  m_typeCombo->setCurrentIndex(idx);
  slotTypeChanged(fieldMap[field_->type()]); // just setting the text doesn't emit the activated signal

  if(field_->type() == Data::Field::Choice) {
    m_allowEdit->setText(field_->allowed().join(FieldFormat::delimiterString()));
  } else {
    m_allowEdit->clear();
  }

  idx = m_catCombo->findText(field_->category());
  if(idx > -1) {
    m_catCombo->setCurrentIndex(idx); // have to do this here
  } else {
    m_catCombo->lineEdit()->setText(field_->category());
  }
  m_descEdit->setText(field_->description());
  if(field_->hasFlag(Data::Field::Derived)) {
    m_derivedEdit->setText(field_->property(QStringLiteral("template")));
    m_derived->setChecked(true);
    m_defaultEdit->clear();
  } else {
    m_derivedEdit->clear();
    m_derived->setChecked(false);
    m_defaultEdit->setText(field_->defaultValue());
  }
  slotDerivedChecked(m_derived->isChecked());

  m_formatCombo->setCurrentData(field_->formatType());

  m_complete->setChecked(field_->hasFlag(Data::Field::AllowCompletion));
  m_multiple->setChecked(field_->hasFlag(Data::Field::AllowMultiple));
  m_grouped->setChecked(field_->hasFlag(Data::Field::AllowGrouped));

  m_btnDelete->setEnabled(!field_->hasFlag(Data::Field::NoDelete));
}

// only certain type changes are allowed
QStringList CollectionFieldsDialog::newTypesAllowed(int type_ /*=0*/) {
  // Undef means return all
  if(type_ == Data::Field::Undef) {
    return Data::Field::typeTitles();
  }

  const Data::Field::FieldMap& fieldMap = Data::Field::typeMap();

  QStringList newTypes;
  switch(type_) {
    case Data::Field::Line: // might not work if converted to a number or URL, but ok
    case Data::Field::Number:
    case Data::Field::URL:
      newTypes += fieldMap[Data::Field::Line];
      newTypes += fieldMap[Data::Field::Para];
      newTypes += fieldMap[Data::Field::Number];
      newTypes += fieldMap[Data::Field::URL];
      newTypes += fieldMap[Data::Field::Table];
      break;

    case Data::Field::Date:
      newTypes += fieldMap[Data::Field::Line];
      newTypes += fieldMap[Data::Field::Date];
      break;

    case Data::Field::Bool: // doesn't really make sense, but can't hurt
      newTypes += fieldMap[Data::Field::Line];
      newTypes += fieldMap[Data::Field::Para];
      newTypes += fieldMap[Data::Field::Bool];
      newTypes += fieldMap[Data::Field::Number];
      newTypes += fieldMap[Data::Field::URL];
      newTypes += fieldMap[Data::Field::Table];
      break;

    case Data::Field::Choice:
      newTypes += fieldMap[Data::Field::Line];
      newTypes += fieldMap[Data::Field::Para];
      newTypes += fieldMap[Data::Field::Choice];
      newTypes += fieldMap[Data::Field::Number];
      newTypes += fieldMap[Data::Field::URL];
      newTypes += fieldMap[Data::Field::Table];
      newTypes += fieldMap[Data::Field::Rating];
      break;

    case Data::Field::Table: // not really a good idea since the row delimiter will be exposed, but allow it
    case Data::Field::Table2:
      newTypes += fieldMap[Data::Field::Line];
      newTypes += fieldMap[Data::Field::Number];
      newTypes += fieldMap[Data::Field::Table];
      break;

    case Data::Field::Para:
      newTypes += fieldMap[Data::Field::Line];
      newTypes += fieldMap[Data::Field::Para];
      break;

    case Data::Field::Rating:
      newTypes += fieldMap[Data::Field::Choice];
      newTypes += fieldMap[Data::Field::Rating];
      break;

    // these can never be changed
    case Data::Field::Image:
      newTypes += fieldMap[static_cast<Data::Field::Type>(type_)];
      break;

    default:
      myDebug() << "no match for " << type_;
      newTypes = Data::Field::typeTitles();
      break;
  }
  return newTypes;
}
