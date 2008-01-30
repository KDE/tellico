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

#include "gcstarpluginfetcher.h"
#include "messagehandler.h"
#include "fetchmanager.h"
#include "../collection.h"
#include "../entry.h"
#include "../translators/tellicoimporter.h"
#include "../gui/combobox.h"
#include "../gui/collectiontypecombo.h"
#include "../filehandler.h"
#include "../tellico_kernel.h"
#include "../tellico_debug.h"
#include "../latin1literal.h"
#include "../tellico_utils.h"

#include <kconfig.h>
#include <kprocess.h>
#include <kprocio.h>
#include <kstandarddirs.h>
#include <kaccelmanager.h>

#include <qdir.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qwhatsthis.h>

using Tellico::Fetch::GCstarPluginFetcher;

GCstarPluginFetcher::PluginMap GCstarPluginFetcher::pluginMap;
GCstarPluginFetcher::PluginParse GCstarPluginFetcher::pluginParse = NotYet;

//static
GCstarPluginFetcher::PluginList GCstarPluginFetcher::plugins(int collType_) {
  if(!pluginMap.contains(collType_)) {
    GUI::CursorSaver cs;
    QString gcstar = KStandardDirs::findExe(QString::fromLatin1("gcstar"));

    if(pluginParse == NotYet) {
      KProcIO proc;
      proc << gcstar << QString::fromLatin1("--version");
      // wait 5 seconds at most, just a sanity thing, never want to block completely
      if(proc.start(KProcess::Block) && proc.wait(5)) {
        QString output;
        proc.readln(output);
        if(!output.isEmpty()) {
          // always going to be x.y[.z] ?
          QRegExp versionRx(QString::fromLatin1("(\\d+)\\.(\\d+)(?:\\.(\\d+))?"));
          if(versionRx.search(output) > -1) {
            int x = versionRx.cap(1).toInt();
            int y = versionRx.cap(2).toInt();
            int z = versionRx.cap(3).toInt(); // ok to be empty
            myDebug() << QString::fromLatin1("GCstarPluginFetcher() - found %1.%2.%3").arg(x).arg(y).arg(z) << endl;
            // --list-plugins argument was added for 1.3 release
            pluginParse = (x >= 1 && y >=3) ? New : Old;
          }
        }
      }
      // if still zero, then we should use old in future
      if(pluginParse == NotYet) {
        pluginParse = Old;
      }
    }

    if(pluginParse == New) {
      readPluginsNew(collType_, gcstar);
    } else {
      readPluginsOld(collType_, gcstar);
    }
  }

  return pluginMap.contains(collType_) ? pluginMap[collType_] : GCstarPluginFetcher::PluginList();
}

void GCstarPluginFetcher::readPluginsNew(int collType_, const QString& gcstar_) {
  PluginList plugins;

  QString gcstarCollection = gcstarType(collType_);
  if(gcstarCollection.isEmpty()) {
    pluginMap.insert(collType_, plugins);
    return;
  }

  KProcIO proc;
  proc << gcstar_
        << QString::fromLatin1("-x")
        << QString::fromLatin1("--list-plugins")
        << QString::fromLatin1("--collection") << gcstarCollection;

  if(!proc.start(KProcess::Block)) {
    myWarning() << "GCstarPluginFetcher::readPluginsNew() - can't start" << endl;
    return;
  }

  bool hasName = false;
  PluginInfo info;
  QString line;
  for(int length = 0; length > -1; length = proc.readln(line)) {
    if(line.isEmpty()) {
      if(hasName) {
        plugins << info;
      }
      hasName = false;
      info.clear();
    } else {
      // authors have \t at beginning
      line = line.stripWhiteSpace();
      if(!hasName) {
        info.insert(QString::fromLatin1("name"), line);
        hasName = true;
      } else {
        info.insert(QString::fromLatin1("author"), line);
      }
//      myDebug() << line << endl;
    }
  }

  pluginMap.insert(collType_, plugins);
}

