/***************************************************************************
    copyright            : (C) 2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "execexternalfetcher.h"
#include "../document.h"
#include "../collection.h"
#include "../entry.h"
#include "../translators/tellicoimporter.h"
#include "../tellico_debug.h"
#include "../gui/comboboxproxy.h"
#include "../collectionfactory.h"

#include <klocale.h>
#include <kconfig.h>
#include <kprocess.h>
#include <klineedit.h>
#include <kconfig.h>
#include <kurlrequester.h>

#include <qlayout.h>
#include <qlabel.h>
#include <qwhatsthis.h>
#include <qregexp.h>

using Tellico::Fetch::ExecExternalFetcher;

QStringList ExecExternalFetcher::parseArguments(const QString& str_) {
  // be sure to match escaped quotes
  QRegExp quotes(QString::fromLatin1("[^\\\\](['\"])(.*[^\\\\])\\1"));
  quotes.setMinimal(true);
  QRegExp spaces(QString::fromLatin1("\\s+"));
  spaces.setMinimal(true);

  QStringList args;
  int pos = 0;
  for(int nextPos = quotes.search(str_); nextPos > -1; pos = nextPos, nextPos = quotes.search(str_, pos+1)) {
    // a non-quotes arguments runs from pos to nextPos
    args += QStringList::split(spaces, str_.mid(pos, nextPos-pos));
    // move nextpos marker to end of match
    pos = quotes.pos(2); // skip quote
    nextPos += quotes.matchedLength();
    args += str_.mid(pos, nextPos-pos-1);
  }
  // catch the end stuff
  args += QStringList::split(spaces, str_.mid(pos));

#if 0
  for(QStringList::ConstIterator it = args.begin(); it != args.end(); ++it) {
    myDebug() << *it << endl;
  }
#endif

  return args;
}

ExecExternalFetcher::ExecExternalFetcher(QObject* parent_, const char* name_/*=0*/) : Fetcher(parent_, name_),
    m_started(false), m_collType(-1), m_process(0) {
}

ExecExternalFetcher::~ExecExternalFetcher() {
  stop();
}

QString ExecExternalFetcher::source() const {
  return m_name;
}

bool ExecExternalFetcher::canFetch(int type_) const {
  return m_collType == -1 ? true : m_collType == type_;
}

void ExecExternalFetcher::readConfig(KConfig* config_, const QString& group_) {
  KConfigGroupSaver groupSaver(config_, group_);
  QString s = config_->readEntry("Name");
  if(!s.isEmpty()) {
    m_name = s;
  }
  s = config_->readPathEntry("ExecPath");
  if(!s.isEmpty()) {
    m_path = s;
  }
  s = config_->readEntry("Arguments");
  if(!s.isEmpty()) {
    m_args = s;
  }
  m_collType = config_->readNumEntry("CollectionType", -1);
}

void ExecExternalFetcher::search(FetchKey, const QString& value_, bool) {
  m_started = true;

  if(m_path.isEmpty()) {
    stop();
    return;
  }

  m_process = new KProcess();
  connect(m_process, SIGNAL(receivedStdout(KProcess*, char*, int)), SLOT(slotData(KProcess*, char*, int)));
  connect(m_process, SIGNAL(receivedStderr(KProcess*, char*, int)), SLOT(slotError(KProcess*, char*, int)));
  connect(m_process, SIGNAL(processExited(KProcess*)), SLOT(slotProcessExited(KProcess*)));
  *m_process << m_path;
  // should KProcess::quote() be used?
  *m_process << parseArguments(m_args.arg(value_)); // replace %1 with search value
  if(!m_process->start(KProcess::NotifyOnExit, KProcess::AllOutput)) {
    myDebug() << "ExecExternalFetcher::search() - process failed to start" << endl;
    stop();
  }
}

void ExecExternalFetcher::stop() {
  if(!m_started) {
    return;
  }
  if(m_process) {
    m_process->kill();
    delete m_process;
    m_process = 0;
  }
  emit signalDone(this);
  m_data.truncate(0);
  m_started = false;
}

void ExecExternalFetcher::slotData(KProcess*, char* buffer_, int len_) {
  QDataStream stream(m_data, IO_WriteOnly | IO_Append);
  stream.writeRawBytes(buffer_, len_);
}

void ExecExternalFetcher::slotError(KProcess*, char* buffer_, int len_) {
  myDebug() << QCString(buffer_, len_) << endl;
}

