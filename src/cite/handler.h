/***************************************************************************
    Copyright (C) 2006-2009 Robby Stephenson <robby@periapsis.org>

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
