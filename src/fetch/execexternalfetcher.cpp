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

#include "execexternalfetcher.h"
#include "fetchmanager.h"
#include "../collection.h"
#include "../entry.h"
#include "../fieldformat.h"
#include "../derivedvalue.h"
#include "../tellico_debug.h"
#include "../gui/combobox.h"
#include "../gui/lineedit.h"
#include "../gui/collectiontypecombo.h"
#include "../utils/cursorsaver.h"
#include "../newstuff/manager.h"
#include "../translators/translators.h"
#include "../translators/tellicoimporter.h"
#include "../translators/bibteximporter.h"
#include "../translators/xsltimporter.h"
#include "../translators/risimporter.h"
#include "../utils/datafileregistry.h"

#include <KLocalizedString>
#include <KProcess>
#include <KUrlRequester>
#include <KAcceleratorManager>
#include <KShell>
#include <KConfigGroup>

#include <QLabel>
#include <QGroupBox>
#include <QGridLayout>
#include <QStandardItemModel>

using Tellico::Fetch::ExecExternalFetcher;

QStringList ExecExternalFetcher::parseArguments(const QString& str_) {
  // matching escaped quotes is too hard... :(
  static const QRegularExpression quotes(QLatin1String("(['\"])(.*?)\\1"));
  static const QRegularExpression spaces(QLatin1String("\\s+?"));

  QStringList args;
  QRegularExpressionMatch match;
  int pos = 0;
  for(int nextPos = str_.indexOf(quotes, pos, &match); nextPos > -1; pos = nextPos+1, nextPos = str_.indexOf(quotes, pos, &match)) {
    // a non-quotes arguments runs from pos to nextPos
#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
    args += str_.mid(pos, nextPos-pos).split(spaces, QString::SkipEmptyParts);
#else
    args += str_.mid(pos, nextPos-pos).split(spaces, Qt::SkipEmptyParts);
#endif
    // move nextpos marker to end of match
    nextPos += match.capturedLength();
    args += match.captured(2);
  }
  // catch the end stuff
#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
  args += str_.mid(pos).split(spaces, QString::SkipEmptyParts);
#else
  args += str_.mid(pos).split(spaces, Qt::SkipEmptyParts);
#endif

  return args;
}

ExecExternalFetcher::ExecExternalFetcher(QObject* parent_) : Fetcher(parent_),
    m_started(false), m_collType(-1), m_formatType(-1), m_canUpdate(false), m_process(nullptr), m_deleteOnRemove(false) {
}

ExecExternalFetcher::~ExecExternalFetcher() {
  if(m_process) {
    m_process->kill();
    m_process->deleteLater();
  }
}

QString ExecExternalFetcher::source() const {
  return m_name;
}

bool ExecExternalFetcher::canSearch(Fetch::FetchKey k) const {
  return m_args.contains(k) || (m_canUpdate && k == ExecUpdate);
}

bool ExecExternalFetcher::canFetch(int type_) const {
  return m_collType == -1 ? false : m_collType == type_;
}

void ExecExternalFetcher::readConfigHook(const KConfigGroup& config_) {
  QString s = config_.readPathEntry("ExecPath", QString());
  if(!s.isEmpty()) {
    m_path = s;
  }
  QList<int> argKeys;
  if(config_.hasKey("ArgumentKeys")) {
    argKeys = config_.readEntry("ArgumentKeys", argKeys);
  } else {
    myDebug() << "appending default keyword argument";
    argKeys.append(Keyword);
  }
  QStringList args = config_.readEntry("Arguments", QStringList());
  if(argKeys.count() != args.count()) {
    myWarning() << "unequal number of arguments and keys";
  }
  int n = qMin(argKeys.count(), args.count());
  for(int i = 0; i < n; ++i) {
    m_args.insert(static_cast<FetchKey>(argKeys[i]), args[i]);
  }
  if(config_.hasKey("UpdateArgs")) {
    m_canUpdate = true;
    m_updateArgs = config_.readEntry("UpdateArgs");
  } else {
    m_canUpdate = false;
  }
  m_collType = config_.readEntry("CollectionType", -1);
  m_formatType = config_.readEntry("FormatType", -1);
  m_deleteOnRemove = config_.readEntry("DeleteOnRemove", false);
  m_newStuffName = config_.readEntry("NewStuffName");
}

