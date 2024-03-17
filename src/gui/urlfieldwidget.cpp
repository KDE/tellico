/***************************************************************************
    Copyright (C) 2005-2021 Robby Stephenson <robby@periapsis.org>
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

#include "urlfieldwidget.h"
#include "../field.h"
#include "../document.h"

#include <KLineEdit>
#include <KUrlRequester>
#include <KUrlLabel>

#include <QUrl>
#include <QDesktopServices>

using Tellico::GUI::URLFieldWidget;

// subclass of KUrlCompletion is needed so the KUrlLabel
// can open relative links. I don't want to have to have to update
// the base directory of the completion every time a new document is opened
QString URLFieldWidget::URLCompletion::makeCompletion(const QString& text_) {
  // KUrlCompletion::makeCompletion() uses an internal variable instead
  // of calling KUrlCompletion::dir() so need to set the base dir before completing
  setDir(Data::Document::self()->URL().adjusted(QUrl::PreferLocalFile | QUrl::RemoveFilename));
  return KUrlCompletion::makeCompletion(text_);
}

URLFieldWidget::URLFieldWidget(Tellico::Data::FieldPtr field_, QWidget* parent_)
    : FieldWidget(field_, parent_) {

  // the label is a KUrlLabel
  KUrlLabel* urlLabel = dynamic_cast<KUrlLabel*>(label());
  Q_ASSERT(urlLabel);

  m_requester = new KUrlRequester(this);
  m_requester->lineEdit()->setCompletionObject(new URLCompletion());
  m_requester->lineEdit()->setAutoDeleteCompletionObject(true);
  connect(m_requester, &KUrlRequester::textChanged, this, &URLFieldWidget::checkModified);
  connect(m_requester, &KUrlRequester::textChanged, urlLabel, &KUrlLabel::setUrl);
  void (KUrlLabel::* clickedSignal)(void) = &KUrlLabel::leftClickedUrl;
  connect(urlLabel, clickedSignal, this, &URLFieldWidget::slotOpenURL);
  registerWidget();

  // special case, remember if it's a relative url
  m_logic.setRelative(field_->property(QStringLiteral("relative")) == QLatin1String("true"));
}

URLFieldWidget::~URLFieldWidget() {
}

QString URLFieldWidget::text() const {
  // for comparison purposes and to be consistent with the file listing importer
  // I want the full url here, including the protocol
  m_logic.setBaseUrl(Data::Document::self()->URL());
  return m_logic.urlText(m_requester->url());
}

void URLFieldWidget::setTextImpl(const QString& text_) {
  if(text_.isEmpty()) {
    m_requester->clear();
    return;
  }

  QUrl url = m_logic.isRelative() ?
             Data::Document::self()->URL().resolved(QUrl(text_)) :
             QUrl::fromUserInput(text_);
  m_requester->setUrl(url);
  static_cast<KUrlLabel*>(label())->setUrl(url.url());
}

void URLFieldWidget::clearImpl() {
  m_requester->clear();
  editMultiple(false);
}

void URLFieldWidget::updateFieldHook(Tellico::Data::FieldPtr, Tellico::Data::FieldPtr newField_) {
  m_logic.setRelative(newField_->property(QStringLiteral("relative")) == QLatin1String("true"));
}

void URLFieldWidget::slotOpenURL() {
  const QString url = static_cast<KUrlLabel*>(label())->url();
  if(url.isEmpty()) {
    return;
  }
  QDesktopServices::openUrl(m_logic.isRelative() ?
                            Data::Document::self()->URL().resolved(QUrl(url)) :
                            QUrl::fromUserInput(url));
}

QWidget* URLFieldWidget::widget() {
  return m_requester;
}
