/***************************************************************************
                            bcattributewidget.cpp
                             -------------------
    begin                : Sun Apr 13 2003
    copyright            : (C) 2003 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "bcattributewidget.h"
#include "bcuniteditwidget.h"
#include "bccollection.h"
#include "isbnvalidator.h"
#include "bcutils.h"

#include <kcompletion.h>
#include <kdebug.h>
#include <klineedit.h>
#include <kcombobox.h>
#include <kurlrequester.h>
#include <kurllabel.h>
#include <krun.h>

#include <qtextedit.h>
#include <qlayout.h>
#include <qwhatsthis.h>

BCAttributeWidget::BCAttributeWidget(BCAttribute* att_, QWidget* parent_, const char* name_/*=0*/)
    : QHBox(parent_, name_), m_name(att_->name()), m_type(att_->type()), m_run(0) {
  setSpacing(4);

  // do this to just add a little space in front of the widget
  // a hack, I know
  (void) new QWidget(this);

  if(m_type == BCAttribute::URL) {
    // set URL to null for now
    m_label = new KURLLabel(QString::null, att_->title() + QString::fromLatin1(":"), this);
  } else {
    m_label = new QLabel(att_->title() + QString::fromLatin1(":"), this);
  }
  m_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  m_label->setFixedWidth(m_label->sizeHint().width());
  QWhatsThis::add(m_label, att_->description());

  m_isTextEdit = (m_type == BCAttribute::Line
               || m_type == BCAttribute::Para
               || m_type == BCAttribute::Year
               || m_type == BCAttribute::URL);

  // declare these since creating them within the case switches causes warnings
  KLineEdit* kl;
  QTextEdit* te;
  KComboBox* kc;
  QCheckBox* cb;
  KURLRequester* ku;

  switch (m_type) {
    case BCAttribute::Line:
      kl = new KLineEdit(this);
      connect(kl, SIGNAL(textChanged(const QString&)), SIGNAL(modified()));
      // connect(kl, SIGNAL(returnPressed(const QString&)), this, SLOT(slotHandleReturn()));

      if(att_->flags() & BCAttribute::AllowCompletion) {
        BCUnitEditWidget* w = BCUnitEditWidgetAncestor(parent());
        kl->completionObject()->setItems(w->m_currColl->valuesByAttributeName(att_->name()));
        kl->setAutoDeleteCompletionObject(true);
      }

      if(att_->name() == QString::fromLatin1("isbn")) {
        ISBNValidator* isbn = new ISBNValidator(this);
        kl->setValidator(isbn);
      }
      m_editWidget = kl;
      break;

    case BCAttribute::Para:
      te = new QTextEdit(this);
      te->setTextFormat(Qt::PlainText);
      connect(te, SIGNAL(modificationChanged(bool)), SIGNAL(modified()));
      m_editWidget = te;
      break;

    case BCAttribute::Choice:
      kc = new KComboBox(this);
      connect(kc, SIGNAL(activated(int)), SIGNAL(modified()));
      // always have empty choice
      kc->insertItem(QString::null);
      kc->insertStringList(att_->allowed());
//      kc->setFixedWidth(QMAX(kc->sizeHint().width(), fontMetrics().maxWidth()*5));
      kc->setMinimumWidth(fontMetrics().maxWidth()*5);
      // event filter to capture the return key
      //kc->installEventFilter(this);
      m_editWidget = kc;
      break;

    case BCAttribute::Bool:
      cb = new QCheckBox(this);
      connect(cb, SIGNAL(clicked()), SIGNAL(modified()));
      // event filter to capture the return key
      //cb->installEventFilter(this);
      m_editWidget = cb;
      break;

    case BCAttribute::Year:
      kl = new KLineEdit(this);
      connect(kl, SIGNAL(textChanged(const QString&)), SIGNAL(modified()));
      // connect(kl, SIGNAL(returnPressed(const QString&)), this, SLOT(slotHandleReturn()));

      if(att_->flags() & BCAttribute::AllowMultiple) {
        // regexp is 4 digits followed optionally by any number of
        // groups of a semi-colon, followed optionally by a space, followed by 4 digits
        QRegExp rx(QString::fromLatin1("\\d{4}(;\\s?\\d{4})*"));
        kl->setValidator(new QRegExpValidator(rx, this));
      } else {
        kl->setMaxLength(4);
        kl->setValidator(new QIntValidator(1000, 9999, this));
      }

      m_editWidget = kl;
      break;

    case BCAttribute::ReadOnly:
      kdDebug() << "BCAttributeWidget() - read-only attribute, this shouldn't have been called" << endl;
      break;

    case BCAttribute::URL:
      ku = new KURLRequester(this);
      connect(ku, SIGNAL(textChanged(const QString&)), SIGNAL(modified()));
      connect(ku, SIGNAL(textChanged(const QString&)), m_label, SLOT(setURL(const QString&)));
      connect(m_label, SIGNAL(leftClickedURL(const QString&)), SLOT(openURL(const QString&)));
      m_editWidget = ku;
      break;

    default:
      kdDebug() << "BCAttributeWidget() - unknown attribute type  ("
                << att_->type() << ") named " << att_->name() << endl;
      break;
  } // end switch

  if(m_isTextEdit) {
    setStretchFactor(m_editWidget, 1);
  } else {
    // if it's not a text edit widget, then add a stretch at the end so the comboboxes aren't massive
    // this doesn't work, why?
    // QBoxLayout* l = static_cast<QBoxLayout*>(layout());
    // l->insertStretch(2, 1);
    QWidget* w = new QWidget(this);
    setStretchFactor(w, 1);
  }

  m_editMultiple = new QCheckBox(this);
  m_editMultiple->setChecked(true);
  m_editMultiple->setFixedWidth(m_editMultiple->sizeHint().width()); // don't let it have any extra space
  m_editMultiple->setPaletteForegroundColor(QColor("red"));
  m_editMultiple->hide();
  connect(m_editMultiple, SIGNAL(toggled(bool)), SLOT(setEnabled(bool)));

  QWhatsThis::add(m_editWidget, att_->description());
  m_label->setBuddy(m_editWidget);
}

