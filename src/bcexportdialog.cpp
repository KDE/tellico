/***************************************************************************
                             bcexportdialog.cpp
                             -------------------
    begin                : Sat Jul 12 2003
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

#include "bcexportdialog.h"
#include "bccollection.h"
#include "bookcase.h"

#include "translators/exporter.h"
#include "translators/htmlexporter.h"
#include "translators/csvexporter.h"
#include "translators/bibtexexporter.h"
#include "translators/bibtexmlexporter.h"
#include "translators/xsltexporter.h"
//#include "translators/pilotdbexporter.h"

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

BCExportDialog::BCExportDialog(ExportFormat format_, BCCollection* coll_, Bookcase* parent_, const char* name_)
    : KDialogBase(parent_, name_, true /*modal*/, i18n("Export Options"), Ok|Cancel),
      m_coll(coll_),
      m_exporter(exporter(format_, parent_)) {
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
  m_formatAttributes = new QCheckBox(i18n("Format all fields"), group1);
  m_formatAttributes->setChecked(false);
  QWhatsThis::add(m_formatAttributes, i18n("If checked, the values of the fields will be "
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

  QWidget* w = m_exporter->widget(widget);
  m_exporter->readOptions(KGlobal::config());
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
  connect(this, SIGNAL(okClicked()), SLOT(slotSaveOptions()));
}

QString BCExportDialog::text() {
  return m_exporter ? m_exporter->text(m_formatAttributes->isChecked(), encodeUTF8()) : QString::null;
}

QString BCExportDialog::fileFilter() {
  return m_exporter ? m_exporter->fileFilter() : QString::null;
}

bool BCExportDialog::encodeUTF8() const {
  return m_encodeUTF8->isChecked();
}

void BCExportDialog::readOptions() {
  KConfig* config = KGlobal::config();
  config->setGroup("ExportOptions");
  bool format = config->readBoolEntry("FormatAttributes", false);
  m_formatAttributes->setChecked(format);
  bool encode = config->readBoolEntry("EncodeUTF8", true);
  if(encode) {
    m_encodeUTF8->setChecked(true);
  } else {
    m_encodeLocale->setChecked(true);
  }
}

void BCExportDialog::slotSaveOptions() {
  KConfig* config = KGlobal::config();
  m_exporter->saveOptions(config);

  config->setGroup("ExportOptions");
  config->writeEntry("FormatAttributes", m_formatAttributes->isChecked());
  config->writeEntry("EncodeUTF8", encodeUTF8());
}

Exporter* BCExportDialog::exporter(ExportFormat format_, Bookcase* bookcase_) {
  Exporter* exporter = 0;
  HTMLExporter* exp = 0; // stupid "jump case labels", blah
  switch(format_) {
    case HTML:
      exp = new HTMLExporter(m_coll, m_coll->unitList());
      exp->setGroupBy(bookcase_->groupBy());
      exp->setSortTitles(bookcase_->sortTitles());
      exp->setColumns(bookcase_->visibleColumns());
      exporter = exp;
      break;

    case CSV:
      exporter = new CSVExporter(m_coll, m_coll->unitList());
      break;

    case Bibtex:
      exporter = new BibtexExporter(m_coll, m_coll->unitList());
      break;

    case Bibtexml:
      exporter = new BibtexmlExporter(m_coll, m_coll->unitList());
      break;

    case XSLT:
      exporter = new XSLTExporter(m_coll, m_coll->unitList());
      break;

//    case PilotDB:
//      exporter = new PilotDBExporter(m_coll, m_coll->unitList());
//      break;

    default:
      kdDebug() << "BCExportDialog::exporter() - not implemented!" << endl;
      break;
  }
  if(exporter) {
    exporter->readOptions(KGlobal::config());
  }
  return exporter;
}
