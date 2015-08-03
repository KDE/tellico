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
#include "../utils/guiproxy.h"
#include "../utils/string_utils.h"
#include "../collection.h"
#include "../entry.h"
#include "../core/netaccess.h"
#include "../images/imagefactory.h"
#include "../utils/wallet.h"
#include "../utils/datafileregistry.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <KConfigGroup>
#include <KJobWidgets/KJobWidgets>

#include <QLineEdit>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QPixmap>
#include <QTextCodec>

#define CROSSREF_USE_UNIXREF

namespace {
  static const char* CROSSREF_BASE_URL = "http://www.crossref.org/openurl/";
}

using namespace Tellico;
using namespace Tellico::Fetch;
using Tellico::Fetch::CrossRefFetcher;

CrossRefFetcher::CrossRefFetcher(QObject* parent_)
    : Fetcher(parent_), m_xsltHandler(0), m_job(0), m_started(false) {
}

CrossRefFetcher::~CrossRefFetcher() {
  delete m_xsltHandler;
  m_xsltHandler = 0;
}

QString CrossRefFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool CrossRefFetcher::canFetch(int type) const {
  return type == Data::Collection::Bibtex;
}

void CrossRefFetcher::readConfigHook(const KConfigGroup& config_) {
  m_user = config_.readEntry("User");
  m_password = config_.readEntry("Password");
  m_email = config_.readEntry("Email");
}

void CrossRefFetcher::search() {
  m_started = true;

  readWallet();
  if(m_email.isEmpty() && (m_user.isEmpty() || m_password.isEmpty())) {
    message(i18n("%1 requires a username and password.", source()), MessageHandler::Error);
    stop();
    return;
  }

//  myDebug() << "value = " << value_;

  QUrl u = searchURL(request().key, request().value);
  if(u.isEmpty()) {
    stop();
    return;
  }

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
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
    t.setCodec("UTF-8");
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
  QString str = m_xsltHandler->applyStylesheet(QString::fromUtf8(data.constData(), data.size()));
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

Tellico::Data::EntryPtr CrossRefFetcher::fetchEntryHook(uint uid_) {
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
      QPixmap pix = NetAccess::filePreview(QUrl::fromUserInput(entry->field(QLatin1String("url"))));
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
  QString xsltfile = DataFileRegistry::self()->locate(QLatin1String("unixref2tellico.xsl"));
#else
  QString xsltfile = DataFileRegistry::self()->locate(QLatin1String("crossref2tellico.xsl"));
#endif
  if(xsltfile.isEmpty()) {
    myWarning() << "can not locate xslt file.";
    return;
  }

  QUrl u = QUrl::fromLocalFile(xsltfile);

  delete m_xsltHandler;
  m_xsltHandler = new XSLTHandler(u);
  if(!m_xsltHandler->isValid()) {
    myWarning() << "error in crossref2tellico.xsl.";
    delete m_xsltHandler;
    m_xsltHandler = 0;
    return;
  }
}

QUrl CrossRefFetcher::searchURL(FetchKey key_, const QString& value_) const {
  QUrl u(QString::fromLatin1(CROSSREF_BASE_URL));
  u.addQueryItem(QLatin1String("noredirect"), QLatin1String("true"));
  u.addQueryItem(QLatin1String("multihit"), QLatin1String("true"));
#ifdef CROSSREF_USE_UNIXREF
  u.addQueryItem(QLatin1String("format"), QLatin1String("unixref"));
#endif
  if(m_email.isEmpty()) {
    u.addQueryItem(QLatin1String("pid"), QString::fromLatin1("%1:%2").arg(m_user, m_password));
  } else {
    u.addQueryItem(QLatin1String("pid"), m_email);
  }

  switch(key_) {
    case DOI:
      u.addQueryItem(QLatin1String("rft_id"), QString::fromLatin1("info:doi/%1").arg(value_));
      break;

    default:
      myWarning() << "key not recognized: " << key_;
      return QUrl();
  }

//  myDebug() << "url: " << u.url();
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

void CrossRefFetcher::readWallet() const {
  if(m_user.isEmpty() || m_password.isEmpty()) {
    QMap<QString, QString> map = Wallet::self()->readWalletMap(QLatin1String("crossref.org"));
    if(!map.isEmpty()) {
      m_user = map.value(QLatin1String("username"));
      m_password = map.value(QLatin1String("password"));
    }
  }
}

Tellico::Fetch::ConfigWidget* CrossRefFetcher::configWidget(QWidget* parent_) const {
  return new CrossRefFetcher::ConfigWidget(parent_, this);
}

QString CrossRefFetcher::defaultName() {
  return QLatin1String("CrossRef"); // no translation
}

QString CrossRefFetcher::defaultIcon() {
  return favIcon("http://crossref.org");
}

CrossRefFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const CrossRefFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = 0;

  QLabel* al = new QLabel(i18n("Registration is required for accessing the %1 data source. "
                               "If you agree to the terms and conditions, <a href='%2'>sign "
                               "up for an account</a>, and enter your information below.",
                                preferredName(),
                                QLatin1String("http://www.crossref.org/requestaccount/")),
                          optionsWidget());
  al->setOpenExternalLinks(true);
  al->setWordWrap(true);
  l->addWidget(al, row, 0, 1, 2);
  // richtext gets weird with size
  al->setMinimumWidth(al->sizeHint().width());

  QLabel* label = new QLabel(i18n("&Username: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_userEdit = new QLineEdit(optionsWidget());
  connect(m_userEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_userEdit, row, 1);
  QString w = i18n("A username and password is required to access the CrossRef service.");
  label->setWhatsThis(w);
  m_userEdit->setWhatsThis(w);
  label->setBuddy(m_userEdit);

  label = new QLabel(i18n("&Password: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_passEdit = new QLineEdit(optionsWidget());
//  m_passEdit->setEchoMode(QLineEdit::PasswordEchoOnEdit);
  connect(m_passEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_passEdit, row, 1);
  label->setWhatsThis(w);
  m_passEdit->setWhatsThis(w);
  label->setBuddy(m_passEdit);

  label = new QLabel(i18n("For some accounts, only an email address is required."), optionsWidget());
  l->addWidget(label, ++row, 0, 1, 2);

  label = new QLabel(i18n("Email: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_emailEdit = new QLineEdit(optionsWidget());
  connect(m_emailEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_emailEdit, row, 1);
  label->setBuddy(m_emailEdit);

  if(fetcher_) {
    fetcher_->readWallet(); // make sure that the wallet values are read
    m_userEdit->setText(fetcher_->m_user);
    m_passEdit->setText(fetcher_->m_password);
    m_emailEdit->setText(fetcher_->m_email);
  }
}

void CrossRefFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  QString s = m_userEdit->text().trimmed();
  if(!s.isEmpty()) {
    config_.writeEntry("User", s);
  }
  s = m_passEdit->text().trimmed();
  if(!s.isEmpty()) {
    config_.writeEntry("Password", s);
  }
  s = m_emailEdit->text().trimmed();
  if(!s.isEmpty()) {
    config_.writeEntry("Email", s);
  }

  slotSetModified(false);
}

QString CrossRefFetcher::ConfigWidget::preferredName() const {
  return CrossRefFetcher::defaultName();
}