void ExecExternalFetcher::search() {
  m_started = true;

  if(request().key() != ExecUpdate && !m_args.contains(request().key())) {
    myDebug() << "stopping: not an update and no matching argument for search key";
    stop();
    return;
  }

  if(request().key() == ExecUpdate) {
    // because the rowDelimiterString() is used below
    QStringList args = FieldFormat::splitTable(request().value());
    startSearch(args);
    return;
  }

  // should KShell::quoteArg() be used?
  // %1 gets replaced by the search value, but since the arguments are going to be split
  // the search value needs to be enclosed in quotation marks
  // but first check to make sure the user didn't do that already
  // AND the "%1" wasn't used in the settings
  QString value = request().value();
  if(request().key() == ISBN) {
    value.remove(QLatin1Char('-')); // remove hyphens from isbn values
    // shouldn't hurt and might keep from confusing stupid search sources
  }
  bool hasQuotes = value.startsWith(QLatin1Char('"')) && value.endsWith(QLatin1Char('"'));
  if(!hasQuotes) {
    hasQuotes = value.startsWith(QLatin1Char('\'')) && value.endsWith(QLatin1Char('\''));
  }
  if(!hasQuotes) {
    value = QLatin1Char('"') + value + QLatin1Char('"');
  }
  QString args = m_args.value(request().key());
  static const QRegularExpression rx(QLatin1String("(['\"])%1\\1"));
  args.replace(rx, QStringLiteral("%1"));
  startSearch(parseArguments(args.arg(value))); // replace %1 with search value
}

void ExecExternalFetcher::startSearch(const QStringList& args_) {
  if(m_path.isEmpty()) {
    Q_ASSERT(!m_path.isEmpty());
    stop();
    return;
  }

  m_process = new KProcess();
  connect(m_process, &QProcess::readyReadStandardOutput, this, &ExecExternalFetcher::slotData);
  connect(m_process, &QProcess::readyReadStandardError, this, &ExecExternalFetcher::slotError);
  void (QProcess::* finished)(int, QProcess::ExitStatus) = &QProcess::finished;
  connect(m_process, finished, this, &ExecExternalFetcher::slotProcessExited);
  m_process->setOutputChannelMode(KProcess::SeparateChannels);
  m_process->setProgram(m_path, args_);
  if(m_process && m_process->execute() < 0) {
    myDebug() << "process failed to start";
    stop();
  }
}

void ExecExternalFetcher::stop() {
  if(!m_started) {
    return;
  }
  if(m_process) {
    m_process->kill();
    m_process->deleteLater();
    m_process = nullptr;
  }
  m_data.clear();
  m_started = false;
  m_errors.clear();
  emit signalDone(this);
}

void ExecExternalFetcher::slotData() {
  m_data.append(m_process->readAllStandardOutput());
}

void ExecExternalFetcher::slotError() {
  GUI::CursorSaver cs(Qt::ArrowCursor);
  QString msg = QString::fromLocal8Bit(m_process->readAllStandardError());
  msg.prepend(source() + QLatin1String(": "));
  if(msg.endsWith(QChar::fromLatin1('\n'))) {
    msg.truncate(msg.length()-1);
  }
  myDebug() << msg;
  m_errors << msg;
}

