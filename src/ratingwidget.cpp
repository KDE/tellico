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

#include "ratingwidget.h"
#include "field.h"
#include "latin1literal.h"

#include <kglobal.h>
#include <kiconloader.h>
#include <kdebug.h>

#include <qintdict.h>

namespace {
  static const int RATING_WIDGET_MAX_ICONS = 10;
  static const int RATING_WIDGET_MAX_STAR_SIZE = 24;
}

using Tellico::RatingWidget;

// it's a rating if name is rating, or extended property rating is true, and it has
// up to 10 allowed values, all starting with numbers, between 0 and 10
bool RatingWidget::handleField(const Data::Field* const field_) {
  // use a RatingWidget if the field is a choice, and every value can be converted to a number
  if(field_->type() != Data::Field::Choice) {
    return false;
  }

  if(field_->name() != Latin1Literal("rating")
     && field_->property(QString::fromLatin1("rating")) != Latin1Literal("true")) {
    return false;
  }

  const QStringList& allow = field_->allowed();
  if(allow.count() > 10) {
    return false;
  }

  for(QStringList::ConstIterator it = allow.begin(); it != allow.end(); ++it) {
    bool ok;
    int n = (*it).left(1).toInt(&ok);
    if(!ok) {
      return false;
    }
    if(n == 0 || n == 1) {
      int m = (*it).left(2).toInt(&ok);
      if(ok) {
        n = m;
      }
    }
    if(n > 10) {
      return false;
    }
  }
  return true;
}

const QPixmap& RatingWidget::pixmap(const QString& value_) {
  static QIntDict<QPixmap> pixmaps;
  if(pixmaps.isEmpty()) {
    pixmaps.insert(-1, new QPixmap());
  }
  bool ok;
  int n = value_.left(1).toInt(&ok);
  if(!ok) {
  }
  if(n == 0 || n == 1) {
    int m = value_.left(2).toInt(&ok);
    if(ok) {
      n = m;
    }
  }
  if(n == 0 || n > 10) {
    return *pixmaps[-1];
  }
  if(pixmaps[n]) {
    return *pixmaps[n];
  }

  QString picName = QString::fromLatin1("stars%1").arg(n);
  QPixmap* pix = new QPixmap(KGlobal::iconLoader()->loadIcon(picName, KIcon::User));
  pixmaps.insert(n, pix);
  return *pix;
}

RatingWidget::RatingWidget(const Data::Field* field_, QWidget* parent_, const char* name_/*=0*/)
    : QHBox(parent_, name_), m_field(field_), m_currIndex(-1) {
  m_pixOn = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("star_on"), KIcon::User);
  m_pixOff = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("star_off"), KIcon::User);
  setSpacing(0);

  // find maximum width and height
  int w = KMAX(RATING_WIDGET_MAX_STAR_SIZE, KMAX(m_pixOn.width(), m_pixOff.width()));
  int h = KMAX(RATING_WIDGET_MAX_STAR_SIZE, KMAX(m_pixOn.height(), m_pixOff.height()));
  for(int i = 0; i < RATING_WIDGET_MAX_ICONS; ++i) {
    QLabel* l = new QLabel(this);
    l->setFixedSize(w, h);
    m_widgets.append(l);
  }
  init();
}

void RatingWidget::init() {
  updateAllowed();
  m_total = KMIN(m_allowed.count(), m_widgets.count());
  uint i = 0;
  for( ; static_cast<int>(i) < m_total; ++i) {
    m_widgets.at(i)->setPixmap(m_pixOff);
  }
  for( ; i < m_widgets.count(); ++i) {
    m_widgets.at(i)->setPixmap(QPixmap());
  }
  update();
}

void RatingWidget::updateAllowed() {
  // don't use QStringList::sort() since 10 would get sorted before 2
  QMap<int, QString> map;
  for(QStringList::ConstIterator it = m_field->allowed().begin(); it != m_field->allowed().end(); ++it) {
    uint pos = 0;
    while(pos < (*it).length() && (*it).at(pos).isDigit()) {
      ++pos;
    }
    map.insert((*it).left(pos+1).toInt(), *it);
  }
  m_allowed = map.values();
}

void RatingWidget::update() {
  int i = 0;
  for( ; i <= m_currIndex; ++i) {
    m_widgets.at(i)->setPixmap(m_pixOn);
  }
  for( ; i < m_total; ++i) {
    m_widgets.at(i)->setPixmap(m_pixOff);
  }

  QHBox::update();
}

void RatingWidget::mousePressEvent(QMouseEvent* event_) {
  // only react to left button
  if(event_->button() != LeftButton) {
    return;
  }

  int idx;
  QWidget* child = childAt(event_->pos());
  if(child) {
    idx = m_widgets.findRef(static_cast<QLabel*>(child));
    // if the widget is clicked beyond the maximum number of allowed values, clear it
    if(idx >= m_total) {
      idx = -1;
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

const QString& RatingWidget::text() const {
  return m_currIndex == -1 ? QString::null : m_allowed[m_currIndex];
}

void RatingWidget::setText(const QString& text_) {
  m_currIndex = m_allowed.findIndex(text_);
  update();
}

void RatingWidget::updateField(const Data::Field* field_) {
  m_field = field_;
  init();
}

#include "ratingwidget.moc"
