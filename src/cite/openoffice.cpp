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

#include "openoffice.h"
#include "handler.h"
#include "../collections/bibtexcollection.h"
#include "../entry.h"
#include "../tellico_utils.h"
#include "../statusbar.h"
#include "../tellico_kernel.h"
#include "../tellico_debug.h"

#include <klibloader.h>
#include <kdialog.h>
#include <knuminput.h>
#include <kapplication.h>
#include <kconfig.h>
#include <klineedit.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kglobal.h>

#include <QFile>
#include <QLabel>
#include <QGridLayout>
#include <QBoxLayout>
#include <QGroupBox>
#include <QRadioButton>

using Tellico::Cite::OpenOffice;

class OpenOffice::Private {
  friend class OpenOffice;

  Private() : handler(0), port(-1) {
    KLibrary* library = Tellico::openLibrary(QLatin1String("tellico_ooo"));
    if(library) {
       KLibrary::void_function_ptr func = library->resolveFunction("handler");
      if(func) {
        handler = ((Handler* (*)())func)();
      }
    }
  }

  Handler* handler;
  // empty pipe string indicates tcp should be used
  QString host;
  int port;
  QString pipe;
};

OpenOffice::OpenOffice() : Action(), d(new Private()) {
}

OpenOffice::~OpenOffice() {
  delete d;
}

Tellico::Cite::State OpenOffice::state() const {
  if(!d->handler) {
    return NoConnection;
  }
  return d->handler->state();
}

bool OpenOffice::connect() {
  if(!d->handler) {
    myDebug() << "OpenOffice::connect() - unable to open OpenOffice.org plugin" << endl;
    return false;
  }

  StatusBar::self()->setStatus(i18n("Connecting to OpenOffice.org..."));

  if(d->port == -1) {
    KConfigGroup config(KGlobal::config(), "OpenOffice.org");
    d->host = config.readEntry("Host", "localhost");
    d->port = config.readEntry("Port", 2083);
    d->pipe = config.readPathEntry("Pipe", QString());
    // the ooohandler will depend on pipe.isEmpty() to indicate the port should be used
    d->handler->setHost(d->host.toUtf8());
    d->handler->setPort(d->port);
    if(!d->pipe.isEmpty()) {
      d->handler->setPipe(QFile::encodeName(d->pipe));
    }
  }

  bool success = d->handler->connect();
  bool needInput = !success;
  while(needInput) {
    switch(state()) {
      case NoConnection:
        myDebug() << "OpenOffice::connect() - NoConnection" << endl;
        // try to reconnect
        if(connectionDialog()) {
          success = d->handler->connect();
          needInput = !success;
        } else {
          needInput = false;
        }
        break;
      case NoDocument:
        myDebug() << "OpenOffice::connect() - NoDocument" << endl;
        break;
      default:
        myDebug() << "OpenOffice::connect() - weird state" << endl;
        break;
    }
  }
  StatusBar::self()->clearStatus();
  return success;
}

bool OpenOffice::cite(Tellico::Data::EntryList entries_) {
  if(!connect()) {
    myDebug() << "OpenOffice::cite() - can't connect to OpenOffice" << endl;
    return false;
  }
  if(entries_.isEmpty()) {
    myDebug() << "OpenOffice::cite() - no entries" << endl;
    return false;
  }

  Data::CollPtr coll = entries_.front()->collection();
  if(!coll || coll->type() != Data::Collection::Bibtex) {
    myDebug() << "OpenOffice::cite() - not a bibtex collection" << endl;
    return false;
  }

  const QString bibtex = QLatin1String("bibtex");
  Data::FieldList vec = coll->fields();
//  for(Data::EntryListIt entry = entries_.begin(); entry != entries_.end(); ++entry) {
  foreach(Data::EntryPtr entry, entries_) {
    Cite::Map values;
//    for(Data::FieldList::Iterator it = vec.begin(); it != vec.end(); ++it) {
    foreach(Data::FieldPtr field, vec) {
      QString bibtexField = field->property(bibtex);
      if(!bibtexField.isEmpty()) {
        QString s = entry->field(field->name());
        if(!s.isEmpty()) {
          values[bibtexField.toStdString()] = s.toStdString();
        }
      } else if(field->name() == QLatin1String("isbn")) {
        // OOO includes ISBN
        QString s = entry->field(field->name());
        if(!s.isEmpty()) {
          values[field->name().toStdString()] = s.toStdString();
        }
      }
    }
    d->handler->cite(values);
  }
  return true;
}

