/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
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
#include "messagehandler.h"
#include "../collection.h"
#include "../entry.h"
#include "../importdialog.h"
#include "../translators/tellicoimporter.h"
#include "../tellico_debug.h"
#include "../gui/comboboxproxy.h"
#include "../collectionfactory.h"
#include "fetchmanager.h"

#include <klocale.h>
#include <kconfig.h>
#include <kprocess.h>
#include <klineedit.h>
#include <kconfig.h>
#include <kurlrequester.h>
#include <kaccelmanager.h>

#include <qlayout.h>
#include <qlabel.h>
#include <qwhatsthis.h>
#include <qregexp.h>
#include <qvgroupbox.h>
#include <qgrid.h>

using Tellico::Fetch::ExecExternalFetcher;

QStringList ExecExternalFetcher::parseArguments(const QString& str_) {
  // matching escaped quotes is too hard... :(
//  QRegExp quotes(QString::fromLatin1("[^\\\\](['\"])(.*[^\\\\])\\1"));
  QRegExp quotes(QString::fromLatin1("(['\"])(.*)\\1"));
  quotes.setMinimal(true);
  QRegExp spaces(QString::fromLatin1("\\s+"));
  spaces.setMinimal(true);

  QStringList args;
  int pos = 0;
  for(int nextPos = quotes.search(str_); nextPos > -1; pos = nextPos+1, nextPos = quotes.search(str_, pos)) {
    // a non-quotes arguments runs from pos to nextPos
    args += QStringList::split(spaces, str_.mid(pos, nextPos-pos));
    // move nextpos marker to end of match
    pos = quotes.pos(2); // skip quotation mark
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
    m_started(false), m_collType(-1), m_formatType(-1), m_canUpdate(false), m_process(0) {
}

ExecExternalFetcher::~ExecExternalFetcher() {
  stop();
}

QString ExecExternalFetcher::defaultName() {
  return i18n("External Application");
}

QString ExecExternalFetcher::source() const {
  return m_name;
}

bool ExecExternalFetcher::canFetch(int type_) const {
  return m_collType == -1 ? false : m_collType == type_;
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
  QValueList<int> il;
  if(config_->hasKey("ArgumentKeys")) {
    il = config_->readIntListEntry("ArgumentKeys");
  } else {
    il.append(Keyword);
  }
  QStringList sl = config_->readListEntry("Arguments");
  if(il.count() != sl.count()) {
    kdWarning() << "ExecExternalFetcher::readConfig() - unequal number of arguments and keys" << endl;
  }
  int n = KMIN(il.count(), sl.count());
  for(int i = 0; i < n; ++i) {
    m_args[static_cast<FetchKey>(il[i])] = sl[i];
  }
  if(config_->hasKey("UpdateArgs")) {
    m_canUpdate = true;
    m_updateArgs = config_->readEntry("UpdateArgs");
  } else {
    m_canUpdate = false;
  }
  m_collType = config_->readNumEntry("CollectionType", -1);
  m_formatType = config_->readNumEntry("FormatType", -1);
}

void ExecExternalFetcher::search(FetchKey key_, const QString& value_) {
  m_started = true;

  if(!m_args.contains(key_)) {
    stop();
    return;
  }

  // should KProcess::quote() be used?
  // %1 gets replaced by the search value, but since the arguments are going to be split
  // the search value needs to be enclosed in quotation marks
  // but first check to make sure the user didn't do that already
  // AND the "%1" wasn't used in the settings
  QString value = value_;
  QRegExp rx1(QString::fromLatin1("['\"].*\\1"));
  if(!rx1.exactMatch(value)) {
    value.prepend('"').append('"');
  }
  QString args = m_args[key_];
  QRegExp rx2(QString::fromLatin1("['\"]%1\\1"));
  args.replace(rx2, QString::fromLatin1("%1"));
  startSearch(parseArguments(args.arg(value))); // replace %1 with search value
}

void ExecExternalFetcher::startSearch(const QStringList& args_) {
  if(m_path.isEmpty()) {
    stop();
    return;
  }

  m_process = new KProcess();
  connect(m_process, SIGNAL(receivedStdout(KProcess*, char*, int)), SLOT(slotData(KProcess*, char*, int)));
  connect(m_process, SIGNAL(receivedStderr(KProcess*, char*, int)), SLOT(slotError(KProcess*, char*, int)));
  connect(m_process, SIGNAL(processExited(KProcess*)), SLOT(slotProcessExited(KProcess*)));
  *m_process << m_path << args_;
  if(!m_process->start(KProcess::NotifyOnExit, KProcess::AllOutput)) {
    myDebug() << "ExecExternalFetcher::startSearch() - process failed to start" << endl;
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
  m_data.truncate(0);
  m_started = false;
  m_errors.clear();
  emit signalDone(this);
}

void ExecExternalFetcher::slotData(KProcess*, char* buffer_, int len_) {
  QDataStream stream(m_data, IO_WriteOnly | IO_Append);
  stream.writeRawBytes(buffer_, len_);
}

void ExecExternalFetcher::slotError(KProcess*, char* buffer_, int len_) {
  QString msg = QString::fromLocal8Bit(buffer_, len_);
  msg.prepend(source() + QString::fromLatin1(": "));
  myDebug() << "ExecExternalFetcher::slotError() - " << msg << endl;
  m_errors << msg;
}

void ExecExternalFetcher::slotProcessExited(KProcess*) {
//  myDebug() << "ExecExternalFetcher::slotProcessExited()" << endl;
  if(!m_process->normalExit() || m_process->exitStatus()) {
    myDebug() << "ExecExternalFetcher::slotProcessExited() - "<< source() << ": process did not exit successfully" << endl;
    if(!m_errors.isEmpty()) {
      message(m_errors.join(QChar('\n')), MessageHandler::Error);
    }
    stop();
    return;
  }
  if(!m_errors.isEmpty()) {
    message(m_errors.join(QChar('\n')), MessageHandler::Warning);
  }

  if(m_data.isEmpty()) {
    myDebug() << "ExecExternalFetcher::slotProcessExited() - "<< source() << ": no data" << endl;
    stop();
    return;
  }

  Import::Format format = static_cast<Import::Format>(m_formatType > -1 ? m_formatType : Import::TellicoXML);
  Import::Importer* imp = ImportDialog::importer(format, KURL());
  if(!imp) {
    stop();
    return;
  }

  imp->setText(QString::fromUtf8(m_data, m_data.size()));
  Data::CollPtr coll = imp->collection();
  if(!coll) {
    if(!imp->statusMessage().isEmpty()) {
      message(imp->statusMessage(), MessageHandler::Status);
    }
    myDebug() << "ExecExternalFetcher::slotProcessExited() - "<< source() << ": no collection pointer" << endl;
    delete imp;
    stop();
    return;
  }

  delete imp;
  if(coll->entryCount() == 0) {
//    myDebug() << "ExecExternalFetcher::slotProcessExited() - no results" << endl;
    stop();
    return;
  }

  Data::EntryVec entries = coll->entries();
  for(Data::EntryVec::Iterator entry = entries.begin(); entry != entries.end(); ++entry) {
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
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
  }
  stop(); // be sure to call this
}

Tellico::Data::EntryPtr ExecExternalFetcher::fetchEntry(uint uid_) {
  return m_entries[uid_];
}

void ExecExternalFetcher::updateEntry(Data::EntryPtr entry_) {
  if(!m_canUpdate) {
    emit signalDone(this); // must do this
  }

  m_started = true;

  Data::ConstEntryPtr e(entry_.data());
  QStringList args = parseArguments(m_updateArgs);
  for(QStringList::Iterator it = args.begin(); it != args.end(); ++it) {
    *it = Data::Entry::dependentValue(e, *it, false);
  }
  startSearch(args);
}

Tellico::Fetch::ConfigWidget* ExecExternalFetcher::configWidget(QWidget* parent_) const {
  return new ExecExternalFetcher::ConfigWidget(parent_, this);
}

ExecExternalFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const ExecExternalFetcher* fetcher_/*=0*/)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget(), 5, 2);
  l->setSpacing(4);
  l->setColStretch(1, 10);

  int row = 0;

  QLabel* label = new QLabel(i18n("Collection &type:"), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_collCombo = new CBProxy(optionsWidget());
  CollectionNameMap collMap = CollectionFactory::nameMap();
  for(CollectionNameMap::Iterator it = collMap.begin(); it != collMap.end(); ++it) {
    m_collCombo->insertItem(it.data(), it.key());
  }
  connect(m_collCombo->comboBox(), SIGNAL(activated(int)), SLOT(slotSetModified()));
  l->addWidget(m_collCombo->comboBox(), row, 1);
  QString w = i18n("Set the collection type of the data returned from the external application.");
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_collCombo->comboBox(), w);
  label->setBuddy(m_collCombo->comboBox());

  label = new QLabel(i18n("&Result type: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_formatCombo = new CBProxy(optionsWidget());
  Import::FormatMap formatMap = ImportDialog::formatMap();
  for(Import::FormatMap::Iterator it = formatMap.begin(); it != formatMap.end(); ++it) {
    m_formatCombo->insertItem(it.data(), it.key());
  }
  connect(m_formatCombo->comboBox(), SIGNAL(activated(int)), SLOT(slotSetModified()));
  l->addWidget(m_formatCombo->comboBox(), row, 1);
  w = i18n("Set the result type of the data returned from the external application.");
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_formatCombo->comboBox(), w);
  label->setBuddy(m_formatCombo->comboBox());

  label = new QLabel(i18n("Application &path: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_pathEdit = new KURLRequester(optionsWidget());
  connect(m_pathEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_pathEdit, row, 1);
  w = i18n("Set the path of the application to run that should output a valid Tellico data file.");
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_pathEdit, w);
  label->setBuddy(m_pathEdit);

  w = i18n("Select the search keys supported by the data source.");
  QString w2 = i18n("Add any arguments that may be needed. <b>%1</b> will be replaced by the search term.");
  QVGroupBox* box = new QVGroupBox(i18n("Arguments"), optionsWidget());
  ++row;
  l->addMultiCellWidget(box, row, row, 0, 1);
  QGrid* grid = new QGrid(2, box);
  for(int i = FetchFirst+1; i < FetchLast; ++i) {
    if(i == Raw) {
      continue;
    }
    FetchKey key = static_cast<FetchKey>(i);
    QCheckBox* cb = new QCheckBox(Fetch::Manager::self()->fetchKeyString(key), grid);
    m_cbDict.insert(key, cb);
    KLineEdit* le = new KLineEdit(grid);
    m_leDict.insert(key, le);
    if(fetcher_ && fetcher_->m_args.contains(key)) {
      cb->setChecked(true);
      le->setEnabled(true);
      le->setText(fetcher_->m_args[key]);
    } else {
      cb->setChecked(false);
      le->setEnabled(false);
    }
    connect(cb, SIGNAL(toggled(bool)), le, SLOT(setEnabled(bool)));
    QWhatsThis::add(cb, w);
    QWhatsThis::add(le, w2);
  }
  m_cbUpdate = new QCheckBox(i18n("Update"), grid);
  m_leUpdate = new KLineEdit(grid);
  w2 = i18n("<p>Enter the arguments which should be used to search for available updates to an entry.</p><p>"
           "The format is the same as for <i>Dependent</i> fields, where field values "
           "are contained inside braces, such as <i>%{author}</i> See the documentation for details.</p>");
  QWhatsThis::add(m_cbUpdate, w);
  QWhatsThis::add(m_leUpdate, w2);
  if(fetcher_ && fetcher_->m_canUpdate) {
    m_cbUpdate->setChecked(true);
    m_leUpdate->setEnabled(true);
    m_leUpdate->setText(fetcher_->m_updateArgs);
  } else {
    m_cbUpdate->setChecked(false);
    m_leUpdate->setEnabled(false);
  }
  connect(m_cbUpdate, SIGNAL(toggled(bool)), m_leUpdate, SLOT(setEnabled(bool)));

  l->setRowStretch(++row, 1);

  if(fetcher_) {
    m_pathEdit->setURL(fetcher_->m_path);
  }
  if(fetcher_ && fetcher_->m_collType > -1) {
    m_collCombo->comboBox()->setCurrentItem(collMap[static_cast<Data::Collection::Type>(fetcher_->m_collType)]);
  } else {
    m_collCombo->comboBox()->setCurrentItem(collMap[Data::Collection::Book]);
  }
  if(fetcher_ && fetcher_->m_formatType > -1) {
    m_formatCombo->comboBox()->setCurrentItem(formatMap[static_cast<Import::Format>(fetcher_->m_formatType)]);
  } else {
    m_formatCombo->comboBox()->setCurrentItem(formatMap[Import::TellicoXML]);
  }
  KAcceleratorManager::manage(optionsWidget());
}

ExecExternalFetcher::ConfigWidget::~ConfigWidget() {
  delete m_collCombo;
  delete m_formatCombo;
}

void ExecExternalFetcher::ConfigWidget::saveConfig(KConfig* config_) {
  QString s = m_pathEdit->url();
  if(!s.isEmpty()) {
    config_->writePathEntry("ExecPath", s);
  }
  QValueList<int> keys;
  QStringList args;
  for(QIntDictIterator<QCheckBox> it(m_cbDict); it.current(); ++it) {
    if(it.current()->isChecked()) {
      keys << it.currentKey();
      args << m_leDict[it.currentKey()]->text();
    }
  }
  config_->writeEntry("ArgumentKeys", keys);
  config_->writeEntry("Arguments", args);

  if(m_cbUpdate->isChecked()) {
    config_->writeEntry("UpdateArgs", m_leUpdate->text());
  } else {
    config_->deleteEntry("UpdateArgs");
  }

  config_->writeEntry("CollectionType", m_collCombo->currentData());
  config_->writeEntry("FormatType", m_formatCombo->currentData());
  slotSetModified(false);
}

#include "execexternalfetcher.moc"
