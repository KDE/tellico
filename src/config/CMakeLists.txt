########### next target ###############

set(config_STAT_SRCS
    tellico_config_addons.cpp
)

set(config_KCFG_SRCS
    tellico_config.kcfgc
)

kconfig_add_kcfg_files(config_STAT_SRCS ${config_KCFG_SRCS})

add_custom_target(tellico_config DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/tellico_config.h)

add_library(config STATIC ${config_STAT_SRCS})

target_link_libraries(config
    Qt${QT_MAJOR_VERSION}::Core
    KF${KF_MAJOR_VERSION}::I18n
    KF${KF_MAJOR_VERSION}::ConfigCore
    KF${KF_MAJOR_VERSION}::ConfigGui
    KF${KF_MAJOR_VERSION}::ConfigWidgets
)

########### install files ###############

install(FILES tellico_config.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
install(FILES tellico-rename.upd tellico.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})

install(PROGRAMS tellico-1-3-update.pl tellico-3-4-update.pl DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})
