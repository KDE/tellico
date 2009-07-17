/***************************************************************************
    Copyright (C) 2007-2009 Robby Stephenson <robby@periapsis.org>
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

#include "crossreffetcher.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../gui/guiproxy.h"
#include "../tellico_utils.h"
#include "../tellico_kernel.h"
#include "../collection.h"
#include "../entry.h"
#include "../core/netaccess.h"
#include "../images/imagefactory.h"
#include "../entrymerger.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <kstandarddirs.h>
#include <klineedit.h>
#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <KConfigGroup>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QPixmap>

// #define CROSSREF_TEST

#define CROSSREF_USE_UNIXREF

namespace {
  static const char* CROSSREF_BASE_URL = "http://www.crossref.org/openurl/?url_ver=Z39.88-2004&noredirect=true";
}

using Tellico::Fetch::CrossRefFetcher;

CrossRefFetcher::CrossRefFetcher(QObject* parent_)
    : Fetcher(parent_), m_xsltHandler(0), m_job(0), m_started(false) {
}

CrossRefFetcher::~CrossRefFetcher() {
  delete m_xsltHandler;
  m_xsltHandler = 0;
}

QString CrossRefFetcher::defaultName() {
  return QLatin1String("CrossRef");
}

QString CrossRefFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool CrossRefFetcher::canFetch(int type) const {
  return type == Data::Collection::Bibtex;
}

void CrossRefFetcher::readConfigHook(const KConfigGroup& config_) {
  QMap<QString, QString> map = Kernel::self()->readWalletMap(QLatin1String("crossref.org"));
  if(!map.isEmpty()) {
    m_user = map.value(QLatin1String("username"));
    m_password = map.value(QLatin1String("password"));
  } else {
    m_user = config_.readEntry("User");
    m_password = config_.readEntry("Password");
  }
}

void CrossRefFetcher::search() {
  m_started = true;

  if(m_user.isEmpty() || m_password.isEmpty()) {
    message(i18n("%1 requires a username and password.", source()), MessageHandler::Error);
    stop();
    return;
  }

//  myDebug() << "value = " << value_;

  KUrl u = searchURL(request().key, request().value);
  if(u.isEmpty()) {
    stop();
    return;
  }

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  m_job->ui()->setWindow(GUI::Proxy::widget());
  connect(m_job, SIGNAL(result(KJob*)),
          SLOT(slotComplete(KJob*)));
}

void CrossRefFetcher::stop() {
  if(!m_started) {
    return;
  }
//  myDebug();
  if(m_job) {
    m_job->kill();
    m_job = 0;
  }
  m_started = false;
  emit signalDone(this);
}

void CrossRefFetcher::slotComplete(KJob*) {
//  myDebug();

  if(m_job->error()) {
    m_job->ui()->showErrorMessage();
    stop();
    return;
  }

  QByteArray data = m_job->data();
  if(data.isEmpty()) {
    myDebug() << "no data";
    stop();
    return;
  }

  // since the fetch is done, don't worry about holding the job pointer
  m_job = 0;
#if 0
  myWarning() << "Remove debug from crossreffetcher.cpp";
  QFile f(QLatin1String("/tmp/test.xml"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setEncoding(QTextStream::UnicodeUTF8);
    t << data;
  }
  f.close();
#endif

  if(!m_xsltHandler) {
    initXSLTHandler();
    if(!m_xsltHandler) { // probably an error somewhere in the stylesheet loading
      stop();
      return;
    }
  }

  // assume result is always utf-8
  QString str = m_xsltHandler->applyStylesheet(QString::fromUtf8(data, data.size()));
  Import::TellicoImporter imp(str);
  Data::CollPtr coll = imp.collection();

  if(!coll) {
    myDebug() << "no valid result";
    stop();
    return;
  }

  Data::EntryList entries = coll->entries();
  foreach(Data::EntryPtr entry, entries) {
    if(!m_started) {
      // might get aborted
      break;
    }

    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, Data::EntryPtr(entry));
    emit signalResultFound(r);
  }

  stop(); // required
}

Tellico::Data::EntryPtr CrossRefFetcher::fetchEntry(uint uid_) {
  Data::EntryPtr entry = m_entries[uid_];
  // if URL but no cover image, fetch it
  if(!entry->field(QLatin1String("url")).isEmpty()) {
    Data::CollPtr coll = entry->collection();
    Data::FieldPtr field = coll->fieldByName(QLatin1String("cover"));
    if(!field && !coll->imageFields().isEmpty()) {
      field = coll->imageFields().front();
    } else if(!field) {
      field = new Data::Field(QLatin1String("cover"), i18n("Front Cover"), Data::Field::Image);
      coll->addField(field);
    }
    if(entry->field(field).isEmpty()) {
      QPixmap pix = NetAccess::filePreview(entry->field(QLatin1String("url")));
      if(!pix.isNull()) {
        QString id = ImageFactory::addImage(pix, QLatin1String("PNG"));
        if(!id.isEmpty()) {
          entry->setField(field, id);
        }
      }
    }
  }
  return entry;
}

void CrossRefFetcher::initXSLTHandler() {
#ifdef CROSSREF_USE_UNIXREF
  QString xsltfile = KStandardDirs::locate("appdata", QLatin1String("unixref2tellico.xsl"));
#else
  QString xsltfile = KStandardDirs::locate("appdata", QLatin1String("crossref2tellico.xsl"));
#endif
  if(xsltfile.isEmpty()) {
    myWarning() << "can not locate xslt file.";
    return;
  }

  KUrl u;
  u.setPath(xsltfile);

  delete m_xsltHandler;
  m_xsltHandler = new XSLTHandler(u);
  if(!m_xsltHandler->isValid()) {
    myWarning() << "error in crossref2tellico.xsl.";
    delete m_xsltHandler;
    m_xsltHandler = 0;
    return;
  }
}

KUrl CrossRefFetcher::searchURL(FetchKey key_, const QString& value_) const {
  KUrl u(CROSSREF_BASE_URL);
#ifdef CROSSREF_USE_UNIXREF
  u.addQueryItem(QLatin1String("format"), QLatin1String("unixref"));
#endif
  u.addQueryItem(QLatin1String("req_dat"), QString::fromLatin1("ourl_%1:%2").arg(m_user, m_password));

  switch(key_) {
    case DOI:
      u.addQueryItem(QLatin1String("rft_id"), QString::fromLatin1("info:doi/%1").arg(value_));
      break;

    default:
      myWarning() << "key not recognized: " << key_;
      return KUrl();
  }

#ifdef CROSSREF_TEST
  u = KUrl("/home/robby/crossref.xml");
#endif
  myDebug() << "url: " << u.url();
  return u;
}

Tellico::Fetch::FetchRequest CrossRefFetcher::updateRequest(Data::EntryPtr entry_) {
  QString doi = entry_->field(QLatin1String("doi"));
  if(!doi.isEmpty()) {
    return FetchRequest(Fetch::DOI, doi);
  }

#if 0
  // optimistically try searching for title and rely on Collection::sameEntry() to figure things out
  QString t = entry_->field(QLatin1String("title"));
  if(!t.isEmpty()) {
    return FetchRequest(Fetch::Title, t);
  }
#endif
  return FetchRequest();
}

void CrossRefFetcher::updateEntrySynchronous(Tellico::Data::EntryPtr entry) {
  if(!entry) {
    return;
  }
  if(m_user.isEmpty() || m_password.isEmpty()) {
    myDebug() << "username and password is required";
    return;
  }
  QString doi = entry->field(QLatin1String("doi"));
  if(doi.isEmpty()) {
    return;
  }

  KUrl u = searchURL(DOI, doi);
  QString xml = FileHandler::readTextFile(u, true, true);
  if(xml.isEmpty()) {
    return;
  }

  if(!m_xsltHandler) {
    initXSLTHandler();
    if(!m_xsltHandler) { // probably an error somewhere in the stylesheet loading
      return;
    }
  }

  // assume result is always utf-8
  QString str = m_xsltHandler->applyStylesheet(xml);
  Import::TellicoImporter imp(str);
  Data::CollPtr coll = imp.collection();
  if(coll && coll->entryCount() > 0) {
    myLog() << "found DOI result, merging";
    EntryMerger::mergeEntry(entry, coll->entries().front(), false /*overwrite*/);
  }
}

