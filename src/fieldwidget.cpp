/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "fieldwidget.h"
#include "collection.h"
#include "isbnvalidator.h"
#include "utils.h"
#include "imagewidget.h"
#include "fieldcompletion.h"

#include <kcompletion.h>
#include <kdebug.h>
#include <klineedit.h>
#include <kcombobox.h>
#include <kurlrequester.h>
#include <kurllabel.h>
#include <krun.h>
#include <kpushbutton.h>

#include <qcheckbox.h>
#include <qtextedit.h>
#include <qlayout.h>
#include <qwhatsthis.h>
#include <qtable.h>

static const int MIN_TABLE_ROWS = 5;

using Bookcase::FieldWidget;

QGuardedPtr<Bookcase::Data::Collection> FieldWidget::s_coll = 0;
const QRegExp FieldWidget::s_semiColon = QRegExp(QString::fromLatin1("\\s*;\\s*"));
const QRegExp FieldWidget::s_comma = QRegExp(QString::fromLatin1("\\s*,\\s*"));

FieldWidget::FieldWidget(const Data::Field* const field_, QWidget* parent_, const char* name_/*=0*/)
    : QWidget(parent_, name_), m_type(field_->type()), m_run(0) {
  QHBoxLayout* l = new QHBoxLayout(this, 0, 4); // parent, margin, spacing
  l->addSpacing(4);

  if(m_type == Data::Field::URL) {
    // set URL to null for now
    m_label = new KURLLabel(QString::null, field_->title() + QString::fromLatin1(":"), this);
  } else {
    m_label = new QLabel(field_->title() + QString::fromLatin1(":"), this);
  }
  m_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  m_label->setFixedWidth(m_label->sizeHint().width());
  l->addWidget(m_label);

  // expands indicates if the edit widget should expand to full width of widget
  m_expands = (m_type == Data::Field::Line
               || m_type == Data::Field::Para
               || m_type == Data::Field::Number
               || m_type == Data::Field::URL
               || m_type == Data::Field::Table
               || m_type == Data::Field::Table2
               || m_type == Data::Field::Image);

  switch (m_type) {
    case Data::Field::Line:
      {
        KLineEdit* kl = new KLineEdit(this);
        connect(kl, SIGNAL(textChanged(const QString&)), SIGNAL(modified()));
        // connect(kl, SIGNAL(returnPressed(const QString&)), this, SLOT(slotHandleReturn()));

        if(field_->flags() & Data::Field::AllowCompletion) {
          FieldCompletion* completion = new FieldCompletion(field_->flags() & Data::Field::AllowMultiple);
          if(s_coll) {
            completion->setItems(s_coll->valuesByFieldName(field_->name()));
          }
          completion->setIgnoreCase(true);
          kl->setCompletionObject(completion);
          kl->setAutoDeleteCompletionObject(true);
        }

        if(field_->name() == QString::fromLatin1("isbn")) {
          kl->setValidator(new ISBNValidator(this));
        }

        m_editWidget = kl;
      }
      break;

    case Data::Field::Para:
      {
        QTextEdit* te = new QTextEdit(this);
        te->setTextFormat(Qt::PlainText);
        connect(te, SIGNAL(modificationChanged(bool)), SIGNAL(modified()));
        m_editWidget = te;
      }
      break;

    case Data::Field::Choice:
      {
        KComboBox* kc = new KComboBox(this);
        connect(kc, SIGNAL(activated(int)), SIGNAL(modified()));
        // always have empty choice
        kc->insertItem(QString::null);
        kc->insertStringList(field_->allowed());
        kc->setMinimumWidth(5*fontMetrics().maxWidth());
        m_editWidget = kc;
      }
      break;

    case Data::Field::Bool:
      {
        QCheckBox* cb = new QCheckBox(this);
        connect(cb, SIGNAL(clicked()), SIGNAL(modified()));
        m_editWidget = cb;
      }
      break;

    case Data::Field::Number:
      {
        KLineEdit* kl = new KLineEdit(this);
        connect(kl, SIGNAL(textChanged(const QString&)), SIGNAL(modified()));
        // connect(kl, SIGNAL(returnPressed(const QString&)), this, SLOT(slotHandleReturn()));

        if(field_->flags() & Data::Field::AllowMultiple) {
          // regexp is any number of digits followed optionally by any number of
          // groups of a semi-colon followed optionally by a space, followed by digits
          QRegExp rx(QString::fromLatin1("-?\\d*(; ?-?\\d*)*"));
          kl->setValidator(new QRegExpValidator(rx, this));
        } else {
          kl->setValidator(new QIntValidator(this));
        }

        m_editWidget = kl;
      }
      break;

    case Data::Field::URL:
      {
        KURLRequester* ku = new KURLRequester(this);
        connect(ku, SIGNAL(textChanged(const QString&)), SIGNAL(modified()));
        connect(ku, SIGNAL(textChanged(const QString&)), m_label, SLOT(setURL(const QString&)));
        connect(m_label, SIGNAL(leftClickedURL(const QString&)), SLOT(slotOpenURL(const QString&)));
        m_editWidget = ku;
      }
      break;

    case Data::Field::Table:
      {
        QTable* qt = new QTable(MIN_TABLE_ROWS, 1, this);
        qt->setTopMargin(0);
        qt->horizontalHeader()->hide();
        qt->verticalHeader()->setClickEnabled(false);
        qt->verticalHeader()->setResizeEnabled(false);
//        qt->setMaximumHeight(MIN_TABLE_ROWS*qt->rowHeight(0));
//        qt->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        qt->setSelectionMode(QTable::Single);
        qt->setDragEnabled(false);
        qt->setRowMovingEnabled(false);
        qt->setColumnMovingEnabled(false);
        qt->setColumnStretchable(0, true);
        qt->adjustColumn(0);
        connect(qt, SIGNAL(valueChanged(int, int)), SIGNAL(modified()));
        connect(qt, SIGNAL(currentChanged(int, int)), SLOT(slotCheckRows(int, int)));
        m_editWidget = qt;
      }
      break;

    case Data::Field::Table2:
      {
        QTable* qt = new QTable(MIN_TABLE_ROWS, 2, this);
        qt->setTopMargin(0);
        qt->horizontalHeader()->hide();
        qt->verticalHeader()->setClickEnabled(false);
        qt->verticalHeader()->setResizeEnabled(false);
//        qt->setMaximumHeight(MIN_TABLE_ROWS*qt->rowHeight(0));
//        qt->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        qt->setSelectionMode(QTable::NoSelection);
        qt->setDragEnabled(false);
        qt->setRowMovingEnabled(false);
        qt->setColumnMovingEnabled(false);
        qt->setColumnStretchable(1, true);
        qt->adjustColumn(1);
        connect(qt, SIGNAL(valueChanged(int, int)), SIGNAL(modified()));
        connect(qt, SIGNAL(currentChanged(int, int)), SLOT(slotCheckRows(int, int)));
        m_editWidget = qt;
      }
      break;

    case Data::Field::Image:
      {
        ImageWidget* w = new ImageWidget(this);
        w->slotClear();
        connect(w, SIGNAL(signalModified()), SIGNAL(modified()));
        m_editWidget = w;
      }
      break;

    case Data::Field::ReadOnly:
      kdDebug() << "FieldWidget() - read-only field, this shouldn't have been called" << endl;
      break;

    default:
      kdDebug() << "FieldWidget() - unknown field type  ("
                << field_->type() << ") named " << field_->name() << endl;
      break;
  } // end switch

  l->addWidget(m_editWidget);
  if(m_expands) {
    l->setStretchFactor(m_editWidget, 1);
  } else {
    // if it's not a text edit widget, then add a stretch at the end so the comboboxes aren't massive
    l->addStretch(1);
  }

  m_editMultiple = new QCheckBox(this);
  m_editMultiple->setChecked(true);
  m_editMultiple->setFixedWidth(m_editMultiple->sizeHint().width()); // don't let it have any extra space
  m_editMultiple->hide();
  connect(m_editMultiple, SIGNAL(toggled(bool)), SLOT(setEnabled(bool)));
  l->addWidget(m_editMultiple);

  QWhatsThis::add(m_label, field_->description());
  QWhatsThis::add(m_editWidget, field_->description());
  m_label->setBuddy(m_editWidget);
}

