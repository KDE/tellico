/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 ***************************************************************************/

#include "importdialog.h"
#include "document.h"
#include "tellico_kernel.h"
#include "tellico_debug.h"
#include "collection.h"
#include "progressmanager.h"

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
#include "translators/griffithimporter.h"
#include "translators/pdfimporter.h"
#include "translators/referencerimporter.h"
#include "translators/deliciousimporter.h"

#include <klocale.h>
#include <kstandarddirs.h>
#include <kstandardguiitem.h>

#include <QGroupBox>
#include <QButtonGroup>
#include <QRadioButton>
#include <QCheckBox>
#include <QTimer>
#include <QVBoxLayout>

using Tellico::ImportDialog;

ImportDialog::ImportDialog(Tellico::Import::Format format_, const KUrl::List& urls_, QWidget* parent_)
    : KDialog(parent_),
      m_importer(importer(format_, urls_)) {
  setModal(true);
  setCaption(i18n("Import Options"));
  setButtons(Ok|Cancel);

  QWidget* widget = new QWidget(this);
  QVBoxLayout* topLayout = new QVBoxLayout(widget);

  QGroupBox* groupBox = new QGroupBox(i18n("Import Options"), widget);
  QVBoxLayout* vlay = new QVBoxLayout(groupBox);
  topLayout->addWidget(groupBox, 0);

  m_radioReplace = new QRadioButton(i18n("&Replace current collection"), groupBox);
  m_radioReplace->setWhatsThis(i18n("Replace the current collection with the contents "
                                     "of the imported file."));
  m_radioAppend = new QRadioButton(i18n("A&ppend to current collection"), groupBox);
  m_radioAppend->setWhatsThis(i18n("Append the contents of the imported file to the "
                                   "current collection. This is only possible when the "
                                   "collection types match."));
  m_radioMerge = new QRadioButton(i18n("&Merge with current collection"), groupBox);
  m_radioMerge->setWhatsThis(i18n("Merge the contents of the imported file to the "
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

  vlay->addWidget(m_radioReplace);
  vlay->addWidget(m_radioAppend);
  vlay->addWidget(m_radioMerge);

  m_buttonGroup = new QButtonGroup(widget);
  m_buttonGroup->addButton(m_radioReplace, Import::Replace);
  m_buttonGroup->addButton(m_radioAppend, Import::Append);
  m_buttonGroup->addButton(m_radioMerge, Import::Merge);

  QWidget* w = m_importer->widget(widget);
//  m_importer->readOptions(KGlobal::config());
  if(w) {
    w->layout()->setMargin(0);
    topLayout->addWidget(w, 0);
  }

  connect(m_buttonGroup, SIGNAL(buttonClicked(int)), m_importer, SLOT(slotActionChanged(int)));

  topLayout->addStretch();
  setMainWidget(widget);

  KGuiItem ok = KStandardGuiItem::ok();
  ok.setText(i18n("&Import"));
  setButtonGuiItem(Ok, ok);

  // want to grab default button action, too
  // since the importer might do something with widgets, don't just call it, do it after layout is done
  QTimer::singleShot(0, this, SLOT(slotUpdateAction()));

  connect(this, SIGNAL(okClicked()), SLOT(slotOk()));
}

ImportDialog::~ImportDialog() {
  delete m_importer;
  m_importer = 0;
}

Tellico::Data::CollPtr ImportDialog::collection() {
  if(m_importer && !m_coll) {
    ProgressItem& item = ProgressManager::self()->newProgressItem(m_importer, m_importer->progressLabel(), true);
    connect(m_importer, SIGNAL(signalTotalSteps(QObject*, qulonglong)),
            ProgressManager::self(), SLOT(setTotalSteps(QObject*, qulonglong)));
    connect(m_importer, SIGNAL(signalProgress(QObject*, qulonglong)),
            ProgressManager::self(), SLOT(setProgress(QObject*, qulonglong)));
    connect(&item, SIGNAL(signalCancelled(ProgressItem*)), m_importer, SLOT(slotCancel()));
    ProgressItem::Done done(m_importer);
    m_coll = m_importer->collection();
  }
  return m_coll;
}

QString ImportDialog::statusMessage() const {
  return m_importer ? m_importer->statusMessage() : QString();
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
Tellico::Import::Importer* ImportDialog::importer(Tellico::Import::Format format_, const KUrl::List& urls_) {
#define CHECK_SIZE if(urls_.size() > 1) myWarning() << "only importing first URL"
  KUrl firstURL = urls_.isEmpty() ? KUrl() : urls_[0];
  Import::Importer* importer = 0;
  switch(format_) {
    case Import::TellicoXML:
      CHECK_SIZE;
      importer = new Import::TellicoImporter(firstURL);
      break;

    case Import::Bibtex:
      importer = new Import::BibtexImporter(urls_);
      break;

    case Import::Bibtexml:
      CHECK_SIZE;
      importer = new Import::BibtexmlImporter(firstURL);
      break;

    case Import::CSV:
      CHECK_SIZE;
      importer = new Import::CSVImporter(firstURL);
      break;

    case Import::XSLT:
      CHECK_SIZE;
      importer = new Import::XSLTImporter(firstURL);
      break;

    case Import::MODS:
      CHECK_SIZE;
      importer = new Import::XSLTImporter(firstURL);
      {
        QString xsltFile = KStandardDirs::locate("appdata", QLatin1String("mods2tellico.xsl"));
        if(!xsltFile.isEmpty()) {
          KUrl u;
          u.setPath(xsltFile);
          static_cast<Import::XSLTImporter*>(importer)->setXSLTURL(u);
        } else {
          myWarning() << "unable to find mods2tellico.xml!";
        }
      }
      break;

    case Import::AudioFile:
      CHECK_SIZE;
      importer = new Import::AudioFileImporter(firstURL);
      break;

    case Import::Alexandria:
      CHECK_SIZE;
      importer = new Import::AlexandriaImporter();
      break;

    case Import::FreeDB:
      CHECK_SIZE;
      importer = new Import::FreeDBImporter();
      break;

    case Import::RIS:
      importer = new Import::RISImporter(urls_);
      break;

    case Import::GCfilms:
      CHECK_SIZE;
      importer = new Import::GCfilmsImporter(firstURL);
      break;

    case Import::FileListing:
      CHECK_SIZE;
      importer = new Import::FileListingImporter(firstURL);
      break;

    case Import::AMC:
      CHECK_SIZE;
      importer = new Import::AMCImporter(firstURL);
      break;

    case Import::Griffith:
      importer = new Import::GriffithImporter();
      break;

    case Import::PDF:
      importer = new Import::PDFImporter(urls_);
      break;

    case Import::Referencer:
      CHECK_SIZE;
      importer = new Import::ReferencerImporter(firstURL);
      break;

    case Import::Delicious:
      CHECK_SIZE;
      importer = new Import::DeliciousImporter(firstURL);
      break;

    case Import::GRS1:
      myDebug() << "GRS1 not implemented";
      break;
  }
  if(!importer) {
    myWarning() << "importer not created!";
    return 0;
  }
  importer->setCurrentCollection(Data::Document::self()->collection());
  return importer;
#undef CHECK_SIZE
}

// static
QString ImportDialog::fileFilter(Tellico::Import::Format format_) {
  QString text;
  switch(format_) {
    case Import::TellicoXML:
      text = i18n("*.tc *.bc|Tellico Files (*.tc)") + QLatin1Char('\n');
      text += i18n("*.xml|XML Files (*.xml)") + QLatin1Char('\n');
      break;

    case Import::Bibtex:
      text = i18n("*.bib|Bibtex Files (*.bib)") + QLatin1Char('\n');
      break;

    case Import::CSV:
      text = i18n("*.csv|CSV Files (*.csv)") + QLatin1Char('\n');
      break;

    case Import::Bibtexml:
    case Import::XSLT:
      text = i18n("*.xml|XML Files (*.xml)") + QLatin1Char('\n');
      break;

    case Import::MODS:
    case Import::Delicious:
      text = i18n("*.xml|XML Files (*.xml)") + QLatin1Char('\n');
      break;

    case Import::RIS:
      text = i18n("*.ris|RIS Files (*.ris)") + QLatin1Char('\n');
      break;

    case Import::GCfilms:
      text = i18n("*.gcs|GCstar Data Files (*.gcs)") + QLatin1Char('\n');
      text += i18n("*.gcf|GCfilms Data Files (*.gcf)") + QLatin1Char('\n');
      break;

    case Import::AMC:
      text = i18n("*.amc|AMC Data Files (*.amc)") + QLatin1Char('\n');
      break;

    case Import::PDF:
      text = i18n("*.pdf|PDF Files (*.pdf)") + QLatin1Char('\n');
      break;

    case Import::Referencer:
      text = i18n("*.reflib|Referencer Files (*.reflib)") + QLatin1Char('\n');
      break;

    case Import::AudioFile:
    case Import::Alexandria:
    case Import::FreeDB:
    case Import::FileListing:
    case Import::GRS1:
    case Import::Griffith:
      break;
  }

  return text + i18n("*|All Files");
}

// audio files are imported by directory
// alexandria is a defined location, as is freedb
// all others are files
Tellico::Import::Target ImportDialog::importTarget(Tellico::Import::Format format_) {
  switch(format_) {
    case Import::AudioFile:
    case Import::FileListing:
      return Import::Dir;
    case Import::Griffith:
    case Import::Alexandria:
    case Import::FreeDB:
      return Import::None;
    default:
      return Import::File;
  }
}

Tellico::Import::FormatMap ImportDialog::formatMap() {
  // at one point, these were translated, but after some thought
  // I decided they were likely to be the same in any language
  // transliteration is unlikely
  Import::FormatMap map;
  map[Import::TellicoXML] = QLatin1String("Tellico");
  map[Import::Bibtex]     = QLatin1String("Bibtex");
  map[Import::Bibtexml]   = QLatin1String("Bibtexml");
//  map[Import::CSV]        = QLatin1String("CSV");
  map[Import::MODS]       = QLatin1String("MODS");
  map[Import::RIS]        = QLatin1String("RIS");
  map[Import::GCfilms]    = QLatin1String("GCstar");
  map[Import::AMC]        = QLatin1String("AMC");
  map[Import::Griffith]   = QLatin1String("Griffith");
  map[Import::PDF]        = QLatin1String("PDF");
  map[Import::Referencer] = QLatin1String("Referencer");
  map[Import::Delicious ] = QLatin1String("Delicious Library");
  return map;
}

bool ImportDialog::formatImportsText(Tellico::Import::Format format_) {
  return format_ != Import::AMC &&
         format_ != Import::Griffith &&
         format_ != Import::PDF;
}

QString ImportDialog::startDir(Tellico::Import::Format format_) {
  if(format_ == Import::GCfilms) {
    QDir dir = QDir::home();
    // able to cd if exists and readable
    if(dir.cd(QLatin1String(".local/share/gcfilms/"))) {
      return dir.absolutePath();
    }
  }
  return QLatin1String(":import");
}

void ImportDialog::slotOk() {
  // some importers, like the CSV importer, can validate their settings
  if(!m_importer || m_importer->validImport()) {
    accept();
  } else {
    myLog() << "not a valid import";
  }
}

void ImportDialog::slotUpdateAction() {
  m_importer->slotActionChanged(m_buttonGroup->checkedId());
}

// static
Tellico::Data::CollPtr ImportDialog::importURL(Tellico::Import::Format format_, const KUrl& url_) {
  Import::Importer* imp = importer(format_, url_);

  ProgressItem& item = ProgressManager::self()->newProgressItem(imp, imp->progressLabel(), true);
  connect(imp, SIGNAL(signalTotalSteps(QObject*, qulonglong)),
          ProgressManager::self(), SLOT(setTotalSteps(QObject*, qulonglong)));
  connect(imp, SIGNAL(signalProgress(QObject*, qulonglong)),
          ProgressManager::self(), SLOT(setProgress(QObject*, qulonglong)));
  connect(&item, SIGNAL(signalCancelled(ProgressItem*)), imp, SLOT(slotCancel()));
  ProgressItem::Done done(imp);

  Data::CollPtr c = imp->collection();
  if(!c && !imp->statusMessage().isEmpty()) {
    Kernel::self()->sorry(imp->statusMessage());
  }
  delete imp;
  return c;
}


#include "importdialog.moc"
