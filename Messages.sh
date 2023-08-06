#!/bin/sh
# extract rc, ui and kcfg files to rc.cpp
$EXTRACTRC `find . -name '*.rc' -o -name '*.ui' -o -name '*.kcfg'` >> rc.cpp
# extract other translation files
./prepare_i18n_xslt >> rc.cpp
# extract messages
$XGETTEXT `find . -name '*.cpp' -o -name '*.h' -o -name '*.c'` -o $podir/tellico.pot
# cleanup
rm -f rc.cpp
rm -f tips.cpp
