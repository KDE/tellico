/***************************************************************************
                             bcattributewidget.h
                             -------------------
    begin                : Sun Apr 13 2003
    copyright            : (C) 2003 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef BCATTRIBUTEWIDGET_H
#define BCATTRIBUTEWIDGET_H

class QLabel;
class QCheckBox;

class KRun;

#include "bcattribute.h"

#include <qhbox.h>
#include <qcheckbox.h>

/**
 * The BCAttributeWidget class is a box that shows a label, then a widget which depends
 * on the attribute type, and then a checkbox for multiple editing.
 *
 * @author Robby Stephenson
 * @version $Id: bcattributewidget.h,v 1.1 2003/05/02 06:04:21 robby Exp $
 */
class BCAttributeWidget : public QHBox {
Q_OBJECT

public:
  BCAttributeWidget(BCAttribute* att, QWidget* parent, const char* name=0);
  ~BCAttributeWidget();

  QString text() const;
  void setText(const QString& text);
  int labelWidth() const;
  void setLabelWidth(int width);
  bool isEnabled() const;
  void addCompletionObjectItem(const QString& text);
  bool isTextEdit() const;
  void editMultiple(bool show);
  void setHighlighted(const QString& highlight) const;
  void updateAttribute(BCAttribute* att);

public slots:
  void clear();
  void setEnabled(bool enabled);

signals:
  void modified();

protected slots:
  void openURL(const QString& url);

private:
  QString m_name;
  BCAttribute::AttributeType m_type;
  QLabel* m_label;
  QWidget* m_editWidget;
  QCheckBox* m_editMultiple;

  KRun* m_run;

  bool m_isTextEdit;
};

#endif
