/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#include "stringmapdialog.h"
#include "stringmapwidget.h"

#include <QHeaderView>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>

using Tellico::StringMapDialog;

StringMapDialog::StringMapDialog(const QMap<QString, QString>& map_, QWidget* parent_, bool modal_/*=false*/)
    : QDialog(parent_) {
  setModal(modal_);

  QVBoxLayout* mainLayout = new QVBoxLayout(this);
  setLayout(mainLayout);
  m_widget = new GUI::StringMapWidget(map_, this);
  mainLayout->addWidget(m_widget);

  QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
  QPushButton* okButton = buttonBox->button(QDialogButtonBox::Ok);
  okButton->setDefault(true);
  okButton->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return));
  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  mainLayout->addWidget(buttonBox);

  setMinimumWidth(400);
}

void StringMapDialog::setLabels(const QString& label1_, const QString& label2_) {
  m_widget->setLabels(label1_, label2_);
}

QMap<QString, QString> StringMapDialog::stringMap() {
  return m_widget->stringMap();
}
