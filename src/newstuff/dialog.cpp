/***************************************************************************
    copyright            : (C) 2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "dialog.h"
#include "providerloader.h"
#include "../gui/listview.h"
#include "../latin1literal.h"
#include "../tellico_utils.h"

#include <klocale.h>
#include <kpushbutton.h>
#include <kconfig.h>
#include <kiconloader.h>
#include <kstatusbar.h>
#include <kio/job.h>
#include <kio/netaccess.h>
#include <kaccelmanager.h>
#include <knewstuff/entry.h>
#include <knewstuff/provider.h>
#include <ktempfile.h>

#include <qlabel.h>
#include <qtextedit.h>
#include <qlayout.h>
#include <qwhatsthis.h>
#include <qregexp.h>
#include <qvbox.h>
#include <qimage.h>
#include <qtimer.h>
#include <qprogressbar.h>

#if KDE_IS_VERSION(3,4,90)
#define ENTRYNAME(e) e->name(m_lang)
#define ENTRYSUMM(e) e->summary(m_lang)
#define ENTRYEMAIL(e) e->authorEmail()
#else
#define ENTRYNAME(e) e->name()
#define ENTRYSUMM(e) e->summary()
#define ENTRYEMAIL(e) QString()
#endif

namespace {
  static const int NEW_STUFF_MIN_WIDTH = 600;
  static const int NEW_STUFF_MIN_HEIGHT = 400;
  static const int PROGRESS_STATUS_ID = 0;
}

using Tellico::NewStuff::Dialog;

class Dialog::Item : public GUI::ListViewItem {
public:
  Item(GUI::ListView* parent) : GUI::ListViewItem(parent) {}

  InstallStatus status() const { return m_status; }
  void setStatus(InstallStatus status) {
    m_status = status;
    if(m_status  == Current) {
      setPixmap(0, SmallIcon(QString::fromLatin1("ok")));
    } else if(m_status == OldVersion) {
      setPixmap(0, SmallIcon(QString::fromLatin1("reload")));
    }
  }

  QString key(int col, bool asc) const {
    if(col == 2 || col == 3) {
      QString s;
      s.sprintf("%08d", text(col).toInt());
      return s;
    } else if(col == 4) {
      QString s;
      QDate date = KGlobal::locale()->readDate(text(col));
      s.sprintf("%08d", date.year() * 366 + date.dayOfYear());
      return s;
    }
    return GUI::ListViewItem::key(col, asc);
  }

private:
  InstallStatus m_status;
};

Dialog::Dialog(NewStuff::DataType type_, QWidget* parent_)
    : KDialogBase(KDialogBase::Plain, i18n("Get Hot New Stuff"), 0, (KDialogBase::ButtonCode)0, parent_)
    , m_manager(new Manager(this))
    , m_type(type_)
    , m_timer(new QTimer(this))
    , m_cursorSaver(new GUI::CursorSaver())
    , m_tempPreviewImage(0)
    , m_lastPreviewItem(0) {

  m_lang = KGlobal::locale()->language();

  QFrame* frame = plainPage();
  QBoxLayout* boxLayout = new QVBoxLayout(frame, 0, KDialog::spacingHint());

  m_split = new QSplitter(Qt::Vertical, frame);
  boxLayout->addWidget(m_split);

  m_listView = new GUI::ListView(m_split);
  m_listView->setAllColumnsShowFocus(true);
  m_listView->setSelectionMode(QListView::Single);
  m_listView->addColumn(i18n("Name"));
  m_listView->addColumn(i18n("Version"));
  m_listView->addColumn(i18n("Rating"));
  m_listView->addColumn(i18n("Downloads"));
  m_listView->addColumn(i18n("Release Date"));
  m_listView->setSorting(2, false);
  m_listView->setResizeMode(QListView::AllColumns);
  connect(m_listView, SIGNAL(clicked(QListViewItem*)), SLOT(slotSelected(QListViewItem*)));
  QWhatsThis::add(m_listView, i18n("This is a list of all the items available for download. "
                                   "Previously installed items have a checkmark icon, while "
                                   "items with new version available have an update icon"));

  QWidget* widget = new QWidget(m_split);
  QBoxLayout* boxLayout2 = new QVBoxLayout(widget, 0, KDialog::spacingHint());

  m_iconLabel = new QLabel(widget);
  m_iconLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  m_iconLabel->setMargin(0);

  m_nameLabel = new QLabel(widget);
  QFont font = m_nameLabel->font();
  font.setBold(true);
  font.setItalic(true);
  m_nameLabel->setFont(font);
  QWhatsThis::add(m_nameLabel, i18n("The name and license of the selected item"));

  m_infoLabel = new QLabel(widget);
  QWhatsThis::add(m_infoLabel, i18n("The author of the selected item"));

  m_install = new KPushButton(i18n("Install"), widget);
  m_install->setIconSet(SmallIconSet(QString::fromLatin1("knewstuff")));
  m_install->setEnabled(false);
  connect(m_install, SIGNAL(clicked()), SLOT(slotInstall()));

  // the button's text changes later
  // I don't want it resizing, so figure out the maximum size and set that
  m_install->polish();
  int maxWidth = m_install->sizeHint().width();
  int maxHeight = m_install->sizeHint().height();
  m_install->setGuiItem(KGuiItem(i18n("Update"), SmallIconSet(QString::fromLatin1("knewstuff"))));
  maxWidth = QMAX(maxWidth, m_install->sizeHint().width());
  maxHeight = QMAX(maxHeight, m_install->sizeHint().height());
  m_install->setMinimumWidth(maxWidth);
  m_install->setMinimumHeight(maxHeight);

  QPixmap pix;
  if(m_type == EntryTemplate) {
    pix = DesktopIcon(QString::fromLatin1("looknfeel"), KIcon::SizeLarge);
    QWhatsThis::add(m_install, i18n("Download and install the selected template."));
  } else {
    pix = UserIcon(QString::fromLatin1("script"));
    QWhatsThis::add(m_install, i18n("Download and install the selected script. Some scripts "
                                    "may need to be configured after being installed."));
  }
  m_iconLabel->setPixmap(pix);

  QBoxLayout* boxLayout3 = new QHBoxLayout(boxLayout2);

  QBoxLayout* boxLayout4 = new QVBoxLayout(boxLayout3);
  boxLayout4->addWidget(m_iconLabel);
  boxLayout4->addStretch(10);

  boxLayout3->addSpacing(4);

  QBoxLayout* boxLayout5 = new QVBoxLayout(boxLayout3);
  boxLayout5->addWidget(m_nameLabel);
  boxLayout5->addWidget(m_infoLabel);
  boxLayout5->addStretch(10);

  boxLayout3->addStretch(10);

  QBoxLayout* boxLayout6 = new QVBoxLayout(boxLayout3);
  boxLayout6->addWidget(m_install);
  boxLayout6->addStretch(10);

  m_descLabel = new QTextEdit(widget);
  m_descLabel->setReadOnly(true);
  m_descLabel->setTextFormat(Qt::RichText);
  m_descLabel->setPaper(colorGroup().background());
  m_descLabel->setMinimumHeight(5 * fontMetrics().height());
  boxLayout2->addWidget(m_descLabel, 10);
  QWhatsThis::add(m_descLabel, i18n("A description of the selected item is shown here."));

  QHBox* box = new QHBox(frame, "statusbox");
  boxLayout->addWidget(box);
  box->setSpacing(KDialog::spacingHint());

  m_statusBar = new KStatusBar(box, "statusbar");
  m_statusBar->insertItem(QString::null, PROGRESS_STATUS_ID, 1, false);
  m_statusBar->setItemAlignment(PROGRESS_STATUS_ID, AlignLeft | AlignVCenter);
  m_progress = new QProgressBar(m_statusBar, "progress");
  m_progress->setTotalSteps(0);
  m_progress->setFixedHeight(fontMetrics().height()+2);
  m_statusBar->addWidget(m_progress, 0, true);

  KPushButton* closeButton = new KPushButton(KStdGuiItem::close(), box);
  connect(closeButton, SIGNAL(clicked()), SLOT(slotClose()));
  closeButton->setFocus();

  connect(m_timer, SIGNAL(timeout()), SLOT(slotMoveProgress()));

  setMinimumWidth(QMAX(minimumWidth(), NEW_STUFF_MIN_WIDTH));
  setMinimumHeight(QMAX(minimumHeight(), NEW_STUFF_MIN_HEIGHT));
  resize(configDialogSize(QString::fromLatin1("NewStuff Dialog Options")));

  KConfigGroup dialogConfig(KGlobal::config(), "NewStuff Dialog Options");
  QValueList<int> splitList = dialogConfig.readIntListEntry("Splitter Sizes");
  if(!splitList.empty()) {
    m_split->setSizes(splitList);
  }

  setStatus(i18n("Downloading information..."));

  ProviderLoader* loader = new Tellico::NewStuff::ProviderLoader(this);
  connect(loader, SIGNAL(providersLoaded(QPtrList<KNS::Provider>*)), SLOT(slotProviders(QPtrList<KNS::Provider>*)));
  connect(loader, SIGNAL(percent(KIO::Job*, unsigned long)), SLOT(slotShowPercent(KIO::Job*, unsigned long)));
  connect(loader, SIGNAL(error()), SLOT(slotProviderError()));

  KConfigGroup config(KGlobal::config(), "KNewStuff");
  QString prov = config.readEntry("ProvidersUrl");
  if(prov.isEmpty()) {
    if(m_type == EntryTemplate) {
      prov = QString::fromLatin1("http://periapsis.org/tellico/newstuff/tellicotemplates-providers.php");
      QString alt = QString::fromLatin1("http://download.kde.org/khotnewstuff/tellicotemplates-providers.xml");
      loader->setAlternativeProvider(alt);
    } else {
      prov = QString::fromLatin1("http://periapsis.org/tellico/newstuff/tellicoscripts-providers.php");
    }
  }
  if(m_type == EntryTemplate) {
    m_typeName = QString::fromLatin1("tellico/entry-template");
  } else {
    m_typeName = QString::fromLatin1("tellico/data-source");
  }
  loader->load(m_typeName, prov);

  KAcceleratorManager::manage(this);
}

Dialog::~Dialog() {
  delete m_cursorSaver;
  m_cursorSaver = 0;

  saveDialogSize(QString::fromLatin1("NewStuff Dialog Options"));
  KConfigGroup config(KGlobal::config(), "NewStuff Dialog Options");
  config.writeEntry("Splitter Sizes", m_split->sizes());
}

void Dialog::slotProviderError() {
  if(m_listView->childCount() == 0) {
    myDebug() << "NewStuff::Dialog::slotCheckError() - no available items" << endl;
    setStatus(QString());

    delete m_cursorSaver;
    m_cursorSaver = 0;
  }
}

void Dialog::slotProviders(QPtrList<KNS::Provider>* list_) {
  for(KNS::Provider* prov = list_->first(); prov; prov = list_->next()) {
    KIO::TransferJob* job = KIO::get(prov->downloadUrl());
    m_jobs[job] = prov;
    connect(job, SIGNAL(data(KIO::Job*, const QByteArray&)),
            SLOT(slotData(KIO::Job*, const QByteArray&)));
    connect(job, SIGNAL(result(KIO::Job*)), SLOT(slotResult(KIO::Job*)));
    connect(job, SIGNAL(percent(KIO::Job*, unsigned long)),
            SLOT(slotShowPercent(KIO::Job*, unsigned long)));
  }
}

void Dialog::slotData(KIO::Job* job_, const QByteArray& data_) {
  QDataStream stream(m_data[job_], IO_WriteOnly | IO_Append);
  stream.writeRawBytes(data_.data(), data_.size());
}

void Dialog::slotResult(KIO::Job* job_) {
//  myDebug() << "NewStuff::Dialog::slotResult()" << endl;
  QDomDocument dom;
  if(!dom.setContent(m_data[job_])) {
    KNS::Provider* prov = m_jobs[job_];
    KURL u = prov ? prov->downloadUrl() : KURL();
    myDebug() << "NewStuff::Dialog::slotResult() - can't load result: " << u.url() << endl;
    m_jobs.remove(job_);
    if(m_jobs.isEmpty()) {
      setStatus(i18n("Ready."));
      delete m_cursorSaver;
      m_cursorSaver = 0;
    }
    return;
  }

  QDomElement knewstuff = dom.documentElement();

  for(QDomNode pn = knewstuff.firstChild(); !pn.isNull(); pn = pn.nextSibling()) {
    QDomElement stuff = pn.toElement();
    if(stuff.isNull()) {
      continue;
    }

    if(stuff.tagName() == Latin1Literal("stuff")) {
      KNS::Entry* entry = new KNS::Entry(stuff);
      if(!entry->type().isEmpty() && entry->type() != m_typeName) {
        myLog() << "NewStuff::Dialog::slotResult() - type mismatch, skipping " << ENTRYNAME(entry) << endl;
        continue;
      }

      addEntry(entry);
    }
  }
  m_jobs.remove(job_);
  if(m_jobs.isEmpty()) {
    setStatus(i18n("Ready."));
    delete m_cursorSaver;
    m_cursorSaver = 0;
  }
}

void Dialog::addEntry(KNS::Entry* entry_) {
  if(!entry_) {
    return;
  }

  Item* item = new Item(m_listView);
  item->setText(0, ENTRYNAME(entry_));
  item->setText(1, entry_->version());
  item->setText(2, QString::number(entry_->rating()));
  item->setText(3, QString::number(entry_->downloads()));
  item->setText(4, KGlobal::locale()->formatDate(entry_->releaseDate(), true /*short format */));
  item->setStatus(NewStuff::Manager::installStatus(entry_));
  m_entryMap.insert(item, entry_);

  if(!m_listView->selectedItem()) {
    m_listView->setSelected(item, true);
    slotSelected(item);
  }
}

