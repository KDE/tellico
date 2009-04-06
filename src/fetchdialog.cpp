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

#include "fetchdialog.h"
#include "fetch/fetchmanager.h"
#include "fetch/fetcher.h"
#include "entryview.h"
#include "utils/isbnvalidator.h"
#include "utils/upcvalidator.h"
#include "tellico_kernel.h"
#include "filehandler.h"
#include "collection.h"
#include "entry.h"
#include "document.h"
#include "tellico_debug.h"
#include "gui/combobox.h"
#include "gui/cursorsaver.h"
#include "utils/stringset.h"

#include <klocale.h>
#include <klineedit.h>
#include <kpushbutton.h>
#include <kstatusbar.h>
#include <khtmlview.h>
#include <kprogressdialog.h>
#include <kconfig.h>
#include <kfiledialog.h>
#include <kacceleratormanager.h>
#include <ktextedit.h>
#include <kmessagebox.h>
#include <kapplication.h>
#include <KHBox>
#include <KVBox>

#include <QGroupBox>
#include <QSplitter>
#include <QTimer>
#include <QCheckBox>
#include <QImage>
#include <QLabel>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QTreeWidget>
#include <QHeaderView>

#include <config.h>
#include <kglobal.h>
#ifdef ENABLE_WEBCAM
#include "barcode/barcode.h"
#endif

namespace {
  static const int FETCH_MIN_WIDTH = 600;

  static const char* FETCH_STRING_SEARCH = I18N_NOOP("&Search");
  static const char* FETCH_STRING_STOP   = I18N_NOOP("&Stop");

  static const int StringDataType = QEvent::User;
  static const int ImageDataType = QEvent::User+1;

  class StringDataEvent : public QEvent {
  public:
    StringDataEvent(const QString& str) : QEvent(static_cast<QEvent::Type>(StringDataType)), m_string(str) {}
    QString string() const { return m_string; }
  private:
    QString m_string;
  };

  class ImageDataEvent : public QEvent {
  public:
    ImageDataEvent(const QImage& img) : QEvent(static_cast<QEvent::Type>(ImageDataType)), m_image(img) {}
    QImage image() const { return m_image; }
  private:
    QImage m_image;
  };
}

using Tellico::FetchDialog;
using barcodeRecognition::barcodeRecognitionThread;

class FetchDialog::SearchResultItem : public QTreeWidgetItem {
  friend class FetchDialog;
  // always add to end
  SearchResultItem(QTreeWidget* lv, Fetch::SearchResult* r)
      : QTreeWidgetItem(lv), m_result(r) {
    setData(1, Qt::DisplayRole, r->title);
    setData(2, Qt::DisplayRole, r->desc);
    setData(3, Qt::DisplayRole, r->fetcher->source());
    setData(3, Qt::DecorationRole, Fetch::Manager::self()->fetcherIcon(r->fetcher));
  }
  Fetch::SearchResult* m_result;
};