BCAttributeWidget::~BCAttributeWidget() {
  if(m_run) {
    m_run->abort();
  }
}

void BCAttributeWidget::clear() {
  // declare these since creating them within the case switches causes warnings
  KLineEdit* kl;
  QTextEdit* te;
  KComboBox* kc;
  QCheckBox* cb;
  KURLRequester* ku;

  switch (m_type) {
    case BCAttribute::Line:
      kl = dynamic_cast<KLineEdit*>(m_editWidget);
      if(kl) {
        kl->clear();
      }
      break;

    case BCAttribute::Para:
      te = dynamic_cast<QTextEdit*>(m_editWidget);
      if(te) {
        te->clear();
      }
      break;

    case BCAttribute::Choice:
      kc = dynamic_cast<KComboBox*>(m_editWidget);
      if(kc) {
        kc->setCurrentItem(0);
      }
      break;

    case BCAttribute::Bool:
      cb = dynamic_cast<QCheckBox*>(m_editWidget);
      if(cb) {
        cb->setChecked(false);
      }
      break;

    case BCAttribute::Year:
      kl = dynamic_cast<KLineEdit*>(m_editWidget);
      if(kl) {
        kl->clear();
      }
      break;

    case BCAttribute::URL:
      ku = dynamic_cast<KURLRequester*>(m_editWidget);
      if(ku) {
        ku->clear();
      }
      break;

    default:
      break;
  }

  editMultiple(false);
}

QString BCAttributeWidget::text() const {
  QString text;

  // declare these since creating them within the case switches causes warnings
  KLineEdit* kl;
  QTextEdit* te;
  KComboBox* kc;
  QCheckBox* cb;
  KURLRequester* ku;

  switch(m_type) {
    case BCAttribute::Line:
      kl = dynamic_cast<KLineEdit*>(m_editWidget);
      if(kl) {
        text = kl->text();
        text.replace(QRegExp(QString::fromLatin1(";\\s*")), QString::fromLatin1("; "));
        text.replace(QRegExp(QString::fromLatin1(",\\s*")), QString::fromLatin1(", "));
        text.stripWhiteSpace();
      }
      break;

    case BCAttribute::Para:
      te = dynamic_cast<QTextEdit*>(m_editWidget);
      if(te) {
        text = te->text();
      }
      break;

    case BCAttribute::Choice:
      kc = dynamic_cast<KComboBox*>(m_editWidget);
      if(kc) {
        text = kc->currentText();
      }
      break;

    case BCAttribute::Bool:
      cb = dynamic_cast<QCheckBox*>(m_editWidget);
      if(cb && cb->isChecked()) {
        text = QString::fromLatin1("1");
      }
      break;

    case BCAttribute::Year:
      kl = dynamic_cast<KLineEdit*>(m_editWidget);
      if(kl) {
        text = kl->text();
        text.replace(QRegExp(QString::fromLatin1(";\\s*")), QString::fromLatin1("; "));
        text.replace(QRegExp(QString::fromLatin1(",\\s*")), QString::fromLatin1(", "));
        text.stripWhiteSpace();
      }
      break;

    case BCAttribute::URL:
      ku = dynamic_cast<KURLRequester*>(m_editWidget);
      if(ku) {
        text = ku->url();
      }

    default:
      break;
  }
  return text;
}

