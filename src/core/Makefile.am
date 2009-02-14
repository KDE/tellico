AM_CPPFLAGS = $(all_includes)

noinst_LIBRARIES = libcore.a
libcore_a_SOURCES = dcopinterface.cpp dcopinterface.skel drophandler.cpp \
	netaccess.cpp tellico_config.cpp tellico_config.kcfgc tellico_config_addons.cpp

libcore_a_METASOURCES = AUTO

KDE_OPTIONS = noautodist

EXTRA_DIST = \
tellico_config.kcfg tellico_config.kcfgc \
tellico_config_addons.h tellico_config_addons.cpp \
dcopinterface.h dcopinterface.cpp \
netaccess.h netaccess.cpp \
drophandler.h drophandler.cpp \
tellico-rename.upd tellico.upd \
tellico-1-3-update.pl

dcopinterface_DCOPIDLNG = ALL

kde_kcfg_DATA = tellico_config.kcfg

updatedir = $(kde_datadir)/kconf_update
update_DATA = tellico-rename.upd tellico.upd
update_SCRIPTS = tellico-1-3-update.pl

CLEANFILES = *~ *.loT tellico_config.h tellico_config.cpp

