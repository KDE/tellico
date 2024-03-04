/***************************************************************************
    Copyright (C) 2003-2014 Robby Stephenson <robby@periapsis.org>
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

#include <config.h>
#include "importdialog.h"
#include "document.h"
#include "tellico_debug.h"
#include "collection.h"
#include "progressmanager.h"
#include "utils/guiproxy.h"

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
#include "translators/gcstarimporter.h"
#include "translators/filelistingimporter.h"
#include "translators/amcimporter.h"
#include "translators/griffithimporter.h"
#include "translators/pdfimporter.h"
#include "translators/referencerimporter.h"
#include "translators/deliciousimporter.h"
#include "translators/goodreadsimporter.h"
#include "translators/ciwimporter.h"
#include "translators/vinoxmlimporter.h"
#include "translators/boardgamegeekimporter.h"
#include "translators/librarythingimporter.h"
#include "translators/collectorzimporter.h"
#include "translators/datacrowimporter.h"
#include "translators/marcimporter.h"
#include "translators/ebookimporter.h"
#include "translators/discogsimporter.h"
#include "utils/datafileregistry.h"

#include <KLocalizedString>
#include <KStandardGuiItem>

#include <QGroupBox>
#include <QButtonGroup>
#include <QRadioButton>
#include <QCheckBox>
#include <QTimer>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>

using Tellico::ImportDialog;

ImportDialog::ImportDialog(Tellico::Import::Format format_, const QList<QUrl>& urls_, QWidget* parent_)
    : QDialog(parent_),
      m_importer(importer(format_, urls_)) {
  setModal(true);
  setWindowTitle(i18n("Import Options"));

  QVBoxLayout* mainLayout = new QVBoxLayout();
  setLayout(mainLayout);

  QWidget* widget = new QWidget(this);
  mainLayout->addWidget(widget);
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
  if(m_importer->canImport(Data::Document::self()->collection()->type())) {
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
//  m_importer->readOptions(KSharedConfig::openConfig());
  if(w) {
    w->layout()->setContentsMargins(0, 0, 0, 0);
    topLayout->addWidget(w, 0);
  }

#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
  connect(m_buttonGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked),
          m_importer, &Tellico::Import::Importer::slotActionChanged);
#else
  connect(m_buttonGroup, &QButtonGroup::idClicked,
          m_importer, &Tellico::Import::Importer::slotActionChanged);
#endif

  topLayout->addStretch();

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
  mainLayout->addWidget(buttonBox);
  QPushButton* okButton = buttonBox->button(QDialogButtonBox::Ok);
  okButton->setDefault(true);
  okButton->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return));
  connect(okButton, &QPushButton::clicked, this, &ImportDialog::slotOk);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &ImportDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &ImportDialog::reject);

  KGuiItem ok = KStandardGuiItem::ok();
  ok.setText(i18n("&Import"));
  KGuiItem::assign(okButton, ok);

  // want to grab default button action, too
  // since the importer might do something with widgets, don't just call it, do it after layout is done
  QTimer::singleShot(0, this, &ImportDialog::slotUpdateAction);
}

ImportDialog::~ImportDialog() {
  delete m_importer;
  m_importer = nullptr;
}

Tellico::Data::CollPtr ImportDialog::collection() {
  if(m_importer && !m_coll) {
    ProgressItem& item = ProgressManager::self()->newProgressItem(m_importer, m_importer->progressLabel(), true);
    connect(m_importer, &Import::Importer::signalTotalSteps,
            ProgressManager::self(), &ProgressManager::setTotalSteps);
    connect(m_importer, &Import::Importer::signalProgress,
            ProgressManager::self(), &ProgressManager::setProgress);
    connect(&item, &ProgressItem::signalCancelled, m_importer, &Import::Importer::slotCancel);
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
Tellico::Import::Importer* ImportDialog::importer(Tellico::Import::Format format_, const QList<QUrl>& urls_) {
#define CHECK_SIZE if(urls_.size() > 1) myWarning() << "only importing first URL"
  QUrl firstURL = urls_.isEmpty() ? QUrl() : urls_[0];
  Import::Importer* importer = nullptr;
  switch(format_) {
    case Import::TellicoXML:
      CHECK_SIZE;
      importer = new Import::TellicoImporter(firstURL);
      break;

    case Import::Bibtex:
      importer = new Import::BibtexImporter(urls_);
#ifndef ENABLE_BTPARSE
      myLog() << "Bibtex importing is not available due to lack of btparse library";
#endif
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
        QString xsltFile = DataFileRegistry::self()->locate(QStringLiteral("mods2tellico.xsl"));
        if(!xsltFile.isEmpty()) {
          QUrl u = QUrl::fromLocalFile(xsltFile);
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

    case Import::GCstar:
      CHECK_SIZE;
      importer = new Import::GCstarImporter(firstURL);
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
      CHECK_SIZE;
      importer = new Import::GriffithImporter(firstURL);
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

    case Import::Goodreads:
      CHECK_SIZE;
      importer = new Import::GoodreadsImporter();
      break;

    case Import::GRS1:
      myDebug() << "GRS1 not implemented";
      break;

    case Import::CIW:
      importer = new Import::CIWImporter(urls_);
      break;

    case Import::VinoXML:
      CHECK_SIZE;
      importer = new Import::VinoXMLImporter(firstURL);
      break;

    case Import::BoardGameGeek:
      CHECK_SIZE;
      importer = new Import::BoardGameGeekImporter();
      break;

    case Import::LibraryThing:
      CHECK_SIZE;
      importer = new Import::LibraryThingImporter();
      break;

    case Import::Collectorz:
      CHECK_SIZE;
      importer = new Import::CollectorzImporter(firstURL);
      break;

    case Import::DataCrow:
      CHECK_SIZE;
      importer = new Import::DataCrowImporter(firstURL);
      break;

    case Import::MARC:
      CHECK_SIZE;
      importer = new Import::MarcImporter(firstURL);
      break;

    case Import::EBook:
      importer = new Import::EBookImporter(urls_);
      break;

    case Import::Discogs:
      CHECK_SIZE;
      importer = new Import::DiscogsImporter();
      break;
  }
  if(!importer) {
    myWarning() << "importer not created!";
    return nullptr;
  }
  importer->setCurrentCollection(Data::Document::self()->collection());
  return importer;
#undef CHECK_SIZE
}

//static
Tellico::Import::Importer* ImportDialog::importerForText(Tellico::Import::Format format_, const QString& text_) {
  Import::Importer* importer = nullptr;
  switch(format_) {
    case Import::Bibtex:
      importer = new Import::BibtexImporter(text_);
      break;

    default:
      break;
  }

  if(!importer) {
    myWarning() << "importer not created!";
    return nullptr;
  }
  importer->setCurrentCollection(Data::Document::self()->collection());
  return importer;
}

// static
QString ImportDialog::fileFilter(Tellico::Import::Format format_) {
  QString text;
  switch(format_) {
    case Import::TellicoXML:
      text = i18n("Tellico Files") + QLatin1String(" (*.tc *.bc)") + QLatin1String(";;");
      text += i18n("XML Files") + QLatin1String(" (*.xml)") + QLatin1String(";;");
      break;

    case Import::Bibtex:
      text = i18n("Bibtex Files") + QLatin1String(" (*.bib)") + QLatin1String(";;");
      break;

    case Import::CSV:
      text = i18n("CSV Files") + QLatin1String(" (*.csv)") + QLatin1String(";;");
      break;

    case Import::Bibtexml:
    case Import::XSLT:
    case Import::MODS:
    case Import::Delicious:
    case Import::Griffith:
    case Import::Collectorz:
    case Import::DataCrow:
      text = i18n("XML Files") + QLatin1String(" (*.xml)") + QLatin1String(";;");
      break;

    case Import::RIS:
      text = i18n("RIS Files") + QLatin1String(" (*.ris)") + QLatin1String(";;");
      break;

    case Import::GCstar:
      text = i18n("GCstar Data Files") + QLatin1String(" (*.gcs *.gcf)") + QLatin1String(";;");
      break;

    case Import::AMC:
      text = i18n("AMC Data Files") + QLatin1String(" (*.amc)") + QLatin1String(";;");
      break;

    case Import::PDF:
      text = i18n("PDF Files") + QLatin1String(" (*.pdf)") + QLatin1String(";;");
      break;

    case Import::Referencer:
      text = i18n("Referencer Files") + QLatin1String(" (*.reflib)") + QLatin1String(";;");
      break;

    case Import::CIW:
      text = i18n("CIW Files") + QLatin1String(" (*.ciw)") + QLatin1String(";;");
      break;

    case Import::VinoXML:
      text = i18n("VinoXML Data Files") + QLatin1String(" (*.vinoxml)") + QLatin1String(";;");
      text += i18n("XML Files") + QLatin1String(" (*.xml)") + QLatin1String(";;");
      break;

    case Import::EBook:
      // KFileMetaData has extractors that support mimetypes with these typical extensions
      text = i18n("eBook Files") + QLatin1String(" (*.epub *.fb2 *.fb2zip *.mobi)") + QLatin1String(";;");
      break;

    case Import::AudioFile:
    case Import::Alexandria:
    case Import::FreeDB:
    case Import::FileListing:
    case Import::GRS1:
    case Import::Goodreads:
    case Import::BoardGameGeek:
    case Import::LibraryThing:
    case Import::MARC:
    case Import::Discogs:
      break;
  }

  return text + i18n("All Files") + QLatin1String(" (*)");
}

// audio files are imported by directory
// alexandria is a defined location, as is freedb
// all others are files
Tellico::Import::Target ImportDialog::importTarget(Tellico::Import::Format format_) {
  switch(format_) {
    case Import::AudioFile:
    case Import::FileListing:
      return Import::Dir;
    case Import::Alexandria:
    case Import::FreeDB:
    case Import::Goodreads:
    case Import::BoardGameGeek:
    case Import::LibraryThing:
    case Import::Discogs:
      return Import::None;
    default:
      return Import::File;
  }
}

QString ImportDialog::startDir(Tellico::Import::Format format_) {
  if(format_ == Import::GCstar) {
    QDir dir = QDir::home();
    // able to cd if exists and readable
    if(dir.cd(QStringLiteral(".local/share/gcstar/"))) {
      return dir.absolutePath();
    }
  }
  return QString();
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
Tellico::Data::CollPtr ImportDialog::importURL(Tellico::Import::Format format_, const QUrl& url_) {
  QScopedPointer<Import::Importer> imp(importer(format_, QList<QUrl>() << url_));
  if(!imp) {
    return Data::CollPtr();
  }

  Data::CollPtr c;
  {
    // do this in a block to ensure the progress item is deleted before the importer
    ProgressItem& item = ProgressManager::self()->newProgressItem(imp.data(), imp->progressLabel(), true);
    connect(imp.data(), &Import::Importer::signalTotalSteps,
            ProgressManager::self(), &ProgressManager::setTotalSteps);
    connect(imp.data(), &Import::Importer::signalProgress,
            ProgressManager::self(), &ProgressManager::setProgress);
    connect(&item, &ProgressItem::signalCancelled, imp.data(), &Import::Importer::slotCancel);
    ProgressItem::Done done(imp.data());

    c = imp->collection();
  }
  if(!c && !imp->statusMessage().isEmpty()) {
    GUI::Proxy::sorry(imp->statusMessage());
  }
  return c;
}

Tellico::Data::CollPtr ImportDialog::importText(Tellico::Import::Format format_, const QString& text_) {
  Import::Importer* imp = importerForText(format_, text_);
  if(!imp) {
    return Data::CollPtr();
  }

  // the Done() constructor crashes for some reason, so just don't use it
  // 5/18/19 -> uncomment the progress Done again
  ProgressItem& item = ProgressManager::self()->newProgressItem(imp, imp->progressLabel(), true);
  connect(imp, &Import::Importer::signalTotalSteps,
          ProgressManager::self(), &ProgressManager::setTotalSteps);
  connect(imp, &Import::Importer::signalProgress,
          ProgressManager::self(), &ProgressManager::setProgress);
  connect(&item, &ProgressItem::signalCancelled, imp, &Import::Importer::slotCancel);
  ProgressItem::Done done(imp);

  Data::CollPtr c = imp->collection();
  if(!c && !imp->statusMessage().isEmpty()) {
    GUI::Proxy::sorry(imp->statusMessage());
  }
  delete imp;
  return c;
}