void GCstarPluginFetcher::readPluginsOld(int collType_, const QString& gcstar_) {
  QDir dir(gcstar_, QString::fromLatin1("GC*.pm"));
  dir.cd(QString::fromLatin1("../../lib/gcstar/GCPlugins/"));

  QRegExp rx(QString::fromLatin1("get(Name|Author|Lang)\\s*\\{\\s*return\\s+['\"](.+)['\"]"));
  rx.setMinimal(true);

  PluginList plugins;

  QString dirName = gcstarType(collType_);
  if(dirName.isEmpty()) {
    pluginMap.insert(collType_, plugins);
    return;
  }

  QStringList files = dir.entryList();
  for(QStringList::ConstIterator file = files.begin(); file != files.end(); ++file) {
    KURL u;
    u.setPath(dir.filePath(*file));
    PluginInfo info;
    QString text = FileHandler::readTextFile(u);
    for(int pos = rx.search(text);
        pos > -1;
        pos = rx.search(text, pos+rx.matchedLength())) {
      info.insert(rx.cap(1).lower(), rx.cap(2));
    }
    // only add if it has a name
    if(info.contains(QString::fromLatin1("name"))) {
      plugins << info;
    }
  }
  // inserting empty map is ok
  pluginMap.insert(collType_, plugins);
}

QString GCstarPluginFetcher::gcstarType(int collType_) {
  switch(collType_) {
    case Data::Collection::Book:      return QString::fromLatin1("GCbooks");
    case Data::Collection::Video:     return QString::fromLatin1("GCfilms");
    case Data::Collection::Game:      return QString::fromLatin1("GCgames");
    case Data::Collection::Album:     return QString::fromLatin1("GCmusics");
    case Data::Collection::Coin:      return QString::fromLatin1("GCcoins");
    case Data::Collection::Wine:      return QString::fromLatin1("GCwines");
    case Data::Collection::BoardGame: return QString::fromLatin1("GCboardgames");
    default: break;
  }
  return QString();
}

GCstarPluginFetcher::GCstarPluginFetcher(QObject* parent_, const char* name_/*=0*/) : Fetcher(parent_, name_),
    m_started(false), m_collType(-1), m_process(0) {
}

GCstarPluginFetcher::~GCstarPluginFetcher() {
  stop();
}

QString GCstarPluginFetcher::defaultName() {
  return i18n("GCstar Plugin");
}

QString GCstarPluginFetcher::source() const {
  return m_name;
}

bool GCstarPluginFetcher::canFetch(int type_) const {
  return m_collType == -1 ? false : m_collType == type_;
}

void GCstarPluginFetcher::readConfigHook(const KConfigGroup& config_) {
  m_collType = config_.readNumEntry("CollectionType", -1);
  m_plugin = config_.readEntry("Plugin");
}

void GCstarPluginFetcher::search(FetchKey key_, const QString& value_) {
  m_started = true;
  m_data.truncate(0);

  if(key_ != Fetch::Title) {
    myDebug() << "GCstarPluginFetcher::search() - only Title searches are supported" << endl;
    stop();
    return;
  }

  QString gcstar = KStandardDirs::findExe(QString::fromLatin1("gcstar"));
  if(gcstar.isEmpty()) {
    myWarning() << "GCstarPluginFetcher::search() - gcstar not found!" << endl;
    stop();
    return;
  }

  QString gcstarCollection = gcstarType(m_collType);

  if(m_plugin.isEmpty()) {
    myWarning() << "GCstarPluginFetcher::search() - no plugin name! " << endl;
    stop();
    return;
  }

  m_process = new KProcess();
  connect(m_process, SIGNAL(receivedStdout(KProcess*, char*, int)), SLOT(slotData(KProcess*, char*, int)));
  connect(m_process, SIGNAL(receivedStderr(KProcess*, char*, int)), SLOT(slotError(KProcess*, char*, int)));
  connect(m_process, SIGNAL(processExited(KProcess*)), SLOT(slotProcessExited(KProcess*)));
  QStringList args;
  args << gcstar << QString::fromLatin1("-x")
       << QString::fromLatin1("--collection") << gcstarCollection
       << QString::fromLatin1("--export")     << QString::fromLatin1("Tellico")
       << QString::fromLatin1("--website")    << m_plugin
       << QString::fromLatin1("--download")   << KProcess::quote(value_);
  myLog() << "GCstarPluginFetcher::search() - " << args.join(QChar(' ')) << endl;
  *m_process << args;
  if(!m_process->start(KProcess::NotifyOnExit, KProcess::AllOutput)) {
    myDebug() << "GCstarPluginFetcher::startSearch() - process failed to start" << endl;
    stop();
  }
}

