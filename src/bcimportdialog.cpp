/***************************************************************************
                             bcimportdialog.cpp
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

#include "bcimportdialog.h"
#include "bookcase.h"
#include "bookcasedoc.h"

#include "translators/importer.h"
#include "translators/bookcasexmlimporter.h"
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
const int Importer::s_stepSize = 20;

BCImportDialog::BCImportDialog(ImportFormat format_, const KURL& url_, Bookcase* parent_, const char* name_)
    : KDialogBase(parent_, name_, true /*modal*/, i18n("Import Options"), Ok|Cancel),
      m_coll(0),
      m_importer(importer(format_, url_)) {
  QWidget* widget = new QWidget(this);
  QVBoxLayout* topLayout = new QVBoxLayout(widget, 0, spacingHint());

  QButtonGroup* bg = new QButtonGroup(1, Qt::Horizontal, i18n("Import Options"), widget);
  topLayout->addWidget(bg, 0);
  m_radioReplace = new QRadioButton(i18n("Replace current collection"), bg);
  m_radioReplace->setChecked(true);
  QWhatsThis::add(m_radioReplace, i18n("Replace the current collection with the contents "
                                       "of the imported file."));
  m_radioAppend = new QRadioButton(i18n("Append to current collection"), bg);
  QWhatsThis::add(m_radioAppend, i18n("Append the contents of the imported file to the "
                                      "current collection. This is only possible when the "
                                      "collection types match."));
//  m_radioAppend->setEnabled(false);

  QWidget* w = m_importer->widget(widget);
//  m_importer->readOptions(KGlobal::config());
  if(w) {
    topLayout->addWidget(w, 0);
  }

  connect(m_importer, SIGNAL(signalFractionDone(float)),
          parent_, SLOT(slotUpdateFractionDone(float)));

  topLayout->addStretch();
  setMainWidget(widget);
}

BCCollection* BCImportDialog::collection() {
  if(m_importer && !m_coll) {
    m_coll = m_importer->collection();
  }
  return m_coll;
}

QString BCImportDialog::statusMessage() const {
  return m_importer ? m_importer->statusMessage() : QString::null;
}

bool BCImportDialog::replaceCollection() const {
  return m_radioReplace->isChecked();
}

Importer* BCImportDialog::importer(ImportFormat format_, const KURL& url_) {
  Importer* importer = 0;
  switch(format_) {
    case BookcaseXML:
      importer = new BookcaseXMLImporter(url_);
      break;

    case Bibtex:
      importer = new BibtexImporter(url_);
      break;

    case Bibtexml:
      importer = new BibtexmlImporter(url_);
      break;

    case CSV:
      importer = new CSVImporter(url_);
      break;

    case XSLT:
      importer = new XSLTImporter(url_);
      break;

    default:
      kdDebug() << "BCImportDialog::importer() - not implemented!" << endl;
      break;
  }
  return importer;
}

// static
QString BCImportDialog::fileFilter(ImportFormat format_) {
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

    default:
      break;
  }

  return text + i18n("*|All files");
}
