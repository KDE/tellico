#ifdef QT_NO_CAST_ASCII
#undef QT_NO_CAST_ASCII
#endif

#include "tellico_utils.h"
#include <kdebug.h>
#include <assert.h>

int main(int, char**) {
  kdDebug() << "\n*****************************************************" << endl;

  assert(Tellico::decodeHTML("robby") == "robby");
  assert(Tellico::decodeHTML("&fake;") == "&fake;");
  assert(Tellico::decodeHTML("&#48;") == "0");
  assert(Tellico::decodeHTML("robby&#48;robby") == "robby0robby");

  kdDebug() << "\ndecodeHTML Test OK !" << endl;
  kdDebug() << "\n*****************************************************" << endl;
}
