/***************************************************************************
    copyright            : (C) 2003-2008 by Robby Stephenson
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
#include "translators/tellicozipexporter.h"
#include "translators/htmlexporter.h"
#include "translators/csvexporter.h"
#include "translators/bibtexexporter.h"
#include "translators/bibtexmlexporter.h"
#include "translators/xsltexporter.h"
#include "translators/pilotdbexporter.h"
#include "translators/alexandriaexporter.h"
#include "translators/onixexporter.h"
#include "translators/gcfilmsexporter.h"

#include <klocale.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kconfig.h>

#include <QLayout>
#include <QCheckBox>
#include <QGroupBox>
#include <QButtonGroup>
#include <QRadioButton>
#include <QTextCodec>
#include <QVBoxLayout>

using Tellico::ExportDialog;

ExportDialog::ExportDialog(Tellico::Export::Format format_, Tellico::Data::CollPtr coll_, QWidget* parent_)
    : KDialog(parent_),
      m_format(format_), m_coll(coll_), m_exporter(exporter(format_)) {
  setModal(true);
  setCaption(i18n("Export Options"));
  setButtons(Ok|Cancel);

  QWidget* widget = new QWidget(this);
  QVBoxLayout* topLayout = new QVBoxLayout(widget);

  QGroupBox* group1 = new QGroupBox(i18n("Formatting"), widget);
  topLayout->addWidget(group1, 0);
  QVBoxLayout* vlay = new QVBoxLayout(group1);

  m_formatFields = new QCheckBox(i18n("Format all fields"), group1);
  m_formatFields->setChecked(false);
  m_formatFields->setWhatsThis(i18n("If checked, the values of the fields will be "
                                    "automatically formatted according to their format type."));
  vlay->addWidget(m_formatFields);

  m_exportSelected = new QCheckBox(i18n("Export selected entries only"), group1);
  m_exportSelected->setChecked(false);
  m_exportSelected->setWhatsThis(i18n("If checked, only the currently selected entries will "
                                      "be exported."));
  vlay->addWidget(m_exportSelected);

  QGroupBox* group2 = new QGroupBox(i18n("Encoding"), widget);
  topLayout->addWidget(group2, 0);

  QVBoxLayout* vlay2 = new QVBoxLayout(group2);

  m_encodeUTF8 = new QRadioButton(i18n("Encode in Unicode (UTF-8)"), group2);
  m_encodeUTF8->setChecked(true);
  m_encodeUTF8->setWhatsThis(i18n("Encode the exported file in Unicode (UTF-8)."));
  vlay2->addWidget(m_encodeUTF8);

  QString localStr = i18n("Encode in user locale (%1)",
                          QLatin1String(QTextCodec::codecForLocale()->name()));
  m_encodeLocale = new QRadioButton(localStr, group2);
  m_encodeLocale->setWhatsThis(i18n("Encode the exported file in the local encoding."));
  vlay2->addWidget(m_encodeLocale);

  QButtonGroup* bg = new QButtonGroup(widget);
  bg->addButton(m_encodeUTF8);
  bg->addButton(m_encodeLocale);

  QWidget* w = m_exporter->widget(widget);
  if(w) {
    w->layout()->setMargin(0);
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
    // no encoding options enabled
    group2->setEnabled(false);
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
  KConfigGroup config(KGlobal::config(), "ExportOptions");
  bool format = config.readEntry("FormatFields", false);
  m_formatFields->setChecked(format);
  bool selected = config.readEntry("ExportSelectedOnly", false);
  m_exportSelected->setChecked(selected);
  bool encode = config.readEntry("EncodeUTF8", true);
  if(encode) {
    m_encodeUTF8->setChecked(true);
  } else {
    m_encodeLocale->setChecked(true);
  }
}

void ExportDialog::slotSaveOptions() {
  KSharedConfigPtr config = KGlobal::config();
  // each exporter sets its own group
  m_exporter->saveOptions(config);

  KConfigGroup configGroup(config, "ExportOptions");
  configGroup.writeEntry("FormatFields", m_formatFields->isChecked());
  configGroup.writeEntry("ExportSelectedOnly", m_exportSelected->isChecked());
  configGroup.writeEntry("EncodeUTF8", m_encodeUTF8->isChecked());
}

// static
Tellico::Export::Exporter* ExportDialog::exporter(Tellico::Export::Format format_) {
  Export::Exporter* exporter = 0;

  switch(format_) {
    case Export::TellicoXML:
      exporter = new Export::TellicoXMLExporter();
      break;

    case Export::TellicoZip:
      exporter = new Export::TellicoZipExporter();
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

    case Export::GCfilms:
      exporter = new Export::GCfilmsExporter();
      break;

    default:
      kDebug() << "ExportDialog::exporter() - not implemented!";
      break;
  }
  if(exporter) {
    exporter->readOptions(KGlobal::config());
  }
  return exporter;
}

bool ExportDialog::exportURL(const KUrl& url_/*=KUrl()*/) const {
  if(!m_exporter) {
    return false;
  }

  if(!url_.isEmpty() && !FileHandler::queryExists(url_)) {
    return false;
  }

  // exporter might need to know final URL, say for writing images or something
  m_exporter->setURL(url_);
  if(m_exportSelected->isChecked()) {
    m_exporter->setEntries(Controller::self()->selectedEntries());
  } else {
    m_exporter->setEntries(m_coll->entries());
  }
  long opt = Export::ExportImages | Export::ExportComplete | Export::ExportProgress; // for now, always export images
  if(m_formatFields->isChecked()) {
    opt |= Export::ExportFormatted;
  }
  if(m_encodeUTF8->isChecked()) {
    opt |= Export::ExportUTF8;
  }
  // since we already asked about overwriting the file, force the save
  opt |= Export::ExportForce;

  m_exporter->setOptions(opt);

  return m_exporter->exec();
}

// static
// alexandria is exported to known directory
// all others are files
Tellico::Export::Target ExportDialog::exportTarget(Tellico::Export::Format format_) {
  switch(format_) {
    case Export::Alexandria:
      return Export::None;
    default:
      return Export::File;
  }
}

// static
bool ExportDialog::exportCollection(Tellico::Export::Format format_, const KUrl& url_) {
  Export::Exporter* exp = exporter(format_);

  exp->setURL(url_);
  exp->setEntries(Data::Document::self()->collection()->entries());

  KConfigGroup config(KGlobal::config(), "ExportOptions");
  long options = 0;
  if(config.readEntry("FormatFields", false)) {
    options |= Export::ExportFormatted;
  }
  if(config.readEntry("EncodeUTF8", true)) {
    options |= Export::ExportUTF8;
  }
  exp->setOptions(options | Export::ExportForce);

  bool success = exp->exec();
  delete exp;
  return success;
}

#include "exportdialog.moc"