void BCAttributeWidget::setText(const QString& text_) {
  // declare these since creating them within the case switches causes warnings
  KLineEdit* kl;
  QTextEdit* te;
  KComboBox* kc;
  QCheckBox* cb;
  KURLRequester* ku;

  m_editWidget->blockSignals(true);
  blockSignals(true);

  switch (m_type) {
    case BCAttribute::Line:
      kl = dynamic_cast<KLineEdit*>(m_editWidget);
      if(kl) {
        kl->setText(text_);
//        if(kl->validator()) {
//          QString text = text_;
//          kl->validator()->validate(text, 0);
//          if(text != text_) {
//            BCUnitEditWidget* w = BCUnitEditWidgetAncestor(parent());
//            w->slotSetModified();
//            kdDebug() << "Changed " << text_ << " to " << text << " for " << m_name << endl;
//            kl->setText(text);
//          }
//        }
      }
      break;

    case BCAttribute::Para:
      te = dynamic_cast<QTextEdit*>(m_editWidget);
      if(te) {
        te->setText(text_);
      }
      break;

    case BCAttribute::Choice:
      kc = dynamic_cast<KComboBox*>(m_editWidget);
      if(kc) {
        for(int i = 0; i < kc->count(); ++i) {
          if(kc->text(i) == text_) {
            kc->setCurrentItem(i);
            break;
          }
        }
      }
      break;

    case BCAttribute::Bool:
      cb = dynamic_cast<QCheckBox*>(m_editWidget);
      if(cb) {
        cb->setChecked(text_ == QString::fromLatin1("1"));
      }
      break;

    case BCAttribute::Year:
      kl = dynamic_cast<KLineEdit*>(m_editWidget);
      if(kl) {
        kl->setText(text_);
      }
      break;

    case BCAttribute::URL:
      ku = dynamic_cast<KURLRequester*>(m_editWidget);
      if(ku) {
        ku->setURL(text_);
      }

    default:
      break;
  }

  blockSignals(false);
  m_editWidget->blockSignals(false);
}

bool BCAttributeWidget::isEnabled() const {
  return m_editWidget->isEnabled();
}

void BCAttributeWidget::setEnabled(bool enabled_) {
  if(enabled_ == m_editWidget->isEnabled()) {
    return;
  }

  m_editWidget->setEnabled(enabled_);
  m_editMultiple->setChecked(enabled_);
}

int BCAttributeWidget::labelWidth() const {
  return m_label->width();
}

void BCAttributeWidget::setLabelWidth(int width_) {
  m_label->setFixedWidth(width_);
}

void BCAttributeWidget::addCompletionObjectItem(const QString& text_) {
  if(m_type == BCAttribute::Line) {
    KLineEdit* kl = dynamic_cast<KLineEdit*>(m_editWidget);
    kl->completionObject()->addItem(text_);
  }
}

bool BCAttributeWidget::isTextEdit() const {
  return m_isTextEdit;
}

void BCAttributeWidget::editMultiple(bool show_) {
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

void BCAttributeWidget::setHighlighted(const QString& highlight_) const {
  if(m_isTextEdit) {
    KLineEdit* kl;
    QTextEdit* te;
    KURLRequester* ku;

    switch (m_type) {
      case BCAttribute::Line:
        kl = dynamic_cast<KLineEdit*>(m_editWidget);
        if(kl) {
          kl->selectAll();
        }
        break;

      case BCAttribute::Para:
        te = dynamic_cast<QTextEdit*>(m_editWidget);
        if(te) {
          // TODO: it would be nice if this selected only the highlighted text
          te->selectAll();
        }
        break;

      case BCAttribute::Year:
        kl = dynamic_cast<KLineEdit*>(m_editWidget);
        if(kl) {
          kl->selectAll();
        }
        break;

      case BCAttribute::URL:
        ku = dynamic_cast<KURLRequester*>(m_editWidget);
        if(ku) {
          ku->lineEdit()->selectAll();
        }
        break;

      default:
        break;
    }
  }
}

void BCAttributeWidget::openURL(const QString& url_) {
  KURL url(url_);
  if(url.isValid()) {
    m_run = new KRun(url);
  }
}

void BCAttributeWidget::updateAttribute(BCAttribute* att_) {
  m_label->setText(att_->title());
  QWhatsThis::add(m_label, att_->description());
  // TODO: fix if the validator might have changed
  if(m_type == BCAttribute::Choice) {
    KComboBox* kc = dynamic_cast<KComboBox*>(m_editWidget);
    if(kc) {
      kc->clear();
      // always have empty choice
      kc->insertItem(QString::null);
      kc->insertStringList(att_->allowed());
    }
  } else if(m_type == BCAttribute::Line) {
    KLineEdit* kl = dynamic_cast<KLineEdit*>(m_editWidget);
    if(kl) {
      BCUnitEditWidget* w = BCUnitEditWidgetAncestor(parent());
      if(att_->flags() & BCAttribute::AllowCompletion) {
        kl->completionObject()->setItems(w->m_currColl->valuesByAttributeName(att_->name()));
      } else {
        kl->completionObject()->clear();
      }
    }
  }
}
