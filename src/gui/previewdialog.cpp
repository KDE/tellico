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
#include "../images/imagefactory.h"
#include "../utils/styleoptions.h"
#include "../utils/string_utils.h"

#include <KLocalizedString>

#include <QTemporaryDir>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QStandardPaths>

using Tellico::GUI::PreviewDialog;

PreviewDialog::PreviewDialog(QWidget* parent_)
        : QDialog(parent_)
        , m_tempDir(new QTemporaryDir()) {
  setModal(false);
  setWindowTitle(i18n("Template Preview"));

  QVBoxLayout* mainLayout = new QVBoxLayout;
  setLayout(mainLayout);

  m_view = new EntryView(this);
  mainLayout->addWidget(m_view);

  QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
  QPushButton* okButton = buttonBox->button(QDialogButtonBox::Ok);
  okButton->setDefault(true);
  okButton->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return));
  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  mainLayout->addWidget(buttonBox);

  resize(QSize(800, 600));

  m_tempDir->setAutoRemove(true);
}

PreviewDialog::~PreviewDialog() {
  delete m_tempDir;
  m_tempDir = nullptr;
}

void PreviewDialog::setXSLTFile(const QString& file_) {
  m_view->setXSLTFile(file_);
}

void PreviewDialog::setXSLTOptions(Tellico::StyleOptions options_) {
  options_.imgDir = m_tempDir->path(); // images always get written to temp dir
  m_view->setXSLTOptions(options_);
}

void PreviewDialog::showPreview(Tellico::Data::CollPtr coll_) {
  // always want to include a url to show link color too
  bool hasLink = false;
  Data::EntryPtr e(new Data::Entry(coll_));
  foreach(Data::FieldPtr f, coll_->fields()) {
    if(f->name() == QLatin1String("title")) {
      e->setField(f, coll_->title());
    } else if(f->type() == Data::Field::Image) {
      const QString imagePath = QStandardPaths::locate(
          QStandardPaths::GenericDataLocation,
          QStringLiteral("doc/HTML/en/tellico/main-window.png")
      );
      if(!imagePath.isEmpty()) {
        const QUrl u = QUrl::fromLocalFile(imagePath);
        // addImage(url, quiet, referer, link)
        e->setField(f, ImageFactory::addImage(u, false, QUrl(), true));
      }
      continue;
    } else if(f->type() == Data::Field::Choice) {
      e->setField(f, f->allowed().front());
    } else if(f->type() == Data::Field::Number) {
      e->setField(f, QStringLiteral("1"));
    } else if(f->type() == Data::Field::Bool) {
      e->setField(f, QStringLiteral("true"));
    } else if(f->type() == Data::Field::Rating) {
      e->setField(f, QStringLiteral("4"));
    } else if(f->type() == Data::Field::URL) {
      e->setField(f, QStringLiteral("https://tellico-project.org"));
      hasLink = true;
    } else if(f->type() == Data::Field::Table) {
      QStringList values;
      bool ok;
      int ncols = Tellico::toUInt(f->property(QStringLiteral("columns")), &ok);
      ncols = qMax(ncols, 1);
      for(int ncol = 1; ncol <= ncols; ++ncol) {
        const auto prop = QStringLiteral("column%1").arg(ncol);
        const auto col = f->property(prop);
        values += col.isEmpty() ? prop : col;
      }
      QStringList rows(3, values.join(FieldFormat::columnDelimiterString()));
      e->setField(f, rows.join(FieldFormat::columnDelimiterString()));
    } else if(f->type() == Data::Field::Para) {
      e->setField(f, QStringLiteral("Lorem ipsum dolor sit amet, consectetur adipiscing elit, "
                                    "sed do eiusmod tempor incididunt ut labore et dolore magna "
                                    "aliqua. Ut enim ad minim veniam, quis nostrud exercitation "
                                    "ullamco laboris nisi ut aliquip ex ea commodo consequat. "
                                    "Duis aute irure dolor in reprehenderit in voluptate velit "
                                    "esse cillum dolore eu fugiat nulla pariatur. Excepteur "
                                    "sint occaecat cupidatat non proident, sunt in culpa qui "
                                    "officia deserunt mollit anim id est laborum."));
    } else {
      e->setField(f, f->title());
    }
  }
  if(!hasLink) {
    Data::FieldPtr f(new Data::Field(QStringLiteral("url"),
                                     QStringLiteral("URL"),
                                     Data::Field::URL));
    f->setCategory(i18n("General"));
    coll_->addField(f);
    e->setField(f, QStringLiteral("https://tellico-project.org"));
  }

  m_view->showEntry(e);
}
