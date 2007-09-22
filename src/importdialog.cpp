/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "importdialog.h"
#include "document.h"
#include "tellico_kernel.h"
#include "tellico_debug.h"
#include "collection.h"

#include "translators/importer.h"
#include "translators/tellicoimporter.h"
#include "translators/bibteximporter.h"
#include "translators/bibtexmlimporter.h"
#include "translators/csvimporter.h"
#include "translators/xsltimporter.h"
#include "translators/audiofileimporter.h"
#include "translators/alexandriaimporter.h"
#include "translators/freedbimporter.h"
#include "translators/risimporter.h"
#include "translators/gcfilmsimporter.h"
#include "translators/filelistingimporter.h"
#include "translators/amcimporter.h"

#include <klocale.h>
#include <kstandarddirs.h>

#include <qlayout.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>
#include <qtimer.h>

// really according to taste or computer speed
const unsigned Tellico::Import::Importer::s_stepSize = 20;

using Tellico::ImportDialog;

ImportDialog::ImportDialog(Import::Format format_, const KURL& url_, QWidget* parent_, const char* name_)
    : KDialogBase(parent_, name_, true /*modal*/, i18n("Import Options"), Ok|Cancel),
      m_coll(0),
      m_importer(importer(format_, url_)) {
  QWidget* widget = new QWidget(this);
  QVBoxLayout* topLayout = new QVBoxLayout(widget, 0, spacingHint());

  QButtonGroup* bg = new QButtonGroup(1, Qt::Horizontal, i18n("Import Options"), widget);
  topLayout->addWidget(bg, 0);
  m_radioReplace = new QRadioButton(i18n("&Replace current collection"), bg);
  QWhatsThis::add(m_radioReplace, i18n("Replace the current collection with the contents "
                                       "of the imported file."));
  m_radioAppend = new QRadioButton(i18n("A&ppend to current collection"), bg);
  QWhatsThis::add(m_radioAppend, i18n("Append the contents of the imported file to the "
                                      "current collection. This is only possible when the "
                                      "collection types match."));
  m_radioMerge = new QRadioButton(i18n("&Merge with current collection"), bg);
  QWhatsThis::add(m_radioMerge, i18n("Merge the contents of the imported file to the "
                                     "current collection. This is only possible when the "
                                     "collection types match. Entries must match exactly "
                                     "in order to be merged."));
  if(m_importer->canImport(Kernel::self()->collectionType())) {
    // append by default?
    m_radioAppend->setChecked(true);
  } else {
    m_radioReplace->setChecked(true);
    m_radioAppend->setEnabled(false);
    m_radioMerge->setEnabled(false);
  }

  QWidget* w = m_importer->widget(widget, "importer_widget");
//  m_importer->readOptions(KGlobal::config());
  if(w) {
    topLayout->addWidget(w, 0);
  }

  connect(bg, SIGNAL(clicked(int)), m_importer, SLOT(slotActionChanged(int)));

  topLayout->addStretch();
  setMainWidget(widget);

  // want to grab default button action, too
  // since the importer might do something with widgets, don't just call it, do it after layout is done
  QTimer::singleShot(0, this, SLOT(slotUpdateAction()));
}

ImportDialog::~ImportDialog() {
  delete m_importer;
  m_importer = 0;
}

Tellico::Data::CollPtr ImportDialog::collection() {
  if(m_importer && !m_coll) {
    m_coll = m_importer->collection();
  }
  return m_coll;
}

QString ImportDialog::statusMessage() const {
  return m_importer ? m_importer->statusMessage() : QString::null;
}

Tellico::Import::Action ImportDialog::action() const {
  if(m_radioReplace->isChecked()) {
    return Import::Replace;
  } else if(m_radioAppend->isChecked()) {
    return Import::Append;
  } else {
    return Import::Merge;
  }
}

