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

#include "bibtexkeydialog.h"
#include "collections/bibtexcollection.h"
#include "entry.h"
#include "tellico_debug.h"

#include <KLocale>
#include <KTitleWidget>

#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>

using Tellico::BibtexKeyDialog;

// default button is going to be used as a print button, so it's separated
BibtexKeyDialog::BibtexKeyDialog(Data::CollPtr coll_, QWidget* parent_)
    : KDialog(parent_), m_coll(coll_) {
  Q_ASSERT(m_coll);
  setModal(false);
  setCaption(i18n("Citation Key Manager"));
  setButtons(User1|User2|Close);
  setDefaultButton(Close);

  setButtonGuiItem(User1, KGuiItem(i18n("Filter for duplicates"), QLatin1String("view-filter")));
  setButtonGuiItem(User2, KGuiItem(i18n("Check for duplicates"), QLatin1String("system-search")));
 
  QWidget* mainWidget = new QWidget(this);
  setMainWidget(mainWidget);
  QVBoxLayout* topLayout = new QVBoxLayout(mainWidget);
   
  m_dupeLabel = new KTitleWidget(this);
  m_dupeLabel->setText(m_coll->title(), KTitleWidget::PlainMessage);
  m_dupeLabel->setComment(i18n("Checking for entries with duplicate citation keys..."));
  m_dupeLabel->setPixmap(KIcon(QLatin1String("tools-wizard")).pixmap(64, 64), KTitleWidget::ImageLeft);
  topLayout->addWidget(m_dupeLabel);

  KConfigGroup config(KGlobal::config(), QLatin1String("Bibtex Key Dialog Options"));
  restoreDialogSize(config);

  if(m_coll->type() != Data::Collection::Bibtex)  {
    // if it's not a bibliography, no need to save a pointer
    m_coll = Data::CollPtr();
    myWarning() << "not a bibliography";
  } else {
    // the button is enabled when duplicates are found
    enableButton(User1, false);
    connect(this, SIGNAL(user1Clicked()), SLOT(slotFilterDuplicates()));
    connect(this, SIGNAL(user2Clicked()), SLOT(slotCheckDuplicates()));
    QTimer::singleShot(0, this, SLOT(slotCheckDuplicatesImpl()));
  }
}

BibtexKeyDialog::~BibtexKeyDialog() {
  KConfigGroup config(KGlobal::config(), QLatin1String("Bibtex Key Dialog Options"));
  saveDialogSize(config);
}

void BibtexKeyDialog::slotCheckDuplicates() {
  if(!m_coll) {
    return;
  }
  m_dupeLabel->setComment(i18n("Checking for entries with duplicate citation keys..."));
  QTimer::singleShot(0, this, SLOT(slotCheckDuplicatesImpl()));
}

void BibtexKeyDialog::slotCheckDuplicatesImpl() {
  const Data::BibtexCollection* c = static_cast<Data::BibtexCollection*>(m_coll.data());
  m_dupes = c->duplicateBibtexKeys();
  
  enableButton(User1, false);
  if(m_dupes.isEmpty())  {
    m_dupeLabel->setComment(i18n("There are no duplicate citation keys."));
    enableButton(User1, false);
  } else {
    m_dupeLabel->setComment(i18np("There is %1 duplicate citation key.", "There are %1 duplicate citation keys.", m_dupes.count()));
    enableButton(User1, true);
  }
}

void BibtexKeyDialog::slotFilterDuplicates() {
  if(!m_coll || m_dupes.isEmpty()) {
    return;
  }

  FilterPtr filter(new Filter(Filter::MatchAny));

  QSet<QString> keys;
  foreach(Data::EntryPtr entry, m_dupes) {
    const QString key = entry->field(QLatin1String("bibtex-key"));
    if(!keys.contains(key)) {
      filter->append(new FilterRule(QLatin1String("bibtex-key"), key, FilterRule::FuncEquals));
      keys << key;
    }
  }
  
  if(!filter->isEmpty()) {
    emit signalUpdateFilter(filter);
  }
}

#include "bibtexkeydialog.moc"
