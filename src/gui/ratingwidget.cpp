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

#include "ratingwidget.h"
#include "../field.h"
#include "../utils/string_utils.h"
#include "../tellico_debug.h"

#include <QHash>
#include <QBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QToolButton>

namespace {
  static const int RATING_WIDGET_MAX_ICONS = 10; // same as in Field::convertOldRating()
  static const int RATING_WIDGET_MAX_STAR_SIZE = 24;
}

using Tellico::GUI::RatingWidget;

RatingWidget::RatingWidget(Tellico::Data::FieldPtr field_, QWidget* parent_)
    : QWidget(parent_), m_field(field_), m_currIndex(-1) {
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  m_pixOn = QIcon(QLatin1String(":/icons/star_on")).pixmap(QSize(18, 18));
  m_pixOff = QIcon(QLatin1String(":/icons/star_off")).pixmap(QSize(18, 18));

  m_widgets.reserve(RATING_WIDGET_MAX_ICONS);
  // find maximum width and height
  int w = qMax(RATING_WIDGET_MAX_STAR_SIZE, qMax(m_pixOn.width(), m_pixOff.width()));
  int h = qMax(RATING_WIDGET_MAX_STAR_SIZE, qMax(m_pixOn.height(), m_pixOff.height()));
  for(int i = 0; i < RATING_WIDGET_MAX_ICONS; ++i) {
    QLabel* l = new QLabel(this);
    l->setFixedSize(w, h);
    m_widgets.append(l);
    layout->addWidget(l);
  }

  m_clearButton = new QToolButton(this);
  if(layoutDirection() == Qt::LeftToRight) {
    m_clearButton->setIcon(QIcon::fromTheme(QStringLiteral("edit-clear-locationbar-rtl")));
  } else {
    m_clearButton->setIcon(QIcon::fromTheme(QStringLiteral("edit-clear-locationbar-ltr")));
  }
  connect(m_clearButton, &QAbstractButton::clicked, this, &RatingWidget::clearClicked);
  layout->addWidget(m_clearButton);

  // to keep the widget from resizing when the clear button is shown/hidden
  // have a fixed width spacer to swap with
  const int mw = m_clearButton->minimumSizeHint().width();
  m_clearButton->setFixedWidth(mw);
  m_clearSpacer = new QSpacerItem(mw, mw, QSizePolicy::Fixed, QSizePolicy::Fixed);

  init();
}

void RatingWidget::init() {
  updateBounds();
  m_total = qMin(m_max, static_cast<int>(m_widgets.count()));
  int i = 0;
  for( ; i < m_total; ++i) {
    m_widgets.at(i)->setPixmap(m_pixOff);
  }
  setUpdatesEnabled(false);

  QBoxLayout* l = ::qobject_cast<QBoxLayout*>(layout());
  // move the clear button to right after the last star
  l->removeWidget(m_clearButton);
  l->insertWidget(i, m_clearButton);
  l->removeItem(m_clearSpacer);
  l->insertSpacerItem(i+1, m_clearSpacer);
  m_clearButton->hide();

  setUpdatesEnabled(true);

  for( ; i < m_widgets.count(); ++i) {
    m_widgets.at(i)->setPixmap(QPixmap());
  }
  update();
}

void RatingWidget::updateBounds() {
  bool ok; // not used;
  m_min = Tellico::toUInt(m_field->property(QStringLiteral("minimum")), &ok);
  m_max = Tellico::toUInt(m_field->property(QStringLiteral("maximum")), &ok);
  if(m_max > RATING_WIDGET_MAX_ICONS) {
    myDebug() << "max is too high: " << m_max;
    m_max = RATING_WIDGET_MAX_ICONS;
  }
  if(m_min < 1) {
    m_min = 1;
  }
}

void RatingWidget::update() {
  int i = 0;
  for( ; i <= m_currIndex; ++i) {
    m_widgets.at(i)->setPixmap(m_pixOn);
  }
  for( ; i < m_total; ++i) {
    m_widgets.at(i)->setPixmap(m_pixOff);
  }

  QWidget::update();
}

void RatingWidget::mousePressEvent(QMouseEvent* event_) {
  // only react to left button
  if(event_->button() != Qt::LeftButton) {
    return;
  }

  int idx;
  QWidget* child = childAt(event_->pos());
  if(child) {
    idx = m_widgets.indexOf(static_cast<QLabel*>(child));
    // if the widget is clicked beyond the maximum value, clear it
    // remember total and min are values, but index is zero-based!
    if(idx > m_total-1) {
      idx = -1;
    } else if(idx < m_min-1) {
      idx = m_min-1; // limit to minimum, remember index is zero-based
    }
  } else {
    idx = -1;
  }
  if(m_currIndex != idx) {
    m_currIndex = idx;
    update();
    emit signalModified();
  }
}

void RatingWidget::enterEvent(QEvent* event_) {
  Q_UNUSED(event_);
  setUpdatesEnabled(false);
  m_clearButton->show();
  QBoxLayout* l = ::qobject_cast<QBoxLayout*>(layout());
  l->removeItem(m_clearSpacer);
  setUpdatesEnabled(true);
}

void RatingWidget::leaveEvent(QEvent* event_) {
  Q_UNUSED(event_);
  setUpdatesEnabled(false);
  m_clearButton->hide();
  QBoxLayout* l = ::qobject_cast<QBoxLayout*>(layout());
  l->insertSpacerItem(m_total+1, m_clearSpacer);
  setUpdatesEnabled(true);
}

void RatingWidget::clear() {
  m_currIndex = -1;
  update();
}

QString RatingWidget::text() const {
  // index is index of the list, which is zero-based. Add 1!
  return m_currIndex == -1 ? QString() : QString::number(m_currIndex+1);
}

void RatingWidget::setText(const QString& text_) {
  bool ok;
  // text is value, subtract one to get index
  m_currIndex = Tellico::toUInt(text_, &ok)-1;
  if(ok) {
    if(m_currIndex > m_total-1) {
      m_currIndex = -1;
    } else if(m_currIndex < m_min-1) {
      myWarning() << "RatingWidget:: Trying to set value to" << text_ << "but min is" << m_min;
      m_currIndex = -1; // trying to set a value outside the bounds results in no value
    }
  } else {
    m_currIndex = -1;
  }
  update();
}

void RatingWidget::updateField(Tellico::Data::FieldPtr field_) {
  const QString value = text();
  m_field = field_;
  init();
  setText(value);
}

void RatingWidget::clearClicked() {
  if(m_currIndex != -1) {
    m_currIndex = -1;
    update();
    emit signalModified();
  }
}
