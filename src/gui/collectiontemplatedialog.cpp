/***************************************************************************
    Copyright (C) 2022 Robby Stephenson <robby@periapsis.org>
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

#include "collectiontemplatedialog.h"
#include "ui_collectiontemplatedialog.h"

#include <KLocalizedString>
#include <KIconButton>

using Tellico::CollectionTemplateDialog;

class CollectionTemplateDialog::Private {
public:
  Private(CollectionTemplateDialog* parent) : q(parent) { }
  void init();

  CollectionTemplateDialog* const q;
  QString templateName;
  Ui::CollectionTemplateDialog ui;
};

void CollectionTemplateDialog::Private::init() {
  ui.setupUi(q);
}

CollectionTemplateDialog::CollectionTemplateDialog(QWidget* parent_)
    : QDialog(parent_)
    , d(new Private(this)) {
  d->init();
  d->ui.labelName->setText(i18n("Template Name:"));
  d->ui.labelComment->setText(i18n("Comment:"));
  d->ui.labelIcon->setText(i18n("Icon:"));
  d->ui.editIcon->setIconSize(KIconLoader::SizeMedium);
  d->ui.editIcon->setIconType(KIconLoader::NoGroup, KIconLoader::Any);
  setWindowTitle(i18n("Save As Collection Template"));
}

CollectionTemplateDialog::~CollectionTemplateDialog() = default;

QString CollectionTemplateDialog::templateName() const {
  return d->ui.editName->text().trimmed();
}

QString CollectionTemplateDialog::templateComment() const {
  return d->ui.editComment->text().trimmed();
}

QString CollectionTemplateDialog::templateIcon() const {
  return d->ui.editIcon->icon();
}