FetchDialog::FetchDialog(QWidget* parent_)
    : KDialog(parent_),
      m_timer(new QTimer(this)), m_started(false) {
  setModal(false);
  setCaption(i18n("Internet Search"));
  setButtons(0);

  m_collType = Kernel::self()->collectionType();

  QWidget* mainWidget = new QWidget(this);
  setMainWidget(mainWidget);
  QBoxLayout* topLayout = new QVBoxLayout(mainWidget);

  QGroupBox* queryBox = new QGroupBox(i18n("Search Query"), mainWidget);
  QBoxLayout* queryLayout = new QVBoxLayout(queryBox);
  topLayout->addWidget(queryBox);

  KHBox* box1 = new KHBox(queryBox);
  box1->setSpacing(spacingHint());
  queryLayout->addWidget(box1);

  QLabel* label = new QLabel(i18nc("Start the search", "S&earch:"), box1);

  m_valueLineEdit = new KLineEdit(box1);
  label->setBuddy(m_valueLineEdit);
  m_valueLineEdit->setWhatsThis(i18n("Enter a search value. An ISBN search must include the full ISBN."));
  m_keyCombo = new GUI::ComboBox(box1);
  Fetch::KeyMap map = Fetch::Manager::self()->keyMap();
  for(Fetch::KeyMap::ConstIterator it = map.constBegin(); it != map.constEnd(); ++it) {
    m_keyCombo->addItem(it.value(), it.key());
  }
  connect(m_keyCombo, SIGNAL(activated(int)), SLOT(slotKeyChanged(int)));
  m_keyCombo->setWhatsThis(i18n("Choose the type of search"));

  m_searchButton = new KPushButton(box1);
  m_searchButton->setGuiItem(KGuiItem(i18n(FETCH_STRING_STOP),
                                      KIcon(QLatin1String("dialog-cancel"))));
  connect(m_searchButton, SIGNAL(clicked()), SLOT(slotSearchClicked()));
  m_searchButton->setWhatsThis(i18n("Click to start or stop the search"));

  // the search button's text changes from search to stop
  // I don't want it resizing, so figure out the maximum size and set that
  m_searchButton->ensurePolished();
  int maxWidth = m_searchButton->sizeHint().width();
  int maxHeight = m_searchButton->sizeHint().height();
  m_searchButton->setGuiItem(KGuiItem(i18n(FETCH_STRING_SEARCH),
                                      KIcon(QLatin1String("edit-find"))));
  maxWidth = qMax(maxWidth, m_searchButton->sizeHint().width());
  maxHeight = qMax(maxHeight, m_searchButton->sizeHint().height());
  m_searchButton->setMinimumWidth(maxWidth);
  m_searchButton->setMinimumHeight(maxHeight);

  KHBox* box2 = new KHBox(queryBox);
  box2->setSpacing(spacingHint());
  queryLayout->addWidget(box2);

  m_multipleISBN = new QCheckBox(i18n("&Multiple ISBN/UPC search"), box2);
  m_multipleISBN->setWhatsThis(i18n("Check this box to search for multiple ISBN or UPC values."));
  connect(m_multipleISBN, SIGNAL(toggled(bool)), SLOT(slotMultipleISBN(bool)));

  m_editISBN = new KPushButton(KGuiItem(i18n("Edit List..."), KIcon(QLatin1String("format-justify-fill"))), box2);
  m_editISBN->setEnabled(false);
  m_editISBN->setWhatsThis(i18n("Click to open a text edit box for entering or editing multiple ISBN values."));
  connect(m_editISBN, SIGNAL(clicked()), SLOT(slotEditMultipleISBN()));

  // add for spacing
  box2->setStretchFactor(new QWidget(box2), 10);

  label = new QLabel(i18n("Search s&ource:"), box2);
  m_sourceCombo = new KComboBox(box2);
  label->setBuddy(m_sourceCombo);
  Fetch::FetcherVec sources = Fetch::Manager::self()->fetchers(m_collType);
  foreach(Fetch::Fetcher::Ptr fetcher, sources) {
    m_sourceCombo->addItem(Fetch::Manager::self()->fetcherIcon(fetcher), fetcher->source());
  }
  connect(m_sourceCombo, SIGNAL(activated(const QString&)), SLOT(slotSourceChanged(const QString&)));
  m_sourceCombo->setWhatsThis(i18n("Select the database to search"));

  QSplitter* split = new QSplitter(Qt::Vertical, mainWidget);
  topLayout->addWidget(split);

  m_treeWidget = new QTreeWidget(split);
  m_treeWidget->header()->setSortIndicatorShown(true);
  m_treeWidget->sortItems(1, Qt::AscendingOrder);
  m_treeWidget->setAllColumnsShowFocus(true);
  m_treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
  m_treeWidget->setHeaderLabels(QStringList() << QString()
                                              << i18n("Title")
                                              << i18n("Description")
                                              << i18n("Source"));
  m_treeWidget->setColumnWidth(0, 20); // will show a check mark when added
  m_treeWidget->model()->setHeaderData(0, Qt::Horizontal, Qt::AlignHCenter, Qt::TextAlignmentRole); // align checkmark in middle
  m_treeWidget->viewport()->installEventFilter(this);

  connect(m_treeWidget, SIGNAL(itemSelectionChanged()), SLOT(slotShowEntry()));
  // double clicking should add the entry
  connect(m_treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), SLOT(slotAddEntry()));
  m_treeWidget->setWhatsThis(i18n("As results are found, they are added to this list. Selecting one "
                                  "will fetch the complete entry and show it in the view below."));

  m_entryView = new EntryView(split);
  // don't bother creating funky gradient images for compact view
  m_entryView->setUseGradientImages(false);
  // set the xslt file AFTER setting the gradient image option
  m_entryView->setXSLTFile(QLatin1String("Compact.xsl"));
  m_entryView->view()->setWhatsThis(i18n("An entry may be shown here before adding it to the "
                                         "current collection by selecting it in the list above"));

  KHBox* box3 = new KHBox(mainWidget);
  topLayout->addWidget(box3);
  box3->setSpacing(KDialog::spacingHint());

  m_addButton = new KPushButton(i18n("&Add Entry"), box3);
  m_addButton->setEnabled(false);
  m_addButton->setIcon(KIcon(Kernel::self()->collectionTypeName()));
  connect(m_addButton, SIGNAL(clicked()), SLOT(slotAddEntry()));
  m_addButton->setWhatsThis(i18n("Add the selected entry to the current collection"));

  m_moreButton = new KPushButton(KGuiItem(i18n("Get More Results"), KIcon(QLatin1String("edit-find"))), box3);
  m_moreButton->setEnabled(false);
  connect(m_moreButton, SIGNAL(clicked()), SLOT(slotMoreClicked()));
  m_moreButton->setWhatsThis(i18n("Fetch more results from the current data source"));

  KPushButton* clearButton = new KPushButton(KStandardGuiItem::clear(), box3);
  connect(clearButton, SIGNAL(clicked()), SLOT(slotClearClicked()));
  clearButton->setWhatsThis(i18n("Clear all search fields and results"));

  KHBox* bottombox = new KHBox(mainWidget);
  topLayout->addWidget(bottombox);
  bottombox->setSpacing(KDialog::spacingHint());

  m_statusBar = new KStatusBar(bottombox);
  m_statusLabel = new QLabel(m_statusBar);
  m_statusBar->addPermanentWidget(m_statusLabel, 1);
  m_progress = new QProgressBar(m_statusBar);
  m_progress->setMaximum(0);
  m_progress->setFixedHeight(fontMetrics().height()+2);
  m_progress->hide();
  m_statusBar->addPermanentWidget(m_progress);

  KPushButton* closeButton = new KPushButton(KStandardGuiItem::close(), bottombox);
  connect(closeButton, SIGNAL(clicked()), SLOT(accept()));

  connect(m_timer, SIGNAL(timeout()), SLOT(slotMoveProgress()));

  setMinimumWidth(qMax(minimumWidth(), FETCH_MIN_WIDTH));
  setStatus(i18n("Ready."));

  KConfigGroup sizeGroup(KGlobal::config(), QLatin1String("Fetch Dialog Options"));
  restoreDialogSize(sizeGroup);

  KConfigGroup config(KGlobal::config(), "Fetch Dialog Options");
  QList<int> splitList = config.readEntry("Splitter Sizes", QList<int>());
  if(!splitList.empty()) {
    split->setSizes(splitList);
  }

  connect(Fetch::Manager::self(), SIGNAL(signalResultFound(Tellico::Fetch::SearchResult*)),
                                  SLOT(slotResultFound(Tellico::Fetch::SearchResult*)));
  connect(Fetch::Manager::self(), SIGNAL(signalStatus(const QString&)),
                                  SLOT(slotStatus(const QString&)));
  connect(Fetch::Manager::self(), SIGNAL(signalDone()),
                                  SLOT(slotFetchDone()));

  KAcceleratorManager::manage(this);
  // initialize combos
  QTimer::singleShot(0, this, SLOT(slotInit()));

