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
#include "filehandler.h"
#include "controller.h"

#include "translators/exporter.h"
#include "translators/bookcasexmlexporter.h"
#include "translators/htmlexporter.h"
#include "translators/csvexporter.h"
#include "translators/bibtexexporter.h"
#include "translators/bibtexmlexporter.h"
#include "translators/xsltexporter.h"
#include "translators/pilotdbexporter.h"
#include "translators/alexandriaexporter.h"

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
      m_format(format_), m_coll(coll_), m_exporter(exporter(format_, parent_)) {
  QWidget* widget = new QWidget(this);
  QVBoxLayout* topLayout = new QVBoxLayout(widget, 0, spacingHint());

  QGroupBox* group1 = new QGroupBox(1, Qt::Horizontal, i18n("Formatting"), widget);
  topLayout->addWidget(group1, 0);
  m_formatFields = new QCheckBox(i18n("Format all fields"), group1);
  m_formatFields->setChecked(false);
  QWhatsThis::add(m_formatFields, i18n("If checked, the values of the fields will be "
                                       "automatically formatted according to their format type."));
  m_exportSelected = new QCheckBox(i18n("Export selected entries only"), group1);
  m_exportSelected->setChecked(false);
  QWhatsThis::add(m_exportSelected, i18n("If checked, only the currently selected entries will "
                                         "be exported."));

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

ExportDialog::~ExportDialog() {
  delete m_exporter;
  m_exporter = 0;
}

QString ExportDialog::fileFilter() {
  return m_exporter ? m_exporter->fileFilter() : QString::null;
}

void ExportDialog::readOptions() {
  KConfig* config = KGlobal::config();
  config->setGroup("ExportOptions");
  bool format = config->readBoolEntry("FormatAttributes", false);
  m_formatFields->setChecked(format);
  bool selected = config->readBoolEntry("ExportSelectedOnly", false);
  m_exportSelected->setChecked(selected);
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
  config->writeEntry("ExportSelectedOnly", m_exportSelected->isChecked());
  config->writeEntry("EncodeUTF8", m_encodeUTF8->isChecked());
}

Bookcase::Export::Exporter* ExportDialog::exporter(ExportFormat format_, MainWindow* bookcase_) {
  Export::Exporter* exporter = 0;

  switch(format_) {
    case XML:
      exporter = new Export::BookcaseXMLExporter(m_coll);
      break;

    case HTML:
      {
        Export::HTMLExporter* htmlExp = new Export::HTMLExporter(m_coll);
        htmlExp->setGroupBy(bookcase_->groupBy());
        htmlExp->setSortTitles(bookcase_->sortTitles());
        htmlExp->setColumns(bookcase_->visibleColumns());
        exporter = htmlExp;
      }
      break;

    case CSV:
      exporter = new Export::CSVExporter(m_coll);
      break;

    case Bibtex:
      exporter = new Export::BibtexExporter(m_coll);
      break;

    case Bibtexml:
      exporter = new Export::BibtexmlExporter(m_coll);
      break;

    case XSLT:
      exporter = new Export::XSLTExporter(m_coll);
      break;

    case PilotDB:
      {
        Export::PilotDBExporter* pdbExp = new Export::PilotDBExporter(m_coll);
        pdbExp->setColumns(bookcase_->visibleColumns());
        exporter = pdbExp;
      }
      break;

    case Alexandria:
      exporter = new Export::AlexandriaExporter(m_coll);
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

bool ExportDialog::exportURL(const KURL& url_/*=KURL()*/) const {
  if(!m_exporter) {
    return false;
  }

  // exporter might need to know final URL, say for writing images or something
  m_exporter->setURL(url_);
  m_exporter->setEntryList(m_exportSelected->isChecked() ? Controller::self()->selectedEntries() : m_coll->entryList());

  if(exportTarget(m_format) == ExportNone) {
    return m_exporter->exportEntries(m_formatFields->isChecked());
  } else {
    if(m_exporter->isText()) {
      bool encodeUTF8 = m_encodeUTF8->isChecked();
      return FileHandler::writeTextURL(url_, m_exporter->text(m_formatFields->isChecked(), encodeUTF8), encodeUTF8);
    } else {
      return FileHandler::writeDataURL(url_, m_exporter->data(m_formatFields->isChecked()));
    }
  }
}

// alexandria is exported to known directory
// all others are files
ExportDialog::ExportTarget ExportDialog::exportTarget(ExportFormat format_) {
  switch(format_) {
    case Alexandria:
      return ExportNone;
    default:
      return ExportFile;
  }
}

#include "exportdialog.moc"
