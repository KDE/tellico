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

#include "fetchdialog.h"
#include "fetch/fetchmanager.h"
#include "fetch/fetcher.h"
#include "entryview.h"
#include "isbnvalidator.h"
#include "tellico_kernel.h"
#include "filehandler.h"

#include <klocale.h>
#include <kdebug.h>
#include <kcombobox.h>
#include <klineedit.h>
#include <kpushbutton.h>
#include <kstatusbar.h>
#include <khtmlview.h>
#include <kprogress.h>
#include <kconfig.h>
#include <kiconloader.h>
#include <kdialogbase.h>
#include <kmessagebox.h>
#include <kfiledialog.h>

#include <qlayout.h>
#include <qhbox.h>
#include <qvgroupbox.h>
#include <qsplitter.h>
#include <qtimer.h>
#include <qwhatsthis.h>
#include <qcheckbox.h>
#include <qvbox.h>
#include <qtimer.h>

namespace {
  static const int FETCH_STATUS_ID = 0;
  static const int FETCH_PROGRESS_ID = 0;
  static const int FETCH_MIN_WIDTH = 600;

  static const char* FETCH_STRING_SEARCH = I18N_NOOP("&Search");
  static const char* FETCH_STRING_STOP   = I18N_NOOP("&Stop");
}

using Tellico::FetchDialog;

// always add to end
FetchDialog::SearchResultItem::SearchResultItem(KListView* lv, const Fetch::SearchResult& r)
    : KListViewItem(lv, lv->lastItem(), QString::null, r.title, r.desc, r.fetcher->source()), m_result(r) {
}

