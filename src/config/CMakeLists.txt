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
    Qt6::Core
    KF6::I18n
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::ConfigWidgets
    KF6::ColorScheme
)

########### install files ###############

install(FILES tellico_config.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
install(FILES tellico.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})

install(PROGRAMS tellico-3-4-update.pl DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})
