/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "fieldcompletion.h"
#include "fieldformat.h"

using Tellico::FieldCompletion;

FieldCompletion::FieldCompletion(bool multiple_) : KCompletion(), m_multiple(multiple_) {
}

QString FieldCompletion::makeCompletion(const QString& string_) {
  if(completionMode() == KGlobalSettings::CompletionNone) {
    m_beginText.clear();
    return QString();
  }

  if(!m_multiple) {
    return KCompletion::makeCompletion(string_);
  }

  static QRegExp rx = FieldFormat::delimiter();
  int pos = rx.lastIndexIn(string_);
  if(pos == -1) {
    m_beginText.clear();
    return KCompletion::makeCompletion(string_);
  }

  pos += rx.matchedLength();
  QString final = string_.mid(pos);
  m_beginText = string_.mid(0, pos);
  return m_beginText + KCompletion::makeCompletion(final);
}

void FieldCompletion::clear() {
  m_beginText.clear();
  KCompletion::clear();
}

void FieldCompletion::postProcessMatch(QString* match_) const {
  if(m_multiple) {
    match_->prepend(m_beginText);
  }
}

void FieldCompletion::postProcessMatches(QStringList* matches_) const {
  if(m_multiple) {
    for(QStringList::Iterator it = matches_->begin(); it != matches_->end(); ++it) {
      (*it).prepend(m_beginText);
    }
  }
}

void FieldCompletion::postProcessMatches(KCompletionMatches* matches_) const {
  if(m_multiple) {
    for(KCompletionMatches::Iterator it = matches_->begin(); it != matches_->end(); ++it) {
      (*it).value().prepend(m_beginText);
    }
  }
}

#include "fieldcompletion.moc"
