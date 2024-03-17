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

#include <KLocalizedString>
#include <KTitleWidget>
#include <KSharedConfig>
#include <KGuiItem>

#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>
#include <QDialogButtonBox>
#include <QPushButton>

using Tellico::BibtexKeyDialog;

// default button is going to be used as a print button, so it's separated
BibtexKeyDialog::BibtexKeyDialog(Tellico::Data::CollPtr coll_, QWidget* parent_)
    : QDialog(parent_), m_coll(coll_) {
  Q_ASSERT(m_coll);
  setModal(false);
  setWindowTitle(i18n("Citation Key Manager"));

  QVBoxLayout* topLayout = new QVBoxLayout;
  setLayout(topLayout);

  QWidget* mainWidget = new QWidget(this);
  topLayout->addWidget(mainWidget);

  m_dupeLabel = new KTitleWidget(this);
  m_dupeLabel->setText(m_coll->title(), KTitleWidget::PlainMessage);
  m_dupeLabel->setComment(i18n("Checking for entries with duplicate citation keys..."));
  m_dupeLabel->setIcon(QIcon::fromTheme(QStringLiteral("tools-wizard")), KTitleWidget::ImageLeft);
  topLayout->addWidget(m_dupeLabel);

  QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
  buttonBox->button(QDialogButtonBox::Close)->setDefault(true);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

  QPushButton* checkDuplicates = new QPushButton(buttonBox);
  KGuiItem::assign(checkDuplicates, KGuiItem(i18n("Check for duplicates"), QStringLiteral("system-search")));
  buttonBox->addButton(checkDuplicates, QDialogButtonBox::ActionRole);
  m_filterButton = new QPushButton(buttonBox);
  KGuiItem::assign(m_filterButton, KGuiItem(i18n("Filter for duplicates"), QStringLiteral("view-filter")));
  buttonBox->addButton(m_filterButton, QDialogButtonBox::ActionRole);

  topLayout->addWidget(buttonBox);

  if(m_coll->type() != Data::Collection::Bibtex)  {
    // if it's not a bibliography, no need to save a pointer
    m_coll = Data::CollPtr();
    myWarning() << "not a bibliography";
  } else {
    // the button is enabled when duplicates are found
    m_filterButton->setEnabled(false);
    connect(m_filterButton, &QAbstractButton::clicked, this, &BibtexKeyDialog::slotFilterDuplicates);
    connect(checkDuplicates, &QAbstractButton::clicked, this, &BibtexKeyDialog::slotCheckDuplicates);
    QTimer::singleShot(0, this, &BibtexKeyDialog::slotCheckDuplicatesImpl);
  }
}

BibtexKeyDialog::~BibtexKeyDialog() {
}

void BibtexKeyDialog::slotCheckDuplicates() {
  if(!m_coll) {
    return;
  }
  m_dupeLabel->setComment(i18n("Checking for entries with duplicate citation keys..."));
  QTimer::singleShot(0, this, &BibtexKeyDialog::slotCheckDuplicatesImpl);
}

void BibtexKeyDialog::slotCheckDuplicatesImpl() {
  const Data::BibtexCollection* c = static_cast<Data::BibtexCollection*>(m_coll.data());
  m_dupes = c->duplicateBibtexKeys();

  m_filterButton->setEnabled(false);
  if(m_dupes.isEmpty())  {
    m_dupeLabel->setComment(i18n("There are no duplicate citation keys."));
    m_filterButton->setEnabled(false);
  } else {
    m_dupeLabel->setComment(i18np("There is %1 duplicate citation key.", "There are %1 duplicate citation keys.", m_dupes.count()));
    m_filterButton->setEnabled(true);
  }
}

void BibtexKeyDialog::slotFilterDuplicates() {
  if(!m_coll || m_dupes.isEmpty()) {
    return;
  }

  FilterPtr filter(new Filter(Filter::MatchAny));

  QSet<QString> keys;
  foreach(Data::EntryPtr entry, m_dupes) {
    const QString key = entry->field(QStringLiteral("bibtex-key"));
    if(!keys.contains(key)) {
      filter->append(new FilterRule(QStringLiteral("bibtex-key"), key, FilterRule::FuncEquals));
      keys << key;
    }
  }

  if(!filter->isEmpty()) {
    emit signalUpdateFilter(filter);
  }
}