FieldWidget::~FieldWidget() {
  if(m_run) {
    m_run->abort();
  }
}

void FieldWidget::clear() {
  switch (m_type) {
    case Data::Field::Line:
    case Data::Field::Number:
      {
        KLineEdit* kl = static_cast<KLineEdit*>(m_editWidget);
        kl->clear();
      }
      break;

    case Data::Field::Para:
      {
        QTextEdit* te = static_cast<QTextEdit*>(m_editWidget);
        te->clear();
      }
      break;

    case Data::Field::Choice:
      {
        KComboBox* kc = static_cast<KComboBox*>(m_editWidget);
        kc->setCurrentItem(0);
      }
      break;

    case Data::Field::Bool:
      {
        QCheckBox* cb = static_cast<QCheckBox*>(m_editWidget);
        cb->setChecked(false);
      }
      break;

    case Data::Field::URL:
      {
        KURLRequester* ku = static_cast<KURLRequester*>(m_editWidget);
        ku->clear();
      }
      break;

    case Data::Field::Table:
    case Data::Field::Table2:
      {
        QTable* qt = static_cast<QTable*>(m_editWidget);
        for(int row = 0; row < qt->numRows(); ++row) {
          for(int col = 0; col < qt->numCols(); ++col) {
            qt->setText(row, col, QString::null);
          }
        }
      }
      break;

    case Data::Field::Image:
      {
        ImageWidget* w = static_cast<ImageWidget*>(m_editWidget);
        w->slotClear();
      }
      break;

    default:
      break;
  }

  editMultiple(false);
}