void Dialog::slotSelected(QListViewItem* item_) {
  if(!item_) {
    return;
  }

  KNS::Entry* entry = m_entryMap[item_];
  if(!entry) {
    return;
  }

  KURL preview = entry->preview(m_lang);
  if(!preview.isEmpty() && preview.isValid()) {
    delete m_tempPreviewImage;
    m_tempPreviewImage = new KTempFile();
    m_tempPreviewImage->setAutoDelete(true);
    KURL dest;
    dest.setPath(m_tempPreviewImage->name());
    KIO::FileCopyJob* job = KIO::file_copy(preview, dest, -1, true, false, false);
    connect(job, SIGNAL(result(KIO::Job*)), SLOT(slotPreviewResult(KIO::Job*)));
    connect(job, SIGNAL(percent(KIO::Job*, unsigned long)),
            SLOT(slotShowPercent(KIO::Job*, unsigned long)));
    m_lastPreviewItem = item_;
  }
  QPixmap pix = m_type == EntryTemplate
              ? DesktopIcon(QString::fromLatin1("looknfeel"), KIcon::SizeLarge)
              : UserIcon(QString::fromLatin1("script"));
  m_iconLabel->setPixmap(pix);

  QString license = entry->license();
  if(!license.isEmpty()) {
    license.prepend('(').append(')');
  }
  QString name = QString::fromLatin1("%1 %2").arg(ENTRYNAME(entry)).arg(license);
  QFont font = m_nameLabel->font();
  font.setBold(true);
  font.setItalic(false);
  m_nameLabel->setFont(font);
  m_nameLabel->setText(name);

  m_infoLabel->setText(entry->author());

  QString desc = entry->summary(m_lang);
  desc.replace(QRegExp(QString::fromLatin1("\\n")), QString::fromLatin1("<br>"));
  m_descLabel->setText(desc);

  InstallStatus installed = static_cast<Item*>(item_)->status();
  m_install->setText(installed == OldVersion ? i18n("Update Stuff", "Update") : i18n("Install"));
  m_install->setEnabled(installed != Current);
}