#ifdef ENABLE_WEBCAM
  // barcode recognition
  m_barcodeRecognitionThread = new barcodeRecognitionThread();
  if (m_barcodeRecognitionThread->isWebcamAvailable()) {
    m_barcodePreview = new QLabel(0);
    m_barcodePreview->move( KGlobalSettings::desktopGeometry(m_barcodePreview).width() - 350, 30 );
    m_barcodePreview->adjustSize();
    m_barcodePreview->show();
  } else {
    m_barcodePreview = 0;
  }
  connect( m_barcodeRecognitionThread, SIGNAL(recognized(const QString&)), this, SLOT(slotBarcodeRecognized(const QString&)) );
  connect( m_barcodeRecognitionThread, SIGNAL(gotImage(const QImage&)), this, SLOT(slotBarcodeGotImage(const QImage&)) );
  m_barcodeRecognitionThread->start();
/*  //DEBUG
  QImage img( "/home/sebastian/white.png", "PNG" );
  m_barcodeRecognitionThread->recognizeBarcode( img );*/
#endif
}

FetchDialog::~FetchDialog() {
#ifdef ENABLE_WEBCAM
  m_barcodeRecognitionThread->stop();
  if (!m_barcodeRecognitionThread->wait( 1000 )) {
    m_barcodeRecognitionThread->terminate();
  }
  delete m_barcodeRecognitionThread;
  if (m_barcodePreview) {
    delete m_barcodePreview;
  }
#endif

  qDeleteAll(m_results);
  m_results.clear();

  // we might have downloaded a lot of images we don't need to keep
  Data::EntryList entriesToCheck;
  foreach(Data::EntryPtr entry, m_entries) {
    entriesToCheck.append(entry);
  }
  // no additional entries to check images to keep though
  Data::Document::self()->removeImagesNotInCollection(entriesToCheck, Data::EntryList());

  KConfigGroup config(KGlobal::config(), QLatin1String("Fetch Dialog Options"));
  saveDialogSize(config);

  config.writeEntry("Splitter Sizes", static_cast<QSplitter*>(m_treeWidget->parentWidget())->sizes());
  config.writeEntry("Search Key", m_keyCombo->currentData().toInt());
  config.writeEntry("Search Source", m_sourceCombo->currentText());
}

