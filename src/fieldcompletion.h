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

#ifndef TELLICOFIELDCOMPLETION_H
#define TELLICOFIELDCOMPLETION_H

#include <kcompletion.h>

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class FieldCompletion : public KCompletion {
Q_OBJECT

public:
  FieldCompletion(bool multiple);

  void setMultiple(bool m) { m_multiple = m; }
  virtual QString makeCompletion(const QString& string);
  virtual void clear();

protected:
  virtual void postProcessMatch(QString* match) const;
  virtual void postProcessMatches(QStringList* matches) const;
  virtual void postProcessMatches(KCompletionMatches* matches) const;

private:
  bool m_multiple;
  QString m_beginText;
};

} // end namespace
#endif
