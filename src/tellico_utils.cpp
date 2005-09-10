/***************************************************************************
    copyright            : (C) 2003-2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "tellico_utils.h"
#include "tellico_kernel.h"
#include "latin1literal.h"

#include <kapplication.h>
#include <klocale.h>
#include <kglobal.h>
#include <kstringhandler.h>

#include <qregexp.h>
#include <qdir.h>
#include <qcursor.h>
#include <qscrollview.h>

QColor Tellico::contrastColor;

QString Tellico::decodeHTML(QString text) {
  QRegExp rx(QString::fromLatin1("&#(\\d+);"));
  int pos = rx.search(text);
  while(pos > -1) {
    text.replace(pos, rx.matchedLength(), static_cast<char>(rx.cap(1).toInt()));
    pos = rx.search(text, pos+1);
  }
  return text;
}

QString Tellico::uid(int l) {
  QString uid = QString::fromLatin1("Tellico");
  uid.append(kapp->randomString(QMAX(l - uid.length(), 0)));
  return uid;
}

uint Tellico::toUInt(const QString& s, bool* ok) {
  if(s.isEmpty()) {
    if(ok) {
      *ok = false;
    }
    return 0;
  }

  uint idx = 0;
  while(s[idx].isDigit()) {
    ++idx;
  }
  if(idx == 0) {
    if(ok) {
      *ok = false;
    }
    return 0;
  }
  return s.left(idx).toUInt(ok);
}

QString Tellico::i18nReplace(QString text) {
  QRegExp rx(QString::fromLatin1("<i18n>(.*)</i18n>"));
  rx.setMinimal(true);
  int pos = rx.search(text);
  while(pos > -1) {
    text.replace(pos, rx.matchedLength(), i18n(rx.cap(1).utf8()));
    pos = rx.search(text, pos+1);
  }
  return text;
}

QStringList Tellico::findAllSubDirs(const QString& dir_) {
  if(dir_.isEmpty()) {
    return QStringList();
  }

  // TODO: build in symlink chekcing, for now, prohibit
  QDir dir(dir_, QString::null, QDir::Name | QDir::IgnoreCase, QDir::Dirs | QDir::Readable | QDir::NoSymLinks);

  QStringList allSubdirs; // the whole list

  // find immediate sub directories
  const QStringList subdirs = dir.entryList();
  for(QStringList::ConstIterator subdir = subdirs.begin(); subdir != subdirs.end(); ++subdir) {
    if((*subdir).isEmpty() || *subdir == Latin1Literal(".") || *subdir == Latin1Literal("..")) {
      continue;
    }
    QString absSubdir = dir.absFilePath(*subdir);
    allSubdirs += findAllSubDirs(absSubdir);
    allSubdirs += absSubdir;
  }
  return allSubdirs;
}

void Tellico::updateContrastColor(const QColorGroup& cg_) {
  // if the value difference between background and highlight is more than ???
  // use highlight, else go lighter or darker
  int h1, s1, v1, h2, s2, v2;
  cg_.background().getHsv(&h1, &s1, &v1);

  QColor hl = cg_.highlight();
  hl.getHsv(&h2, &s2, &v2);
  h2 += 120;
  s2 = 255;
  hl.setHsv(h2, s2, v2);

  if(KABS(v2-v1) < 48) {
    if(v1 < 128) {
      contrastColor = hl.light();
    } else {
      contrastColor = hl.dark();
    }
  } else {
    contrastColor = hl;
  }
}

QString Tellico::rPixelSqueeze(const QString& str_, const QFontMetrics& fm_, uint pixels_) {
#if KDE_IS_VERSION(3,1,90)
  return KStringHandler::rPixelSqueeze(str_, fm_, pixels_);
#else
  uint width = fm_.width(str_);
  if(pixels_ < width) {
    QString tmp = str_;
    const uint em = fm_.maxWidth();
    pixels_ -= fm_.width(QString::fromLatin1("..."));

    while(pixels_ < width && !tmp.isEmpty()) {
      int length = tmp.length();
      int delta = em ? (width - pixels_) / em : length;
      delta = kClamp(delta, 1, length);
      tmp.remove(length - delta, delta);
      width = fm_.width(tmp);
    }

    return tmp + QString::fromLatin1("...");
  }

  return str_;
#endif
}

Tellico::GUI::CursorSaver::CursorSaver(const QCursor& cursor_) : m_restored(false) {
  kapp->setOverrideCursor(cursor_);
}

Tellico::GUI::CursorSaver::~CursorSaver() {
  if(!m_restored) {
    kapp->restoreOverrideCursor();
  }
}

void Tellico::GUI::CursorSaver::restore() {
  kapp->restoreOverrideCursor();
  m_restored = true;
}

Tellico::GUI::WidgetUpdateBlocker::WidgetUpdateBlocker(QScrollView* widget_) : m_widget(widget_), m_wasEnabled(false) {
  if(m_widget) {
    m_wasEnabled = m_widget->isUpdatesEnabled();
    m_widget->viewport()->setUpdatesEnabled(false);
    m_widget->setUpdatesEnabled(false);
  }
}

Tellico::GUI::WidgetUpdateBlocker::~WidgetUpdateBlocker() {
  if(m_widget) {
    // I think it's a qt bug, but the viewport for scroll view doesn't refresh properly
    m_widget->viewport()->setUpdatesEnabled(m_wasEnabled);
    m_widget->setUpdatesEnabled(m_wasEnabled);
    m_widget->viewport()->update();
    m_widget->update();
  }
}
