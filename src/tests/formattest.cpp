#ifdef QT_NO_CAST_ASCII
#undef QT_NO_CAST_ASCII
#endif

#include "../field.h"

#include <kdebug.h>

bool checkTitle(QString a, QString b) {
  QString a2 = Tellico::Data::Field::formatTitle(a);
  if(a2 == b) {
    kdDebug() << "Title: checking '" << a2 << "' against expected '" << b << "'... " << "ok" << endl;
  } else {
    kdDebug() << "Title: checking '" << a2 << "' against expected '" << b << "'... " << "KO!" << endl;
    exit(1);
  }
  return true;
}

bool checkName(QString a, QString b, bool multiple=true) {
  QString a2 = Tellico::Data::Field::formatName(a, multiple);
  if(a2 == b) {
    kdDebug() << "Name: checking '" << a2 << "' against expected '" << b << "'... " << "ok" << endl;
  } else {
    kdDebug() << "Name: checking '" << a2 << "' against expected '" << b << "'... " << "KO!" << endl;
    exit(1);
  }
  return true;
}

int main(int, char**) {
  kdDebug() << "\n*****************************************************" << endl;

  // title checks
  Tellico::Data::Field::setArticleList(Tellico::Data::Field::defaultArticleList());
  checkTitle("Title", "Title");
  checkTitle("title", "Title");
  checkTitle("the title", "Title, The");
  checkTitle("the return of the king", "Return of the King, The");

  // name checks
  Tellico::Data::Field::setSuffixList(Tellico::Data::Field::defaultSuffixList());
  QStringList prefixes = Tellico::Data::Field::defaultSurnamePrefixList();
  prefixes += QString::fromLatin1("l'");
  Tellico::Data::Field::setSurnamePrefixList(prefixes);
  checkName("Name", "Name");
  checkName("First Name", "Name, First");
  checkName("First Name", "Name, First", false);
  checkName("tom swift, jr.", "Swift, Jr., Tom");
  checkName("swift, jr., tom", "Swift, Jr., Tom");
  checkName("tom de swift, jr.", "de Swift, Jr., Tom");
  checkName("tom van der swift, jr.", "van der Swift, Jr., Tom");

  kdDebug() << "\n Formatting Test OK !" << endl;
  kdDebug() << "\n*****************************************************" << endl;
}
