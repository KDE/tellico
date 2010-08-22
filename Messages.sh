#!/bin/sh
# extract rc, ui and kcfg files to rc.cpp
$EXTRACTRC `find . -name '*.rc' -o -name '*.ui' -o -name '*.kcfg' |
            grep -v src/tests/tellicotestconfig.rc` >> rc.cpp
# extract tips
./preparetips tellico.tips > tips.cpp
# extract other translation files
./prepare_i18n_xslt > xslt.cpp
./prepare_desktop src/fetch/z3950-servers.cfg > z3950.cpp
./prepare_desktop src/fetch/scripts/*.spec > scripts.cpp
# extract messages
$XGETTEXT `find . -name '*.cpp' -o -name '*.h' -o -name '*.c'` -o $podir/tellico.pot
# cleanup
rm -f rc.cpp
rm -f tips.cpp
rm -f xslt.cpp
rm -f z3950.cpp
rm -f scripts.cpp