QString FieldWidget::text() const {
  QString text;

  switch(m_type) {
    case Data::Field::Line:
    case Data::Field::Number:
      {
        KLineEdit* kl = static_cast<KLineEdit*>(m_editWidget);
        text = kl->text();
        text.replace(s_semiColon, QString::fromLatin1("; "));
        text.replace(s_comma, QString::fromLatin1(", "));
        text.simplifyWhiteSpace();
      }
      break;

    case Data::Field::Para:
      {
        QTextEdit* te = static_cast<QTextEdit*>(m_editWidget);
        text = te->text();
      }
      break;

    case Data::Field::Choice:
      {
        KComboBox* kc = static_cast<KComboBox*>(m_editWidget);
        text = kc->currentText();
      }
      break;

    case Data::Field::Bool:
      {
        QCheckBox* cb = static_cast<QCheckBox*>(m_editWidget);
        if(cb->isChecked()) {
          text = QString::fromLatin1("true");
        }
      }
      break;

    case Data::Field::URL:
      {
        KURLRequester* ku = static_cast<KURLRequester*>(m_editWidget);
        text = ku->url();
      }
      break;

    case Data::Field::Table:
    case Data::Field::Table2:
      {
        QTable* qt = static_cast<QTable*>(m_editWidget);
         QString delim, rowStr;
        for(int row = 0; row < qt->numRows(); ++row) {
          rowStr.truncate(0);
          for(int col = 0; col < qt->numCols(); ++col) {
            rowStr += qt->text(row, col).simplifyWhiteSpace();
            if(!rowStr.isEmpty() && col < qt->numCols()-1) {
              rowStr += QString::fromLatin1("::");
            }
          }
          if(rowStr.isEmpty() && text.isEmpty()) {
            delim += QString::fromLatin1("; ");
          } else if(!rowStr.isEmpty()) {
            text += delim + rowStr + QString::fromLatin1("; ");
          }
        }
        if(!text.isEmpty()) {
          text.truncate(text.length()-2); // remove last semi-colon and space
        }

        // now reduce number of rows if necessary
        while(qt->numRows() > MIN_TABLE_ROWS
              && qt->text(qt->numRows()-1, 0).isEmpty()
              && qt->text(qt->numRows()-1, qt->numCols()-1).isEmpty()) {
          qt->removeRow(qt->numRows()-1);
        }
      }
      break;

    case Data::Field::Image:
      {
        ImageWidget* w = static_cast<ImageWidget*>(m_editWidget);
        text = w->id();
      }
      break;

    default:
      break;
  }
  return text;
}