void Dialog::slotInstall() {
  QListViewItem* item = m_listView->currentItem();
  if(!item) {
    return;
  }

  KNS::Entry* entry = m_entryMap[item];
  if(!entry) {
    return;
  }

  delete m_cursorSaver;
  m_cursorSaver = new GUI::CursorSaver();
  setStatus(i18n("Installing item..."));
  m_progress->show();
  m_timer->start(100);
  connect(m_manager, SIGNAL(signalInstalled(KNS::Entry*)), SLOT(slotDoneInstall(KNS::Entry*)));
  m_manager->install(m_type, entry);
}

void Dialog::slotDoneInstall(KNS::Entry* entry_) {
  QMap<QListViewItem*, KNS::Entry*>::Iterator it;
  for(it = m_entryMap.begin(); it != m_entryMap.end(); ++it) {
    if(it.data() == entry_) {
      InstallStatus installed = Manager::installStatus(entry_);
      static_cast<Item*>(it.key())->setStatus(installed);
      m_install->setEnabled(installed != Current);
      break;
    }
  }
  delete m_cursorSaver;
  m_cursorSaver = 0;
  setStatus(i18n("Ready."));
  m_timer->stop();
  m_progress->hide();
}

void Dialog::slotMoveProgress() {
  m_progress->setProgress(m_progress->progress()+5);
}

void Dialog::setStatus(const QString& text_) {
  m_statusBar->changeItem(QChar(' ') + text_, PROGRESS_STATUS_ID);
}

void Dialog::slotShowPercent(KIO::Job*, unsigned long pct_) {
  if(pct_ >= 100) {
    m_progress->hide();
  } else {
    m_progress->show();
    m_progress->setProgress(static_cast<int>(pct_), 100);
  }
}

void Dialog::slotPreviewResult(KIO::Job* job_) {
  KIO::FileCopyJob* job = static_cast<KIO::FileCopyJob*>(job_);
  if(job->error()) {
    return;
  }
  QString tmpFile = job->destURL().path(); // might be different than m_tempPreviewImage->name()
  QPixmap pix(tmpFile);

  if(!pix.isNull()) {
    if(pix.width() > 64 || pix.height() > 64) {
      pix.convertFromImage(pix.convertToImage().smoothScale(64, 64, QImage::ScaleMin));
    }
    // only set label if it's still current
    if(m_listView->selectedItem() == m_lastPreviewItem) {
      m_iconLabel->setPixmap(pix);
    }
  }
  delete m_tempPreviewImage;
  m_tempPreviewImage = 0;
}

#include "dialog.moc"