void FetchDialog::slotSearchClicked() {
  m_valueLineEdit->selectAll();
  if(m_started) {
    setStatus(i18n("Cancelling the search..."));
    Fetch::Manager::self()->stop();
    slotFetchDone();
  } else {
    QString value = m_valueLineEdit->text().simplified();
    if(value != m_oldSearch) {
      m_treeWidget->clear();
      m_entryView->clear();
    }
    m_resultCount = 0;
    m_oldSearch = value;
    m_started = true;
    m_searchButton->setGuiItem(KGuiItem(i18n(FETCH_STRING_STOP),
                                        KIcon(QLatin1String("dialog-cancel"))));
    startProgress();
    setStatus(i18n("Searching..."));
    kapp->processEvents();
    Fetch::Manager::self()->startSearch(m_sourceCombo->currentText(),
                                        static_cast<Fetch::FetchKey>(m_keyCombo->currentData().toInt()),
                                        value);
  }
}

void FetchDialog::slotClearClicked() {
  slotFetchDone(false);
  m_treeWidget->clear();
  m_entryView->clear();
  Fetch::Manager::self()->stop();
  m_multipleISBN->setChecked(false);
  m_valueLineEdit->clear();
  m_valueLineEdit->setFocus();
  m_addButton->setEnabled(false);
  m_moreButton->setEnabled(false);
  m_isbnList.clear();
  m_statusMessages.clear();
  setStatus(i18n("Ready.")); // because slotFetchDone() writes text
}

void FetchDialog::slotStatus(const QString& status_) {
  m_statusMessages.push_back(status_);
  // if the queue was empty, start the timer
  if(m_statusMessages.count() == 1) {
    // wait 2 seconds
    QTimer::singleShot(2000, this, SLOT(slotUpdateStatus()));
  }
}

void FetchDialog::slotUpdateStatus() {
  if(m_statusMessages.isEmpty()) {
    return;
  }
  setStatus(m_statusMessages.front());
  m_statusMessages.pop_front();
  if(!m_statusMessages.isEmpty()) {
    // wait 2 seconds
    QTimer::singleShot(2000, this, SLOT(slotUpdateStatus()));
  }
}

void FetchDialog::setStatus(const QString& text_) {
  m_statusLabel->setText(QLatin1Char(' ') + text_);
}

