/***************************************************************************
    copyright            : (C) 2005-2008 by Robby Stephenson
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
  ReferencerImporter(const KUrl& url);

  /**
   */
  virtual Data::CollPtr collection();
  /**
   */
  virtual QWidget* widget(QWidget*) { return 0; }
  virtual bool canImport(int type) const;

private:
  // private so it can't be changed accidently
  void setXSLTURL(const KUrl& url);
};

  } // end namespace
} // end namespace
#endif