Tellico::Fetch::ConfigWidget* CrossRefFetcher::configWidget(QWidget* parent_) const {
  return new CrossRefFetcher::ConfigWidget(parent_, this);
}

CrossRefFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const CrossRefFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = 0;

  QLabel* al = new QLabel(i18n("CrossRef requires an account for access. "
                               "If you agree to the terms and conditions, "
                               "<a href='http://www.crossref.org/requestaccount/'>"
                               "request an account</a>, and enter your OpenURL "
                               "account information below."),
                          optionsWidget());
  al->setOpenExternalLinks(true);
  al->setWordWrap(true);
  ++row;
  l->addWidget(al, row, 0, 1, 2);
  // richtext gets weird with size
  al->setMinimumWidth(al->sizeHint().width());

  QLabel* label = new QLabel(i18n("&Username: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_userEdit = new KLineEdit(optionsWidget());
  connect(m_userEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_userEdit, row, 1);
  QString w = i18n("A username and password is required to access the CrossRef service.");
  label->setWhatsThis(w);
  m_userEdit->setWhatsThis(w);
  label->setBuddy(m_userEdit);

  label = new QLabel(i18n("&Password: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_passEdit = new KLineEdit(optionsWidget());
  m_passEdit->setEchoMode(QLineEdit::PasswordEchoOnEdit);
  connect(m_passEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_passEdit, row, 1);
  label->setWhatsThis(w);
  m_passEdit->setWhatsThis(w);
  label->setBuddy(m_passEdit);

  if(fetcher_) {
    m_userEdit->setText(fetcher_->m_user);
    m_passEdit->setText(fetcher_->m_password);
  }
}

void CrossRefFetcher::ConfigWidget::saveConfig(KConfigGroup& config_) {
  QMap<QString, QString> map;
  map.insert(QLatin1String("username"), m_userEdit->text().trimmed());
  map.insert(QLatin1String("password"), m_passEdit->text().trimmed());
  Kernel::self()->writeWalletMap(QLatin1String("crossref.org"), map);

  // used to store username and password in plain text
  config_.deleteEntry("User");
  config_.deleteEntry("Password");

  slotSetModified(false);
}

QString CrossRefFetcher::ConfigWidget::preferredName() const {
  return CrossRefFetcher::defaultName();
}

#include "crossreffetcher.moc"
