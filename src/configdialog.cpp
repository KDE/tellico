/***************************************************************************
                              configdialog.cpp
                             -------------------
    begin                : Wed Dec 5 2001
    copyright            : (C) 2001 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "configdialog.h"
#include "bcattribute.h"
#include "bccollection.h"
#include "bookcasedoc.h"
#include "bookcollection.h"

#include <kcombobox.h>
#include <klineedit.h>
#include <klocale.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <klistbox.h>
#include <kbuttonbox.h>

#include <qsize.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qstringlist.h>
#include <qptrlist.h>
#include <qpixmap.h>
#include <qgrid.h>
#include <qwhatsthis.h>
#include <qregexp.h>
#include <qhgroupbox.h>
#include <qvgroupbox.h>
#include <qpushbutton.h>
#include <qvbox.h>
#include <qhbox.h>

static const int CONFIG_MIN_WIDTH = 600;
static const int CONFIG_MIN_HEIGHT = 400;

ConfigDialog::ConfigDialog(BookcaseDoc* doc_, QWidget* parent_, const char* name_/*=0*/)
    : KDialogBase(IconList, i18n("Configure Bookcase"), Ok|Apply|Cancel|Default,
                  Ok, parent_, name_, true, false), m_doc(doc_) {
  setupGeneralPage();
  setupPrintingPage();
  //setupBookPage();
  //setupAudioPage();
  //setupVideoPage();
  /*
  QWidget* page = addPage(i18n("Books"));
  QVBoxLayout* topLayout = new QVBoxLayout(page);
  QLabel* label = new QLabel(i18n("Text"), page);
  topLayout->addWidget(label);

  KComboBox* box1 = new KComboBox(page);
  BCCollection* books = BCCollection::Books(-1);
  BCAttributeListIterator attIt1(books->attributeList());
  for( ; attIt1.current(); ++attIt1) {
    if(attIt1.current()->flags() & BCAttribute::AllowGrouped) {
      box1->insertItem(attIt1.current()->title());
    }
  }
  topLayout->addWidget(box1);

  KComboBox* box2 = new KComboBox(page);
  BCCollection* videos = BCCollection::Videos(-1);
  BCAttributeListIterator attIt2(videos->attributeList());
  for( ; attIt2.current(); ++attIt2) {
    if(attIt2.current()->flags() & BCAttribute::AllowGrouped) {
      box2->insertItem(attIt2.current()->title());
    }
  }
  topLayout->addWidget(box2);

  KComboBox* box3 = new KComboBox(page);
  BCCollection* cds = BCCollection::CDs(-1);
  BCAttributeListIterator attIt3(cds->attributeList());
  for( ; attIt3.current(); ++attIt3) {
    if(attIt3.current()->flags() & BCAttribute::AllowGrouped) {
      box3->insertItem(attIt3.current()->title());
    }
  }
  topLayout->addWidget(box3);

  delete books;
  delete videos;
  delete cds;
  */
  QSize s = sizeHint();
  resize(QSize(QMAX(s.width(), CONFIG_MIN_WIDTH),
               QMAX(s.height(), CONFIG_MIN_HEIGHT)));
}

void ConfigDialog::slotOk() {
  slotApply();
  accept();
}

void ConfigDialog::slotApply() {
  emit signalConfigChanged();
}

void ConfigDialog::slotDefault() {
  m_cbOpenLastFile->setChecked(true);
  m_cbCapitalize->setChecked(true);
  m_cbFormat->setChecked(true);
  m_cbShowCount->setChecked(false);
  m_leArticles->setText(BCAttribute::defaultArticleList().join(QString::fromLatin1(", ")));
  m_leSuffixes->setText(BCAttribute::defaultSuffixList().join(QString::fromLatin1(", ")));

  m_cbPrintHeaders->setChecked(false);
  m_cbPrintFormatted->setChecked(true);
  m_cbPrintGrouped->setChecked(true);
  QString authorTitle = m_doc->collectionById(0)->attributeTitleByName(QString::fromLatin1("author"));
  m_cbPrintGroupAttribute->setCurrentItem(authorTitle);

  QStringList printAttNames = BookCollection::defaultPrintAttributes();
  QStringList printAttTitles;
  QStringList::iterator it;
  for(it = printAttNames.begin(); it != printAttNames.end(); ++it) {
    //TODO:: fix me for multiple collections
    printAttTitles += m_doc->collectionById(0)->attributeByName(*it)->title();
  }
  m_lbSelectedFields->clear();
  m_lbSelectedFields->insertStringList(printAttTitles);

  QStringList availTitles;
  BCAttributeList list = m_doc->uniqueAttributes(BCCollection::Book);
  BCAttributeListIterator attIt(list);
  for( ; attIt.current(); ++attIt) {
    if(printAttTitles.contains(attIt.current()->title()) == 0) {
      availTitles += attIt.current()->title();
    }
  }
  m_lbAvailableFields->clear();
  m_lbAvailableFields->insertStringList(availTitles);
}

