#!/bin/sh
# Run this to generate all the initial makefiles, etc.
# 
# Modified from autogen.sh script from Eterm, www.eterm.org

DIE=0

echo "Generating configuration files for bookcase, please wait...."

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
	echo
        echo "You must have autoconf installed to compile bookcase."
        echo "Download the appropriate package for your distribution,"
        echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
        DIE=1
}

(automake --version) < /dev/null > /dev/null 2>&1 || {
        echo
        echo "You must have automake installed to compile bookcase."
        echo "Download the appropriate package for your distribution,"
        echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
        DIE=1
}

if test "$DIE" -eq 1; then
        exit 1
fi

if [ -f Makefile.dist ]; then
  make -f Makefile.dist
fi

if [ -f configure ] ; then
  ./configure "$@"
fi

if [ -f cvs.motd ]; then
  echo "ATTENTION CVS Users!"
  echo ""
  cat cvs.motd
  echo ""
fi
