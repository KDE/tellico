/***************************************************************************
    Copyright (C) 2021 Robby Stephenson <robby@periapsis.org>
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

#include "mergeconflictresolver.h"
#include "../collectionfactory.h"
#include "../tellico_debug.h"

using namespace Tellico;

bool Merge::mergeEntry(Data::EntryPtr e1, Data::EntryPtr e2, Merge::ConflictResolver* resolver_) {
  if(!e1 || !e2) {
    myDebug() << "bad entry pointer";
    return false;
  }
  bool ret = true;
  foreach(Data::FieldPtr field, e1->collection()->fields()) {
    if(e2->field(field).isEmpty()) {
      continue;
    }
    // never try to merge entry id, creation date or mod date. Those are unique to each entry
    if(field->name() == QLatin1String("id") ||
       field->name() == QLatin1String("cdate") ||
       field->name() == QLatin1String("mdate")) {
      continue;
    }
//    myLog() << "reading field: " << field->name();
    if(e1->field(field) == e2->field(field)) {
      continue;
    } else if(e1->field(field).isEmpty()) {
//      myLog() << e1->title() << ": updating field(" << field->name() << ") to " << e2->field(field);
      e1->setField(field, e2->field(field));
      ret = true;
    } else if(field->type() == Data::Field::Table) {
      // if field F is a table-type field (album tracks, files, etc.), merge rows (keep their position)
      // if e1's F val in [row i, column j] empty, replace with e2's val at same position
      // if different (non-empty) vals at same position, CONFLICT!
      QStringList vals1 = FieldFormat::splitTable(e1->field(field));
      QStringList vals2 = FieldFormat::splitTable(e2->field(field));
      while(vals1.count() < vals2.count()) {
        vals1 += QString();
      }
      for(int i = 0; i < vals2.count(); ++i) {
        if(vals2[i].isEmpty()) {
          continue;
        }
        if(vals1[i].isEmpty()) {
          vals1[i] = vals2[i];
          ret = true;
        } else {
          QStringList parts1 = FieldFormat::splitRow(vals1[i]);
          QStringList parts2 = FieldFormat::splitRow(vals2[i]);
          bool changedPart = false;
          while(parts1.count() < parts2.count()) {
            parts1 += QString();
          }
          for(int j = 0; j < parts2.count(); ++j) {
            if(parts2[j].isEmpty()) {
              continue;
            }
            if(parts1[j].isEmpty()) {
              parts1[j] = parts2[j];
              changedPart = true;
            } else if(resolver_ && parts1[j] != parts2[j]) {
              const int resolverResponse = resolver_->resolve(e1, e2, field, parts1[j], parts2[j]);
              if(resolverResponse == Merge::ConflictResolver::CancelMerge) {
                return false; // cancel all the merge right now
              } else if(resolverResponse == Merge::ConflictResolver::KeepSecond) {
                parts1[j] = parts2[j];
                changedPart = true;
              }
            }
          }
          if(changedPart) {
            vals1[i] = parts1.join(FieldFormat::columnDelimiterString());
            ret = true;
          }
        }
      }
      if(ret) {
        e1->setField(field, vals1.join(FieldFormat::rowDelimiterString()));
      }
// remove the merging due to user comments
// maybe in the future have a more intelligent way
#if 0
    } else if(field->hasFlag(Data::Field::AllowMultiple)) {
      // if field F allows multiple values and not a Table (see above case),
      // e1's F values = (e1's F values) U (e2's F values) (union)
      // replace e1's field with union of e1's and e2's values for this field
      QStringList items1 = e1->fields(field, false);
      QStringList items2 = e2->fields(field, false);
      foreach(const QString& item2, items2) {
        // possible to have one value formatted and the other one not...
        if(!items1.contains(item2) && !items1.contains(Field::format(item2, field->formatType()))) {
          items1.append(item2);
        }
      }
// not sure if I think it should be sorted or not
//      items1.sort();
      e1->setField(field, items1.join(FieldFormat::delimiterString()));
      ret = true;
#endif
    } else if(resolver_) {
      const int resolverResponse = resolver_->resolve(e1, e2, field);
      if(resolverResponse == Merge::ConflictResolver::CancelMerge) {
        return false; // cancel all the merge right now
      } else if(resolverResponse == Merge::ConflictResolver::KeepSecond) {
        e1->setField(field, e2->field(field));
      }
    } else {
//      myDebug() << "Keeping value of" << field->name() << "for" << e1->field(QStringLiteral("title"));
    }
  }
  return ret;
}

QPair<Tellico::Data::FieldList, Tellico::Data::FieldList> Merge::mergeFields(Data::CollPtr coll_,
                                                                                Data::FieldList fields_,
                                                                                Data::EntryList entries_) {
  Data::FieldList modified, created;
  foreach(Data::FieldPtr field, fields_) {
    // don't add a field if it's a default field and not in the current collection
    if(coll_->hasField(field->name()) || CollectionFactory::isDefaultField(coll_->type(), field->name())) {
      // special case for choice fields, since we might want to add a value
      if(field->type() == Data::Field::Choice && coll_->hasField(field->name())) {
        // a2 are the existing fields in the collection, keep them in the same order
        QStringList a1 = coll_->fieldByName(field->name())->allowed();
        foreach(const QString& newAllowedValue, field->allowed()) {
          if(!a1.contains(newAllowedValue)) {
            // could be slow for large merges, but we do only want to add new value
            // IF that value is actually used by an entry
            foreach(Data::EntryPtr entry, entries_) {
              if(entry->field(field) == newAllowedValue) {
                a1 += newAllowedValue;
                break;
              }
            }
          }
        }
        if(a1.count() != coll_->fieldByName(field->name())->allowed().count()) {
          Data::FieldPtr f(new Data::Field(*coll_->fieldByName(field->name())));
          f->setAllowed(a1);
          modified.append(f);
        }
      }
      continue;
    }
    // add field if any values are not empty
    foreach(Data::EntryPtr entry, entries_) {
      if(!entry->field(field).isEmpty()) {
        created.append(Data::FieldPtr(new Data::Field(*field)));
        break;
      }
    }
  }
  return qMakePair(modified, created);
}