void GCstarPluginFetcher::stop() {
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

void GCstarPluginFetcher::slotData(KProcess*, char* buffer_, int len_) {
  QDataStream stream(m_data, IO_WriteOnly | IO_Append);
  stream.writeRawBytes(buffer_, len_);
}

void GCstarPluginFetcher::slotError(KProcess*, char* buffer_, int len_) {
  QString msg = QString::fromLocal8Bit(buffer_, len_);
  msg.prepend(source() + QString::fromLatin1(": "));
  myDebug() << "GCstarPluginFetcher::slotError() - " << msg << endl;
  m_errors << msg;
}

void GCstarPluginFetcher::slotProcessExited(KProcess*) {
//  myDebug() << "GCstarPluginFetcher::slotProcessExited()" << endl;
  if(!m_process->normalExit() || m_process->exitStatus()) {
    myDebug() << "GCstarPluginFetcher::slotProcessExited() - "<< source() << ": process did not exit successfully" << endl;
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
    myDebug() << "GCstarPluginFetcher::slotProcessExited() - "<< source() << ": no data" << endl;
    stop();
    return;
  }

  Import::TellicoImporter imp(QString::fromUtf8(m_data, m_data.size()));

  Data::CollPtr coll = imp.collection();
  if(!coll) {
    if(!imp.statusMessage().isEmpty()) {
      message(imp.statusMessage(), MessageHandler::Status);
    }
    myDebug() << "GCstarPluginFetcher::slotProcessExited() - "<< source() << ": no collection pointer" << endl;
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

      case Data::Collection::ComicBook:
        desc = entry->field(QString::fromLatin1("publisher"))
               + QChar('/')
               + entry->field(QString::fromLatin1("pub_year"));
        break;

     case Data::Collection::BoardGame:
       desc = entry->field(QString::fromLatin1("designer"))
              + QChar('/')
              + entry->field(QString::fromLatin1("publisher"))
              + QChar('/')
              + entry->field(QString::fromLatin1("year"));
       break;

      default:
        break;
    }
    SearchResult* r = new SearchResult(this, entry->title(), desc, entry->field(QString::fromLatin1("isbn")));
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
  }
  stop(); // be sure to call this
}

Tellico::Data::EntryPtr GCstarPluginFetcher::fetchEntry(uint uid_) {
  return m_entries[uid_];
}

void GCstarPluginFetcher::updateEntry(Data::EntryPtr entry_) {
  // ry searching for title and rely on Collection::sameEntry() to figure things out
  QString t = entry_->field(QString::fromLatin1("title"));
  if(!t.isEmpty()) {
    search(Fetch::Title, t);
    return;
  }

  myDebug() << "GCstarPluginFetcher::updateEntry() - insufficient info to search" << endl;
  emit signalDone(this); // always need to emit this if not continuing with the search
}

Tellico::Fetch::ConfigWidget* GCstarPluginFetcher::configWidget(QWidget* parent_) const {
  return new GCstarPluginFetcher::ConfigWidget(parent_, this);
}

GCstarPluginFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const GCstarPluginFetcher* fetcher_/*=0*/)
    : Fetch::ConfigWidget(parent_), m_needPluginList(true) {
  QGridLayout* l = new QGridLayout(optionsWidget(), 3, 4);
  l->setSpacing(4);
  l->setColStretch(1, 10);

  int row = -1;

  QLabel* label = new QLabel(i18n("Collection &type:"), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_collCombo = new GUI::CollectionTypeCombo(optionsWidget());
  connect(m_collCombo, SIGNAL(activated(int)), SLOT(slotSetModified()));
  connect(m_collCombo, SIGNAL(activated(int)), SLOT(slotTypeChanged()));
  l->addMultiCellWidget(m_collCombo, row, row, 1, 3);
  QString w = i18n("Set the collection type of the data returned from the plugin.");
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_collCombo, w);
  label->setBuddy(m_collCombo);

  label = new QLabel(i18n("&Plugin: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_pluginCombo = new GUI::ComboBox(optionsWidget());
  connect(m_pluginCombo, SIGNAL(activated(int)), SLOT(slotSetModified()));
  connect(m_pluginCombo, SIGNAL(activated(int)), SLOT(slotPluginChanged()));
  l->addMultiCellWidget(m_pluginCombo, row, row, 1, 3);
  w = i18n("Select the GCstar plugin used for the data source.");
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_pluginCombo, w);
  label->setBuddy(m_pluginCombo);

  label = new QLabel(i18n("Author: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_authorLabel = new QLabel(optionsWidget());
  l->addWidget(m_authorLabel, row, 1);

//  label = new QLabel(i18n("Language: "), optionsWidget());
//  l->addWidget(label, row, 2);
//  m_langLabel = new QLabel(optionsWidget());
//  l->addWidget(m_langLabel, row, 3);

  if(fetcher_ && fetcher_->m_collType > -1) {
    m_collCombo->setCurrentType(fetcher_->m_collType);
  } else {
    m_collCombo->setCurrentType(Kernel::self()->collectionType());
  }

  if(fetcher_) {
    m_originalPluginName = fetcher_->m_plugin;
  }

  KAcceleratorManager::manage(optionsWidget());
}

GCstarPluginFetcher::ConfigWidget::~ConfigWidget() {
}

void GCstarPluginFetcher::ConfigWidget::saveConfig(KConfigGroup& config_) {
  config_.writeEntry("CollectionType", m_collCombo->currentType());
  config_.writeEntry("Plugin", m_pluginCombo->currentText());
}

QString GCstarPluginFetcher::ConfigWidget::preferredName() const {
  return QString::fromLatin1("GCstar - ") + m_pluginCombo->currentText();
}

void GCstarPluginFetcher::ConfigWidget::slotTypeChanged() {
  int collType = m_collCombo->currentType();
  m_pluginCombo->clear();
  QStringList pluginNames;
  GCstarPluginFetcher::PluginList list = GCstarPluginFetcher::plugins(collType);
  for(GCstarPluginFetcher::PluginList::ConstIterator it = list.begin(); it != list.end(); ++it) {
    pluginNames << (*it)[QString::fromLatin1("name")].toString();
    m_pluginCombo->insertItem(pluginNames.last(), *it);
  }
  slotPluginChanged();
  emit signalName(preferredName());
}

void GCstarPluginFetcher::ConfigWidget::slotPluginChanged() {
  PluginInfo info = m_pluginCombo->currentData().toMap();
  m_authorLabel->setText(info[QString::fromLatin1("author")].toString());
//  m_langLabel->setText(info[QString::fromLatin1("lang")].toString());
  emit signalName(preferredName());
}

void GCstarPluginFetcher::ConfigWidget::showEvent(QShowEvent*) {
  if(m_needPluginList) {
    m_needPluginList = false;
    slotTypeChanged(); // update plugin combo box
    if(!m_originalPluginName.isEmpty()) {
      m_pluginCombo->setCurrentText(m_originalPluginName);
      slotPluginChanged();
    }
  }
}

#include "gcstarpluginfetcher.moc"
