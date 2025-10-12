/***************************************************************************
    Copyright (C) 2003-2020 Robby Stephenson <robby@periapsis.org>
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

#include "fetchdialog.h"
#include "fetch/fetchmanager.h"
#include "fetch/fetcher.h"
#include "fetch/fetchresult.h"
#include "config/tellico_config.h"
#include "entryview.h"
#include "utils/isbnvalidator.h"
#include "utils/upcvalidator.h"
#include "tellico_kernel.h"
#include "core/filehandler.h"
#include "collection.h"
#include "entry.h"
#include "document.h"
#include "field.h"
#include "fieldformat.h"
#include "gui/combobox.h"
#include "utils/cursorsaver.h"
#include "images/image.h"
#include "tellico_debug.h"

#ifdef ENABLE_WEBCAM
#include "barcode/barcode.h"
#endif

#include <KLocalizedString>
#include <KSharedConfig>
#include <KAcceleratorManager>
#include <KTextEdit>
#include <KMessageBox>
#include <KStandardGuiItem>
#include <KWindowConfig>

#include <QDialogButtonBox>
#include <QPushButton>
#include <QLineEdit>
#include <QGroupBox>
#include <QSplitter>
#include <QTimer>
#include <QCheckBox>
#include <QImage>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QTreeWidget>
#include <QHeaderView>
#include <QApplication>
#include <QFileDialog>
#include <QStatusBar>
#include <QScreen>
#include <QWindow>

namespace {
  static const int FETCH_MIN_WIDTH = 600;

  static const int StringDataType = QEvent::User;
  static const int ImageDataType = QEvent::User+1;

  class StringDataEvent : public QEvent {
  public:
    StringDataEvent(const QString& str) : QEvent(static_cast<QEvent::Type>(StringDataType)), m_string(str) {}
    QString string() const { return m_string; }
  private:
    Q_DISABLE_COPY(StringDataEvent)
    QString m_string;
  };

  class ImageDataEvent : public QEvent {
  public:
    ImageDataEvent(const QImage& img) : QEvent(static_cast<QEvent::Type>(ImageDataType)), m_image(img) {}
    QImage image() const { return m_image; }
  private:
    Q_DISABLE_COPY(ImageDataEvent)
    QImage m_image;
  };

  // class exists just to make sizeHintForColumn() public
  class TreeWidget : public QTreeWidget {
  public:
    TreeWidget(QWidget* p) : QTreeWidget(p) {}
    virtual int sizeHintForColumn(int c) const override { return QTreeWidget::sizeHintForColumn(c); }
  };
}

using Tellico::FetchDialog;
using barcodeRecognition::barcodeRecognitionThread;

class FetchDialog::FetchResultItem : public QTreeWidgetItem {
  friend class FetchDialog;
  // always add to end
  FetchResultItem(QTreeWidget* lv, Fetch::FetchResult* r)
      : QTreeWidgetItem(lv), m_result(r) {
    setData(1, Qt::DisplayRole, r->title);
    setData(2, Qt::DisplayRole, r->desc);
    setData(3, Qt::DisplayRole, r->fetcher()->source());
    setData(3, Qt::DecorationRole, Fetch::Manager::self()->fetcherIcon(r->fetcher()));
  }
  Fetch::FetchResult* m_result;

private:
  Q_DISABLE_COPY(FetchResultItem)
};

FetchDialog::FetchDialog(QWidget* parent_)
    : QDialog(parent_)
    , m_timer(new QTimer(this))
    , m_started(false)
    , m_resultCount(0)
    , m_treeWasResized(false)
    , m_barcodePreview(nullptr)
    , m_barcodeRecognitionThread(nullptr) {
  setModal(false);
  setWindowTitle(i18n("Internet Search"));

  QVBoxLayout* mainLayout = new QVBoxLayout();
  setLayout(mainLayout);

  m_collType = Kernel::self()->collectionType();

  QWidget* mainWidget = new QWidget(this);
  mainLayout->addWidget(mainWidget);
  QBoxLayout* topLayout = new QVBoxLayout(mainWidget);

  QGroupBox* queryBox = new QGroupBox(i18n("Search Query"), mainWidget);
  QBoxLayout* queryLayout = new QVBoxLayout(queryBox);
  topLayout->addWidget(queryBox);

  QWidget* box1 = new QWidget(queryBox);
  QHBoxLayout* box1HBoxLayout = new QHBoxLayout(box1);
  box1HBoxLayout->setContentsMargins(0, 0, 0, 0);
  queryLayout->addWidget(box1);

  QLabel* label = new QLabel(i18nc("Start the search", "S&earch:"), box1);
  box1HBoxLayout->addWidget(label);

  m_valueLineEdit = new QLineEdit(box1);
  box1HBoxLayout->addWidget(m_valueLineEdit);
  label->setBuddy(m_valueLineEdit);
  m_valueLineEdit->setWhatsThis(i18n("Enter a search value. An ISBN search must include the full ISBN."));

  m_keyCombo = new GUI::ComboBox(box1);
  box1HBoxLayout->addWidget(m_keyCombo);
  Fetch::KeyMap map = Fetch::Manager::self()->keyMap();
  for(Fetch::KeyMap::ConstIterator it = map.constBegin(); it != map.constEnd(); ++it) {
    m_keyCombo->addItem(it.value(), it.key());
  }
  void (QComboBox::* activatedInt)(int) = &QComboBox::activated;
  connect(m_keyCombo, activatedInt, this, &FetchDialog::slotKeyChanged);
  m_keyCombo->setWhatsThis(i18n("Choose the type of search"));

  m_searchButton = new QPushButton(box1);
  box1HBoxLayout->addWidget(m_searchButton);
  KGuiItem::assign(m_searchButton, KGuiItem(i18n("&Stop"),
                                            QIcon::fromTheme(QStringLiteral("dialog-cancel"))));
  connect(m_searchButton, &QAbstractButton::clicked, this, &FetchDialog::slotSearchClicked);
  m_searchButton->setWhatsThis(i18n("Click to start or stop the search"));

  // the search button's text changes from search to stop
  // I don't want it resizing, so figure out the maximum size and set that
  m_searchButton->ensurePolished();
  int maxWidth = m_searchButton->sizeHint().width();
  int maxHeight = m_searchButton->sizeHint().height();
  KGuiItem::assign(m_searchButton, KGuiItem(i18n("&Search"),
                                            QIcon::fromTheme(QStringLiteral("edit-find"))));
  maxWidth = qMax(maxWidth, m_searchButton->sizeHint().width());
  maxHeight = qMax(maxHeight, m_searchButton->sizeHint().height());
  m_searchButton->setMinimumWidth(maxWidth);
  m_searchButton->setMinimumHeight(maxHeight);

  QWidget* box2 = new QWidget(queryBox);
  QHBoxLayout* box2HBoxLayout = new QHBoxLayout(box2);
  box2HBoxLayout->setContentsMargins(0, 0, 0, 0);
  queryLayout->addWidget(box2);

  m_multipleISBN = new QCheckBox(i18n("&Multiple ISBN/UPC search"), box2);
  box2HBoxLayout->addWidget(m_multipleISBN);
  m_multipleISBN->setWhatsThis(i18n("Check this box to search for multiple ISBN or UPC values."));
  connect(m_multipleISBN, &QAbstractButton::toggled, this, &FetchDialog::slotMultipleISBN);

  m_editISBN = new QPushButton(box2);
  KGuiItem::assign(m_editISBN, KGuiItem(i18n("Edit ISBN/UPC values..."),
                                        QIcon::fromTheme(QStringLiteral("format-justify-fill"))));
  box2HBoxLayout->addWidget(m_editISBN);
  m_editISBN->setEnabled(false);
  m_editISBN->setWhatsThis(i18n("Click to open a text edit box for entering or editing multiple ISBN or UPC values."));
  connect(m_editISBN, &QAbstractButton::clicked, this, &FetchDialog::slotEditMultipleISBN);

  // add for spacing
  box2HBoxLayout->addStretch(10);

  label = new QLabel(i18n("Search s&ource:"), box2);
  box2HBoxLayout->addWidget(label);
  m_sourceCombo = new KComboBox(box2);
  box2HBoxLayout->addWidget(m_sourceCombo);
  label->setBuddy(m_sourceCombo);
  Fetch::FetcherVec sources = Fetch::Manager::self()->fetchers(m_collType);
  foreach(Fetch::Fetcher::Ptr fetcher, sources) {
    m_sourceCombo->addItem(Fetch::Manager::self()->fetcherIcon(fetcher.data()), fetcher->source());
  }
  connect(m_sourceCombo, &QComboBox::textActivated, this, &FetchDialog::slotSourceChanged);
  m_sourceCombo->setWhatsThis(i18n("Select the database to search"));

  // for whatever reason, the dialog window could get shrunk and truncate the text
  box2->setMinimumWidth(box2->minimumSizeHint().width());

  QSplitter* split = new QSplitter(Qt::Vertical, mainWidget);
  topLayout->addWidget(split);
  split->setChildrenCollapsible(false);

  // using <anonymous>::TreeWidget as a lazy step to make a protected method public
  m_treeWidget = new TreeWidget(split);
  m_treeWidget->sortItems(1, Qt::AscendingOrder);
  m_treeWidget->setAllColumnsShowFocus(true);
  m_treeWidget->setSortingEnabled(true);
  m_treeWidget->setRootIsDecorated(false);
  m_treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
  m_treeWidget->setHeaderLabels(QStringList() << QString()
                                              << i18n("Title")
                                              << i18n("Description")
                                              << i18n("Source"));
  m_treeWidget->model()->setHeaderData(0, Qt::Horizontal, Qt::AlignHCenter, Qt::TextAlignmentRole); // align checkmark in middle
  m_treeWidget->viewport()->installEventFilter(this);
  m_treeWidget->header()->setSortIndicatorShown(true);
  m_treeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents); // will show a check mark when added
  m_treeWidget->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

  connect(m_treeWidget, &QTreeWidget::itemSelectionChanged, this, &FetchDialog::slotShowEntry);
  // double clicking should add the entry
  connect(m_treeWidget, &QTreeWidget::itemDoubleClicked, this, &FetchDialog::slotAddEntry);
  connect(m_treeWidget->header(), &QHeaderView::sectionResized, this, &FetchDialog::columnResized);
  m_treeWidget->setWhatsThis(i18n("As results are found, they are added to this list. Selecting one "
                                  "will fetch the complete entry and show it in the view below."));

  m_entryView = new EntryView(split);
  // don't bother creating funky gradient images for compact view
  m_entryView->setUseGradientImages(false);
  // set the xslt file AFTER setting the gradient image option
  m_entryView->setXSLTFile(QStringLiteral("Compact.xsl"));
  m_entryView->addXSLTStringParam("skip-fields", "id,mdate,cdate");
  m_entryView->setWhatsThis(i18n("An entry may be shown here before adding it to the "
                                 "current collection by selecting it in the list above"));

  QWidget* box3 = new QWidget(mainWidget);
  QHBoxLayout* box3HBoxLayout = new QHBoxLayout(box3);
  box3HBoxLayout->setContentsMargins(0, 0, 0, 0);
  topLayout->addWidget(box3);

  m_addButton = new QPushButton(i18n("&Add Entry"), box3);
  box3HBoxLayout->addWidget(m_addButton);
  m_addButton->setEnabled(false);
  m_addButton->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
  connect(m_addButton, &QAbstractButton::clicked, this, &FetchDialog::slotAddEntry);
  m_addButton->setWhatsThis(i18n("Add the selected entry to the current collection"));

  m_moreButton = new QPushButton(box3);
  KGuiItem::assign(m_moreButton, KGuiItem(i18n("Get More Results"), QIcon::fromTheme(QStringLiteral("edit-find"))));
  box3HBoxLayout->addWidget(m_moreButton);
  m_moreButton->setEnabled(false);
  connect(m_moreButton, &QAbstractButton::clicked, this, &FetchDialog::slotMoreClicked);
  m_moreButton->setWhatsThis(i18n("Fetch more results from the current data source"));

  QPushButton* clearButton = new QPushButton(box3);
  KGuiItem::assign(clearButton, KStandardGuiItem::clear());
  box3HBoxLayout->addWidget(clearButton);
  connect(clearButton, &QAbstractButton::clicked, this, &FetchDialog::slotClearClicked);
  clearButton->setWhatsThis(i18n("Clear all search fields and results"));

  QWidget* bottombox = new QWidget(mainWidget);
  QHBoxLayout* bottomboxHBoxLayout = new QHBoxLayout(bottombox);
  bottomboxHBoxLayout->setContentsMargins(0, 0, 0, 0);
  topLayout->addWidget(bottombox);

  m_statusBar = new QStatusBar(bottombox);
  bottomboxHBoxLayout->addWidget(m_statusBar);
  m_statusLabel = new QLabel(m_statusBar);
  m_statusBar->addPermanentWidget(m_statusLabel, 1);
  m_progress = new QProgressBar(m_statusBar);
  m_progress->setFormat(i18nc("%p is the percent value, % is the percent sign", "%p%"));
  m_progress->setMaximum(0);
  m_progress->setFixedHeight(fontMetrics().height()+2);
  m_progress->hide();
  m_statusBar->addPermanentWidget(m_progress);
  m_statusBar->setSizeGripEnabled(false);

  QPushButton* closeButton = new QPushButton(bottombox);
  KGuiItem::assign(closeButton, KStandardGuiItem::close());
  bottomboxHBoxLayout->addWidget(closeButton);
  connect(closeButton, &QAbstractButton::clicked, this, &QDialog::accept);

  connect(m_timer, &QTimer::timeout, this, &FetchDialog::slotMoveProgress);

  setMinimumWidth(qMax(minimumWidth(), qMax(FETCH_MIN_WIDTH, minimumSizeHint().width())));
  setStatus(i18n("Ready."));

  KConfigGroup config(KSharedConfig::openConfig(), QLatin1String("Fetch Dialog Options"));
  const auto splitList = config.readEntry("Splitter Sizes", QList<int>());
  if(splitList.empty()) {
    split->setSizes({3000, 5000}); // default to 3:5 ratio
  } else {
    split->setSizes(splitList);
  }

  connect(Fetch::Manager::self(), &Fetch::Manager::signalResultFound,
                                  this, &FetchDialog::slotResultFound);
  connect(Fetch::Manager::self(), &Fetch::Manager::signalStatus,
                                  this, &FetchDialog::slotStatus);
  connect(Fetch::Manager::self(), &Fetch::Manager::signalDone,
                                  this, &FetchDialog::slotFetchDone);

  KAcceleratorManager::manage(this);
  // initialize combos
  QTimer::singleShot(0, this, &FetchDialog::slotInit);
}

FetchDialog::~FetchDialog() {
#ifdef ENABLE_WEBCAM
  if(m_barcodeRecognitionThread) {
    m_barcodeRecognitionThread->stop();
    if(!m_barcodeRecognitionThread->wait(1000)) {
      m_barcodeRecognitionThread->terminate();
    }
    delete m_barcodeRecognitionThread;
    m_barcodeRecognitionThread = nullptr;
  }
  if(m_barcodePreview) {
    delete m_barcodePreview;
    m_barcodePreview = nullptr;
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

  KConfigGroup config(KSharedConfig::openConfig(), QLatin1String("Fetch Dialog Options"));
  KWindowConfig::saveWindowSize(windowHandle(), config);

  config.writeEntry("Splitter Sizes", static_cast<QSplitter*>(m_treeWidget->parentWidget())->sizes());
  config.writeEntry("Search Key", m_keyCombo->currentData().toInt());
  config.writeEntry("Search Source", m_sourceCombo->currentText());
}

void FetchDialog::closeEvent(QCloseEvent* event_) { // stop fetchers when the dialog is closed
  if(m_started) {
    QTimer::singleShot(0, Fetch::Manager::self(), &Fetch::Manager::stop);
  }
  QDialog::closeEvent(event_);
}

void FetchDialog::slotSearchClicked() {
  m_valueLineEdit->selectAll();
  if(m_started) {
    setStatus(i18n("Cancelling the search..."));
    Fetch::Manager::self()->stop();
  } else {
    const QString value = m_valueLineEdit->text().simplified();
    if(value.isEmpty()) {
      return;
    }
    m_resultCount = 0;
    m_oldSearch = value;
    m_started = true;
    KGuiItem::assign(m_searchButton, KGuiItem(i18n("&Stop"),
                                              QIcon::fromTheme(QStringLiteral("dialog-cancel"))));
    startProgress();
    setStatus(i18n("Searching..."));
    qApp->processEvents();
    Fetch::Manager::self()->startSearch(m_sourceCombo->currentText(),
                                        static_cast<Fetch::FetchKey>(m_keyCombo->currentData().toInt()),
                                        value,
                                        Data::Document::self()->collection()->type());
  }
}

void FetchDialog::slotClearClicked() {
  fetchDone(false);
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
    QTimer::singleShot(2000, this, &FetchDialog::slotUpdateStatus);
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
    QTimer::singleShot(2000, this, &FetchDialog::slotUpdateStatus);
  }
}

void FetchDialog::setStatus(const QString& text_) {
  m_statusLabel->setText(QLatin1Char(' ') + text_);
}

void FetchDialog::slotFetchDone() {
  fetchDone(true);
}

void FetchDialog::fetchDone(bool checkISBN_) {
//  myDebug() << "fetchDone";
  m_started = false;
  KGuiItem::assign(m_searchButton, KGuiItem(i18n("&Search"),
                                            QIcon::fromTheme(QStringLiteral("edit-find"))));
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
    QStringList searchValues = FieldFormat::splitValue(m_oldSearch.simplified());
    QStringList resultValues;
    resultValues.reserve(m_treeWidget->topLevelItemCount());
    for(int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
      resultValues << static_cast<FetchResultItem*>(m_treeWidget->topLevelItem(i))->m_result->isbn;
    }
    // Google Book Search can have an error, returning a different ISBN in the initial search
    // than the one returned by fetchEntryHook(). As a small workaround, if only a single ISBN value
    // is in the search term, then don't show
    if(searchValues.count() > 1) {
      const QStringList valuesNotFound = ISBNValidator::listDifference(searchValues, resultValues);
      if(!valuesNotFound.isEmpty()) {
        KMessageBox::informationList(this,
                                     i18n("No results were found for the following ISBN values:"),
                                     valuesNotFound,
                                     i18n("No Results"));
      }
    }
  }
}

void FetchDialog::slotResultFound(Tellico::Fetch::FetchResult* result_) {
  m_results.append(result_);
  (void) new FetchResultItem(m_treeWidget, result_);
  // resize final column to size of contents if the user has never resized anything before
  if(!m_treeWasResized) {
    m_treeWidget->header()->setStretchLastSection(false);

    // do math to try to make a nice resizing, emphasizing sections 1 and 2 over 3
    const int w0 = m_treeWidget->columnWidth(0);
    const int w1 = m_treeWidget->columnWidth(1);
    const int w2 = m_treeWidget->columnWidth(2);
    const int w3 = m_treeWidget->columnWidth(3);
    const int wt = m_treeWidget->width();

    // whatever is leftover from resizing 3, split between 1 and 2
    if(wt > (w0 + w1 + w2 + w3)) {
      const int w1rec = static_cast<TreeWidget*>(m_treeWidget)->sizeHintForColumn(1);
      const int w2rec = static_cast<TreeWidget*>(m_treeWidget)->sizeHintForColumn(2);
      const int w3rec = static_cast<TreeWidget*>(m_treeWidget)->sizeHintForColumn(3);
      if(w1 < w1rec || w2 < w2rec) {
        const int w3new = qMin(w3, w3rec);
        const int diff = wt - w0 - w1 - w2 - w3new;
        const int w1new = qBound(w1, w1rec, w1 + diff/2 - 4);
        const int w2new = w2 < w2rec ? qBound(w2, wt - w0 - w1new - w3new, w2rec)
                                     : qBound(w2rec, wt - w0 - w1new - w3new, w2);
        m_treeWidget->setColumnWidth(1, w1new);
        m_treeWidget->setColumnWidth(2, w2new);
        m_treeWidget->setColumnWidth(3, w3new);
      }
    }
    m_treeWidget->header()->setStretchLastSection(true);
    // because calling setColumnWidth() will change this
    m_treeWasResized = false;
  }
  ++m_resultCount;
}

void FetchDialog::slotAddEntry() {
  GUI::CursorSaver cs;
  Data::EntryList vec;
  foreach(QTreeWidgetItem* item_, m_treeWidget->selectedItems()) {
    FetchResultItem* item = static_cast<FetchResultItem*>(item_);

    Fetch::FetchResult* r = item->m_result;
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
    if(entry->collection()->hasField(QStringLiteral("fetchdialog_source"))) {
      entry->collection()->removeField(QStringLiteral("fetchdialog_source"));
    }
    // add a copy, intentionally allowing multiple copies to be added
    vec.append(Data::EntryPtr(new Data::Entry(*entry)));
    item->setData(0, Qt::DecorationRole,
                  QIcon::fromTheme(QStringLiteral("checkmark"), QIcon(QLatin1String(":/icons/checkmark"))));
  }
  if(!vec.isEmpty()) {
    Kernel::self()->addEntries(vec, true);
  }
}

void FetchDialog::slotMoreClicked() {
  if(m_started) {
    myDebug() << "can't continue while running";
    return;
  }

  m_started = true;
  KGuiItem::assign(m_searchButton, KGuiItem(i18n("&Stop"),
                                            QIcon::fromTheme(QStringLiteral("dialog-cancel"))));
  startProgress();
  setStatus(i18n("Searching..."));
  qApp->processEvents();
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

  FetchResultItem* item = static_cast<FetchResultItem*>(items.first());
  Fetch::FetchResult* r = item->m_result;
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
  if(!entry || !entry->collection())  {
    myDebug() << "no entry or collection pointer";
    setStatus(i18n("Ready."));
    return;
  }
  if(!entry->collection()->hasField(QStringLiteral("fetchdialog_source"))) {
    Data::FieldPtr f(new Data::Field(QStringLiteral("fetchdialog_source"), i18n("Attribution"), Data::Field::Para));
    entry->collection()->addField(f);
  }

  const QPixmap sourceIcon = Fetch::Manager::self()->fetcherIcon(r->fetcher());
  const QByteArray ba = Data::Image::byteArray(sourceIcon.toImage(), "PNG");
  QString text = QStringLiteral("<qt><img style='vertical-align: top' src='data:image/png;base64,%1'/> %2<br/>%3</qt>")
                 .arg(QLatin1String(ba.toBase64()), r->fetcher()->source(), r->fetcher()->attribution());
  entry->setField(QStringLiteral("fetchdialog_source"), text);

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
  const QSize availableSize = windowHandle()->screen()->availableSize();
  windowHandle()->resize(qMin(800, int(availableSize.width() * 0.5)),
                         qMin(800, int(availableSize.height() * 0.7))); // default size

  // do this in the singleShot slot so it works
  // see note in entryeditdialog.cpp (Feb 2017)
  KConfigGroup config(KSharedConfig::openConfig(), QLatin1String("Fetch Dialog Options"));
  KWindowConfig::restoreWindowSize(windowHandle(), config);

  if(!Fetch::Manager::self()->canFetch(Data::Document::self()->collection()->type())) {
    m_searchButton->setEnabled(false);
    Kernel::self()->sorry(i18n("No Internet sources are available for your current collection type."), this);
  }

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
      connect(upc, &UPCValidator::signalISBN, this, &FetchDialog::slotUPC2ISBN);
      m_valueLineEdit->setValidator(upc);
      // only want to convert to ISBN if ISBN is accepted by the fetcher
      Fetch::KeyMap map = Fetch::Manager::self()->keyMap(m_sourceCombo->currentText());
      upc->setCheckISBN(map.contains(Fetch::ISBN));
    }
  } else {
    m_multipleISBN->setChecked(false);
    m_multipleISBN->setEnabled(false);
//    slotMultipleISBN(false);
    m_valueLineEdit->setValidator(nullptr);
  }

  if(key == Fetch::ISBN || key == Fetch::UPC) {
    openBarcodePreview();
  } else {
    closeBarcodePreview();
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
  QDialog dlg(this);
  dlg.setModal(true);
  dlg.setWindowTitle(i18n("Edit ISBN/UPC Values"));

  QVBoxLayout* mainLayout = new QVBoxLayout();
  dlg.setLayout(mainLayout);

  QWidget* box = new QWidget(&dlg);
  QVBoxLayout* boxVBoxLayout = new QVBoxLayout(box);
  boxVBoxLayout->setContentsMargins(0, 0, 0, 0);
  boxVBoxLayout->setSpacing(10);
  mainLayout->addWidget(box);

  QString s = i18n("<qt>Enter the ISBN or UPC values, one per line.</qt>");
  QLabel* l = new QLabel(s, box);
  boxVBoxLayout->addWidget(l);
  m_isbnTextEdit = new KTextEdit(box);
  boxVBoxLayout->addWidget(m_isbnTextEdit);
  if(m_isbnList.isEmpty()) {
    m_isbnTextEdit->setText(m_valueLineEdit->text());
  } else {
    m_isbnTextEdit->setText(m_isbnList.join(QLatin1String("\n")));
  }
  m_isbnTextEdit->setWhatsThis(s);
  connect(m_isbnTextEdit.data(), &QTextEdit::textChanged, this, &FetchDialog::slotISBNTextChanged);

  QPushButton* fromFileBtn = new QPushButton(box);
  boxVBoxLayout->addWidget(fromFileBtn);
  KGuiItem::assign(fromFileBtn, KStandardGuiItem::open());
  fromFileBtn->setText(i18n("&Load From File..."));
  fromFileBtn->setWhatsThis(i18n("<qt>Load the list from a text file.</qt>"));
  connect(fromFileBtn, &QAbstractButton::clicked, this, &FetchDialog::slotLoadISBNList);

  QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
  boxVBoxLayout->addWidget(buttonBox);
  connect(buttonBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

  dlg.setMinimumWidth(qMax(dlg.minimumWidth(), FETCH_MIN_WIDTH*2/3));

  if(dlg.exec() == QDialog::Accepted) {
    m_isbnList = m_isbnTextEdit->toPlainText().split(QStringLiteral("\n"));
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
    m_valueLineEdit->setText(m_isbnList.join(FieldFormat::delimiterString()));
  }
  m_isbnTextEdit = nullptr; // gets auto-deleted
}

void FetchDialog::slotLoadISBNList() {
  if(!m_isbnTextEdit) {
    return;
  }
  QUrl u = QUrl::fromLocalFile(QFileDialog::getOpenFileName(this, QString(), QString(), QString()));
  if(u.isValid()) {
    m_isbnTextEdit->setText(m_isbnTextEdit->toPlainText() + FileHandler::readTextFile(u));
    m_isbnTextEdit->moveCursor(QTextCursor::End);
    m_isbnTextEdit->ensureCursorVisible();
  }
}

void FetchDialog::slotISBNTextChanged() {
  if(!m_isbnTextEdit) {
    return;
  }
  const QValidator* val = m_valueLineEdit->validator();
  if(!val) {
    return;
  }
  const QString text = m_isbnTextEdit->toPlainText();
  if(text.isEmpty())  {
    return;
  }
  const QTextCursor cursor = m_isbnTextEdit->textCursor();
  // only try to validate if char before cursor is an eol
  if(cursor.atStart() || text.at(cursor.position()-1) != QLatin1Char('\n')) {
    return;
  }
  QStringList lines = text.left(cursor.position()-1).split(QStringLiteral("\n"));
  QString newLine = lines.last();
  int pos = 0;
  // validate() changes the input
  if(val->validate(newLine, pos) != QValidator::Acceptable) {
    return;
  }
  lines.replace(lines.count()-1, newLine);
  QString newText = lines.join(QLatin1String("\n")) + text.mid(cursor.position()-1);
  if(newText == text) {
    return;
  }

  if(newText.isEmpty()) {
    m_isbnTextEdit->clear();
  } else {
    m_isbnTextEdit->blockSignals(true);
    m_isbnTextEdit->setPlainText(newText);
    m_isbnTextEdit->setTextCursor(cursor);
    m_isbnTextEdit->blockSignals(false);
  }
}

void FetchDialog::slotUPC2ISBN() {
  int key = m_keyCombo->currentData().toInt();
  if(key == Fetch::UPC) {
    m_keyCombo->setCurrentData(Fetch::ISBN);
    slotKeyChanged(m_keyCombo->currentIndex());
  }
}

void FetchDialog::columnResized(int column_) {
  // only care about the middle two. First is the checkmark icon, last is not resizeable
  if(column_ == 1 || column_ == 2) {
    m_treeWasResized = true;
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
    m_sourceCombo->addItem(Fetch::Manager::self()->fetcherIcon(fetcher.data()), fetcher->source());
  }

  if(Fetch::Manager::self()->canFetch(Data::Document::self()->collection()->type())) {
    m_searchButton->setEnabled(true);
  } else {
    m_searchButton->setEnabled(false);
    Kernel::self()->sorry(i18n("No Internet sources are available for your current collection type."), this);
  }
}

void FetchDialog::slotBarcodeRecognized(const QString& string_) {
  // attention: this slot is called in the context of another thread => do not use GUI-functions!
  StringDataEvent* e = new StringDataEvent(string_);
  qApp->postEvent(this, e); // the event loop will call FetchDialog::customEvent() in the context of the GUI thread
}

void FetchDialog::slotBarcodeGotImage(const QImage& img_)  {
  // attention: this slot is called in the context of another thread => do not use GUI-functions!
  ImageDataEvent* e = new ImageDataEvent(img_);
  qApp->postEvent(this, e); // the event loop will call FetchDialog::customEvent() in the context of the GUI thread
}

void FetchDialog::openBarcodePreview() {
  if(!Config::enableWebcam()) {
    return;
  }
#ifdef ENABLE_WEBCAM
  if(m_barcodePreview) {
    m_barcodePreview->show();
    m_barcodeRecognitionThread->start();
    return;
  }

  // barcode recognition
  m_barcodeRecognitionThread = new barcodeRecognitionThread();
  if(m_barcodeRecognitionThread->isWebcamAvailable()) {
    m_barcodePreview = new QLabel(nullptr);
    m_barcodePreview->resize(m_barcodeRecognitionThread->getPreviewSize());
    const QRect desk = QApplication::primaryScreen()->geometry();
    m_barcodePreview->move(desk.width() - m_barcodePreview->frameGeometry().width(), 30);
    m_barcodePreview->show();

    connect(m_barcodeRecognitionThread, &barcodeRecognition::barcodeRecognitionThread::recognized,
            this, &FetchDialog::slotBarcodeRecognized);
    connect(m_barcodeRecognitionThread, &barcodeRecognition::barcodeRecognitionThread::gotImage,
            this, &FetchDialog::slotBarcodeGotImage);
//    connect(m_barcodePreview, SIGNAL(destroyed(QObject *)), this, SLOT(slotBarcodeStop()));
    m_barcodeRecognitionThread->start();
  }
#endif
}

void FetchDialog::closeBarcodePreview() {
#ifdef ENABLE_WEBCAM
  if(!m_barcodePreview || !m_barcodeRecognitionThread) {
    return;
  }

  m_barcodePreview->hide();
  m_barcodeRecognitionThread->stop();
#endif
}

void FetchDialog::customEvent(QEvent* e) {
  if(!e) {
    return;
  }
  if(e->type() == StringDataType) {
    // slotBarcodeRecognized() queued call
    qApp->beep();
    m_valueLineEdit->setText(static_cast<StringDataEvent*>(e)->string());
    m_searchButton->animateClick();
  } else if(e->type() == ImageDataType) {
    // slotBarcodegotImage() queued call
    m_barcodePreview->setPixmap(QPixmap::fromImage(static_cast<ImageDataEvent*>(e)->image()));
  }
}