FetchDialog::FetchDialog(QWidget* parent_, const char* name_)
    : KDialogBase(parent_, name_, false, i18n("Internet Search"), 0),
      m_timer(new QTimer(this)), m_started(false) {
  m_entries.setAutoDelete(true);
  QWidget* mainWidget = new QWidget(this, "FetchDialog mainWidget");
  setMainWidget(mainWidget);
  QVBoxLayout* topLayout = new QVBoxLayout(mainWidget, 0, KDialog::spacingHint());

  QVGroupBox* queryBox = new QVGroupBox(i18n("Search Query"), mainWidget, "FetchDialog queryBox");
  topLayout->addWidget(queryBox);

  QHBox* box1 = new QHBox(queryBox, "FetchDialog box1");
  box1->setSpacing(KDialog::spacingHint());

  (void) new QLabel(i18n("Search:"), box1);

  m_valueLineEdit = new KLineEdit(box1);
  QWhatsThis::add(m_valueLineEdit, i18n("Enter a search value. An ISBN search must include the full ISBN."));
  m_keyCombo = new KComboBox(box1);
  m_keyCombo->insertStringList(Fetch::Manager::self()->keys(QString::null));
  connect(m_keyCombo, SIGNAL(activated(const QString&)), SLOT(slotKeyChanged(const QString&)));
  QWhatsThis::add(m_keyCombo, i18n("Choose the type of search"));

  m_searchButton = new KPushButton(box1);
  m_searchButton->setGuiItem(KGuiItem(i18n(FETCH_STRING_STOP),
                             KGlobal::iconLoader()->loadIcon(QString::fromLatin1("cancel"),
                                                             KIcon::Small)));
  connect(m_searchButton, SIGNAL(clicked()), SLOT(slotSearchClicked()));
  QWhatsThis::add(m_searchButton, i18n("Click to start or stop the search"));

  // the search button's text changes from search to stop
  // I don't want it resizing, so figure out the maximum size and set that
  m_searchButton->polish();
  int maxWidth = m_searchButton->sizeHint().width();
  m_searchButton->setGuiItem(KGuiItem(i18n(FETCH_STRING_SEARCH),
                             KGlobal::iconLoader()->loadIcon(QString::fromLatin1("find"),
                                                             KIcon::Small)));
  maxWidth = KMAX(maxWidth, m_searchButton->sizeHint().width());
  m_searchButton->setMinimumWidth(maxWidth);

  QHBox* box2 = new QHBox(queryBox);
  box2->setSpacing(KDialog::spacingHint());

  m_multipleISBN = new QCheckBox(i18n("Multiple ISBN search"), box2);
  QWhatsThis::add(m_multipleISBN, i18n("Check this box to search for multiple ISBN values."));
  connect(m_multipleISBN, SIGNAL(toggled(bool)), SLOT(slotMultipleISBN(bool)));

  m_editISBN = new KPushButton(i18n("Edit ISBN list..."), box2);
  m_editISBN->setEnabled(false);
  QWhatsThis::add(m_editISBN, i18n("Click to open a text edit box for entering or editing multiple ISBN values."));
  connect(m_editISBN, SIGNAL(clicked()), SLOT(slotEditMultipleISBN()));

  // add for spacing
  box2->setStretchFactor(new QWidget(box2), 1);

  (void) new QLabel(i18n("Search Source:"), box2);
  m_sourceCombo = new KComboBox(box2);
  m_sourceCombo->insertStringList(Fetch::Manager::self()->sources());
  connect(m_sourceCombo, SIGNAL(activated(const QString&)), SLOT(slotSourceChanged(const QString&)));
  QWhatsThis::add(m_sourceCombo, i18n("Select the database to search"));

  QSplitter* split = new QSplitter(QSplitter::Vertical, mainWidget);
  topLayout->addWidget(split);

  m_listView = new KListView(split);
//  topLayout->addWidget(m_listView);
//  topLayout->setStretchFactor(m_listView, 1);
  m_listView->setSorting(10); // greater than number of columns, so not sorting until user clicks column header
  m_listView->setShowSortIndicator(true);
  m_listView->setAllColumnsShowFocus(true);
  m_listView->setSelectionMode(QListView::Extended);
  m_listView->addColumn(QString::null); // will show a check mark when added
  m_listView->addColumn(i18n("Title"));
  m_listView->addColumn(i18n("Description"));
  m_listView->addColumn(i18n("Source"));
  connect(m_listView, SIGNAL(clicked(QListViewItem*)), SLOT(slotShowEntry(QListViewItem*)));
  // double clicking should add the entry
  connect(m_listView, SIGNAL(doubleClicked(QListViewItem*)), SLOT(slotAddEntry()));
  QWhatsThis::add(m_listView, i18n("As results are found, they are added to this list. Selecting one "
                                   "will fetch the complete entry and show it in the view below."));

  m_entryView = new EntryView(split, "entry_view");
//  topLayout->addWidget(m_entryView->view());
//  topLayout->setStretchFactor(m_entryView->view(), 2);
  m_entryView->setXSLTFile(QString::fromLatin1("Compact.xsl"));
  QWhatsThis::add(m_entryView->view(), i18n("An entry may be shown here before adding it to the "
                                            "current collection by selecting it in the list above"));

  QHBox* box3 = new QHBox(mainWidget);
  topLayout->addWidget(box3);
  box3->setSpacing(KDialog::spacingHint());

  m_addButton = new KPushButton(i18n("Add Entry"), box3);
  m_addButton->setEnabled(false);
  connect(m_addButton, SIGNAL(clicked()), SLOT(slotAddEntry()));
  QWhatsThis::add(m_addButton, i18n("Add the selected entry to the current collection"));
/*  m_viewButton = new KPushButton(i18n("View Entry"), box3);
  connect(m_viewButton, SIGNAL(clicked()), SLOT(slotViewEntry()));*/
  KPushButton* clearButton = new KPushButton(i18n("Clear"), box3);
  connect(clearButton, SIGNAL(clicked()), SLOT(slotClearClicked()));
  QWhatsThis::add(clearButton, i18n("Clear all search fields and results"));

  QHBox* box = new QHBox(mainWidget, "box");
  topLayout->addWidget(box);
  box->setSpacing(KDialog::spacingHint());

  m_statusBar = new KStatusBar(box, "statusbar");
  m_statusBar->insertItem(QString::null, FETCH_STATUS_ID, 1, false);
  m_statusBar->setItemAlignment(FETCH_STATUS_ID, AlignLeft | AlignVCenter);
  m_progress = new QProgressBar(m_statusBar, "progress");
  m_progress->setTotalSteps(0);
  m_progress->setFixedHeight(fontMetrics().height()+2);
  m_progress->hide();
  m_statusBar->addWidget(m_progress, 0, true);

  KPushButton* closeButton = new KPushButton(KStdGuiItem::close(), box);
  connect(closeButton, SIGNAL(clicked()), SLOT(slotClose()));

  connect(m_timer, SIGNAL(timeout()), SLOT(slotMoveProgress()));

  setMinimumWidth(KMAX(minimumWidth(), FETCH_MIN_WIDTH));
  slotUpdateStatus(i18n("Ready."));

  resize(configDialogSize(QString::fromLatin1("Fetch Dialog Options")));

  KConfig* config = kapp->config();
  KConfigGroupSaver group(config, "Fetch Dialog Options");
  QValueList<int> splitList = config->readIntListEntry("Splitter Sizes");
  if(!splitList.empty()) {
    split->setSizes(splitList);
  }

  connect(Fetch::Manager::self(), SIGNAL(signalResultFound(const Tellico::Fetch::SearchResult&)),
                          SLOT(slotResultFound(const Tellico::Fetch::SearchResult&)));
  connect(Fetch::Manager::self(), SIGNAL(signalStatus(const QString&)), SLOT(slotUpdateStatus(const QString&)));
  connect(Fetch::Manager::self(), SIGNAL(signalDone()), SLOT(slotFetchDone()));

  // initialie combos
  QTimer::singleShot(0, this, SLOT(slotInit()));
}