void ConfigDialog::setupGeneralPage() {
  QPixmap pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("bookcase"), KIcon::User,
                                                KIcon::SizeMedium);
  QFrame* frame = addPage(i18n("General"), i18n("General Options"), pix);
  QVBoxLayout* l = new QVBoxLayout(frame, 0, KDialog::spacingHint());

  m_cbOpenLastFile = new QCheckBox(i18n("Reopen file at startup"), frame);
  QWhatsThis::add(m_cbOpenLastFile, i18n("If checked, the file that was last open "
                                         "will be re-opened at program start-up."));
  l->addWidget(m_cbOpenLastFile);
  m_cbDict.insert(QString::fromLatin1("openLastFile"), m_cbOpenLastFile);

  m_cbCapitalize = new QCheckBox(i18n("Auto capitalize titles and names"), frame);
  QWhatsThis::add(m_cbCapitalize, i18n("If checked, titles and names will "
                                       "be automatically capitalized."));
  l->addWidget(m_cbCapitalize);
  m_cbDict.insert(QString::fromLatin1("capitalize"), m_cbCapitalize);

  m_cbFormat = new QCheckBox(i18n("Auto format titles and names"), frame);
  QWhatsThis::add(m_cbFormat, i18n("If checked, titles and names will "
                                   "be automatically formatted."));
  l->addWidget(m_cbFormat);
  m_cbDict.insert(QString::fromLatin1("format"), m_cbFormat);

  m_cbShowCount = new QCheckBox(i18n("Show number of items in group"), frame);
  QWhatsThis::add(m_cbShowCount, i18n("If checked, the number of items in the group "
                                      "will be appended to the group name."));
  l->addWidget(m_cbShowCount);
  m_cbDict.insert(QString::fromLatin1("showCount"), m_cbShowCount);

  QGrid* g1 = new QGrid(2, frame);
  QLabel* l1 = new QLabel(i18n("Articles:"), g1);
  m_leArticles = new KLineEdit(g1);
  QStringList articles = BCAttribute::articleList();
  if(!articles.isEmpty()) {
    m_leArticles->setText(articles.join(QString::fromLatin1(", ")));
  }
  QWhatsThis::add(l1, i18n("A comma-separated list of words which should be "
                           "considered as articles if they are the first word "
                           "in a title."));
  QWhatsThis::add(m_leArticles, i18n("A comma-separated list of words which should be "
                                     "considered as articles if they are the first word "
                                     "in a title."));

  QStringList suffixes = BCAttribute::suffixList();
  QLabel* l2 = new QLabel(i18n("Personal suffixes:"), g1);
  m_leSuffixes = new KLineEdit(g1);
  if(!suffixes.isEmpty()) {
    m_leSuffixes->setText(suffixes.join(QString::fromLatin1(", ")));
  }
  QWhatsThis::add(l2, i18n("A comma-separated list of suffixes which might "
                           "be used in personal names."));
  QWhatsThis::add(m_leSuffixes, i18n("A comma-separated list of suffixes which might "
                                     "be used in personal names."));
  l->addWidget(g1);

  // stretch to fill lower area
  l->addStretch(1);
}

