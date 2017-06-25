/***************************************************************************
    Copyright (C) 2016 Robby Stephenson <robby@periapsis.org>
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

#include "bedethequefetcher.h"
#include "../utils/guiproxy.h"
#include "../utils/string_utils.h"
#include "../utils/isbnvalidator.h"
#include "../collections/comicbookcollection.h"
#include "../entry.h"
#include "../fieldformat.h"
#include "../core/filehandler.h"
#include "../images/imagefactory.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KIO/Job>
#include <KJobUiDelegate>
#include <KJobWidgets/KJobWidgets>

#include <QRegExp>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QUrlQuery>

namespace {
  static const char* BD_BASE_URL = "http://m.bedetheque.com/album";
}

using namespace Tellico;
using Tellico::Fetch::BedethequeFetcher;

BedethequeFetcher::BedethequeFetcher(QObject* parent_)
    : Fetcher(parent_), m_total(0), m_started(false) {
}

BedethequeFetcher::~BedethequeFetcher() {
}

QString BedethequeFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

Fetch::Type BedethequeFetcher::type() const {
  return Bedetheque;
}

bool BedethequeFetcher::canFetch(int type) const {
  return type == Data::Collection::ComicBook;
}

// No UPC or Raw for now.
bool BedethequeFetcher::canSearch(FetchKey k) const {
  return k == Title || k == Keyword || k == ISBN;
}

void BedethequeFetcher::readConfigHook(const KConfigGroup& config_) {
  Q_UNUSED(config_);
}

void BedethequeFetcher::search() {
  m_started = true;
  m_matches.clear();

  // special case for updates which include the BD link as Raw request
  if(request().key == Raw) {
    QUrl u(request().value);
    u.setHost(QLatin1String("m.bedetheque.com")); // use mobile site for easier parsing
    m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
    m_job->addMetaData(QLatin1String("referrer"), QString::fromLatin1(BD_BASE_URL));
    KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
    // different slot here
    connect(m_job, SIGNAL(result(KJob*)), SLOT(slotLinkComplete(KJob*)));
    return;
  }

  QUrl u(QString::fromLatin1(BD_BASE_URL));

/*
  fetchToken();
  if(m_token.isEmpty()) {
    myDebug() << "empty token";
    stop();
    return;
  }
*/

  QUrlQuery q;
  switch(request().key) {
    case Title:
      q.addQueryItem(QLatin1String("RechTitre"), request().value);
      break;

    case Keyword:
      q.addQueryItem(QLatin1String("RechSerie"), request().value);
      break;

    case ISBN:
      q.addQueryItem(QLatin1String("RechISBN"), ISBNValidator::cleanValue(request().value));
      break;

    default:
      myWarning() << "key not recognized: " << request().key;
      stop();
      return;
  }
//  q.addQueryItem(QLatin1String("csrf_token_bedetheque"), m_token);
  u.setQuery(q);
//  myDebug() << "url: " << u.url();

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  m_job->addMetaData(QLatin1String("referrer"), QString::fromLatin1(BD_BASE_URL));
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job, SIGNAL(result(KJob*)), SLOT(slotComplete(KJob*)));
}

void BedethequeFetcher::stop() {
  if(!m_started) {
    return;
  }

  if(m_job) {
    m_job->kill();
    m_job = nullptr;
  }
  m_started = false;
  emit signalDone(this);
}

void BedethequeFetcher::slotComplete(KJob*) {
  if(m_job->error()) {
    m_job->uiDelegate()->showErrorMessage();
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
  m_job = nullptr;

  QString output = Tellico::decodeHTML(data);
#if 0
  myWarning() << "Remove debug from bedethequefetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/testbd.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << output;
  }
  f.close();
#endif

  const int pos_list = output.indexOf(QLatin1String("<li data-role=\"list-divider\" role=\"heading\">"), 0, Qt::CaseInsensitive);
  if(pos_list == -1) {
    myDebug() << "No results found";
    stop();
    return;
  }
  const int pos_end = output.indexOf(QLatin1String("</ul>"), pos_list+1, Qt::CaseInsensitive);
  output = output.mid(pos_list, pos_end-pos_list);

  QString pat = QLatin1String("http://m.bedetheque.com/BD");
  QRegExp anchorRx(QLatin1String("<a\\s+[^>]*href\\s*=\\s*[\"'](") +
                   QRegExp::escape(pat) +
                   QLatin1String("[^\"']*)\"[^>]*>(.*)</a"), Qt::CaseInsensitive);
  anchorRx.setMinimal(true);

  QRegExp spanRx(QLatin1String("\\sclass\\s*=\\s*\"(.*)\">(.*)<"));
  spanRx.setMinimal(true);

  for(int pos = anchorRx.indexIn(output); m_started && pos > -1; pos = anchorRx.indexIn(output, pos+anchorRx.matchedLength())) {
    QString url = anchorRx.cap(1);
    if(url.isEmpty()) {
      continue;
    }

    const QString result = anchorRx.cap(2);
    if(result.isEmpty()) {
      continue;
    }

    QString title;
    QStringList desc;
    for(int pos2 = spanRx.indexIn(result); pos2 > -1; pos2 = spanRx.indexIn(result, pos2+spanRx.matchedLength())) {
      QString cname = spanRx.cap(1);
      QString value = spanRx.cap(2);
      if(cname == QLatin1String("serie")) {
        desc += value;
      } else if(cname == QLatin1String("titre")) {
        title = value;
      } else if(cname == QLatin1String("dl")) {
        desc += value;
      }
    }

    if(!title.isEmpty() && !url.isEmpty()) {
      FetchResult* r = new FetchResult(Fetcher::Ptr(this), title, desc.join(QLatin1String(" ")));
      m_matches.insert(r->uid, QUrl(url));
      emit signalResultFound(r);
    }
  }

  stop();
}

