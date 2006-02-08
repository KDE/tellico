INCLUDES = $(all_includes)
METASOURCES = AUTO
KDE_OPTIONS = noautodist
CLEANFILES = *~ *.loT
noinst_LIBRARIES = libcite.a
libcite_a_SOURCES = lyxpipe.cpp actionmanager.cpp clipboard.cpp openoffice.cpp

EXTRA_DIST = \
actionmanager.h actionmanager.cpp \
lyxpipe.h lyxpipe.cpp \
actionmanager.h actionmanager.cpp \
openoffice.h openoffice.cpp \
clipboard.h clipboard.cpp \
handler.h

OOO_SUBDIR = ooo

if WITH_OOO
SUBDIRS = $(OOO_SUBDIR)
endif
