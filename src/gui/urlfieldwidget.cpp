/***************************************************************************
    copyright            : (C) 2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "urlfieldwidget.h"
#include "../field.h"
#include "../latin1literal.h"
#include "../tellico_kernel.h"

#include <klineedit.h>
#include <kurlrequester.h>
#include <kurllabel.h>
#include <kdeversion.h>

using Tellico::GUI::URLFieldWidget;

// subclass of KURLCompletion is needed so the KURLLabel
// can open relative links. I don't want to have to have to update
// the base directory of the completion every time a new document is opened
QString URLFieldWidget::URLCompletion::makeCompletion(const QString& text_) {
  // KURLCompletion::makeCompletion() uses an internal variable instead
  // of calling KURLCompletion::dir() so need to set the base dir before completing
  setDir(Kernel::self()->URL().directory());
  return KURLCompletion::makeCompletion(text_);
}

URLFieldWidget::URLFieldWidget(const Data::Field* field_, QWidget* parent_, const char* name_/*=0*/)
    : FieldWidget(field_, parent_, name_), m_run(0) {

  m_requester = new KURLRequester(this);
  m_requester->lineEdit()->setCompletionObject(new URLCompletion());
  m_requester->lineEdit()->setAutoDeleteCompletionObject(true);
  connect(m_requester, SIGNAL(textChanged(const QString&)), SIGNAL(modified()));
  connect(m_requester, SIGNAL(textChanged(const QString&)), label(), SLOT(setURL(const QString&)));
  connect(label(), SIGNAL(leftClickedURL(const QString&)), SLOT(slotOpenURL(const QString&)));
  registerWidget();

  // special case, remember if it's a relative url
  m_isRelative = field_->property(QString::fromLatin1("relative")) == Latin1Literal("true");
}

URLFieldWidget::~URLFieldWidget() {
  if(m_run) {
    m_run->abort();
  }
}

QString URLFieldWidget::text() const {
  if(m_isRelative) {
#if KDE_IS_VERSION(3,1,90)
    return KURL::relativeURL(Kernel::self()->URL(), m_requester->url());
#else
    kdWarning() << "KDE 3.2 or higher is required to use relative URLs." << endl;
    return m_requester->url();
#endif
  }
  return m_requester->url();
}

void URLFieldWidget::setText(const QString& text_) {
  blockSignals(true);

  m_requester->blockSignals(true);
  m_requester->setURL(text_);
  m_requester->blockSignals(false);
  static_cast<KURLLabel*>(label())->setURL(text_);

  blockSignals(false);
}

void URLFieldWidget::clear() {
  m_requester->clear();
  editMultiple(false);
}

void URLFieldWidget::updateFieldHook(Data::Field*, Data::Field* newField_) {
  m_isRelative = newField_->property(QString::fromLatin1("relative")) == Latin1Literal("true");
}

void URLFieldWidget::slotOpenURL(const QString& url_) {
  if(url_.isEmpty()) {
    return;
  }
  // just in case, interpret string relative to document url
  m_run = new KRun(KURL(Kernel::self()->URL(), url_));
}

QWidget* URLFieldWidget::widget() {
  return m_requester;
}

#include "urlfieldwidget.moc"
