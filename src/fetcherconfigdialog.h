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

#include "fetch/fetch.h"
#include "fetch/configwidget.h"

#include <kdialogbase.h>

#include <qintdict.h>

class KLineEdit;
class QCheckBox;
class QWidgetStack;

namespace Tellico {
  namespace GUI {
    class ComboBox;
  }

/**
 * @author Robby Stephenson
 */
class FetcherConfigDialog : public KDialogBase {
Q_OBJECT

public:
  FetcherConfigDialog(QWidget* parent);
  FetcherConfigDialog(const QString& sourceName, Fetch::Type type, bool updateOverwrite,
                      Fetch::ConfigWidget* configWidget, QWidget* parent);

  QString sourceName() const;
  Fetch::Type sourceType() const;
  bool updateOverwrite() const;
  Fetch::ConfigWidget* configWidget() const;

private slots:
  void slotNewSourceSelected(int idx);
  void slotNameChanged(const QString& name);
  void slotPossibleNewName(const QString& name);

private:
  void init(Fetch::Type type);

  bool m_newSource : 1;
  bool m_useDefaultName : 1;
  Fetch::ConfigWidget* m_configWidget;
  QLabel* m_iconLabel;
  KLineEdit* m_nameEdit;
  GUI::ComboBox* m_typeCombo;
  QCheckBox* m_cbOverwrite;
  QWidgetStack* m_stack;
  QIntDict<Fetch::ConfigWidget> m_configWidgets;
};

} // end namespace
#endif
