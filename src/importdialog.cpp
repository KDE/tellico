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

#include "importdialog.h"
#include "mainwindow.h"
#include "document.h"
#include "kernel.h"

#include "translators/importer.h"
#include "translators/bookcaseimporter.h"
#include "translators/bibteximporter.h"
#include "translators/bibtexmlimporter.h"
#include "translators/csvimporter.h"
#include "translators/xsltimporter.h"
#include "translators/audiofileimporter.h"
#include "translators/alexandriaimporter.h"
#include "translators/freedbimporter.h"

#include <klocale.h>
#include <kdebug.h>
#include <kstandarddirs.h>

#include <qlayout.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>
#include <qtimer.h>

// really according to taste or computer speed
const unsigned Bookcase::Import::Importer::s_stepSize = 20;

using Bookcase::ImportDialog;

ImportDialog::ImportDialog(ImportFormat format_, const KURL& url_, MainWindow* parent_, const char* name_)
    : KDialogBase(parent_, name_, true /*modal*/, i18n("Import Options"), Ok|Cancel),
      m_coll(0),
      m_importer(importer(format_, url_)) {
  QWidget* widget = new QWidget(this);
  QVBoxLayout* topLayout = new QVBoxLayout(widget, 0, spacingHint());

  QButtonGroup* bg = new QButtonGroup(1, Qt::Horizontal, i18n("Import Options"), widget);
  topLayout->addWidget(bg, 0);
  m_radioReplace = new QRadioButton(i18n("Replace current collection"), bg);
  QWhatsThis::add(m_radioReplace, i18n("Replace the current collection with the contents "
                                       "of the imported file."));
  m_radioAppend = new QRadioButton(i18n("Append to current collection"), bg);
  QWhatsThis::add(m_radioAppend, i18n("Append the contents of the imported file to the "
                                      "current collection. This is only possible when the "
                                      "collection types match."));
  m_radioMerge = new QRadioButton(i18n("Merge with current collection"), bg);
  QWhatsThis::add(m_radioMerge, i18n("Merge the contents of the imported file to the "
                                     "current collection. This is only possible when the "
                                     "collection types match. Entries must match exactly "
                                     "in order to be merged."));
  if(m_importer->canImport(Kernel::self()->collection()->type())) {
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
  connect(m_importer, SIGNAL(signalFractionDone(float)),
          parent_, SLOT(slotUpdateFractionDone(float)));

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

Bookcase::Data::Collection* ImportDialog::collection() {
  if(m_importer && !m_coll) {
    m_coll = m_importer->collection();
  }
  return m_coll;
}

QString ImportDialog::statusMessage() const {
  return m_importer ? m_importer->statusMessage() : QString::null;
}

ImportDialog::ImportAction ImportDialog::action() const {
  if(m_radioReplace->isChecked()) {
    return Replace;
  } else if(m_radioAppend->isChecked()) {
    return Append;
  } else {
    return Merge;
  }
}

Bookcase::Import::Importer* ImportDialog::importer(ImportFormat format_, const KURL& url_) {
  Import::Importer* importer = 0;
  switch(format_) {
    case BookcaseXML:
      importer = new Import::BookcaseImporter(url_);
      break;

    case Bibtex:
      importer = new Import::BibtexImporter(url_);
      break;

    case Bibtexml:
      importer = new Import::BibtexmlImporter(url_);
      break;

    case CSV:
      importer = new Import::CSVImporter(url_);
      break;

    case XSLT:
      importer = new Import::XSLTImporter(url_);
      break;

    case MODS:
      importer = new Import::XSLTImporter(url_);
      {
        QString xsltFile = KGlobal::dirs()->findResource("appdata", QString::fromLatin1("mods2bookcase.xsl"));
        if(!xsltFile.isEmpty()) {
          KURL u;
          u.setPath(xsltFile);
          static_cast<Import::XSLTImporter*>(importer)->setXSLTURL(u);
        } else {
          kdWarning() << "ImportDialog::importer - unable to find mods2bookcase.xml!" << endl;
        }
      }
      break;

    case AudioFile:
      importer = new Import::AudioFileImporter(url_);
      break;

    case Alexandria:
      importer = new Import::AlexandriaImporter();
      break;

    case FreeDB:
      importer = new Import::FreeDBImporter();
      break;

    default:
      kdDebug() << "ImportDialog::importer() - not implemented!" << endl;
      break;
  }
#ifndef NDEBUG
  if(!m_importer) {
    kdWarning() << "ImportDialog::importer() - importer not created!" << endl;
  }
#endif
  return importer;
}

// static
QString ImportDialog::fileFilter(ImportFormat format_) {
  QString text;
  switch(format_) {
    case BookcaseXML:
      text = i18n("*.bc|Bookcase files (*.bc)") + QChar('\n');
      break;

    case Bibtex:
      text = i18n("*.bib|Bibtex files (*.bib)") + QChar('\n');
      break;

    case CSV:
      text = i18n("*.csv|CSV files (*.csv)") + QChar('\n');
      break;

    case Bibtexml:
    case XSLT:
      text = i18n("*.xml|XML files (*.xml)") + QChar('\n');
      break;

    case MODS:
      text = i18n("*.xml|XML files (*.xml)") + QChar('\n');
      break;

    case AudioFile:
    case Alexandria:
    case FreeDB:
    default:
      break;
  }

  return text + i18n("*|All files");
}

// audio files are imported by directory
// alexandria is a defined location, as is freedb
// all others are files
ImportDialog::ImportTarget ImportDialog::importTarget(ImportFormat format_) {
  switch(format_) {
    case AudioFile:
      return ImportDir;
    case Alexandria:
    case FreeDB:
      return ImportNone;
    default:
      return ImportFile;
  }
}

void ImportDialog::slotUpdateAction() {
  // distasteful hack
  // selectedId() is new in QT 3.2
//  m_importer->slotActionChanged(dynamic_cast<QButtonGroup*>(m_radioAppend->parentWidget())->selectedId());
  QButtonGroup* bg = dynamic_cast<QButtonGroup*>(m_radioAppend->parentWidget());
  m_importer->slotActionChanged(bg->id(bg->selected()));
}

#include "importdialog.moc"
