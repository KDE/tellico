/* *************************************************************************
                              configdialog.cpp
                             -------------------
    begin                : Wed Dec 5 2001
    copyright            : (C) 2001 by Robby Stephenson
    email                : robby@periapsis.org
 * *************************************************************************/

/* *************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 * *************************************************************************/

#include "configdialog.h"
#include "bcattribute.h"
#include "bccollection.h"

#include <kcombobox.h>
#include <klineedit.h>
#include <klocale.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kconfig.h>

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

static const int CONFIG_MIN_WIDTH = 400;
static const int CONFIG_MIN_HEIGHT = 250;

ConfigDialog::ConfigDialog(QWidget* parent_, const char* name_/*=0*/)
    : KDialogBase(IconList, i18n("Configure"), Ok|Apply|Cancel|Default,
                   Ok, parent_, name_, true, false) {
  setupGeneralPage();
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
  QPtrListIterator<BCAttribute> attIt1(books->attributeList());
  for( ; attIt1.current(); ++attIt1) {
    if(attIt1.current()->flags() & BCAttribute::AllowGrouped) {
      box1->insertItem(attIt1.current()->title());
    }
  }
  topLayout->addWidget(box1);

  KComboBox* box2 = new KComboBox(page);
  BCCollection* videos = BCCollection::Videos(-1);
  QPtrListIterator<BCAttribute> attIt2(videos->attributeList());
  for( ; attIt2.current(); ++attIt2) {
    if(attIt2.current()->flags() & BCAttribute::AllowGrouped) {
      box2->insertItem(attIt2.current()->title());
    }
  }
  topLayout->addWidget(box2);

  KComboBox* box3 = new KComboBox(page);
  BCCollection* cds = BCCollection::CDs(-1);
  QPtrListIterator<BCAttribute> attIt3(cds->attributeList());
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
  m_cbCapitalize->setChecked(true);
  m_cbOpenLastFile->setChecked(true);
  m_cbShowCount->setChecked(false);
  m_leArticles->setText(BCAttribute::defaultArticleList().join(", "));
  m_leSuffixes->setText(BCAttribute::defaultSuffixList().join(", "));
}

void ConfigDialog::setupGeneralPage() {
  QPixmap pix = KGlobal::iconLoader()->loadIcon("bookcase", KIcon::User,
                                                 KIcon::SizeMedium);
  QFrame* frame = addPage(i18n("General"), i18n("General Options"), pix);
  QVBoxLayout* l = new QVBoxLayout(frame, 0, spacingHint());

  m_cbOpenLastFile = new QCheckBox(i18n("Reopen file at startup"), frame);
  QWhatsThis::add(m_cbOpenLastFile, i18n("If checked, the file that was last open "
                                          "will be re-opened at program start-up."));
  l->addWidget(m_cbOpenLastFile);

  m_cbCapitalize = new QCheckBox(i18n("Auto capitalize titles and authors"), frame);
  QWhatsThis::add(m_cbCapitalize, i18n("If checked, the title and the author(s) will "
                                        "be automatically capitalized."));
  l->addWidget(m_cbCapitalize);

  m_cbShowCount = new QCheckBox(i18n("Show number of items in group"), frame);
  QWhatsThis::add(m_cbShowCount, i18n("If checked, the number of items in the group "
                                       "will be appended to the group name."));
  l->addWidget(m_cbShowCount);

  QGrid* g1 = new QGrid(2, frame);
  (void) new QLabel(i18n("Articles:"), g1);
  m_leArticles = new KLineEdit("", g1);
  QStringList articles = BCAttribute::articleList();
  if(!articles.isEmpty()) {
    m_leArticles->setText(articles.join(", "));
  }
  QWhatsThis::add(m_leArticles, i18n("A comma-separated list of words which should be "
                                      "considered as articles if they are the first word "
                                      "in a title."));

  QStringList suffixes = BCAttribute::suffixList();
  (void) new QLabel(i18n("Personal suffixes:"), g1);
  m_leSuffixes = new KLineEdit("", g1);
  if(!suffixes.isEmpty()) {
    m_leSuffixes->setText(suffixes.join(", "));
  }
  QWhatsThis::add(m_leSuffixes, i18n("A comma-separated list of suffixes which might "
                                      "be used in personal names."));
  l->addWidget(g1);

  // stretch to fill lower area
  l->addStretch(1);
}

void ConfigDialog::setupBookPage() {
  QPixmap pix = KGlobal::iconLoader()->loadIcon("book", KIcon::User,
                                                 KIcon::SizeMedium);
  QFrame* frame = addPage(i18n("Books"), i18n("Book Collection Options"), pix);
  QVBoxLayout* l = new QVBoxLayout(frame, 0, spacingHint());
  l->addStretch(1);
}

void ConfigDialog::setupAudioPage() {
  QPixmap pix = KGlobal::iconLoader()->loadIcon("cd", KIcon::User, KIcon::SizeMedium);
  QFrame* frame = addPage(i18n("CDs"), i18n("Audio Collection Options"), pix);
  QVBoxLayout* l = new QVBoxLayout(frame, 0, spacingHint());
  l->addStretch(1);
}

void ConfigDialog::setupVideoPage() {
  QPixmap pix = KGlobal::iconLoader()->loadIcon("video", KIcon::User, KIcon::SizeMedium);
  QFrame* frame = addPage(i18n("Videos"), i18n("Video Collection Options"), pix);
  QVBoxLayout* l = new QVBoxLayout(frame, 0, spacingHint());
  l->addStretch(1);
}

void ConfigDialog::readConfiguration(KConfig* config_) {
  config_->setGroup("General Options");
  bool openLastFile = config_->readBoolEntry("Reopen Last File", true);
  m_cbOpenLastFile->setChecked(openLastFile);

  bool autoCapitals = config_->readBoolEntry("Auto Capitalization", true);
  m_cbCapitalize->setChecked(autoCapitals);

  bool showCount = config_->readBoolEntry("Show Group Count", false);
  m_cbShowCount->setChecked(showCount);
}

void ConfigDialog::saveConfiguration(KConfig* config_) {
  config_->setGroup("General Options");
  config_->writeEntry("Reopen Last File", m_cbOpenLastFile->isChecked());

  bool autoCapitals = m_cbCapitalize->isChecked();
  config_->writeEntry("Auto Capitalization", autoCapitals);
  // TODO: somehow, this should take immediate effect, but that's complicated
  BCAttribute::setAutoCapitalization(autoCapitals);

  config_->writeEntry("Show Group Count", m_cbShowCount->isChecked());
  emit signalShowCount(m_cbShowCount->isChecked());

  // there might be spaces after the commas in the lineedit box
  QString articlesStr = m_leArticles->text().replace(QRegExp(",\\s+"), ",");
  QStringList articles = QStringList::split(",", articlesStr, false);
  if(!articles.isEmpty()) {
    config_->writeEntry("Articles", articles, ',');
    BCAttribute::setArticleList(articles);
  }

  // there might be spaces after the commas in the lineedit box
  QString suffixesStr = m_leSuffixes->text().replace(QRegExp(",\\s+"), ",");
  QStringList suffixes = QStringList::split(",", suffixesStr, false);
  if(!suffixes.isEmpty()) {
    config_->writeEntry("Name Suffixes", suffixes, ',');
    BCAttribute::setSuffixList(suffixes);
  }

  config_->sync();
}