void ExecExternalFetcher::slotProcessExited(KProcess*) {
//  myDebug() << "ExecExternalFetcher::slotProcessExited()" << endl;
  if(!m_process->normalExit() || m_process->exitStatus()) {
    myDebug() << "ExecExternalFetcher::slotProcessExited() - process did not exit successfully" << endl;
    stop();
    return;
  }

  if(m_data.isEmpty()) {
    myDebug() << "ExecExternalFetcher::slotProcessExited() - no data" << endl;
    stop();
    return;
  }

  Import::TellicoImporter imp(QString::fromLocal8Bit(m_data, m_data.size()));
  Data::Collection* coll = imp.collection();
  if(!coll) {
    if(!imp.statusMessage().isEmpty()) {
      signalStatus(imp.statusMessage());
    }
    myDebug() << "ExecExternalFetcher::slotProcessExited() - no collection pointer" << endl;
    stop();
    return;
  }
  if(coll->entryCount() == 0) {
    myDebug() << "ExecExternalFetcher::slotProcessExited() - no results" << endl;
    stop();
    return;
  }

  for(Data::EntryVec::ConstIterator entry = coll->entries().begin(); entry != coll->entries().end(); ++entry) {
    QString desc;
    switch(coll->type()) {
      case Data::Collection::Book:
      case Data::Collection::Bibtex:
        desc = entry->field(QString::fromLatin1("author"))
               + QChar('/')
               + entry->field(QString::fromLatin1("publisher"));
        if(!entry->field(QString::fromLatin1("cr_year")).isEmpty()) {
          desc += QChar('/') + entry->field(QString::fromLatin1("cr_year"));
        } else if(!entry->field(QString::fromLatin1("pub_year")).isEmpty()){
          desc += QChar('/') + entry->field(QString::fromLatin1("pub_year"));
        }
        break;

      case Data::Collection::Video:
        desc = entry->field(QString::fromLatin1("studio"))
               + QChar('/')
               + entry->field(QString::fromLatin1("director"))
               + QChar('/')
               + entry->field(QString::fromLatin1("year"))
               + QChar('/')
               + entry->field(QString::fromLatin1("medium"));
        break;

      case Data::Collection::Album:
        desc = entry->field(QString::fromLatin1("artist"))
               + QChar('/')
               + entry->field(QString::fromLatin1("label"))
               + QChar('/')
               + entry->field(QString::fromLatin1("year"));
        break;

      case Data::Collection::Game:
        desc = entry->field(QString::fromLatin1("platform"));
        break;

      default:
        break;
    }
    SearchResult* r = new SearchResult(this, entry->title(), desc);
    m_entries.insert(r->uid, Data::ConstEntryPtr(entry));
    emit signalResultFound(r);
  }
  stop(); // be sure to call this
}

Tellico::Data::Entry* ExecExternalFetcher::fetchEntry(uint uid_) {
  return new Data::Entry(*m_entries[uid_], Data::Document::self()->collection());
}

Tellico::Fetch::ConfigWidget* ExecExternalFetcher::configWidget(QWidget* parent_) const {
  return new ExecExternalFetcher::ConfigWidget(parent_, this);
}

ExecExternalFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const ExecExternalFetcher* fetcher_/*=0*/)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(this, 4, 2);
  l->setSpacing(4);

  QLabel* label = new QLabel(i18n("Application &path: "), this);
  l->addWidget(label, 0, 0);
  m_pathEdit = new KURLRequester(this);
  connect(m_pathEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_pathEdit, 0, 1);
  QString w = i18n("Set the path of the application to run that should output a valid Tellico data file.");
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_pathEdit, w);
  label->setBuddy(m_pathEdit);

  label = new QLabel(i18n("&Arguments: "), this);
  l->addWidget(label, 1, 0);
  m_argsEdit = new KLineEdit(this);
  connect(m_argsEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_argsEdit, 1, 1);
  w = i18n("Add any arguments that may be needed. <b>%1</b> will be replaced by the search terms.");
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_argsEdit, w);
  label->setBuddy(m_argsEdit);

  label = new QLabel(i18n("Collection &type: "), this);
  l->addWidget(label, 2, 0);
  m_collCombo = new GUI::ComboBoxProxy<int>(this);
  CollectionNameMap collMap = CollectionFactory::nameMap();
  for(CollectionNameMap::Iterator it = collMap.begin(); it != collMap.end(); ++it) {
    m_collCombo->insertItem(it.data(), it.key());
  }
  connect(m_collCombo->comboBox(), SIGNAL(activated(int)), SLOT(slotSetModified()));
  l->addWidget(m_collCombo->comboBox(), 2, 1);
  w = i18n("Set the collection type of the data returned from the external application.");
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_collCombo->comboBox(), w);
  label->setBuddy(m_collCombo->comboBox());

  l->setRowStretch(3, 1);

  if(fetcher_) {
    m_pathEdit->setURL(fetcher_->m_path);
    m_argsEdit->setText(fetcher_->m_args);
    if(fetcher_->m_collType > -1) {
      m_collCombo->comboBox()->setCurrentItem(collMap[static_cast<Data::Collection::Type>(fetcher_->m_collType)]);
    } else {
      m_collCombo->comboBox()->setCurrentItem(collMap[Data::Collection::Book]);
    }
  }
}

ExecExternalFetcher::ConfigWidget::~ConfigWidget() {
  delete m_collCombo;
}

void ExecExternalFetcher::ConfigWidget::saveConfig(KConfig* config_) {
  QString s = m_pathEdit->url();
  if(!s.isEmpty()) {
    config_->writePathEntry("ExecPath", s);
  }
  s = m_argsEdit->text();
  if(!s.isEmpty()) {
    config_->writeEntry("Arguments", s);
  }
  config_->writeEntry("CollectionType", m_collCombo->currentData());
  slotSetModified(false);
}

#include "execexternalfetcher.moc"
