/***************************************************************************
    copyright            : (C) 2002-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "finddialog.h"
#include "mainwindow.h"
#include "document.h"
#include "collection.h"

#include <klocale.h>
#include <kdebug.h>
#include <klineedit.h>
#include <kcombobox.h>
#include <kmessagebox.h>
#include <kpushbutton.h>
#include <kparts/componentfactory.h>
#include <kregexpeditorinterface.h>

#include <qlayout.h>
#include <qlabel.h>
#include <qstringlist.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qregexp.h>
#include <qwhatsthis.h>

using Bookcase::FindDialog;

//FindDialog::FindDialog(Bookcase* parent_, const char* name_/*=0*/)
//    : KDialogBase(Plain, i18n("Find"), User1|Cancel, User1,
//                  parent_, name_, false, false, i18n("&Find")), m_bookcase(parent_) {
FindDialog::FindDialog(MainWindow* parent_, const char* name_/*=0*/)
    : KDialogBase(parent_, name_, false, i18n("Find Text"), User1|Cancel, User1,
                  false, i18n("&Find")), m_bookcase(parent_), m_editRegExp(0), m_editRegExpDialog(0) {
  QWidget* page = new QWidget(this);
  setMainWidget(page);
        
  QVBoxLayout* topLayout = new QVBoxLayout(page, 0, KDialog::spacingHint());

  topLayout->addWidget(new QLabel(i18n("Text To Find:"), page));

  m_pattern = new KHistoryCombo(true, page);
  QWhatsThis::add(m_pattern, i18n("The search string"));
  m_pattern->setMinimumWidth(fontMetrics().maxWidth()*15);
  m_pattern->setMaxCount(10);
  m_pattern->setDuplicatesEnabled(false);
  topLayout->addWidget(m_pattern);

  connect(m_pattern, SIGNAL(activated(const QString&)),
          m_pattern, SLOT(addToHistory(const QString&)));
  connect(m_pattern, SIGNAL(textChanged(const QString&)),
          this, SLOT(slotPatternChanged(const QString&)));

  topLayout->addWidget(new QLabel(i18n("Search In:"), page));

  m_fields = new KComboBox(page);
  QWhatsThis::add(m_fields, i18n("Select which field should be searched."));
  updateFieldList();
  topLayout->addWidget(m_fields);

  QGroupBox* optionsGroup = new QGroupBox(2, Qt::Horizontal, i18n("Options"), page);
  optionsGroup->layout()->setSpacing(KDialog::spacingHint());
  topLayout->addWidget(optionsGroup);

  m_caseSensitive = new QCheckBox(i18n("Case &Sensitive"), optionsGroup);
  QWhatsThis::add(m_caseSensitive, i18n("If checked, the search is case-sensitive."));
  m_findBackwards = new QCheckBox(i18n("Find &Backwards"), optionsGroup);
  QWhatsThis::add(m_findBackwards, i18n("If checked, the document is searched in reverse."));
  m_wholeWords = new QCheckBox(i18n("&Whole Words Only"), optionsGroup);
  QWhatsThis::add(m_wholeWords, i18n("If checked, the search is limited to whole words."));
  m_fromBeginning = new QCheckBox(i18n("&From Beginning"), optionsGroup);
  QWhatsThis::add(m_fromBeginning, i18n("If checked, the document is searched from the beginning."));
  m_asRegExp = new QCheckBox(i18n("As &Regular Expression"), optionsGroup);
  QWhatsThis::add(m_asRegExp, i18n("If checked, the search string is used as a regular expression."));

  if(!KTrader::self()->query(QString::fromLatin1("KRegExpEditor/KRegExpEditor")).isEmpty()) {
    m_editRegExp = new KPushButton(i18n("&Edit Regular Expression..."), optionsGroup);
    m_editRegExp->setEnabled(false);
    connect(m_asRegExp, SIGNAL(toggled(bool)), m_editRegExp, SLOT(setEnabled(bool)));
    connect(m_editRegExp, SIGNAL(clicked()), this, SLOT(slotEditRegExp())); 
  }

  topLayout->addStretch(1);

  m_pattern->setFocus();
  enableButton(User1, false);
}

void FindDialog::slotUser1() {
//  kdDebug() << "FindDialog::slotUser1()" << endl;
  QString text = m_pattern->currentText();
  m_pattern->addToHistory(text);

  QString field = m_fields->currentText();
  int options = 0;

  if(field == i18n("All Fields")) {
    options |= Data::Document::AllFields;
  }

  // if checking whole words, then necessitates a regexp search
  if(m_asRegExp->isChecked() || m_wholeWords->isChecked()) {
    options = (options & Data::Document::AsRegExp);
    if(!QRegExp(text).isValid()) {
      // TODO: but what about when just checked whole words? Need to escape stuff if there's
      // critical characters in the string, fix later
      KMessageBox::error(this, i18n("Invalid regular expression."));
      return;
    }
  }

  // if whole words, then add the word boundaries to the text
  if(m_wholeWords->isChecked()) {
    text.prepend(QString::fromLatin1("\\b")).append(QString::fromLatin1("\\b"));
  }

  if(m_findBackwards->isChecked()) {
    options |= Data::Document::FindBackwards;
  }

  if(m_caseSensitive->isChecked()) {
    options |= Data::Document::CaseSensitive;
  }

  if(m_fromBeginning->isChecked()) {
    options |= Data::Document::FromBeginning;
  }

  m_bookcase->doc()->search(text, field, options);
}

void FindDialog::slotFindNext() {
  slotUser1();
}

void FindDialog::slotFindPrev() {
// easy way is just to toggle the "find backwards" switch
  m_findBackwards->setChecked(!m_findBackwards->isChecked());
  slotUser1();
  m_findBackwards->setChecked(!m_findBackwards->isChecked());
}

void FindDialog::slotPatternChanged(const QString& text_) {
  enableButton(User1, !text_.isEmpty());
}

void FindDialog::showEvent(QShowEvent* e_) {
  m_pattern->lineEdit()->selectAll();
  KDialogBase::showEvent(e_);
}

void FindDialog::slotEditRegExp() {
  if(m_editRegExpDialog == 0) {
    m_editRegExpDialog = KParts::ComponentFactory::createInstanceFromQuery<QDialog>(QString::fromLatin1("KRegExpEditor/KRegExpEditor"),
                                                                                    QString::null, this);
  }

  KRegExpEditorInterface* iface = static_cast<KRegExpEditorInterface *>(m_editRegExpDialog->qt_cast(QString::fromLatin1("KRegExpEditorInterface")));
  if(iface) {
    iface->setRegExp(m_pattern->currentText());
    if(m_editRegExpDialog->exec() == QDialog::Accepted) {
      m_pattern->changeItem(iface->regExp(), m_pattern->currentItem());
    }
  }
}

void FindDialog::updateFieldList() {
  m_fields->clear();

  m_fields->insertItem(i18n("All Fields"));

  QStringList titles = m_bookcase->doc()->collection()->fieldTitles();

  if(titles.count() > 0) {
    m_fields->insertStringList(titles);
  }
  m_fields->adjustSize();
}
