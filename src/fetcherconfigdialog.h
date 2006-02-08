/***************************************************************************
    copyright            : (C) 2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_FETCHERCONFIGDIALOG_H
#define TELLICO_FETCHERCONFIGDIALOG_H

class KLineEdit;
class KComboBox;
class QWidgetStack;

#include "fetch/configwidget.h"

#include <kdialogbase.h>

#include <qintdict.h>

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class FetcherConfigDialog : public KDialogBase {
Q_OBJECT

public:
  FetcherConfigDialog(QWidget* parent);
  FetcherConfigDialog(const QString& sourceName, Fetch::Type type, Fetch::ConfigWidget* configWidget, QWidget* parent);

  QString sourceName() const;
  Fetch::Type sourceType() const;
  Fetch::ConfigWidget* configWidget() const;

private slots:
  void slotNewSourceSelected(int idx);

private:
  void init(Fetch::Type type);

  bool m_newSource : 1;
  Fetch::ConfigWidget* m_configWidget;
  KLineEdit* m_nameEdit;
  KComboBox* m_typeCombo;
  QWidgetStack* m_stack;
  QIntDict<Fetch::ConfigWidget> m_configWidgets;
};

} // end namespace
#endif
