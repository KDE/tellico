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

#include "translators/importer.h"
#include "translators/bookcaseimporter.h"
#include "translators/bibteximporter.h"
#include "translators/bibtexmlimporter.h"
#include "translators/csvimporter.h"
#include "translators/xsltimporter.h"

#include <klocale.h>
#include <kdebug.h>

#include <qlayout.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>

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
  // append by default?
  m_radioAppend->setChecked(true);


  QWidget* w = m_importer->widget(widget, "importer_widget");
//  m_importer->readOptions(KGlobal::config());
  if(w) {
    topLayout->addWidget(w, 0);
  }

  connect(m_importer, SIGNAL(signalFractionDone(float)),
          parent_, SLOT(slotUpdateFractionDone(float)));

  topLayout->addStretch();
  setMainWidget(widget);
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

    case AudioFile:
    default:
      kdDebug() << "ImportDialog::importer() - not implemented!" << endl;
      break;
  }
  return importer;
}

// static
QString ImportDialog::fileFilter(ImportFormat format_) {
  QString text;
  switch(format_) {
    case BookcaseXML:
      text = i18n("*.bc|Bookcase files (*.bc)") + QString::fromLatin1("\n");
      break;

    case Bibtex:
      text = i18n("*.bib|Bibtex files (*.bib)") + QString::fromLatin1("\n");
      break;

    case CSV:
      text = i18n("*.csv|CSV files (*.csv)") + QString::fromLatin1("\n");
      break;

    case Bibtexml:
    case XSLT:
      text = i18n("*.xml|XML files (*.xml)") + QString::fromLatin1("\n");
      break;

    case AudioFile:
      // FIXME: add mp3, too?
      text = i18n("*.ogg|Ogg files (*.ogg)") + QString::fromLatin1("\n");
      break;

    default:
      break;
  }

  return text + i18n("*|All files");
}

bool ImportDialog::selectFileFirst(ImportFormat format_) {
  return (format_ != AudioFile);
}
