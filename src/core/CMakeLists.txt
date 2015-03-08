########### next target ###############

SET(core_STAT_SRCS
   dbusinterface.cpp
   drophandler.cpp
   filehandler.cpp
   netaccess.cpp
   tellico_config_addons.cpp
   tellico_strings.cpp
   )

SET(core_KCFG_SRCS
   tellico_config.kcfgc
   )

kconfig_add_kcfg_files(core_STAT_SRCS ${core_KCFG_SRCS})

ADD_CUSTOM_TARGET(tellico_config DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/tellico_config.h)

add_library(core STATIC ${core_STAT_SRCS})

TARGET_LINK_LIBRARIES(core
    utils
    )

########### install files ###############

INSTALL(FILES   tellico_config.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR} )
INSTALL(FILES   tellico-rename.upd tellico.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR} )

INSTALL(PROGRAMS   tellico-1-3-update.pl DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR} )
