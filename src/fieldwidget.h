/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef FIELDWIDGET_H
#define FIELDWIDGET_H

class QLabel;
class QCheckBox;

class KRun;

#include "field.h"

#include <qwidget.h>
#include <qguardedptr.h>

namespace Bookcase {
  namespace Data {
    class Collection;
  }

/**
 * The FieldWidget class is a box that shows a label, then a widget which depends
 * on the field type, and then a checkbox for multiple editing.
 *
 * @author Robby Stephenson
 * @version $Id: fieldwidget.h 719 2004-08-01 22:12:46Z robby $
 */
class FieldWidget : public QWidget {
Q_OBJECT

public:
  FieldWidget(const Data::Field* const field, QWidget* parent, const char* name=0);
  ~FieldWidget();

  QString text() const;
  void setText(const QString& text);
  int labelWidth() const;
  void setLabelWidth(int width);
  bool isEnabled() const;
  void addCompletionObjectItem(const QString& text);
  bool expands() const;
  void editMultiple(bool show);
  void setHighlighted(const QString& highlight) const;
  void updateField(Data::Field* newField, Data::Field* oldField);

  // the EntryEditDialog sets this so the completion object can be easily updates
  static QGuardedPtr<Data::Collection> s_coll;

public slots:
  void clear();
  void setEnabled(bool enabled);

signals:
  void modified();

protected slots:
  void slotOpenURL(const QString& url);
  void slotCheckRows(int row, int col);

private:
  static const QRegExp s_semiColon;
  static const QRegExp s_comma;

  Data::Field::Type m_type;
  QLabel* m_label;
  QWidget* m_editWidget;
  QCheckBox* m_editMultiple;

  QGuardedPtr<KRun> m_run;

  bool m_expands;
  bool m_allowMultiple;
};

} // end namespace
#endif