void ExecExternalFetcher::slotProcessExited() {
//  DEBUG_LINE;
  if(m_process->exitStatus() != QProcess::NormalExit || m_process->exitCode() != 0) {
    myDebug() << source() << ": process did not exit successfully";
    if(!m_errors.isEmpty()) {
      message(m_errors.join(QChar::fromLatin1('\n')), MessageHandler::Error);
    }
    stop();
    return;
  }
  if(!m_errors.isEmpty()) {
    message(m_errors.join(QChar::fromLatin1('\n')), MessageHandler::Warning);
  }

  if(m_data.isEmpty()) {
    myDebug() << source() << ": no data";
    stop();
    return;
  }

  const QString text = QString::fromUtf8(m_data.constData(), m_data.size());
#if 0
  myWarning() << "Remove debug from ExecExternalFetcher.cpp";
  QFile f(QStringLiteral("/tmp/test-exec.txt"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << m_data;
  }
  f.close();
#endif
  Import::Format format = static_cast<Import::Format>(m_formatType > -1 ? m_formatType : Import::TellicoXML);
  Import::Importer* imp = nullptr;
  // only 4 formats re supported here
  switch(format) {
    case Import::TellicoXML:
      imp = new Import::TellicoImporter(text);
      break;

    case Import::Bibtex:
      imp = new Import::BibtexImporter(text);
      break;

    case Import::MODS:
      imp = new Import::XSLTImporter(text);
      {
        QString xsltFile = DataFileRegistry::self()->locate(QStringLiteral("mods2tellico.xsl"));
        if(!xsltFile.isEmpty()) {
          QUrl u = QUrl::fromLocalFile(xsltFile);
          static_cast<Import::XSLTImporter*>(imp)->setXSLTURL(u);
        } else {
          myWarning() << "unable to find mods2tellico.xml!";
          delete imp;
          imp = nullptr;
        }
      }
      break;

    case Import::RIS:
      imp = new Import::RISImporter(text);
      break;

    default:
      break;
  }
  if(!imp) {
    stop();
    return;
  }

  Data::CollPtr coll = imp->collection();
  if(!coll) {
    if(!imp->statusMessage().isEmpty()) {
      message(imp->statusMessage(), MessageHandler::Status);
    }
    myDebug() << source() << ": no collection pointer";
    delete imp;
    stop();
    return;
  }

  delete imp;
  if(coll->entryCount() == 0) {
//    myDebug() << "no results";
    stop();
    return;
  }

  Data::EntryList entries = coll->entries();
  foreach(Data::EntryPtr entry, entries) {
    FetchResult* r = new FetchResult(this, entry);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
  }
  stop(); // be sure to call this
}

Tellico::Data::EntryPtr ExecExternalFetcher::fetchEntryHook(uint uid_) {
  return m_entries[uid_];
}

Tellico::Fetch::FetchRequest ExecExternalFetcher::updateRequest(Data::EntryPtr entry_) {
  if(!m_canUpdate) {
    return FetchRequest();
  }

  QStringList args = parseArguments(m_updateArgs);
  for(QStringList::Iterator it = args.begin(); it != args.end(); ++it) {
    Data::DerivedValue dv(*it);
    *it = dv.value(entry_, false);
  }
  return FetchRequest(ExecUpdate, args.join(FieldFormat::rowDelimiterString()));
}

Tellico::Fetch::ConfigWidget* ExecExternalFetcher::configWidget(QWidget* parent_) const {
  return new ExecExternalFetcher::ConfigWidget(parent_, this);
}

QString ExecExternalFetcher::defaultName() {
  return i18n("External Application");
}

QString ExecExternalFetcher::defaultIcon() {
  return QStringLiteral("application-x-executable");
}

ExecExternalFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const ExecExternalFetcher* fetcher_/*=0*/)
    : Fetch::ConfigWidget(parent_), m_deleteOnRemove(false) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;

  QLabel* label = new QLabel(i18n("Collection &type:"), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_collCombo = new GUI::CollectionTypeCombo(optionsWidget());
  void (GUI::ComboBox::* activatedInt)(int) = &GUI::ComboBox::activated;
  connect(m_collCombo, activatedInt, this, &ConfigWidget::slotSetModified);
  l->addWidget(m_collCombo, row, 1);
  QString w = i18n("Set the collection type of the data returned from the external application.");
  label->setWhatsThis(w);
  m_collCombo->setWhatsThis(w);
  label->setBuddy(m_collCombo);

  label = new QLabel(i18n("&Result type: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_formatCombo = new GUI::ComboBox(optionsWidget());
  m_formatCombo->addItem(QStringLiteral("Tellico"), Import::TellicoXML);
  m_formatCombo->addItem(QStringLiteral("Bibtex"), Import::Bibtex);
  m_formatCombo->addItem(QStringLiteral("MODS"), Import::MODS);
  m_formatCombo->addItem(QStringLiteral("RIS"), Import::RIS);
  connect(m_formatCombo, activatedInt, this, &ExecExternalFetcher::ConfigWidget::slotSetModified);
  l->addWidget(m_formatCombo, row, 1);
  w = i18n("Set the result type of the data returned from the external application.");
  label->setWhatsThis(w);
  m_formatCombo->setWhatsThis(w);
  label->setBuddy(m_formatCombo);
#ifndef ENABLE_BTPARSE
  // disable the option for bibtex
  auto formatModel = qobject_cast<const QStandardItemModel*>(m_formatCombo->model());
  auto matchList = formatModel->match(formatModel->index(0, 0), Qt::UserRole, Import::Bibtex);
  if(!matchList.isEmpty()) {
    auto item = formatModel->itemFromIndex(matchList.front());
    item->setEnabled(false);
  }
#endif

  label = new QLabel(i18n("Application &path: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_pathEdit = new KUrlRequester(optionsWidget());
  connect(m_pathEdit, &KUrlRequester::textChanged, this, &ConfigWidget::slotSetModified);
  l->addWidget(m_pathEdit, row, 1);
  w = i18n("Set the path of the application to run that should output a valid Tellico data file.");
  label->setWhatsThis(w);
  m_pathEdit->setWhatsThis(w);
  label->setBuddy(m_pathEdit);

  w = i18n("Select the search keys supported by the data source.");
  // in this string, the %1 is not a placeholder, it's an example
  QString w2 = i18n("Add any arguments that may be needed. <b>%1</b> will be replaced by the search term."); // krazy:exclude=i18ncheckarg
  QGroupBox* gbox = new QGroupBox(i18n("Arguments"), optionsWidget());
  ++row;
  l->addWidget(gbox, row, 0, 1, 2);
  QGridLayout* gridLayout = new QGridLayout(gbox);
  gridLayout->setSpacing(2);
  row = -1;
  const Fetch::KeyMap keyMap = Fetch::Manager::self()->keyMap();
  for(Fetch::KeyMap::ConstIterator it = keyMap.begin(); it != keyMap.end(); ++it) {
    FetchKey key = it.key();
    if(key == Raw) {
      continue;
    }
    QCheckBox* cb = new QCheckBox(it.value(), gbox);
    gridLayout->addWidget(cb, ++row, 0);
    m_cbDict.insert(key, cb);
    GUI::LineEdit* le = new GUI::LineEdit(gbox);
    le->setPlaceholderText(QStringLiteral("%1")); // for example
    le->completionObject()->addItem(QStringLiteral("%1"));
    gridLayout->addWidget(le, row, 1);
    m_leDict.insert(key, le);
    if(fetcher_ && fetcher_->m_args.contains(key)) {
      cb->setChecked(true);
      le->setEnabled(true);
      le->setText(fetcher_->m_args.value(key));
    } else {
      cb->setChecked(false);
      le->setEnabled(false);
    }
    connect(cb, &QAbstractButton::toggled, le, &QWidget::setEnabled);
    cb->setWhatsThis(w);
    le->setWhatsThis(w2);
  }
  m_cbUpdate = new QCheckBox(i18n("Update"), gbox);
  gridLayout->addWidget(m_cbUpdate, ++row, 0);
  m_leUpdate = new GUI::LineEdit(gbox);
  m_leUpdate->setPlaceholderText(QStringLiteral("%{title}")); // for example
  m_leUpdate->completionObject()->addItem(QStringLiteral("%{title}"));
  m_leUpdate->completionObject()->addItem(QStringLiteral("%{isbn}"));
  gridLayout->addWidget(m_leUpdate, row, 1);
  /* TRANSLATORS: Do not translate %{author}. */
  w2 = i18n("<p>Enter the arguments which should be used to search for available updates to an entry.</p><p>"
           "The format is the same as for fields with derived values, where field names "
           "are contained inside braces, such as <i>%{author}</i>. See the documentation for details.</p>");
  m_cbUpdate->setWhatsThis(w);
  m_leUpdate->setWhatsThis(w2);
  if(fetcher_ && fetcher_->m_canUpdate) {
    m_cbUpdate->setChecked(true);
    m_leUpdate->setEnabled(true);
    m_leUpdate->setText(fetcher_->m_updateArgs);
  } else {
    m_cbUpdate->setChecked(false);
    m_leUpdate->setEnabled(false);
  }
  connect(m_cbUpdate, &QAbstractButton::toggled, m_leUpdate, &QWidget::setEnabled);

  l->setRowStretch(++row, 1);

  if(fetcher_) {
    m_pathEdit->setUrl(QUrl::fromLocalFile(fetcher_->m_path));
    m_newStuffName = fetcher_->m_newStuffName;
  }
  if(fetcher_ && fetcher_->m_collType > -1) {
    m_collCombo->setCurrentType(fetcher_->m_collType);
  } else {
    m_collCombo->setCurrentType(Data::Collection::Book);
  }
  if(fetcher_ && fetcher_->m_formatType > -1) {
    m_formatCombo->setCurrentData(fetcher_->m_formatType);
  } else {
    m_formatCombo->setCurrentData(Import::TellicoXML);
  }
  m_deleteOnRemove = fetcher_ && fetcher_->m_deleteOnRemove;
  KAcceleratorManager::manage(optionsWidget());
}

ExecExternalFetcher::ConfigWidget::~ConfigWidget() {
}

void ExecExternalFetcher::ConfigWidget::readConfig(const KConfigGroup& config_) {
  m_pathEdit->setUrl(QUrl::fromLocalFile(config_.readPathEntry("ExecPath", QString())));
  QList<int> argKeys = config_.readEntry("ArgumentKeys", QList<int>());
  QStringList argValues = config_.readEntry("Arguments", QStringList());
  if(argKeys.count() != argValues.count()) {
    myWarning() << "unequal number of arguments and keys";
  }
  int n = qMin(argKeys.count(), argValues.count());
  QMap<FetchKey, QString> args;
  for(int i = 0; i < n; ++i) {
    args[static_cast<FetchKey>(argKeys[i])] = argValues[i];
  }
  for(QList<int>::Iterator it = argKeys.begin(); it != argKeys.end(); ++it) {
    if(*it == Raw) {
      continue;
    }
    FetchKey key = static_cast<FetchKey>(*it);
    QCheckBox* cb = m_cbDict[key];
    QLineEdit* le = m_leDict[key];
    if(cb && le) {
      if(args.contains(key)) {
        cb->setChecked(true);
        le->setEnabled(true);
        le->setText(args[key]);
      } else {
        cb->setChecked(false);
        le->setEnabled(false);
        le->clear();
      }
    }
  }

  if(config_.hasKey("UpdateArgs")) {
    m_cbUpdate->setChecked(true);
    m_leUpdate->setEnabled(true);
    m_leUpdate->setText(config_.readEntry("UpdateArgs"));
  } else {
    m_cbUpdate->setChecked(false);
    m_leUpdate->setEnabled(false);
    m_leUpdate->clear();
  }

  int collType = config_.readEntry("CollectionType", -1);
  m_collCombo->setCurrentType(collType);

  int formatType = config_.readEntry("FormatType", -1);
  m_formatCombo->setCurrentData(static_cast<Import::Format>(formatType));
  m_deleteOnRemove = config_.readEntry("DeleteOnRemove", false);
  m_name = config_.readEntry("Name");
  m_newStuffName = config_.readEntry("NewStuffName");
}

void ExecExternalFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  QUrl u = m_pathEdit->url();
  if(!u.isEmpty()) {
    config_.writePathEntry("ExecPath", u.path());
  }
  QList<int> keys;
  QStringList args;
  QHash<int, QCheckBox*>::const_iterator it = m_cbDict.constBegin();
  for( ; it != m_cbDict.constEnd(); ++it) {
    if(it.value()->isChecked()) {
      keys << it.key();
      args << m_leDict[it.key()]->text();
    }
  }
  config_.writeEntry("ArgumentKeys", keys);
  config_.writeEntry("Arguments", args);

  if(m_cbUpdate->isChecked()) {
    config_.writeEntry("UpdateArgs", m_leUpdate->text());
  } else {
    config_.deleteEntry("UpdateArgs");
  }

  config_.writeEntry("CollectionType", m_collCombo->currentType());
  config_.writeEntry("FormatType", m_formatCombo->currentData().toInt());
  config_.writeEntry("DeleteOnRemove", m_deleteOnRemove);
  if(!m_newStuffName.isEmpty()) {
    config_.writeEntry("NewStuffName", m_newStuffName);
  }
}

void ExecExternalFetcher::ConfigWidget::removed() {
  if(!m_deleteOnRemove) {
    return;
  }
  if(!m_newStuffName.isEmpty()) {
    NewStuff::Manager::self()->removeScript(m_newStuffName);
  }
}

QString ExecExternalFetcher::ConfigWidget::preferredName() const {
  return m_name.isEmpty() ? ExecExternalFetcher::defaultName() : m_name;
}