FetchDialog::~FetchDialog() {
  saveDialogSize(QString::fromLatin1("Fetch Dialog Options"));

  KConfig* config = kapp->config();
  KConfigGroupSaver group(config, "Fetch Dialog Options");
  config->writeEntry("Splitter Sizes", static_cast<QSplitter*>(m_listView->parentWidget())->sizes());
  config->writeEntry("Search Key", m_keyCombo->currentText());
  config->writeEntry("Search Source", m_sourceCombo->currentText());
}

void FetchDialog::slotSearchClicked() {
  if(m_started) {
    Fetch::Manager::self()->stop();
    slotFetchDone();
  } else {
    m_started = true;
    m_origCount = m_listView->childCount();
    m_searchButton->setGuiItem(KGuiItem(i18n(FETCH_STRING_STOP),
                               KGlobal::iconLoader()->loadIcon(QString::fromLatin1("cancel"),
                                                               KIcon::Small)));
    startProgress();
//    kapp->setOverrideCursor(Qt::waitCursor);
    slotUpdateStatus(i18n("Searching..."));
    kapp->processEvents();
    bool multiple = m_multipleISBN->isEnabled() && m_multipleISBN->isChecked();
    Fetch::Manager::self()->startSearch(m_sourceCombo->currentText(),
                                        Fetch::Manager::fetchKey(m_keyCombo->currentText()),
                                        m_valueLineEdit->text().simplifyWhiteSpace(),
                                        multiple);
  }
}

void FetchDialog::slotClearClicked() {
  slotFetchDone();
  m_origCount = 0;
  m_listView->clear();
  m_entryView->clear();
  Fetch::Manager::self()->stop();
  m_valueLineEdit->clear();
  m_addButton->setEnabled(false);
  m_isbnList.clear();
  slotUpdateStatus(i18n("Ready.")); // because slotFetchDone() writes text
}

void FetchDialog::slotUpdateStatus(const QString& status_) {
  m_statusBar->changeItem(QChar(' ') + status_, FETCH_STATUS_ID);
}

