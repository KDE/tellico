/***************************************************************************
    copyright            : (C) 2005-2008 by Robby Stephenson
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
#include "searchresult.h"
#include "fetchmanager.h"
#include "../collection.h"
#include "../entry.h"
#include "../translators/tellicoimporter.h"
#include "../gui/combobox.h"
#include "../gui/collectiontypecombo.h"
#include "../gui/cursorsaver.h"
#include "../core/filehandler.h"
#include "../tellico_kernel.h"
#include "../tellico_debug.h"

#include <KConfigGroup>
#include <KProcess>
#include <kstandarddirs.h>
#include <kacceleratormanager.h>
#include <kshell.h>

#include <QDir>
#include <QLabel>
#include <QShowEvent>
#include <QGridLayout>

using Tellico::Fetch::GCstarPluginFetcher;

GCstarPluginFetcher::PluginMap GCstarPluginFetcher::pluginMap;
GCstarPluginFetcher::PluginParse GCstarPluginFetcher::pluginParse = NotYet;

//static
GCstarPluginFetcher::PluginList GCstarPluginFetcher::plugins(int collType_) {
  if(!pluginMap.contains(collType_)) {
    GUI::CursorSaver cs;
    QString gcstar = KStandardDirs::findExe(QLatin1String("gcstar"));

    if(pluginParse == NotYet) {
      KProcess proc;
      QStringList args;
      args << QLatin1String("--version");
      // wait 5 seconds at most, just a sanity thing, never want to block completely
      if(proc.execute(gcstar, args, 5000) > -1) {
        QString output = QString::fromLocal8Bit(proc.readAllStandardOutput());
        if(!output.isEmpty()) {
          // always going to be x.y[.z] ?
          QRegExp versionRx(QLatin1String("(\\d+)\\.(\\d+)(?:\\.(\\d+))?"));
          if(versionRx.indexIn(output) > -1) {
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

  QStringList args;
  args << QLatin1String("-x")
       << QLatin1String("--list-plugins")
       << QLatin1String("--collection")
       << gcstarCollection;

  KProcess proc;
  if(proc.execute(gcstar_, args) < 0) {
    myWarning() << "GCstarPluginFetcher::readPluginsNew() - can't start" << endl;
    return;
  }

  bool hasName = false;
  PluginInfo info;
  QTextStream stream(&proc);
  for(QString line = stream.readLine(); !stream.atEnd(); line = stream.readLine()) {
    if(line.isEmpty()) {
      if(hasName) {
        plugins << info;
      }
      hasName = false;
      info.clear();
    } else {
      // authors have \t at beginning
      line = line.trimmed();
      if(!hasName) {
        info.insert(QLatin1String("name"), line);
        hasName = true;
      } else {
        info.insert(QLatin1String("author"), line);
      }
//      myDebug() << line << endl;
    }
  }

  pluginMap.insert(collType_, plugins);
}

void GCstarPluginFetcher::readPluginsOld(int collType_, const QString& gcstar_) {
  QDir dir(gcstar_, QLatin1String("GC*.pm"));
  dir.cd(QLatin1String("../../lib/gcstar/GCPlugins/"));

  QRegExp rx(QLatin1String("get(Name|Author|Lang)\\s*\\{\\s*return\\s+['\"](.+)['\"]"));
  rx.setMinimal(true);

  PluginList plugins;

  QString dirName = gcstarType(collType_);
  if(dirName.isEmpty()) {
    pluginMap.insert(collType_, plugins);
    return;
  }

  QStringList files = dir.entryList();
  foreach(const QString& file, files) {
    KUrl u;
    u.setPath(dir.filePath(file));
    PluginInfo info;
    QString text = FileHandler::readTextFile(u);
    for(int pos = rx.indexIn(text); pos > -1; pos = rx.indexIn(text, pos+rx.matchedLength())) {
      info.insert(rx.cap(1).toLower(), rx.cap(2));
    }
    // only add if it has a name
    if(info.contains(QLatin1String("name"))) {
      plugins << info;
    }
  }
  // inserting empty map is ok
  pluginMap.insert(collType_, plugins);
}

QString GCstarPluginFetcher::gcstarType(int collType_) {
  switch(collType_) {
    case Data::Collection::Book:      return QLatin1String("GCbooks");
    case Data::Collection::Video:     return QLatin1String("GCfilms");
    case Data::Collection::Game:      return QLatin1String("GCgames");
    case Data::Collection::Album:     return QLatin1String("GCmusics");
    case Data::Collection::Coin:      return QLatin1String("GCcoins");
    case Data::Collection::Wine:      return QLatin1String("GCwines");
    case Data::Collection::BoardGame: return QLatin1String("GCboardgames");
    default: break;
  }
  return QString();
}

GCstarPluginFetcher::GCstarPluginFetcher(QObject* parent_) : Fetcher(parent_),
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
  m_collType = config_.readEntry("CollectionType", -1);
  m_plugin = config_.readEntry("Plugin");
}

void GCstarPluginFetcher::search(Tellico::Fetch::FetchKey key_, const QString& value_) {
  m_started = true;
  m_data.clear();

  if(key_ != Fetch::Title) {
    myDebug() << "GCstarPluginFetcher::search() - only Title searches are supported" << endl;
    stop();
    return;
  }

  QString gcstar = KStandardDirs::findExe(QLatin1String("gcstar"));
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

  m_process = new KProcess(this);
  connect(m_process, SIGNAL(readyReadStandardOutput()), SLOT(slotData()));
  connect(m_process, SIGNAL(readyReadStandardError()), SLOT(slotError()));
  connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)), SLOT(slotProcessExited()));
  m_process->setOutputChannelMode(KProcess::SeparateChannels);
  QStringList args;
  args << QLatin1String("-x")
       << QLatin1String("--collection") << gcstarCollection
       << QLatin1String("--export")     << QLatin1String("Tellico")
       << QLatin1String("--website")    << m_plugin
       << QLatin1String("--download")   << KShell::quoteArg(value_);
  myLog() << "GCstarPluginFetcher::search() - " << args.join(QLatin1String(" ")) << endl;
  m_process->setProgram(gcstar, args);
  if(!m_process->execute()) {
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
  m_data.clear();
  m_started = false;
  m_errors.clear();
  emit signalDone(this);
}

void GCstarPluginFetcher::slotData() {
  m_data.append(m_process->readAllStandardOutput());
}

void GCstarPluginFetcher::slotError() {
  QString msg = QString::fromLocal8Bit(m_process->readAllStandardError());
  msg.prepend(source() + QLatin1String(": "));
  myDebug() << "GCstarPluginFetcher::slotError() - " << msg << endl;
  m_errors << msg;
}

void GCstarPluginFetcher::slotProcessExited() {
//  myDebug() << "GCstarPluginFetcher::slotProcessExited()" << endl;
  if(m_process->exitStatus() != QProcess::NormalExit || m_process->exitCode() != 0) {
    myDebug() << "GCstarPluginFetcher::slotProcessExited() - "<< source() << ": process did not exit successfully" << endl;
    if(!m_errors.isEmpty()) {
      message(m_errors.join(QLatin1String("\n")), MessageHandler::Error);
    }
    stop();
    return;
  }
  if(!m_errors.isEmpty()) {
    message(m_errors.join(QLatin1String("\n")), MessageHandler::Warning);
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

  Data::EntryList entries = coll->entries();
  foreach(Data::EntryPtr entry, entries) {
    SearchResult* r = new SearchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
  }
  stop(); // be sure to call this
}

Tellico::Data::EntryPtr GCstarPluginFetcher::fetchEntry(uint uid_) {
  return m_entries[uid_];
}

void GCstarPluginFetcher::updateEntry(Tellico::Data::EntryPtr entry_) {
  // ry searching for title and rely on Collection::sameEntry() to figure things out
  QString t = entry_->field(QLatin1String("title"));
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
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;

  QLabel* label = new QLabel(i18n("Collection &type:"), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_collCombo = new GUI::CollectionTypeCombo(optionsWidget());
  connect(m_collCombo, SIGNAL(activated(int)), SLOT(slotSetModified()));
  connect(m_collCombo, SIGNAL(activated(int)), SLOT(slotTypeChanged()));
  l->addWidget(m_collCombo, row, 1, 1, 3);
  QString w = i18n("Set the collection type of the data returned from the plugin.");
  label->setWhatsThis(w);
  m_collCombo->setWhatsThis(w);
  label->setBuddy(m_collCombo);

  label = new QLabel(i18n("&Plugin: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_pluginCombo = new GUI::ComboBox(optionsWidget());
  connect(m_pluginCombo, SIGNAL(activated(int)), SLOT(slotSetModified()));
  connect(m_pluginCombo, SIGNAL(activated(int)), SLOT(slotPluginChanged()));
  l->addWidget(m_pluginCombo, row, 1, 1, 3);
  w = i18n("Select the GCstar plugin used for the data source.");
  label->setWhatsThis(w);
  m_pluginCombo->setWhatsThis(w);
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
  return QLatin1String("GCstar - ") + m_pluginCombo->currentText();
}

void GCstarPluginFetcher::ConfigWidget::slotTypeChanged() {
  int collType = m_collCombo->currentType();
  m_pluginCombo->clear();
  QStringList pluginNames;
  GCstarPluginFetcher::PluginList list = GCstarPluginFetcher::plugins(collType);
  foreach(const GCstarPluginFetcher::PluginInfo& info, list) {
    pluginNames << info.value(QLatin1String("name")).toString();
    m_pluginCombo->addItem(pluginNames.last(), info);
  }
  slotPluginChanged();
  emit signalName(preferredName());
}

void GCstarPluginFetcher::ConfigWidget::slotPluginChanged() {
  PluginInfo info = m_pluginCombo->currentData().toMap();
  m_authorLabel->setText(info[QLatin1String("author")].toString());
//  m_langLabel->setText(info[QLatin1String("lang")].toString());
  emit signalName(preferredName());
}

void GCstarPluginFetcher::ConfigWidget::showEvent(QShowEvent*) {
  if(m_needPluginList) {
    m_needPluginList = false;
    slotTypeChanged(); // update plugin combo box
    if(!m_originalPluginName.isEmpty()) {
      m_pluginCombo->setEditText(m_originalPluginName);
      slotPluginChanged();
    }
  }
}

#include "gcstarpluginfetcher.moc"
