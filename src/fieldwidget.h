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

#include <kurlcompletion.h>

#include <qwidget.h>
#include <qguardedptr.h>

namespace Tellico {
  namespace Data {
    class Collection;
  }

/**
 * The FieldWidget class is a box that shows a label, then a widget which depends
 * on the field type, and then a checkbox for multiple editing.
 *
 * @author Robby Stephenson
 * @version $Id: fieldwidget.h 985 2004-12-02 03:34:56Z robby $
 */
class FieldWidget : public QWidget {
Q_OBJECT

public:
  FieldWidget(const Data::Field* field, QWidget* parent, const char* name=0);
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

  // the EntryEditDialog sets this so the completion object can be easily updated
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
  class MyCompletion : public KURLCompletion {
  public:
    MyCompletion() : KURLCompletion() {}
    virtual QString makeCompletion(const QString& text);
  };

  static const QRegExp s_semiColon;
  static const QRegExp s_comma;

  Data::Field::Type m_type;
  QLabel* m_label;
  QWidget* m_editWidget;
  QCheckBox* m_editMultiple;

  QGuardedPtr<KRun> m_run;

  bool m_expands;
  bool m_allowMultiple;
  bool m_isRating;
  bool m_relativeURL;
};

} // end namespace
#endif