void FieldWidget::setText(const QString& text_) {
  m_editWidget->blockSignals(true);
  blockSignals(true);

  switch (m_type) {
    case Data::Field::Line:
    case Data::Field::Number:
      {
        KLineEdit* kl = static_cast<KLineEdit*>(m_editWidget);
        kl->setText(text_);
      }
      break;

    case Data::Field::Para:
      {
        QTextEdit* te = static_cast<QTextEdit*>(m_editWidget);
        te->setText(text_);
      }
      break;

    case Data::Field::Choice:
      {
        KComboBox* kc = static_cast<KComboBox*>(m_editWidget);
        for(int i = 0; i < kc->count(); ++i) {
          if(kc->text(i) == text_) {
            kc->setCurrentItem(i);
            break;
          }
        }
      }
      break;

    case Data::Field::Bool:
      {
        QCheckBox* cb = static_cast<QCheckBox*>(m_editWidget);
        // be lax, don't have to check for "1" or "true"
        // just check for a non-empty string
        cb->setChecked(!text_.isEmpty());
      }
      break;

    case Data::Field::URL:
      {
        KURLRequester* ku = static_cast<KURLRequester*>(m_editWidget);
        ku->setURL(text_);
        static_cast<KURLLabel*>(m_label)->setURL(text_);
      }
      break;

    case Data::Field::Table:
      {
        QTable* qt = static_cast<QTable*>(m_editWidget);
        QStringList list = Data::Field::split(text_, true);
        if(static_cast<int>(list.count()) > qt->numRows()) {
          qt->insertRows(qt->numRows(), list.count()-qt->numRows());
        }
        for(unsigned row = 0; row < list.count(); ++row) {
          qt->setText(row, 0, list[row]);
        }
        qt->adjustColumn(0);
      }
      break;

    case Data::Field::Table2:
      {
        QTable* qt = static_cast<QTable*>(m_editWidget);
        QStringList list = Data::Field::split(text_, true);
        if(static_cast<int>(list.count()) > qt->numRows()) {
          qt->insertRows(qt->numRows(), list.count()-qt->numRows());
        }
        for(unsigned row = 0; row < list.count(); ++row) {
          qt->setText(row, 0, list[row].section(QString::fromLatin1("::"), 0, 0));
          qt->setText(row, 1, list[row].section(QString::fromLatin1("::"), 1));
        }
        qt->adjustColumn(1);
      }
      break;

    case Data::Field::Image:
      {
        ImageWidget* w = static_cast<ImageWidget*>(m_editWidget);
        w->setImage(text_);
      }
      break;

    default:
      break;
  }

  blockSignals(false);
  m_editWidget->blockSignals(false);
}

bool FieldWidget::isEnabled() const {
  return m_editWidget->isEnabled();
}

void FieldWidget::setEnabled(bool enabled_) {
  if(enabled_ == m_editWidget->isEnabled()) {
    return;
  }

  m_editWidget->setEnabled(enabled_);
  m_editMultiple->setChecked(enabled_);
}

