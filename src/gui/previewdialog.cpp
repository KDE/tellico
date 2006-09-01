/***************************************************************************
    copyright            : (C) 2006 by Robby Stephenson
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

#include <klocale.h>
#include <ktempdir.h>
#include <khtmlview.h>

using Tellico::GUI::PreviewDialog;

PreviewDialog::PreviewDialog(QWidget* parent_)
        : KDialogBase(parent_, "template preview dialog", false /* modal */,
                      i18n("Template Preview"), KDialogBase::Ok)
        , m_tempDir(new KTempDir()) {
  m_tempDir->setAutoDelete(true);
  connect(this, SIGNAL(finished()), SLOT(delayedDestruct()));

  m_view = new EntryView(this);
  setMainWidget(m_view->view());
  setInitialSize(QSize(600, 500));
}

PreviewDialog::~PreviewDialog() {
  delete m_tempDir;
  m_tempDir = 0;
}

QString PreviewDialog::tempDir() const {
  return m_tempDir->name();
}

void PreviewDialog::setXSLTFile(const QString& file_) {
  m_view->setXSLTFile(file_);
}

void PreviewDialog::setXSLTOptions(const StyleOptions& options_) {
  m_view->setXSLTOptions(options_);
}

void PreviewDialog::showEntry(Data::EntryPtr entry_) {
  m_view->showEntry(entry_);
}

#include "previewdialog.moc"