void FetchDialog::slotFetchDone(bool checkISBN_ /* = true */) {
//  myDebug() << "FetchDialog::slotFetchDone()" << endl;
  m_started = false;
  m_searchButton->setGuiItem(KGuiItem(i18n(FETCH_STRING_SEARCH),
                                      KIcon(QLatin1String("edit-find"))));
  stopProgress();
  if(m_resultCount == 0) {
    slotStatus(i18n("The search returned no items."));
  } else {
    /* TRANSLATORS: This is a plural form, you need to translate both lines (except "_n: ") */
    slotStatus(i18np("The search returned 1 item.",
                     "The search returned %1 items.",
                     m_resultCount));
  }
  m_moreButton->setEnabled(Fetch::Manager::self()->hasMoreResults());

  // if we're not checking isbn values, then, ok to return
  if(!checkISBN_) {
    return;
  }

  const Fetch::FetchKey key = static_cast<Fetch::FetchKey>(m_keyCombo->currentData().toInt());
  // no way to currently check EAN/UPC values for non-book items
  if(m_collType & (Data::Collection::Book | Data::Collection::Bibtex) &&
     m_multipleISBN->isChecked() &&
     (key == Fetch::ISBN || key == Fetch::UPC)) {
    QStringList searchValues = m_oldSearch.simplified().split(QLatin1String("; "));
    QStringList resultValues;
    for(int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
      resultValues << static_cast<SearchResultItem*>(m_treeWidget->topLevelItem(i))->m_result->isbn;
    }
    const QStringList valuesNotFound = ISBNValidator::listDifference(searchValues, resultValues);
    if(!valuesNotFound.isEmpty()) {
      KMessageBox::informationList(this, i18n("No results were found for the following ISBN values:"), valuesNotFound,
                                   i18n("No Results"));
    }
  }
}

void FetchDialog::slotResultFound(Tellico::Fetch::SearchResult* result_) {
  m_results.append(result_);
  (void) new SearchResultItem(m_treeWidget, result_);
  ++m_resultCount;
}

void FetchDialog::slotAddEntry() {
  GUI::CursorSaver cs;
  Data::EntryList vec;
  foreach(QTreeWidgetItem* item_, m_treeWidget->selectedItems()) {
    SearchResultItem* item = static_cast<SearchResultItem*>(item_);

    Fetch::SearchResult* r = item->m_result;
    Data::EntryPtr entry = m_entries.value(r->uid);
    if(!entry) {
      setStatus(i18n("Fetching %1...", r->title));
      startProgress();
      entry = r->fetchEntry();
      if(!entry) {
        continue;
      }
      m_entries.insert(r->uid, entry);
      stopProgress();
      setStatus(i18n("Ready."));
    }
    // add a copy, intentionally allowing multiple copies to be added
    vec.append(Data::EntryPtr(new Data::Entry(*entry)));
    item->setData(0, Qt::DecorationRole, KIcon(QLatin1String("checkmark")));
  }
  if(!vec.isEmpty()) {
    Kernel::self()->addEntries(vec, true);
  }
}

void FetchDialog::slotMoreClicked() {
  if(m_started) {
    myDebug() << "FetchDialog::slotMoreClicked() - can't continue while running" << endl;
    return;
  }

  m_started = true;
  m_searchButton->setGuiItem(KGuiItem(i18n(FETCH_STRING_STOP),
                                      KIcon(QLatin1String("dialog-cancel"))));
  startProgress();
  setStatus(i18n("Searching..."));
  kapp->processEvents();
  Fetch::Manager::self()->continueSearch();
}

void FetchDialog::slotShowEntry() {
  // just in case
  m_statusMessages.clear();

  QList<QTreeWidgetItem*> items = m_treeWidget->selectedItems();
  if(items.isEmpty()) {
    m_addButton->setEnabled(false);
    return;
  }

  m_addButton->setEnabled(true);
  if(items.count() > 1) {
    m_entryView->clear();
    return;
  }

  SearchResultItem* item = static_cast<SearchResultItem*>(items.first());
  Fetch::SearchResult* r = item->m_result;
  setStatus(i18n("Fetching %1...", r->title));
  Data::EntryPtr entry = m_entries.value(r->uid);
  if(!entry) {
    GUI::CursorSaver cs;
    startProgress();
    entry = r->fetchEntry();
    if(entry) { // might conceivably be null
      m_entries.insert(r->uid, entry);
    }
    stopProgress();
  }
  setStatus(i18n("Ready."));

  m_entryView->showEntry(entry);
}

void FetchDialog::startProgress() {
  m_progress->show();
  m_timer->start(100);
}