// static
Tellico::Import::Importer* ImportDialog::importer(Import::Format format_, const KURL& url_) {
  Import::Importer* importer = 0;
  switch(format_) {
    case Import::TellicoXML:
      importer = new Import::TellicoImporter(url_);
      break;

    case Import::Bibtex:
      importer = new Import::BibtexImporter(url_);
      break;

    case Import::Bibtexml:
      importer = new Import::BibtexmlImporter(url_);
      break;

    case Import::CSV:
      importer = new Import::CSVImporter(url_);
      break;

    case Import::XSLT:
      importer = new Import::XSLTImporter(url_);
      break;

    case Import::MODS:
      importer = new Import::XSLTImporter(url_);
      {
        QString xsltFile = locate("appdata", QString::fromLatin1("mods2tellico.xsl"));
        if(!xsltFile.isEmpty()) {
          KURL u;
          u.setPath(xsltFile);
          static_cast<Import::XSLTImporter*>(importer)->setXSLTURL(u);
        } else {
          kdWarning() << "ImportDialog::importer - unable to find mods2tellico.xml!" << endl;
        }
      }
      break;

    case Import::AudioFile:
      importer = new Import::AudioFileImporter(url_);
      break;

    case Import::Alexandria:
      importer = new Import::AlexandriaImporter();
      break;

    case Import::FreeDB:
      importer = new Import::FreeDBImporter();
      break;

    case Import::RIS:
      importer = new Import::RISImporter(url_);
      break;

    case Import::GCfilms:
      importer = new Import::GCfilmsImporter(url_);
      break;

    case Import::FileListing:
      importer = new Import::FileListingImporter(url_);
      break;

    case Import::AMC:
      importer = new Import::AMCImporter(url_);
      break;

    default:
      myDebug() << "ImportDialog::importer() - not implemented!" << endl;
      break;
  }
#ifndef NDEBUG
  if(!importer) {
    kdWarning() << "ImportDialog::importer() - importer not created!" << endl;
  }
#endif
  return importer;
}

// static
QString ImportDialog::fileFilter(Import::Format format_) {
  QString text;
  switch(format_) {
    case Import::TellicoXML:
      text = i18n("*.tc *.bc|Tellico Files (*.tc)") + QChar('\n');
      text += i18n("*.xml|XML Files (*.xml)") + QChar('\n');
      break;

    case Import::Bibtex:
      text = i18n("*.bib|Bibtex Files (*.bib)") + QChar('\n');
      break;

    case Import::CSV:
      text = i18n("*.csv|CSV Files (*.csv)") + QChar('\n');
      break;

    case Import::Bibtexml:
    case Import::XSLT:
      text = i18n("*.xml|XML Files (*.xml)") + QChar('\n');
      break;

    case Import::MODS:
      text = i18n("*.xml|XML Files (*.xml)") + QChar('\n');
      break;

    case Import::RIS:
      text = i18n("*.ris|RIS Files (*.ris)") + QChar('\n');
      break;

    case Import::GCfilms:
      text = i18n("*.gcs|GCstar Data Files (*.gcs)") + QChar('\n');
      text += i18n("*.gcf|GCfilms Data Files (*.gcf)") + QChar('\n');
      break;

    case Import::AMC:
      text = i18n("*.amc|AMC Data Files (*.amc)") + QChar('\n');
      break;

    case Import::AudioFile:
    case Import::Alexandria:
    case Import::FreeDB:
    case Import::FileListing:
    default:
      break;
  }

  return text + i18n("*|All Files");
}

// audio files are imported by directory
// alexandria is a defined location, as is freedb
// all others are files
Tellico::Import::Target ImportDialog::importTarget(Import::Format format_) {
  switch(format_) {
    case Import::AudioFile:
    case Import::FileListing:
      return Import::Dir;
    case Import::Alexandria:
    case Import::FreeDB:
      return Import::None;
    default:
      return Import::File;
  }
}

Tellico::Import::FormatMap ImportDialog::formatMap() {
  Import::FormatMap map;
  map[Import::TellicoXML] = QString::fromLatin1("Tellico");
  map[Import::Bibtex]     = i18n("Bibtex");
  map[Import::Bibtexml]   = i18n("Bibtexml");
//  map[Import::CSV]        = i18n("CSV");
  map[Import::MODS]       = i18n("MODS");
  map[Import::RIS]        = i18n("RIS");
  map[Import::GCfilms]    = i18n("GCstar");
  map[Import::AMC]        = i18n("AMC");
  return map;
}

QString ImportDialog::startDir(Import::Format format_) {
  if(format_ == Import::GCfilms) {
    QDir dir = QDir::home();
    // able to cd if exists and readable
    if(dir.cd(QString::fromLatin1(".local/share/gcfilms/"))) {
      return dir.absPath();
    }
  }
  return QString::fromLatin1(":import");
}

void ImportDialog::slotUpdateAction() {
  // distasteful hack
  // selectedId() is new in QT 3.2
//  m_importer->slotActionChanged(dynamic_cast<QButtonGroup*>(m_radioAppend->parentWidget())->selectedId());
  QButtonGroup* bg = static_cast<QButtonGroup*>(m_radioAppend->parentWidget());
  m_importer->slotActionChanged(bg->id(bg->selected()));
}

// static
Tellico::Data::CollPtr ImportDialog::importURL(Import::Format format_, const KURL& url_) {
  Import::Importer* imp = importer(format_, url_);
  Data::CollPtr c = imp->collection();
  if(!c && !imp->statusMessage().isEmpty()) {
    Kernel::self()->sorry(imp->statusMessage());
  }
  delete imp;
  return c;
}


#include "importdialog.moc"