void ConfigDialog::setupPrintingPage() {
  QPixmap pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("print_printer"),
                                                KIcon::Toolbar,
                                                KIcon::SizeMedium);
  QFrame* frame = addPage(i18n("Printing"), i18n("Printing Options"), pix);
  QVBoxLayout* l = new QVBoxLayout(frame, KDialog::marginHint(), KDialog::spacingHint());
  
  QVGroupBox* optionsGroup = new QVGroupBox(i18n("Formatting Options"), frame);
  l->addWidget(optionsGroup);

  m_cbPrintFormatted = new QCheckBox(i18n("Format titles and names"), optionsGroup);
  QWhatsThis::add(m_cbPrintFormatted, i18n("If checked, titles and names will "
                                           "be automatically formatted."));
  m_cbDict.insert(QString::fromLatin1("printFormatted"), m_cbPrintFormatted);

  m_cbPrintHeaders = new QCheckBox(i18n("Print field headers"), optionsGroup);
  QWhatsThis::add(m_cbPrintHeaders, i18n("If checked, the field names will be "
                                         "printed as table headers."));
  m_cbDict.insert(QString::fromLatin1("printHeaders"), m_cbPrintHeaders);

  QHBox* groupingBox = new QHBox(optionsGroup);

  m_cbPrintGrouped = new QCheckBox(i18n("Group the books"), groupingBox);
  QWhatsThis::add(m_cbPrintGrouped, i18n("If checked, the books will be grouped under "
                                         "the selected field."));
  m_cbDict.insert(QString::fromLatin1("printGrouped"), m_cbPrintGrouped);
  connect(m_cbPrintGrouped, SIGNAL(toggled(bool)), this, SLOT(slotTogglePrintGrouped(bool)));

  m_cbPrintGroupAttribute = new KComboBox(groupingBox);
  QWhatsThis::add(m_cbPrintGroupAttribute, i18n("The collection is grouped by this field."));
  
  BCAttributeList list = m_doc->collectionById(0)->attributeList(BCAttribute::AllowGrouped);
  BCAttributeListIterator it(list);
  for( ; it.current(); ++it) {
    m_groupAttributes += it.current()->title();
  }
  m_cbPrintGroupAttribute->insertStringList(m_groupAttributes);

  QHGroupBox* fieldsGroup = new QHGroupBox(i18n("Fields"), frame);
//  fieldsGroup->layout()->setSpacing(KDialog::spacingHint());
  l->addWidget(fieldsGroup);

  QVBox* aBox = new QVBox(fieldsGroup);
  (void) new QLabel(i18n("Available Fields"), aBox);
  m_lbAvailableFields = new KListBox(aBox);
  QWhatsThis::add(m_lbAvailableFields, i18n("These are the available fields in the collection."));

  KButtonBox* bb = new KButtonBox(fieldsGroup, Qt::Vertical);
  //the stretches center the buttons top to bottom
  bb->addStretch();
  QPixmap pixLeft = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("1leftarrow"),
                                                KIcon::Toolbar);
  QPushButton* left = bb->addButton(QString::null, this, SLOT(slotFieldLeft()));
  left->setPixmap(pixLeft);
  QPixmap pixRight = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("1rightarrow"),
                                                KIcon::Toolbar);
  QPushButton* right = bb->addButton(QString::null, this, SLOT(slotFieldRight()));
  right->setPixmap(pixRight);
  bb->addStretch();
  
  QVBox* sBox = new QVBox(fieldsGroup);
  (void) new QLabel(i18n("Selected Fields"), sBox);
  m_lbSelectedFields = new KListBox(sBox);
  QWhatsThis::add(m_lbSelectedFields, i18n("These are the selected fields in the collection."));

  KButtonBox* bb2 = new KButtonBox(fieldsGroup, Qt::Vertical);
  bb2->addStretch();
  QPixmap pixUp = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("1uparrow"),
                                                KIcon::Toolbar);
  QPushButton* up = bb2->addButton(QString::null, this, SLOT(slotFieldUp()));
  up->setPixmap(pixUp);
  QPixmap pixDown = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("1downarrow"),
                                                KIcon::Toolbar);
  QPushButton* down = bb2->addButton(QString::null, this, SLOT(slotFieldDown()));
  down->setPixmap(pixDown);
  bb2->addStretch();

  // stretch to fill lower area
  l->addStretch(1);
}