void FetchDialog::slotMoveProgress() {
  m_progress->setValue(m_progress->value()+5);
}

void FetchDialog::stopProgress() {
  m_timer->stop();
  m_progress->hide();
}

void FetchDialog::slotInit() {
  if(!Fetch::Manager::self()->canFetch()) {
    m_searchButton->setEnabled(false);
    Kernel::self()->sorry(i18n("No Internet sources are available for your current collection type."), this);
  }

  KConfigGroup config(KGlobal::config(), "Fetch Dialog Options");
  int key = config.readEntry("Search Key", int(Fetch::FetchFirst));
  // only change key if valid
  if(key > Fetch::FetchFirst) {
    m_keyCombo->setCurrentData(key);
  }
  slotKeyChanged(m_keyCombo->currentIndex());

  QString source = config.readEntry("Search Source");
  if(!source.isEmpty()) {
    int idx = m_sourceCombo->findText(source);
    if(idx > -1) {
      m_sourceCombo->setCurrentIndex(idx);
    }
  }
  slotSourceChanged(m_sourceCombo->currentText());

  m_valueLineEdit->setFocus();
  m_searchButton->setDefault(true);
}

void FetchDialog::slotKeyChanged(int idx_) {
  int key = m_keyCombo->itemData(idx_).toInt();
  if(key == Fetch::ISBN || key == Fetch::UPC || key == Fetch::LCCN) {
    m_multipleISBN->setEnabled(true);
    if(key == Fetch::ISBN) {
      m_valueLineEdit->setValidator(new ISBNValidator(this));
    } else {
      UPCValidator* upc = new UPCValidator(this);
      connect(upc, SIGNAL(signalISBN()), SLOT(slotUPC2ISBN()));
      m_valueLineEdit->setValidator(upc);
      // only want to convert to ISBN if ISBN is accepted by the fetcher
      Fetch::KeyMap map = Fetch::Manager::self()->keyMap(m_sourceCombo->currentText());
      upc->setCheckISBN(map.contains(Fetch::ISBN));
    }
  } else {
    m_multipleISBN->setChecked(false);
    m_multipleISBN->setEnabled(false);
//    slotMultipleISBN(false);
    m_valueLineEdit->setValidator(0);
  }
}

void FetchDialog::slotSourceChanged(const QString& source_) {
  int curr = m_keyCombo->currentData().toInt();
  m_keyCombo->clear();
  Fetch::KeyMap map = Fetch::Manager::self()->keyMap(source_);
  for(Fetch::KeyMap::ConstIterator it = map.constBegin(); it != map.constEnd(); ++it) {
    m_keyCombo->addItem(it.value(), it.key());
  }
  m_keyCombo->setCurrentData(curr);
  slotKeyChanged(m_keyCombo->currentIndex());
}

void FetchDialog::slotMultipleISBN(bool toggle_) {
  bool wasEnabled = m_valueLineEdit->isEnabled();
  m_valueLineEdit->setEnabled(!toggle_);
  if(!wasEnabled && m_valueLineEdit->isEnabled()) {
    // if we enable it, it probably had multiple isbn values
    // the validator doesn't like that, so only keep the first value
    QString val = m_valueLineEdit->text().section(QLatin1Char(';'), 0, 0);
    m_valueLineEdit->setText(val);
  }
  m_editISBN->setEnabled(toggle_);
  if(toggle_) {
    // if we're editing multiple values, it makes sense to popup the dialog now
    slotEditMultipleISBN();
  }
}

