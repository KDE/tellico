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

#include "exportdialog.h"
#include "collection.h"
#include "mainwindow.h"

#include "translators/exporter.h"
#include "translators/bookcasexmlexporter.h"
#include "translators/htmlexporter.h"
#include "translators/csvexporter.h"
#include "translators/bibtexexporter.h"
#include "translators/bibtexmlexporter.h"
#include "translators/xsltexporter.h"
#include "translators/pilotdbexporter.h"

#include <klocale.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kconfig.h>

#include <qlayout.h>
#include <qcheckbox.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qwhatsthis.h>
#include <qtextcodec.h>

using Bookcase::ExportDialog;

ExportDialog::ExportDialog(ExportFormat format_, Data::Collection* coll_, MainWindow* parent_, const char* name_)
    : KDialogBase(parent_, name_, true /*modal*/, i18n("Export Options"), Ok|Cancel),
      m_coll(coll_), m_exporter(exporter(format_, parent_)) {
  QWidget* widget = new QWidget(this);
  QVBoxLayout* topLayout = new QVBoxLayout(widget, 0, spacingHint());

//  QButtonGroup* bg = new QButtonGroup(1, Qt::Horizontal, i18n("Options"), page1);
//  topLayout->addWidget(bg, 0);
//  QRadioButton* btn1 = new QRadioButton(i18n("Export &complete collection"), bg);
//  btn1->setChecked(true);
//  QRadioButton* btn2 = new QRadioButton(i18n("Export &selected items only"), bg);
//  btn2->setEnabled(false);

  QGroupBox* group1 = new QGroupBox(1, Qt::Horizontal, i18n("Formatting"), widget);
  topLayout->addWidget(group1, 0);
  m_formatFields = new QCheckBox(i18n("Format all fields"), group1);
  m_formatFields->setChecked(false);
  QWhatsThis::add(m_formatFields, i18n("If checked, the values of the fields will be "
                                       "automatically formatted according to their format type."));

  QButtonGroup* bg = new QButtonGroup(1, Qt::Horizontal, i18n("Encoding"), widget);
  topLayout->addWidget(bg, 0);
  m_encodeUTF8 = new QRadioButton(i18n("Encode in Unicode (UTF-8)"), bg);
  m_encodeUTF8->setChecked(true);
  QWhatsThis::add(m_encodeUTF8, i18n("Encode the exported file in Unicode (UTF-8)."));
  QString localStr = i18n("Encode in user locale (%1)").arg(
                     QString::fromLatin1(QTextCodec::codecForLocale()->name()));
  m_encodeLocale = new QRadioButton(localStr, bg);
  QWhatsThis::add(m_encodeLocale, i18n("Encode the exported file in the local format."));

  QWidget* w = m_exporter->widget(widget, "exporter_widget");
  if(w) {
    topLayout->addWidget(w, 0);
  }
  
  topLayout->addStretch();

  setMainWidget(widget);
  readOptions();
  // bibtex, CSV, and text are forced to locale
  if(format_ == Bibtex || format_ == CSV || format_ == Text) {
    m_encodeUTF8->setEnabled(false);
    m_encodeLocale->setChecked(true);
//    m_encodeLocale->setEnabled(false);
  }
  // if binary exporter, no neeed for locale info
  if(!m_exporter->isText()) {
    bg->setEnabled(false);
  }
  connect(this, SIGNAL(okClicked()), SLOT(slotSaveOptions()));
}

bool ExportDialog::isText() const {
  return m_exporter ? m_exporter->isText() : true;
}

QString ExportDialog::text() {
  return m_exporter ? m_exporter->text(m_formatFields->isChecked(), m_encodeUTF8->isChecked()) : QString::null;
}

QByteArray ExportDialog::data() {
  return m_exporter ? m_exporter->data(m_formatFields->isChecked()) : QByteArray();
}

QString ExportDialog::fileFilter() {
  return m_exporter ? m_exporter->fileFilter() : QString::null;
}

bool ExportDialog::encodeUTF8() const {
  return m_encodeUTF8->isChecked();
}

void ExportDialog::readOptions() {
  KConfig* config = KGlobal::config();
  config->setGroup("ExportOptions");
  bool format = config->readBoolEntry("FormatAttributes", false);
  m_formatFields->setChecked(format);
  bool encode = config->readBoolEntry("EncodeUTF8", true);
  if(encode) {
    m_encodeUTF8->setChecked(true);
  } else {
    m_encodeLocale->setChecked(true);
  }
}

void ExportDialog::slotSaveOptions() {
  KConfig* config = KGlobal::config();
  m_exporter->saveOptions(config);

  config->setGroup("ExportOptions");
  config->writeEntry("FormatAttributes", m_formatFields->isChecked());
  config->writeEntry("EncodeUTF8", m_encodeUTF8->isChecked());
}

Bookcase::Export::Exporter* ExportDialog::exporter(ExportFormat format_, MainWindow* bookcase_) {
  Export::Exporter* exporter = 0;

  switch(format_) {
    case XML:
      exporter = new Export::BookcaseXMLExporter(m_coll, m_coll->entryList());
      break;

    case HTML:
      {
        Export::HTMLExporter* htmlExp = new Export::HTMLExporter(m_coll, m_coll->entryList());
        htmlExp->setGroupBy(bookcase_->groupBy());
        htmlExp->setSortTitles(bookcase_->sortTitles());
        htmlExp->setColumns(bookcase_->visibleColumns());
        exporter = htmlExp;
      }
      break;

    case CSV:
      exporter = new Export::CSVExporter(m_coll, m_coll->entryList());
      break;

    case Bibtex:
      exporter = new Export::BibtexExporter(m_coll, m_coll->entryList());
      break;

    case Bibtexml:
      exporter = new Export::BibtexmlExporter(m_coll, m_coll->entryList());
      break;

    case XSLT:
      exporter = new Export::XSLTExporter(m_coll, m_coll->entryList());
      break;

    case PilotDB:
      {
        Export::PilotDBExporter* pdbExp = new Export::PilotDBExporter(m_coll, m_coll->entryList());
        pdbExp->setColumns(bookcase_->visibleColumns());
        exporter = pdbExp;
      }
      break;

    default:
      kdDebug() << "ExportDialog::exporter() - not implemented!" << endl;
      break;
  }
  if(exporter) {
    exporter->readOptions(KGlobal::config());
  }
  return exporter;
}