//void ConfigDialog::setupBookPage() {
//  QPixmap pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("book"), KIcon::User,
//                                                 KIcon::SizeMedium);
//  QFrame* frame = addPage(i18n("Books"), i18n("Book Collection Options"), pix);
//  QVBoxLayout* l = new QVBoxLayout(frame, 0, spacingHint());
//  l->addStretch(1);
//}
//
//void ConfigDialog::setupAudioPage() {
//  QPixmap pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("cd"), KIcon::User,
//                                                KIcon::SizeMedium);
//  QFrame* frame = addPage(i18n("CDs"), i18n("Audio Collection Options"), pix);
//  QVBoxLayout* l = new QVBoxLayout(frame, 0, spacingHint());
//  l->addStretch(1);
//}
//
//void ConfigDialog::setupVideoPage() {
//  QPixmap pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("video"), KIcon::User,
//                                                KIcon::SizeMedium);
//  QFrame* frame = addPage(i18n("Videos"), i18n("Video Collection Options"), pix);
//  QVBoxLayout* l = new QVBoxLayout(frame, 0, spacingHint());
//  l->addStretch(1);
//}

void ConfigDialog::readConfiguration(KConfig* config_) {
  config_->setGroup("General Options");
  
  bool openLastFile = config_->readBoolEntry("Reopen Last File", true);
  m_cbOpenLastFile->setChecked(openLastFile);

  bool autoCapitals = config_->readBoolEntry("Auto Capitalization", true);
  m_cbCapitalize->setChecked(autoCapitals);

  bool autoFormat = config_->readBoolEntry("Auto Format", true);
  m_cbFormat->setChecked(autoFormat);

  bool showCount = config_->readBoolEntry("Show Group Count", false);
  m_cbShowCount->setChecked(showCount);

  // PRINTING
  config_->setGroup("Printing");
  
  bool printHeaders = config_->readBoolEntry("Print Field Headers", false);
  m_cbPrintHeaders->setChecked(printHeaders);

  bool printFormatted = config_->readBoolEntry("Print Formatted", true);
  m_cbPrintFormatted->setChecked(printFormatted);

  bool printGrouped = config_->readBoolEntry("Print Grouped", true);
  m_cbPrintGrouped->setChecked(printGrouped);
  m_cbPrintGroupAttribute->setEnabled(printGrouped);
  
  QString printGroupAttribute = config_->readEntry("Print Grouped Attribute",
                                                   QString::fromLatin1("author"));
  QString selectedGroup = m_doc->collectionById(0)->attributeTitleByName(printGroupAttribute);
  int idx = m_groupAttributes.findIndex(selectedGroup);
  if(idx > -1 && idx < m_cbPrintGroupAttribute->count()) {
    m_cbPrintGroupAttribute->setCurrentItem(idx);
  }

  QStringList printAttNames = config_->readListEntry("Print Fields - book");
  if(printAttNames.isEmpty()) {
    printAttNames = BookCollection::defaultPrintAttributes();
  }
  QStringList printAttTitles;
  QStringList::iterator it;
  for(it = printAttNames.begin(); it != printAttNames.end(); ++it) {
    //TODO:: fix me for multiple collections
    printAttTitles += m_doc->collectionById(0)->attributeTitleByName(*it);
  }
  m_lbSelectedFields->clear();
  m_lbSelectedFields->insertStringList(printAttTitles);
  
  QStringList availTitles;
  BCAttributeList list = m_doc->uniqueAttributes(BCCollection::Book);
  BCAttributeListIterator attIt(list);
  for( ; attIt.current(); ++attIt) {
    if(printAttTitles.contains(attIt.current()->title()) == 0) {
      availTitles += attIt.current()->title();
    }
  }
  m_lbAvailableFields->insertStringList(availTitles);

}

