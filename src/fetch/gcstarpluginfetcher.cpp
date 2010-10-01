/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
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

#include "gcstarpluginfetcher.h"
#include "gcstarthread.cpp"
#include "fetchmanager.h"
#include "../collection.h"
#include "../entry.h"
#include "../translators/gcstarimporter.h"
#include "../gui/combobox.h"
#include "../gui/collectiontypecombo.h"
#include "../gui/cursorsaver.h"
#include "../core/filehandler.h"
#include "../gui/guiproxy.h"
#include "../tellico_debug.h"

#include <KConfigGroup>
#include <KProcess>
#include <kstandarddirs.h>
#include <kacceleratormanager.h>
#include <kshell.h>
#include <KFilterDev>
#include <KTar>
#include <KTempDir>

#include <QDir>
#include <QLabel>
#include <QShowEvent>
#include <QGridLayout>
#include <QBuffer>

#include <memory>

using namespace Tellico;
using Tellico::Fetch::GCstarPluginFetcher;

GCstarPluginFetcher::CollectionPlugins GCstarPluginFetcher::collectionPlugins;
GCstarPluginFetcher::PluginParse GCstarPluginFetcher::pluginParse = NotYet;

//static
GCstarPluginFetcher::PluginList GCstarPluginFetcher::plugins(int collType_) {
  if(!collectionPlugins.contains(collType_)) {
    GUI::CursorSaver cs;
    QString gcstar = KStandardDirs::findExe(QLatin1String("gcstar"));

    if(pluginParse == NotYet) {
      KProcess proc;
      proc.setProgram(gcstar, QStringList() << QLatin1String("--version"));
      proc.setOutputChannelMode(KProcess::OnlyStdoutChannel);
      // wait 5 seconds at most, just a sanity thing, never want to block completely
      if(proc.execute(5000) > -1) {
        QString output = QString::fromLocal8Bit(proc.readAllStandardOutput());
        if(!output.isEmpty()) {
          // always going to be x.y[.z] ?
          QRegExp versionRx(QLatin1String("(\\d+)\\.(\\d+)(?:\\.(\\d+))?"));
          if(versionRx.indexIn(output) > -1) {
            int x = versionRx.cap(1).toInt();
            int y = versionRx.cap(2).toInt();
            int z = versionRx.cap(3).toInt(); // ok to be empty
            myDebug() << QString::fromLatin1("found %1.%2.%3").arg(x).arg(y).arg(z);
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

  return collectionPlugins.contains(collType_) ? collectionPlugins.value(collType_) : GCstarPluginFetcher::PluginList();
}

void GCstarPluginFetcher::readPluginsNew(int collType_, const QString& gcstar_) {
  PluginList plugins;

  const QString gcstarCollection = gcstarType(collType_);
  if(gcstarCollection.isEmpty()) {
    collectionPlugins.insert(collType_, plugins);
    return;
  }

  QStringList args;
  args << QLatin1String("--execute")
       << QLatin1String("--list-plugins")
       << QLatin1String("--collection")
       << gcstarCollection;

  KProcess proc;
  proc.setProgram(gcstar_, args);
  proc.setOutputChannelMode(KProcess::OnlyStdoutChannel);
  if(proc.execute() < 0) {
    myWarning() << "can't start";
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
//      myDebug() << line;
    }
  }

  collectionPlugins.insert(collType_, plugins);
}

void GCstarPluginFetcher::readPluginsOld(int collType_, const QString& gcstar_) {
  QDir dir(gcstar_, QLatin1String("GC*.pm"));
  dir.cd(QLatin1String("../../lib/gcstar/GCPlugins/"));

  QRegExp rx(QLatin1String("get(Name|Author|Lang)\\s*\\{\\s*return\\s+['\"](.+)['\"]"));
  rx.setMinimal(true);

  PluginList plugins;

  const QString dirName = gcstarType(collType_);
  if(dirName.isEmpty()) {
    collectionPlugins.insert(collType_, plugins);
    return;
  }

  foreach(const QString& file, dir.entryList()) {
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
  // inserting empty list is ok
  collectionPlugins.insert(collType_, plugins);
}

QString GCstarPluginFetcher::gcstarType(int collType_) {
  switch(collType_) {
    case Data::Collection::Book:      return QLatin1String("GCbooks");
    case Data::Collection::Video:     return QLatin1String("GCfilms");
    case Data::Collection::Album:     return QLatin1String("GCmusics");
    case Data::Collection::ComicBook: return QLatin1String("GCcomics");
    case Data::Collection::Wine:      return QLatin1String("GCwines");
    case Data::Collection::Coin:      return QLatin1String("GCcoins");
    case Data::Collection::Stamp:     return QLatin1String("GCstamps");
    case Data::Collection::Game:      return QLatin1String("GCgames");
    case Data::Collection::BoardGame: return QLatin1String("GCboardgames");
    default: break;
  }
  return QString();
}

GCstarPluginFetcher::GCstarPluginFetcher(QObject* parent_) : Fetcher(parent_),
    m_started(false), m_collType(-1), m_thread(0) {
}

GCstarPluginFetcher::~GCstarPluginFetcher() {
  stop();
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

void GCstarPluginFetcher::search() {
  m_started = true;
  if(m_plugin.isEmpty() || m_collType == -1) {
    myWarning() << "no plugin information!";
    myDebug() << m_collType << m_plugin;
    stop();
    return;
  }

  m_data.clear();

  const QString gcstar = KStandardDirs::findExe(QLatin1String("gcstar"));
  if(gcstar.isEmpty()) {
    myWarning() << "gcstar not found!";
    stop();
    return;
  }

  QStringList args;
  args << QLatin1String("--execute")
       << QLatin1String("--collection")  << gcstarType(m_collType)
       << QLatin1String("--export")      << QLatin1String("TarGz")
       << QLatin1String("--exportprefs") << QLatin1String("collection=>/tmp/test.gcs,file=>/tmp/test1.tar.gz")
       << QLatin1String("--website")     << m_plugin
       << QLatin1String("--download")    << KShell::quoteArg(request().value);
  myLog() << args;

  m_thread = new GCstarThread(this);
  m_thread->setProgram(gcstar, args);
  connect(m_thread, SIGNAL(standardOutput(const QByteArray&)), SLOT(slotData(const QByteArray&)));
  connect(m_thread, SIGNAL(standardError(const QByteArray&)), SLOT(slotError(const QByteArray&)));
  connect(m_thread, SIGNAL(finished()), SLOT(slotProcessExited()));
  m_thread->start();
}

void GCstarPluginFetcher::stop() {
  if(!m_started) {
    return;
  }
  if(m_thread) {
    if(m_thread->isRunning()) {
      m_thread->terminate();
      m_thread->wait();
    }
    delete m_thread;
    m_thread = 0;
  }
  m_data.clear();
  m_started = false;
  m_errors.clear();
  emit signalDone(this);
}

void GCstarPluginFetcher::slotData(const QByteArray& data_) {
  m_data.append(data_);
}

void GCstarPluginFetcher::slotError(const QByteArray& data_) {
  QString msg = QString::fromLocal8Bit(data_);
  msg.prepend(source() + QLatin1String(": "));
  myDebug() << msg;
  m_errors << msg;
}

void GCstarPluginFetcher::slotProcessExited() {
  // if stop() is called and the thread terminated
  // the finished() signal will still fire
  if(!m_started) {
    return;
  }

  if(!m_errors.isEmpty()) {
    message(m_errors.join(QLatin1String("\n")), MessageHandler::Warning);
  }

  if(m_data.isEmpty()) {
    myDebug() << source() << ": no data";
    stop();
    return;
  }

  QBuffer filterBuffer(&m_data);
  std::auto_ptr<QIODevice> filter(KFilterDev::device(&filterBuffer, QLatin1String("application/x-gzip"), false));
  if(!filter->open(QIODevice::ReadOnly)) {
    myWarning() << "unable to open gzip filter";
    stop();
    return;
  }

  QByteArray tarData = filter->readAll();
  QBuffer buffer(&tarData);

  KTar tar(&buffer);
  if(!tar.open(QIODevice::ReadOnly)) {
    myWarning() << "unable to open tar file";
    stop();
    return;
  }

  const KArchiveDirectory* dir = tar.directory();
  if(!dir) {
    myWarning() << "unable to open tar directory";
    stop();
    return;
  }

  KTempDir tempDir;
  dir->copyTo(tempDir.name());

  // KDE seems to have abug (#252821) for gcstar files where the images are not in the images/ directory
  foreach(const QString& filename, dir->entries()) {
    if(dir->entry(filename)->isFile() && filename != QLatin1String("collection.gcs")) {
      const KArchiveFile* f = static_cast<const KArchiveFile*>(dir->entry(filename));
      f->copyTo(tempDir.name() + QLatin1String("images"));
    }
  }

  KUrl gcsUrl(tempDir.name());
  gcsUrl.addPath(QLatin1String("collection.gcs"));

  Import::GCstarImporter imp(gcsUrl);
  imp.setHasRelativeImageLinks(true);

  Data::CollPtr coll = imp.collection();
  if(!coll) {
    if(!imp.statusMessage().isEmpty()) {
      message(imp.statusMessage(), MessageHandler::Status);
    }
    myWarning() << "no collection pointer";
    stop();
    return;
  }

  foreach(Data::EntryPtr entry, coll->entries()) {
    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
    if(!m_started) {
      return;
    }
  }
  stop(); // be sure to call this
}

Tellico::Data::EntryPtr GCstarPluginFetcher::fetchEntryHook(uint uid_) {
  return m_entries[uid_];
}

Tellico::Fetch::FetchRequest GCstarPluginFetcher::updateRequest(Data::EntryPtr entry_) {
  // ry searching for title and rely on Collection::sameEntry() to figure things out
  QString t = entry_->field(QLatin1String("title"));
  if(!t.isEmpty()) {
    return FetchRequest(Fetch::Title, t);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* GCstarPluginFetcher::configWidget(QWidget* parent_) const {
  return new GCstarPluginFetcher::ConfigWidget(parent_, this);
}

QString GCstarPluginFetcher::defaultName() {
  return i18n("GCstar Plugin");
}

QString GCstarPluginFetcher::defaultIcon() {
  return QLatin1String("gcstar");
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

  if(fetcher_) {
    if(fetcher_->m_collType > -1) {
      m_collCombo->setCurrentType(fetcher_->m_collType);
    } else {
      m_collCombo->setCurrentType(fetcher_->collectionType());
    }
    m_originalPluginName = fetcher_->m_plugin;
  } else {
    // default to Book for now
    m_collCombo->setCurrentType(Data::Collection::Book);
  }

  KAcceleratorManager::manage(optionsWidget());
}

GCstarPluginFetcher::ConfigWidget::~ConfigWidget() {
}

void GCstarPluginFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  config_.writeEntry("CollectionType", m_collCombo->currentType());
  config_.writeEntry("Plugin", m_pluginCombo->currentText());
}

QString GCstarPluginFetcher::ConfigWidget::preferredName() const {
  QString plugin = m_pluginCombo->currentText();
  return plugin.isEmpty() ? plugin : QLatin1String("GCstar - ") + plugin;
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
  PluginInfo info = m_pluginCombo->currentData().toHash();
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
