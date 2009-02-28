########### next target ###############

SET(core_STAT_SRCS
   dbusinterface.cpp
   drophandler.cpp
   filehandler.cpp
   netaccess.cpp
   tellico_config_addons.cpp
   )

SET(core_KCFG_SRCS
   tellico_config.kcfgc
   )

KDE4_ADD_KCFG_FILES(core_STAT_SRCS ${core_KCFG_SRCS})

KDE4_ADD_LIBRARY(core STATIC ${core_STAT_SRCS})

########### install files ###############

INSTALL(FILES   tellico_config.kcfg DESTINATION ${KCFG_INSTALL_DIR} )
INSTALL(FILES   tellico-rename.upd tellico.upd DESTINATION ${KCONF_UPDATE_INSTALL_DIR} )

INSTALL(PROGRAMS   tellico-1-3-update.pl DESTINATION ${KCONF_UPDATE_INSTALL_DIR} )
