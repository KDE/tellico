/***************************************************************************
    Copyright (C) 2006-2009 Robby Stephenson <robby@periapsis.org>
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

#include "previewdialog.h"
#include "../entryview.h"
#include "../entry.h"
#include "../images/imagefactory.h" // for StyleOptions

#include <klocale.h>
#include <ktempdir.h>
#include <khtmlview.h>

using Tellico::GUI::PreviewDialog;

PreviewDialog::PreviewDialog(QWidget* parent_)
        : KDialog(parent_)
        , m_tempDir(new KTempDir()) {
  setModal(false);
  setCaption(i18n("Template Preview"));
  setButtons(Ok);

  m_tempDir->setAutoRemove(true);
  connect(this, SIGNAL(finished()), SLOT(delayedDestruct()));

  m_view = new EntryView(this);
  setMainWidget(m_view->view());
  setInitialSize(QSize(600, 500));
}

PreviewDialog::~PreviewDialog() {
  delete m_tempDir;
  m_tempDir = 0;
}

void PreviewDialog::setXSLTFile(const QString& file_) {
  m_view->setXSLTFile(file_);
}

void PreviewDialog::setXSLTOptions(int collectionType_, Tellico::StyleOptions options_) {
  options_.imgDir = m_tempDir->name(); // images always get written to temp dir
  ImageFactory::createStyleImages(collectionType_, options_);
  m_view->setXSLTOptions(options_);
}

void PreviewDialog::showEntry(Tellico::Data::EntryPtr entry_) {
  m_view->showEntry(entry_);
}

