/***************************************************************************
    copyright            : (C) 2003-2005 by Robby Stephenson
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
#include "filehandler.h"
#include "controller.h"
#include "document.h"

#include "translators/exporter.h"
#include "translators/tellicoxmlexporter.h"
#include "translators/htmlexporter.h"
#include "translators/csvexporter.h"
#include "translators/bibtexexporter.h"
#include "translators/bibtexmlexporter.h"
#include "translators/xsltexporter.h"
#include "translators/pilotdbexporter.h"
#include "translators/alexandriaexporter.h"
#include "translators/onixexporter.h"

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

using Tellico::ExportDialog;

ExportDialog::ExportDialog(Export::Format format_, Data::Collection* coll_, QWidget* parent_, const char* name_)
    : KDialogBase(parent_, name_, true /*modal*/, i18n("Export Options"), Ok|Cancel),
      m_format(format_), m_coll(coll_), m_exporter(exporter(format_)) {
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
  if(format_ == Export::Bibtex || format_ == Export::CSV || format_ == Export::Text) {
    m_encodeUTF8->setEnabled(false);
    m_encodeLocale->setChecked(true);
//    m_encodeLocale->setEnabled(false);
  } else if(format_ == Export::Alexandria || format_ == Export::PilotDB) {
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
  bool format = config->readBoolEntry("FormatFields", false);
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
  config->writeEntry("FormatFields", m_formatFields->isChecked());
  config->writeEntry("ExportSelectedOnly", m_exportSelected->isChecked());
  config->writeEntry("EncodeUTF8", m_encodeUTF8->isChecked());
}

// static
Tellico::Export::Exporter* ExportDialog::exporter(Export::Format format_) {
  Export::Exporter* exporter = 0;

  switch(format_) {
    case Export::TellicoXML:
      exporter = new Export::TellicoXMLExporter();
      break;

    case Export::HTML:
      {
        Export::HTMLExporter* htmlExp = new Export::HTMLExporter();
        htmlExp->setGroupBy(Controller::self()->expandedGroupBy());
        htmlExp->setSortTitles(Controller::self()->sortTitles());
        htmlExp->setColumns(Controller::self()->visibleColumns());
        exporter = htmlExp;
      }
      break;

    case Export::CSV:
      exporter = new Export::CSVExporter();
      break;

    case Export::Bibtex:
      exporter = new Export::BibtexExporter();
      break;

    case Export::Bibtexml:
      exporter = new Export::BibtexmlExporter();
      break;

    case Export::XSLT:
      exporter = new Export::XSLTExporter();
      break;

    case Export::PilotDB:
      {
        Export::PilotDBExporter* pdbExp = new Export::PilotDBExporter();
        pdbExp->setColumns(Controller::self()->visibleColumns());
        exporter = pdbExp;
      }
      break;

    case Export::Alexandria:
      exporter = new Export::AlexandriaExporter();
      break;

    case Export::ONIX:
      exporter = new Export::ONIXExporter();
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
  if(m_exportSelected->isChecked()) {
    m_exporter->setEntries(Controller::self()->selectedEntries());
  } else {
    m_exporter->setEntries(m_coll->entries());
  }
  int opt = Export::ExportImages | Export::ExportComplete; // for now, always export images
  if(m_formatFields->isChecked()) {
    opt |= Export::ExportFormatted;
  }
  if(m_encodeUTF8->isChecked()) {
    opt |= Export::ExportUTF8;
  }
  m_exporter->setOptions(opt);

  return m_exporter->exec();
}

// static
// alexandria is exported to known directory
// all others are files
Tellico::Export::Target ExportDialog::exportTarget(Export::Format format_) {
  switch(format_) {
    case Export::Alexandria:
      return Export::None;
    default:
      return Export::File;
  }
}

// static
bool ExportDialog::exportCollection(Export::Format format_, const KURL& url_) {
  Export::Exporter* exp = exporter(format_);

  exp->setURL(url_);
  exp->setEntries(Data::Document::self()->collection()->entries());

  KConfig* config = KGlobal::config();
  config->setGroup("ExportOptions");
  int options = 0;
  if(config->readBoolEntry("FormatFields", false)) {
    options |= Export::ExportFormatted;
  }
  if(config->readBoolEntry("EncodeUTF8", true)) {
    options |= Export::ExportUTF8;
  }
  exp->setOptions(options | Export::ExportForce);

  bool success = exp->exec();
  delete exp;
  return success;
}

#include "exportdialog.moc"
