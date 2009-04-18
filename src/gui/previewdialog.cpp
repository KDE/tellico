/***************************************************************************
    copyright            : (C) 2006-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
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

void PreviewDialog::setXSLTOptions(Tellico::StyleOptions options_) {
  options_.imgDir = m_tempDir->name(); // images always get written to temp dir
  ImageFactory::createStyleImages(options_);
  m_view->setXSLTOptions(options_);
}

void PreviewDialog::showEntry(Tellico::Data::EntryPtr entry_) {
  m_view->showEntry(entry_);
}

#include "previewdialog.moc"