void FetchDialog::slotFetchDone() {
//  kdDebug() << "FetchDialog::slotFetchDone()" << endl;
  m_started = false;
  m_searchButton->setGuiItem(KGuiItem(i18n(FETCH_STRING_SEARCH),
                             KGlobal::iconLoader()->loadIcon(QString::fromLatin1("find"),
                                                             KIcon::Small)));
  stopProgress();
//  kapp->restoreOverrideCursor();
  if(m_listView->childCount() == 0) {
    slotUpdateStatus(i18n("The search returned no items."));
  } else {
    slotUpdateStatus(i18n("The search returned 1 item.",
                          "The search returned %n items.",
                          m_listView->childCount() - m_origCount));
  }
}

void FetchDialog::slotResultFound(const Fetch::SearchResult& result_) {
  new SearchResultItem(m_listView, result_);
  kapp->processEvents();
}

void FetchDialog::slotAddEntry() {
  Data::EntryList list;
#if QT_VERSION >= 0x030200
  for(QListViewItemIterator it(m_listView, QListViewItemIterator::Selected); it.current(); ++it) {
#else
  for(QListViewItemIterator it(m_listView); it.current(); ++it) {
    if(!it.current()->isSelected()) {
      continue;
    }
#endif
    SearchResultItem* item = static_cast<SearchResultItem*>(it.current());

    const Fetch::SearchResult& r = item->m_result;
    Data::Entry* entry = m_entries[r.uid];
    if(!entry) {
      entry = r.fetcher->fetchEntry(r.uid);
    }
    list.append(entry);
    // remove it from the dict, since all left over get deleted
    m_entries.setAutoDelete(false);
    m_entries.remove(r.uid);
    m_entries.setAutoDelete(true);
    item->setPixmap(0, KGlobal::iconLoader()->loadIcon(QString::fromLatin1("ok"), KIcon::Small));
  }

  if(!list.isEmpty()) {
    emit signalAddEntries(list);
  }
}

void FetchDialog::slotShowEntry(QListViewItem* item_) {
  if(!item_) {
    m_addButton->setEnabled(false);
    return;
  }

  m_addButton->setEnabled(true);
  SearchResultItem* item = static_cast<SearchResultItem*>(item_);
  const Fetch::SearchResult& r = item->m_result;
  slotUpdateStatus(i18n("Fetching %1...").arg(r.title));
  Data::Entry* entry = m_entries[r.uid];
  if(!entry) {
    entry = r.fetcher->fetchEntry(r.uid);
    m_entries.insert(r.uid, entry);
  }
  slotUpdateStatus(i18n("Ready."));

  m_entryView->showEntry(entry);
}

void FetchDialog::startProgress() {
  m_progress->show();
  m_timer->start(100);
}

void FetchDialog::slotMoveProgress() {
  m_progress->setProgress(m_progress->progress()+5);
}

void FetchDialog::stopProgress() {
  m_timer->stop();
  m_progress->hide();
}

void FetchDialog::slotInit() {
  if(!Fetch::Manager::self()->canFetch()) {
    m_searchButton->setEnabled(false);
    KMessageBox::sorry(this, i18n("No Internet sources are available for your current collection type."));
  }

  KConfig* config = kapp->config();
  KConfigGroupSaver group(config, "Fetch Dialog Options");
  QString key = config->readEntry("Search Key");
  // only change key if valid
  if(!key.isEmpty() && Fetch::Manager::self()->fetchKey(key) != Fetch::FetchFirst) {
    slotKeyChanged(key);
    m_keyCombo->setCurrentText(key);
  }
  // only change source if a fetcher by that name exists
  QString source = config->readEntry("Search Source");
  if(!source.isEmpty() && Fetch::Manager::self()->sources().findIndex(source) > -1) {
    slotSourceChanged(source);
    m_sourceCombo->setCurrentText(source);
  }

  m_valueLineEdit->setFocus();
  m_searchButton->setDefault(true);
}

void FetchDialog::slotKeyChanged(const QString& key_) {
  if(key_ == i18n("ISBN")) {
    m_multipleISBN->setEnabled(true);
    if(Kernel::self()->collection()->type() == Data::Collection::Book
       || Kernel::self()->collection()->type() == Data::Collection::Bibtex) {
      m_valueLineEdit->setValidator(new ISBNValidator(this));
    }
  } else {
    m_multipleISBN->setEnabled(false);
    m_valueLineEdit->setValidator(0);
  }
}

void FetchDialog::slotSourceChanged(const QString& source_) {
  QString s = m_keyCombo->currentText();
  m_keyCombo->clear();
  QStringList sources = Fetch::Manager::self()->keys(source_);
  m_keyCombo->insertStringList(sources);
  int idx = sources.findIndex(s);
  if(idx > -1) {
    m_keyCombo->setCurrentItem(idx);
  }
  slotKeyChanged(m_keyCombo->currentText());
}

void FetchDialog::slotMultipleISBN(bool toggle_) {
  m_valueLineEdit->setEnabled(!toggle_);
  m_editISBN->setEnabled(toggle_);
}

void FetchDialog::slotEditMultipleISBN() {
  KDialogBase* dlg = new KDialogBase(this, "isbn edit dialog", true, i18n("Edit ISBN Values"),
                                     KDialogBase::Ok|KDialogBase::Cancel);

  QVBox* box = new QVBox(dlg);
  box->setSpacing(10);
  (void) new QLabel(i18n("<qt>Enter the ISBN values, one per line.</qt>"), box);
  m_isbnTextEdit = new KTextEdit(box, "isbn text edit");
  m_isbnTextEdit->setText(m_isbnList.join(QChar('\n')));
  QWhatsThis::add(m_isbnTextEdit, i18n("<qt>Enter the ISBN values, one per line.</qt>"));
  KPushButton* fromFileBtn = new KPushButton(KGlobal::iconLoader()->loadIcon(QString::fromLatin1("fileopen"), KIcon::Small), i18n("Load From File"), box);
  QWhatsThis::add(fromFileBtn, i18n("<qt>Load the ISBN list from a text file.</qt>"));
  connect(fromFileBtn, SIGNAL(clicked()), SLOT(slotLoadISBNList()));
  dlg->setMainWidget(box);

  if(dlg->exec() == QDialog::Accepted) {
    m_isbnList = QStringList::split('\n', m_isbnTextEdit->text());
    const QValidator* val = m_valueLineEdit->validator();
    if(val) {
      for(QStringList::Iterator it = m_isbnList.begin(); it != m_isbnList.end(); ++it) {
        val->fixup(*it);
        if((*it).isEmpty()) {
          it = m_isbnList.remove(it);
          // this is next item, shift backward
          --it;
        }
      }
    }
    if(m_isbnList.count() > 100) {
      KMessageBox::sorry(Kernel::self()->widget(), i18n("<qt>An ISBN search can contain a maximum of 100 ISBN values. Only the first "
                                                        "100 values in your list will be used.</qt>"));
    }
    while(m_isbnList.count() > 100) {
      m_isbnList.pop_back();
    }
    m_valueLineEdit->setText(m_isbnList.join(QString::fromLatin1("; ")));
  }
  dlg->delayedDestruct();
}

void FetchDialog::slotLoadISBNList() {
  if(!m_isbnTextEdit) {
    return;
  }
  KURL u = KFileDialog::getOpenURL(QString::null, QString::null, this);
  if(u.isValid()) {
    m_isbnTextEdit->setText(m_isbnTextEdit->text() + FileHandler::readTextFile(u));
    m_isbnTextEdit->moveCursor(QTextEdit::MoveEnd, false);
    m_isbnTextEdit->scrollToBottom();
  }
}

#include "fetchdialog.moc"
