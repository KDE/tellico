#ifdef QT_NO_CAST_ASCII
#undef QT_NO_CAST_ASCII
#endif

#include "latin1literal.h"
#include <qstring.h>
#include <kdebug.h>
#include <assert.h>

int main(int, char**) {
  kdDebug() << "\n*****************************************************\n" << endl;

  assert(QString::null == Latin1Literal(0));
  assert(QString::null != Latin1Literal(""));
  assert(QString::fromLatin1("") == Latin1Literal(""));
  assert(QString::fromLatin1("") != Latin1Literal(0));
  assert(QString::fromLatin1("x") != Latin1Literal(""));
  assert(QString::fromLatin1("a") == Latin1Literal("a"));
  assert(QString::fromLatin1("a") != Latin1Literal("b"));
  assert(QString::fromLatin1("\xe4") == Latin1Literal("\xe4"));
  assert(QString::fromUtf8("\xe2\x82\xac") != Latin1Literal("?"));

  kdDebug() << "\nLatin1Literal Test OK !" << endl;
  kdDebug() << "\n*****************************************************" << endl;
}
