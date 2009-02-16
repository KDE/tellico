/***************************************************************************
    copyright            : (C) 2005-2008 by Robby Stephenson
    email                :  robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_FETCH_MESSAGEHANDLER_H
#define TELLICO_FETCH_MESSAGEHANDLER_H

class QString;
class QStringList;

namespace Tellico {
  namespace Fetch {

/**
 * @author Robby Stephenson
 */
class MessageHandler {
public:
  enum Type { Status, Warning, Error, ListError };

  MessageHandler() {}
  virtual ~MessageHandler() {}

  virtual void send(const QString& message, Type type) = 0;
  virtual void infoList(const QString& message, const QStringList& list) = 0;
};

class ManagerMessage : public MessageHandler {
public:
  ManagerMessage() : MessageHandler() {}
  virtual ~ManagerMessage() {}

  virtual void send(const QString& message, Type type);
  virtual void infoList(const QString& message, const QStringList& list);
};

  } // end namespace
} // end namespace

#endif
