/***************************************************************************
    Copyright (C) 2011 Robby Stephenson <robby@periapsis.org>
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

#include "bibliosharefetcher.h"
#include "../utils/isbnvalidator.h"
#include "../entry.h"
#include "../images/imagefactory.h"
#include "../images/imageinfo.h"
#include "../core/tellico_strings.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>

#include <QLineEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QUrlQuery>

namespace {
  static const char* BIBLIOSHARE_BASE_URL = "https://www.biblioshare.org/BNCServices/BNCServices.asmx/";
  static const char* BIBLIOSHARE_TOKEN = "nsnqwebh87kstlty";
}

using namespace Tellico;
using Tellico::Fetch::BiblioShareFetcher;

BiblioShareFetcher::BiblioShareFetcher(QObject* parent_)
    : XMLFetcher(parent_)
    , m_token(QLatin1String(BIBLIOSHARE_TOKEN)) {
  setLimit(1);
  setXSLTFilename(QStringLiteral("biblioshare2tellico.xsl"));
}

BiblioShareFetcher::~BiblioShareFetcher() {
}

QString BiblioShareFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

// https://www.booknetcanada.ca/get-a-token
QString BiblioShareFetcher::attribution() const {
  return TC_I18N3(providedBy, QLatin1String("https://www.booknetcanada.ca/biblioshare"), QLatin1String("BNC BiblioShare"));
}

bool BiblioShareFetcher::canFetch(int type) const {
  return type == Data::Collection::Book || type == Data::Collection::Bibtex;
}

void BiblioShareFetcher::readConfigHook(const KConfigGroup& config_) {
  QString k = config_.readEntry("Token", BIBLIOSHARE_TOKEN);
  if(!k.isEmpty()) {
    m_token = k;
  }
}

QUrl BiblioShareFetcher::searchUrl() {
  QUrl u(QString::fromLatin1(BIBLIOSHARE_BASE_URL));
  u.setPath(u.path() + QStringLiteral("BiblioSimple"));

  QUrlQuery q;
  q.addQueryItem(QStringLiteral("Token"), m_token);

  switch(request().key()) {
    case ISBN:
      {
        // only grab first value
        QString v = request().value().section(QLatin1Char(';'), 0);
        v = ISBNValidator::isbn13(v);
        v.remove(QLatin1Char('-'));
        q.addQueryItem(QStringLiteral("EAN"), v);
      }
      break;

    default:
      return QUrl();
  }
  u.setQuery(q);
//  myDebug() << "url:" << u.url();
  return u;
}

Tellico::Data::EntryPtr BiblioShareFetcher::fetchEntryHookData(Data::EntryPtr entry_) {
  Q_ASSERT(entry_);

  if(!entry_) {
    myWarning() << "no entry";
    return entry_;
  }

  // if the entry cover is not set, go ahead and try to fetch it
  if(entry_->field(QStringLiteral("cover")).isEmpty()) {
    QString isbn = ISBNValidator::cleanValue(entry_->field(QStringLiteral("isbn")));
    if(!isbn.isEmpty()) {
      isbn = ISBNValidator::isbn13(isbn);
      isbn.remove(QLatin1Char('-'));

      QUrl imageUrl(QString::fromLatin1(BIBLIOSHARE_BASE_URL));
      imageUrl.setPath(imageUrl.path() + QStringLiteral("Images"));
      QUrlQuery q;
      q.addQueryItem(QStringLiteral("Token"), m_token);
      q.addQueryItem(QStringLiteral("EAN"), isbn);
      // the actual values for SAN Thumbnail don't seem to matter, they just can't be empty
      q.addQueryItem(QStringLiteral("SAN"), QStringLiteral("string"));
      q.addQueryItem(QStringLiteral("Thumbnail"), QStringLiteral("cover"));
      imageUrl.setQuery(q);
      const QString id = ImageFactory::addImage(imageUrl, true);
      if(!id.isEmpty()) {
        // placeholder images are 120x120 or 1x1
        Data::ImageInfo info = ImageFactory::imageInfo(id);
        if((info.width() != 120 || info.height() != 120) &&
           (info.width() != 1 || info.height() != 1)) {
          entry_->setField(QStringLiteral("cover"), id);
        }
      }
    }
  }

  return entry_;
}

Tellico::Fetch::FetchRequest BiblioShareFetcher::updateRequest(Data::EntryPtr entry_) {
  const QString isbn = entry_->field(QStringLiteral("isbn"));
  if(!isbn.isEmpty()) {
    return FetchRequest(Fetch::ISBN, isbn);
  }

  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* BiblioShareFetcher::configWidget(QWidget* parent_) const {
  return new BiblioShareFetcher::ConfigWidget(parent_, this);
}

QString BiblioShareFetcher::defaultName() {
  return QStringLiteral("BiblioShare");
}

QString BiblioShareFetcher::defaultIcon() {
  return favIcon(QUrl(QLatin1String("https://www.booknetcanada.ca")),
                 QUrl(QLatin1String("https://images.squarespace-cdn.com/content/v1/550334cbe4b0e08b6885e88f/1443713102643-G58N3NF6V7EWOGZ0I4A8/favicon.ico?format=100w")));
}

BiblioShareFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const BiblioShareFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;
  QLabel* al = new QLabel(i18n("Registration is required for accessing this data source. "
                               "If you agree to the terms and conditions, <a href='%1'>sign "
                               "up for an account</a>, and enter your information below.",
                                QStringLiteral("https://www.booknetcanada.ca/get-a-token")),
                          optionsWidget());
  al->setOpenExternalLinks(true);
  al->setWordWrap(true);
  ++row;
  l->addWidget(al, row, 0, 1, 2);
  // richtext gets weird with size
  al->setMinimumWidth(al->sizeHint().width());
  al->setMinimumHeight(al->sizeHint().height());

  QLabel* label = new QLabel(i18n("Access key: "), optionsWidget());
  l->addWidget(label, ++row, 0);

  m_tokenEdit = new QLineEdit(optionsWidget());
  connect(m_tokenEdit, &QLineEdit::textChanged, this, &ConfigWidget::slotSetModified);
  l->addWidget(m_tokenEdit, row, 1);
  QString w = i18n("The default Tellico key may be used, but searching may fail due to reaching access limits.");
  label->setWhatsThis(w);
  m_tokenEdit->setWhatsThis(w);
  label->setBuddy(m_tokenEdit);

  l->setRowStretch(++row, 10);

  if(fetcher_) {
    // only show the key if it is not the default Tellico one...
    // that way the user is prompted to apply for their own
    if(fetcher_->m_token != QLatin1String(BIBLIOSHARE_TOKEN)) {
      m_tokenEdit->setText(fetcher_->m_token);
    }
  }
}

void BiblioShareFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  QString token = m_tokenEdit->text().trimmed();
  if(!token.isEmpty()) {
    config_.writeEntry("Token", token);
  }
}

QString BiblioShareFetcher::ConfigWidget::preferredName() const {
  return BiblioShareFetcher::defaultName();
}