int FieldWidget::labelWidth() const {
  return m_label->sizeHint().width();
}

void FieldWidget::setLabelWidth(int width_) {
  m_label->setFixedWidth(width_);
}

void FieldWidget::addCompletionObjectItem(const QString& text_) {
  if(m_type == Data::Field::Line) {
    KLineEdit* kl = dynamic_cast<KLineEdit*>(m_editWidget);
    kl->completionObject()->addItem(text_);
  }
}

bool FieldWidget::expands() const {
  return m_expands;
}

void FieldWidget::editMultiple(bool show_) {
// QWidget::isShown() is new in Qt 3.1
// QWidget::isVisible() is true only if the tab holding this widget is visible
#if QT_VERSION >= 0x30100
  if(show_ == m_editMultiple->isShown()) {
    return;
  }
#endif

  // TODO: maybe modified should only be signaled when the button is toggle on
  if(show_) {
    m_editMultiple->show();
    connect(m_editMultiple, SIGNAL(clicked()),
            this, SIGNAL(modified()));
  } else {
    m_editMultiple->hide();
    disconnect(m_editMultiple, SIGNAL(clicked()),
               this, SIGNAL(modified()));
  }
  // the widget length needs to be updated since it gets shorter
  m_editWidget->updateGeometry();
}

// TODO: someday, make just the actual string highlighted
void FieldWidget::setHighlighted(const QString&) const {
  switch (m_type) {
    case Data::Field::Line:
    case Data::Field::Number:
      {
        KLineEdit* kl = static_cast<KLineEdit*>(m_editWidget);
        kl->selectAll();
      }
      break;

    case Data::Field::Para:
      {
        QTextEdit* te = static_cast<QTextEdit*>(m_editWidget);
        // TODO: it would be nice if this selected only the highlighted text
        te->selectAll();
      }
      break;

    case Data::Field::URL:
      {
        KURLRequester* ku = static_cast<KURLRequester*>(m_editWidget);
        ku->lineEdit()->selectAll();
      }
      break;

    default:
      break;
  }
}

void FieldWidget::slotOpenURL(const QString& url_) {
  KURL url(url_);
  if(url.isValid()) {
    m_run = new KRun(url);
  }
}

void FieldWidget::slotCheckRows(int row_, int) {
  QTable* qt = dynamic_cast<QTable*>(m_editWidget);
  if(qt && row_ == qt->numRows()-1) { // if is last row
    qt->insertRows(qt->numRows());
    qt->adjustColumn(qt->numCols()-1);
  }
}

void FieldWidget::updateField(Data::Field* newField_, Data::Field* oldField_) {
  m_label->setText(newField_->title() + QString::fromLatin1(":"));
  updateGeometry();
  QWhatsThis::add(m_label, newField_->description());
  QWhatsThis::add(m_editWidget, newField_->description());
  // TODO: fix if the validator might have changed
  if(m_type == Data::Field::Choice) {
    KComboBox* kc = static_cast<KComboBox*>(m_editWidget);
    kc->clear();
    // always have empty choice
    kc->insertItem(QString::null);
    kc->insertStringList(newField_->allowed());
  } else if(m_type == Data::Field::Line) {
    KLineEdit* kl = static_cast<KLineEdit*>(m_editWidget);
    bool wasComplete = (oldField_->flags() & Data::Field::AllowCompletion);
    bool isComplete = (newField_->flags() & Data::Field::AllowCompletion);
    if(!wasComplete && isComplete) {
      FieldCompletion* completion = new FieldCompletion(isComplete);
      if(s_coll) {
        completion->setItems(s_coll->valuesByFieldName(newField_->name()));
      }
      completion->setIgnoreCase(true);
      kl->setCompletionObject(completion);
      kl->setAutoDeleteCompletionObject(true);
    } else if(wasComplete && !isComplete) {
      kl->completionObject()->clear();
    }
  }
}
