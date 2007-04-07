AM_CPPFLAGS = $(all_includes)

noinst_LIBRARIES = libcore.a
libcore_a_SOURCES = tellico_config.kcfgc tellico_config.cpp \
	tellico_config_addons.cpp dcopinterface.skel \
	dcopinterface.cpp netaccess.cpp

libcore_a_METASOURCES = AUTO

KDE_OPTIONS = noautodist

EXTRA_DIST = \
tellico_config.kcfg tellico_config.kcfgc \
tellico_config_addons.h tellico_config_addons.cpp \
dcopinterface.h dcopinterface.cpp \
netaccess.h netaccess.cpp

kde_kcfg_DATA = tellico_config.kcfg

CLEANFILES = *~ *.loT tellico_config.h tellico_config.cpp