// slot called after downloading the exact link
void BedethequeFetcher::slotLinkComplete(KJob*) {
  if(m_job->error()) {
    m_job->uiDelegate()->showErrorMessage();
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
  m_job = nullptr;

  QString output = Tellico::decodeHTML(data);
  Data::EntryPtr entry = parseEntry(output);
  if(!entry) {
    myDebug() << "error in processing entry";
    stop();
    return;
  }

  FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
  m_matches.insert(r->uid, QUrl(request().value));
  m_entries.insert(r->uid, entry); // keep for later

  emit signalResultFound(r);
  stop();
}

Tellico::Data::EntryPtr BedethequeFetcher::fetchEntryHook(uint uid_) {
  // if we already grabbed this one, then just pull it out of the dict
  Data::EntryPtr entry = m_entries[uid_];
  if(entry) {
    return entry;
  }

  QUrl url = m_matches[uid_];
  if(url.isEmpty()) {
    myWarning() << "no url in map";
    return Data::EntryPtr();
  }

  QString results = Tellico::decodeHTML(FileHandler::readDataFile(url, true));
  if(results.isEmpty()) {
    myDebug() << "no text results";
    return Data::EntryPtr();
  }

//  myDebug() << url.url();
#if 0
  myWarning() << "Remove debug from bedethequefetcher.cpp";
  QFile f(QLatin1String("/tmp/testbditem.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << results;
  }
  f.close();
#endif

  entry = parseEntry(results);
  if(!entry) {
    myDebug() << "error in processing entry";
    return Data::EntryPtr();
  }
  m_entries.insert(uid_, entry); // keep for later
  return entry;
}

Tellico::Data::EntryPtr BedethequeFetcher::parseEntry(const QString& str_) {
  Data::CollPtr coll(new Data::ComicBookCollection(true));

 // map captions in HTML to field names
  QHash<QString, QString> fieldMap;
  fieldMap.insert(QString::fromUtf8("Série"),       QLatin1String("series"));
  fieldMap.insert(QLatin1String("Titre"),           QLatin1String("title"));
  fieldMap.insert(QLatin1String("Origine"),         QLatin1String("country"));
//  fieldMap.insert(QLatin1String("Format"),          QLatin1String("binding"));
  fieldMap.insert(QString::fromUtf8("Scénario"),    QLatin1String("writer"));
  fieldMap.insert(QLatin1String("Dessin"),          QLatin1String("artist"));
  fieldMap.insert(QString::fromUtf8("Dépot légal"), QLatin1String("pub_year"));
  fieldMap.insert(QLatin1String("Editeur"),         QLatin1String("publisher"));
  fieldMap.insert(QLatin1String("Planches"),        QLatin1String("pages"));
  fieldMap.insert(QLatin1String("Style"),           QLatin1String("genre"));
  fieldMap.insert(QLatin1String("Tome"),            QLatin1String("issue"));
  fieldMap.insert(QLatin1String("Collection"),      QLatin1String("edition"));

  if(optionalFields().contains(QLatin1String("isbn"))) {
    Data::FieldPtr field(new Data::Field(QLatin1String("isbn"), i18n("ISBN#")));
    field->setCategory(i18n("Publishing"));
    field->setDescription(i18n("International Standard Book Number"));
    coll->addField(field);
    fieldMap.insert(QLatin1String("ISBN"), QLatin1String("isbn"));
  }
  if(optionalFields().contains(QLatin1String("colorist"))) {
    Data::FieldPtr field(new Data::Field(QLatin1String("colorist"), i18n("Colorist")));
    field->setCategory(i18n("General"));
    field->setFlags(Data::Field::AllowCompletion | Data::Field::AllowMultiple | Data::Field::AllowGrouped);
    field->setFormatType(FieldFormat::FormatName);
    coll->addField(field);
    fieldMap.insert(QLatin1String("Couleurs"), QLatin1String("colorist"));
  }
  if(optionalFields().contains(QLatin1String("lien-bel"))) {
    Data::FieldPtr field(new Data::Field(QLatin1String("lien-bel"), i18n("Bedetheque Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }

  QRegExp tagRx(QLatin1String("<.*>"));
  tagRx.setMinimal(true);

  QRegExp yearRx(QLatin1String("\\d{4}"));
  // the negative lookahead with "no-border" is for multiple values
  QString pat = QLatin1String("<label>%1.*</label>(.+)</li>(?!\\s*<li class=\"no-border)");

  Data::EntryPtr entry(new Data::Entry(coll));

  for(QHash<QString, QString>::Iterator it = fieldMap.begin(); it != fieldMap.end(); ++it) {
    QRegExp infoRx(pat.arg(it.key()));
    infoRx.setMinimal(true);
    if(infoRx.indexIn(str_) == -1) {
      continue;
    }
    if(it.value() == QLatin1String("pub_year")) {
      QString data = infoRx.cap(1).remove(tagRx).simplified();
      if(yearRx.indexIn(data) > -1) {
        entry->setField(it.value(), yearRx.cap(0));
      }
    } else if(it.value() == QLatin1String("writer") ||
              it.value() == QLatin1String("artist") ||
              it.value() == QLatin1String("publisher") ||
              it.value() == QLatin1String("colorist")) {
      // catch multiple people
      QString value = infoRx.cap(1);
      // split the values with the "no-border" CSS
      value.replace(QLatin1String("<li class=\"no-border\">"), FieldFormat::delimiterString());
      value = FieldFormat::fixupValue(value.remove(tagRx).simplified());
      entry->setField(it.value(), value);
    } else if(it.value() == QLatin1String("genre")) {
      // replace comma with semi-colons to effectively split string values
      QString value = infoRx.cap(1).remove(tagRx).simplified();
      value.replace(QLatin1String(", "), FieldFormat::delimiterString());
      entry->setField(it.value(), value);
    } else {
      entry->setField(it.value(), infoRx.cap(1).remove(tagRx).simplified());
    }
    // myDebug() << it.value() << entry->field(it.value());
  }

  QRegExp imgRx(QLatin1String("<img[^<]*src\\s*=\\s*\"([^\"]+)\"\\s+alt\\s*=\\s*\"Couverture"));
  imgRx.setMinimal(true);
  if(imgRx.indexIn(str_) > -1) {
    QUrl u(imgRx.cap(1));
    QString id = ImageFactory::addImage(u, true);
    if(!id.isEmpty()) {
      entry->setField(QLatin1String("cover"), id);
    }
  }

  if(optionalFields().contains(QLatin1String("comments"))) {
    QRegExp chronRx(QLatin1String("La chronique\\s*</li>\\s*<li[^>]*>(.*)</ul>"));
    chronRx.setMinimal(true);
    if(chronRx.indexIn(str_) > -1) {
      entry->setField(QLatin1String("comments"), chronRx.cap(1).trimmed());
    }
  }

  if(optionalFields().contains(QLatin1String("lien-bel"))) {
    QRegExp linkRx(QLatin1String("<link\\s+rel\\s*=\\s*\"canonical\"\\s+href\\s*=\\s*\"([^\"]+)\""));
    linkRx.setMinimal(true);
    if(linkRx.indexIn(str_) > -1) {
      entry->setField(QLatin1String("lien-bel"), linkRx.cap(1));
    }
  }

  return entry;
}

Tellico::Fetch::FetchRequest BedethequeFetcher::updateRequest(Data::EntryPtr entry_) {
  QString l = entry_->field(QLatin1String("lien-bel"));
  if(!l.isEmpty()) {
    return FetchRequest(Fetch::Raw, l);
  }
  QString i = entry_->field(QLatin1String("isbn"));
  if(!i.isEmpty()) {
    return FetchRequest(Fetch::ISBN, i);
  }
  QString t = entry_->field(QLatin1String("title"));
  if(!t.isEmpty()) {
    return FetchRequest(Fetch::Title, t);
  }
  return FetchRequest();
}

void BedethequeFetcher::fetchToken() {
  QRegExp tokenRx(QLatin1String("name\\s*=\\s*\"csrf_token_bedetheque\"\\s*value\\s*=\\s*\"([^\"]+)\""));

  const QUrl url(QString::fromLatin1("http://www.bedetheque.com/search/albums"));
  const QString text = FileHandler::readTextFile(url, true /*quiet*/);
  if(tokenRx.indexIn(text) > -1) {
    m_token = tokenRx.cap(1);
  }
}

Tellico::Fetch::ConfigWidget* BedethequeFetcher::configWidget(QWidget* parent_) const {
  return new BedethequeFetcher::ConfigWidget(parent_, this);
}

QString BedethequeFetcher::defaultName() {
  return QLatin1String("Bedetheque");
}

QString BedethequeFetcher::defaultIcon() {
  return favIcon("http://www.bedetheque.com");
}

//static
Tellico::StringHash BedethequeFetcher::allOptionalFields() {
  StringHash hash;
  hash[QLatin1String("colorist")]     = i18n("Colorist");
  hash[QLatin1String("comments")]     = i18n("Comments");
  hash[QLatin1String("isbn")]         = i18n("ISBN#");
  // use the field name that the bedetheque.py script did, to maintain backwards compatibility
  hash[QLatin1String("lien-bel")]     = i18n("Bedetheque Link");
  return hash;
}

BedethequeFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const BedethequeFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  // now add additional fields widget
  addFieldsWidget(BedethequeFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

QString BedethequeFetcher::ConfigWidget::preferredName() const {
  return BedethequeFetcher::defaultName();
}