bool OpenOffice::connectionDialog() {
  KDialog dlg(Kernel::self()->widget());
  dlg.setCaption(i18n("OpenOffice.org Connection"));
  dlg.setModal(true);
  dlg.setButtons(KDialog::Ok | KDialog::Cancel | KDialog::Help);
  dlg.showButtonSeparator( false );
  dlg.setHelp(QLatin1String("openoffice-org"));

  QWidget* widget = new QWidget(&dlg);
  QBoxLayout* topLayout = new QVBoxLayout(widget);
  topLayout->setSpacing(KDialog::spacingHint());
  dlg.setMainWidget(widget);
  // is there a better way to do a multi-line label than to insert newlines in the text?
  QBoxLayout* blay = new QHBoxLayout(widget);
  topLayout->addLayout(blay);
  QLabel* l = new QLabel(widget);
  l->setPixmap(DesktopIcon(QLatin1String("ooo_writer"), 64));
  blay->addWidget(l);
  l = new QLabel(widget);
  l->setText(i18n("Tellico was unable to connect to OpenOffice.org. "
                  "Please verify the connection settings below, and "
                  "that OpenOffice.org Writer is currently running."));
  l->setTextFormat(Qt::RichText);
  blay->addWidget(l);
  blay->setStretchFactor(l, 10);

  QGroupBox* gbox = new QGroupBox(i18n("OpenOffice.org Connection"), widget);
  topLayout->addWidget(gbox);

  QGridLayout* gl = new QGridLayout(gbox);
  gl->setSpacing(KDialog::spacingHint());
  QRadioButton* radioPipe = new QRadioButton(i18n("Pipe"), gbox);
  gl->addWidget(radioPipe, 0, 0);
  QRadioButton* radioTCP = new QRadioButton(i18n("TCP/IP"), gbox);
  gl->addWidget(radioTCP, 1, 0);

  KLineEdit* pipeEdit = new KLineEdit(gbox);
  pipeEdit->setText(d->pipe);
  gl->addWidget(pipeEdit, 0, 0, 1, 2);
  pipeEdit->connect(radioPipe, SIGNAL(toggled(bool)), SLOT(setEnabled(bool)));

  KLineEdit* hostEdit = new KLineEdit(gbox);
  hostEdit->setText(d->host);
  gl->addWidget(hostEdit, 1, 1);
  hostEdit->connect(radioTCP, SIGNAL(toggled(bool)), SLOT(setEnabled(bool)));
  KIntSpinBox* portSpin = new KIntSpinBox(gbox);
  portSpin->setMaximum(99999);
  portSpin->setValue(d->port);
  gl->addWidget(portSpin, 1, 2);
  portSpin->connect(radioTCP, SIGNAL(toggled(bool)), SLOT(setEnabled(bool)));

  if(d->pipe.isEmpty()) {
    radioTCP->setChecked(true);
    hostEdit->setEnabled(true);
    portSpin->setEnabled(true);
    pipeEdit->setEnabled(false);
  } else {
    radioPipe->setChecked(true);
    hostEdit->setEnabled(false);
    portSpin->setEnabled(false);
    pipeEdit->setEnabled(true);
  }
  topLayout->addStretch(10);

  if(dlg.exec() != QDialog::Accepted) {
    return false;
  }

  int p = d->port;
  QString h = d->host;
  QString s;
  if(radioTCP->isChecked()) {
    h = hostEdit->text();
    if(h.isEmpty()) {
      h = QLatin1String("localhost");
    }
    p = portSpin->value();
  } else {
    s = pipeEdit->text();
  }
  d->host = h;
  d->port = p;
  d->pipe = s;

  if(!d->host.isEmpty()) {
    d->handler->setHost(d->host.toUtf8());
  }
  d->handler->setPort(d->port);
  if(!d->pipe.isEmpty()) {
    d->handler->setPipe(QFile::encodeName(d->pipe));
  }

  KConfigGroup config(KGlobal::config(), "OpenOffice.org");
  config.writeEntry("Host", d->host);
  config.writeEntry("Port", d->port);
  config.writePathEntry("Pipe", d->pipe);
  return true;
}

bool OpenOffice::hasLibrary() {
  QString path = KLibLoader::findLibrary(QLatin1String("tellico_ooo"));
  if(!path.isEmpty()) myDebug() << "OpenOffice::hasLibrary() - Found OOo plugin: " << path << endl;
  return !path.isEmpty();
}
