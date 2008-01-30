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

#ifndef TELLICO_IMPORT_REFERENCERIMPORTER_H
#define TELLICO_IMPORT_REFERENCERIMPORTER_H

#include "xsltimporter.h"
#include "../datavectors.h"

namespace Tellico {
  namespace Import {

/**
 * @author Robby Stephenson
*/
class ReferencerImporter : public XSLTImporter {
Q_OBJECT

public:
  /**
   */
  ReferencerImporter(const KURL& url);

  /**
   */
  virtual Data::CollPtr collection();
  /**
   */
  virtual QWidget* widget(QWidget*, const char*) { return 0; }
  virtual bool canImport(int type) const;

private:
  // private so it can't be changed accidently
  void setXSLTURL(const KURL& url);
};

  } // end namespace
} // end namespace
#endif
