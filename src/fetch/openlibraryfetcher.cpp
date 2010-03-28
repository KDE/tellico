/***************************************************************************
    Copyright (C) 2009 Robby Stephenson <robby@periapsis.org>
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

#include "openlibraryfetcher.h"
#include "../collections/bookcollection.h"
#include "../images/imagefactory.h"
#include "../gui/guiproxy.h"
#include "../utils/isbnvalidator.h"
#include "../tellico_utils.h"
#include "../collection.h"
#include "../entry.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <kstandarddirs.h>
#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <KConfigGroup>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>

using namespace Tellico;
using Tellico::Fetch::OpenLibraryFetcher;

OpenLibraryFetcher::OpenLibraryFetcher(QObject* parent_)
    : Fetcher(parent_), m_started(false) {
}

OpenLibraryFetcher::~OpenLibraryFetcher() {
}

QString OpenLibraryFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool OpenLibraryFetcher::canFetch(int type) const {
  return type == Data::Collection::Book;
}

void OpenLibraryFetcher::readConfigHook(const KConfigGroup&) {
}

void OpenLibraryFetcher::search() {
  m_started = true;
  doSearch();
}

void OpenLibraryFetcher::continueSearch() {
  m_started = true;
  doSearch();
}

void OpenLibraryFetcher::doSearch() {
  KUrl imageURL;

  QString s = QString::fromLatin1("http://covers.openlibrary.org/b/isbn/%1-M.jpg?default=false").arg(ISBNValidator::cleanValue(request().value));
  switch(request().key) {
    case ISBN:
      imageURL.setUrl(s);
      break;

    default:
      myWarning() << "key not recognized: " << request().key;
      stop();
      return;
  }

  myDebug() << "url:" << imageURL;

  const QString id = ImageFactory::addImage(imageURL, true);
  if(id.isEmpty()) {
    myWarning() << "no image";
    stop();
    return;
  }

  Data::CollPtr coll(new Data::BookCollection(true));

  Data::EntryPtr entry(new Data::Entry(coll));
  entry->setField(QLatin1String("isbn"), request().value);
  entry->setField(QLatin1String("cover"), id);

  FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
  m_entries.insert(r->uid, Data::EntryPtr(entry));
  emit signalResultFound(r);

  stop();
}

void OpenLibraryFetcher::stop() {
  if(!m_started) {
    return;
  }
  m_started = false;
  emit signalDone(this);
}

Tellico::Data::EntryPtr OpenLibraryFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }
  return entry;
}

Tellico::Fetch::FetchRequest OpenLibraryFetcher::updateRequest(Data::EntryPtr entry_) {
  const QString isbn = entry_->field(QLatin1String("isbn"));
  if(!isbn.isEmpty()) {
    return FetchRequest(ISBN, isbn);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* OpenLibraryFetcher::configWidget(QWidget* parent_) const {
  return new OpenLibraryFetcher::ConfigWidget(parent_, this);
}

QString OpenLibraryFetcher::defaultName() {
  return QLatin1String("Open Library"); // no translation
}

QString OpenLibraryFetcher::defaultIcon() {
  return favIcon("http://www.openlibrary.org");
}

OpenLibraryFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const OpenLibraryFetcher*)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();
}

void OpenLibraryFetcher::ConfigWidget::saveConfigHook(KConfigGroup&) {
}

QString OpenLibraryFetcher::ConfigWidget::preferredName() const {
  return OpenLibraryFetcher::defaultName();
}

#include "openlibraryfetcher.moc"
