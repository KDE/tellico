/***************************************************************************
    copyright            : (C) 2003-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "ratingwidget.h"
#include "../field.h"
#include "../tellico_utils.h"
#include "../tellico_debug.h"

#include <kiconloader.h>

#include <QHash>
#include <QPixmap>
#include <QBoxLayout>
#include <QLabel>
#include <QMouseEvent>

namespace {
  static const int RATING_WIDGET_MAX_ICONS = 10; // same as in Field::ratingValues()
  static const int RATING_WIDGET_MAX_STAR_SIZE = 24;
}

using Tellico::GUI::RatingWidget;

const QPixmap& RatingWidget::pixmap(const QString& value_) {
  static QHash<int, QPixmap*> pixmaps;
  if(pixmaps.isEmpty()) {
    pixmaps.insert(-1, new QPixmap());
  }
  bool ok;
  int n = Tellico::toUInt(value_, &ok);
  if(!ok || n < 1 || n > 10) {
    return *pixmaps[-1];
  }
  if(pixmaps[n]) {
    return *pixmaps[n];
  }

  QString picName = QString::fromLatin1("stars%1").arg(n);
  QPixmap* pix = new QPixmap(UserIcon(picName));
  pixmaps.insert(n, pix);
  return *pix;
}

RatingWidget::RatingWidget(Tellico::Data::FieldPtr field_, QWidget* parent_)
    : KHBox(parent_), m_field(field_), m_currIndex(-1) {
  m_pixOn = UserIcon(QLatin1String("star_on"));
  m_pixOff = UserIcon(QLatin1String("star_off"));
  setSpacing(0);

  // find maximum width and height
  int w = qMax(RATING_WIDGET_MAX_STAR_SIZE, qMax(m_pixOn.width(), m_pixOff.width()));
  int h = qMax(RATING_WIDGET_MAX_STAR_SIZE, qMax(m_pixOn.height(), m_pixOff.height()));
  for(int i = 0; i < RATING_WIDGET_MAX_ICONS; ++i) {
    QLabel* l = new QLabel(this);
    l->setFixedSize(w, h);
    m_widgets.append(l);
  }
  init();

  QBoxLayout* l = dynamic_cast<QBoxLayout*>(layout());
  if(l) {
    l->addStretch(1);
  }
}

void RatingWidget::init() {
  updateBounds();
  m_total = qMin(m_max, static_cast<int>(m_widgets.count()));
  int i = 0;
  for( ; i < m_total; ++i) {
    m_widgets.at(i)->setPixmap(m_pixOff);
  }
  for( ; i < m_widgets.count(); ++i) {
    m_widgets.at(i)->setPixmap(QPixmap());
  }
  update();
}

void RatingWidget::updateBounds() {
  bool ok; // not used;
  m_min = Tellico::toUInt(m_field->property(QLatin1String("minimum")), &ok);
  m_max = Tellico::toUInt(m_field->property(QLatin1String("maximum")), &ok);
  if(m_max > RATING_WIDGET_MAX_ICONS) {
    myDebug() << "RatingWidget::updateBounds() - max is too high: " << m_max << endl;
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

  KHBox::update();
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
    emit modified();
  }
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
      m_currIndex = m_min-1; // limit to minimum, remember index is zero-based
    }
  } else {
    m_currIndex = -1;
  }
  update();
}

void RatingWidget::updateField(Tellico::Data::FieldPtr field_) {
  m_field = field_;
  init();
}

#include "ratingwidget.moc"