void ConfigDialog::saveConfiguration(KConfig* config_) {
  config_->setGroup("General Options");
  config_->writeEntry("Reopen Last File", m_cbOpenLastFile->isChecked());

  bool autoCapitals = m_cbCapitalize->isChecked();
  config_->writeEntry("Auto Capitalization", autoCapitals);
  // TODO: somehow, this should take immediate effect, but that's complicated
  BCAttribute::setAutoCapitalize(autoCapitals);
  
  bool autoFormat = m_cbFormat->isChecked();
  config_->writeEntry("Auto Format", autoFormat);
  // TODO: somehow, this should take immediate effect, but that's complicated
  BCAttribute::setAutoFormat(autoFormat);

  config_->writeEntry("Show Group Count", m_cbShowCount->isChecked());

  // there might be spaces before or after the commas in the lineedit box
  QString articlesStr = m_leArticles->text().replace(QRegExp(QString::fromLatin1("\\s*,\\s*")),
                                                     QString::fromLatin1(","));
  QStringList articles = QStringList::split(QString::fromLatin1(","), articlesStr, false);
  if(!articles.isEmpty()) {
    config_->writeEntry("Articles", articles, ',');
    BCAttribute::setArticleList(articles);
  }

  // there might be spaces before or after the commas in the lineedit box
  QString suffixesStr = m_leSuffixes->text().replace(QRegExp(QString::fromLatin1("\\s*,\\s*")),
                                                     QString::fromLatin1(","));
  QStringList suffixes = QStringList::split(QString::fromLatin1(","), suffixesStr, false);
  if(!suffixes.isEmpty()) {
    config_->writeEntry("Name Suffixes", suffixes, ',');
    BCAttribute::setSuffixList(suffixes);
  }

  config_->setGroup("Printing");
  config_->writeEntry("Print Field Headers", m_cbPrintHeaders->isChecked());
  config_->writeEntry("Print Formatted", m_cbPrintFormatted->isChecked());
  config_->writeEntry("Print Grouped", m_cbPrintGrouped->isChecked());
  QString groupTitle = m_cbPrintGroupAttribute->currentText();
  QString groupName = m_doc->collectionById(0)->attributeNameByTitle(groupTitle);
  config_->writeEntry("Print Grouped Attribute", groupName);

  QStringList printAttNames;
  for(QListBoxItem* item = m_lbSelectedFields->firstItem(); item; item = item->next()) {
    printAttNames += m_doc->collectionById(0)->attributeNameByTitle(item->text());
  }
  config_->writeEntry("Print Fields - book", printAttNames, ',');

  config_->sync();
}

bool ConfigDialog::configValue(const QString& key_) {
  QCheckBox* cb = m_cbDict.find(key_);
  return cb && cb->isChecked();
}

void ConfigDialog::slotFieldLeft() {
  QListBoxItem* item = m_lbSelectedFields->selectedItem();
  if(item) {
    m_lbAvailableFields->insertItem(item->text());
    if(item->next()) {
      m_lbSelectedFields->setSelected(item->next(), true);
    } else if(item->prev()) {
      m_lbSelectedFields->setSelected(item->prev(), true);
    } else {
      m_lbSelectedFields->clearSelection();
    }
    delete item;
  }
  m_lbAvailableFields->clearSelection();
}

void ConfigDialog::slotFieldRight() {
  QListBoxItem* item = m_lbAvailableFields->selectedItem();
  if(item) {
    m_lbSelectedFields->insertItem(item->text());
    if(item->next()) {
      m_lbAvailableFields->setSelected(item->next(), true);
    } else if(item->prev()) {
      m_lbAvailableFields->setSelected(item->prev(), true);
    } else {
      m_lbAvailableFields->clearSelection();
    }
    delete item;
  }
  m_lbSelectedFields->clearSelection();
}

void ConfigDialog::slotFieldUp() {
  QListBoxItem* item = m_lbSelectedFields->selectedItem();
  if(item) {
    QListBoxItem* prev = item->prev(); // could be 0
    if(prev) {
      (void) new QListBoxText(m_lbSelectedFields, prev->text(), item);
      delete prev;
    }
  }
}

void ConfigDialog::slotFieldDown() {
  QListBoxItem* item = m_lbSelectedFields->selectedItem();
  if(item) {
    QListBoxItem* next = item->next(); // could be 0
    if(next) {
      QListBoxItem* newItem = new QListBoxText(m_lbSelectedFields, item->text(), next);
      delete item;
      m_lbSelectedFields->setSelected(newItem, true);
    }
  }
}

void ConfigDialog::slotTogglePrintGrouped(bool checked_) {
  m_cbPrintGroupAttribute->setEnabled(checked_);
}
