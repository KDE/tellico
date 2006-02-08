/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_CITE_HANDLER_H
#define TELLICO_CITE_HANDLER_H

#include <map>
#include <string>

namespace Tellico {
  namespace Cite {

enum State {
  NoConnection,
  NoDocument,
  NoCitation,
  Success
};

typedef std::map<std::string, std::string> Map;

/**
 * @author Robby Stephenson
 */
class Handler {
public:
  Handler() {}
  virtual ~Handler() {}

  virtual bool connect() = 0;
  virtual bool cite(Map& s) = 0;
  virtual State state() const = 0;

 // really specific to ooo
  void setHost(const char* host) { m_host = host; }
  const std::string& host() const { return m_host; }
  void setPort(int port) { m_port = port; }
  int port() const { return m_port; }
  void setPipe(const char* pipe) { m_pipe = pipe; }
  const std::string& pipe() const { return m_pipe; }

private:
  std::string m_host;
  int m_port;
  std::string m_pipe;
};

  }
}

#endif
