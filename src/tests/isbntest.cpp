#ifdef QT_NO_CAST_ASCII
#undef QT_NO_CAST_ASCII
#endif

#include "isbnvalidator.h"

#include <kdebug.h>

bool check(QString a, QString b) {
  static const Tellico::ISBNValidator val(0);
  val.fixup(a);
  if(a == b) {
    kdDebug() << "checking '" << a << "' against expected value '" << b << "'... " << "ok" << endl;
  } else {
    kdDebug() << "checking '" << a << "' against expected value '" << b << "'... " << "KO!" << endl;
    exit(1);
  }
  return true;
}

int main(int, char**) {
  kdDebug() << "\n*****************************************************" << endl;

  // initial checks
  check("0-446-60098-9", "0-446-60098-9");
  // check sum value
  check("0-446-60098", "0-446-60098-9");

  // check EAN stripping
  check("9780672323416", "0-672-32341-9");

  // normal english-language hyphenation
  check("0446600989", "0-446-60098-9");
  check("044660098", "0-446-60098-9");

  // check french hyphenation
  check("2862744867", "2-86274-486-7");

  // check keeping middle hyphens
  check("80-8611913-0", "80-8611913-0");
  check("80-86119-13-0", "80-86119-13-0");
  check("80-8611-9-13-0", "80-8611-913-0");

  // check garbage
  check("My name is robby", "");
  check("http://www.abclinuxu.cz/clanky/show/63080", "6-3080");

  kdDebug() << "\n ISBN Validator Test OK !" << endl;
  kdDebug() << "\n*****************************************************" << endl;
}