void FetchDialog::slotEditMultipleISBN() {
  KDialog dlg(this);
  dlg.setModal(true);
  dlg.setCaption(i18n("Edit ISBN/UPC Values"));
  dlg.setButtons(KDialog::Ok|KDialog::Cancel);

  KVBox* box = new KVBox(&dlg);
  box->setSpacing(10);
  QString s = i18n("<qt>Enter the ISBN or UPC values, one per line.</qt>");
  (void) new QLabel(s, box);
  m_isbnTextEdit = new KTextEdit(box);
  m_isbnTextEdit->setText(m_isbnList.join(QLatin1String("\n")));
  m_isbnTextEdit->setWhatsThis(s);
  KPushButton* fromFileBtn = new KPushButton(KIcon(QLatin1String("document-open")),
                                             i18n("&Load From File..."), box);
  fromFileBtn->setWhatsThis(i18n("<qt>Load the list from a text file.</qt>"));
  connect(fromFileBtn, SIGNAL(clicked()), SLOT(slotLoadISBNList()));
  dlg.setMainWidget(box);
  dlg.setMinimumWidth(qMax(dlg.minimumWidth(), FETCH_MIN_WIDTH*2/3));

  if(dlg.exec() == QDialog::Accepted) {
    m_isbnList = m_isbnTextEdit->toPlainText().split(QLatin1String("\n"));
    const QValidator* val = m_valueLineEdit->validator();
    if(val) {
      for(QStringList::Iterator it = m_isbnList.begin(); it != m_isbnList.end(); ++it) {
        val->fixup(*it);
        if((*it).isEmpty()) {
          it = m_isbnList.erase(it);
          // this is next item, shift backward
          --it;
        }
      }
    }
    if(m_isbnList.count() > 100) {
      Kernel::self()->sorry(i18n("<qt>An ISBN search can contain a maximum of 100 ISBN values. Only the "
                                 "first 100 values in your list will be used.</qt>"), this);
      m_isbnList = m_isbnList.mid(0, 100);
    }
    m_valueLineEdit->setText(m_isbnList.join(QLatin1String("; ")));
  }
  m_isbnTextEdit = 0; // gets auto-deleted
}

void FetchDialog::slotLoadISBNList() {
  if(!m_isbnTextEdit) {
    return;
  }
  KUrl u = KFileDialog::getOpenUrl(KUrl(), QString(), this);
  if(u.isValid()) {
    m_isbnTextEdit->setText(m_isbnTextEdit->toPlainText() + FileHandler::readTextFile(u));
    m_isbnTextEdit->moveCursor(QTextCursor::End);
    m_isbnTextEdit->ensureCursorVisible();
  }
}

void FetchDialog::slotUPC2ISBN() {
  int key = m_keyCombo->currentData().toInt();
  if(key == Fetch::UPC) {
    m_keyCombo->setCurrentData(Fetch::ISBN);
    slotKeyChanged(m_keyCombo->currentIndex());
  }
}

void FetchDialog::slotResetCollection() {
  if(m_collType == Kernel::self()->collectionType()) {
    return;
  }
  m_collType = Kernel::self()->collectionType();
  m_sourceCombo->clear();
  Fetch::FetcherVec sources = Fetch::Manager::self()->fetchers(m_collType);
  foreach(Fetch::Fetcher::Ptr fetcher, sources) {
    m_sourceCombo->addItem(Fetch::Manager::self()->fetcherIcon(fetcher), fetcher->source());
  }

  m_addButton->setIcon(KIcon(Kernel::self()->collectionTypeName()));

  if(Fetch::Manager::self()->canFetch()) {
    m_searchButton->setEnabled(true);
  } else {
    m_searchButton->setEnabled(false);
    Kernel::self()->sorry(i18n("No Internet sources are available for your current collection type."), this);
  }
}

void FetchDialog::slotBarcodeRecognized(const QString& string )
{
  // attention: this slot is called in the context of another thread => do not use GUI-functions!
  StringDataEvent *e = new StringDataEvent( string );
  kapp->postEvent( this, e ); // the event loop will call FetchDialog::customEvent() in the context of the GUI thread
}

void FetchDialog::slotBarcodeGotImage(const QImage& img )
{
  // attention: this slot is called in the context of another thread => do not use GUI-functions!
  ImageDataEvent *e = new ImageDataEvent( img );
  kapp->postEvent( this, e ); // the event loop will call FetchDialog::customEvent() in the context of the GUI thread
}

void FetchDialog::customEvent( QEvent *e )
{
  if (!e) {
    return;
  }
  if ( e->type() == StringDataType) {
    // slotBarcodeRecognized() queued call
    kapp->beep();
    m_valueLineEdit->setText( static_cast<StringDataEvent*>(e)->string() );
    m_searchButton->animateClick();
  } else if ( e->type() == ImageDataType ) {
    // slotBarcodegotImage() queued call
    m_barcodePreview->setPixmap( QPixmap::fromImage(static_cast<ImageDataEvent*>(e)->image()) );
  }
}

#include "fetchdialog.moc"
