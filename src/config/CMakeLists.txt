########### next target ###############

SET(config_STAT_SRCS
   tellico_config_addons.cpp
   )

SET(config_KCFG_SRCS
   tellico_config.kcfgc
   )

kconfig_add_kcfg_files(config_STAT_SRCS ${config_KCFG_SRCS})

ADD_CUSTOM_TARGET(tellico_config DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/tellico_config.h)

add_library(config STATIC ${config_STAT_SRCS})

TARGET_LINK_LIBRARIES(config
    Qt5::Core
    KF5::I18n
    KF5::ConfigCore
    KF5::ConfigGui
    KF5::ConfigWidgets
    )

########### install files ###############

INSTALL(FILES   tellico_config.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR} )
INSTALL(FILES   tellico-rename.upd tellico.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR} )

INSTALL(PROGRAMS   tellico-1-3-update.pl tellico-3-4-update.pl DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR} )
