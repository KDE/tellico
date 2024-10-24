########### next target ###############

set(gui_STAT_SRCS
    boolfieldwidget.cpp
    checkablecombobox.cpp
    choicefieldwidget.cpp
    collectiontemplatedialog.cpp
    collectiontypecombo.cpp
    combobox.cpp
    countdelegate.cpp
    datefieldwidget.cpp
    datewidget.cpp
    detailedentryitemdelegate.cpp
    dockwidget.cpp
    drophandler.cpp
    imagefieldwidget.cpp
    imagewidget.cpp
    fieldwidget.cpp
    fieldwidgetfactory.cpp
    filterrulewidget.cpp
    filterrulewidgetlister.cpp
    kwidgetlister.cpp
    lineedit.cpp
    linefieldwidget.cpp
    listwidgetitem.cpp
    numberfieldwidget.cpp
    parafieldwidget.cpp
    previewdialog.cpp
    progress.cpp
    ratingdelegate.cpp
    ratingfieldwidget.cpp
    ratingwidget.cpp
    spinbox.cpp
    starrating.cpp
    statusbar.cpp
    stringmapdialog.cpp
    stringmapwidget.cpp
    tablefieldwidget.cpp
    tabwidget.cpp
    treeview.cpp
    urlfieldwidget.cpp
)

set(gui_UI
    collectiontemplatedialog.ui
)
qt_wrap_ui(gui_STAT_SRCS ${gui_UI})

add_library(gui STATIC ${gui_STAT_SRCS})

target_link_libraries(gui
    Qt${QT_MAJOR_VERSION}::Core
    Qt${QT_MAJOR_VERSION}::Widgets
    Qt${QT_MAJOR_VERSION}::Gui
    KF${KF_MAJOR_VERSION}::KIOCore
    KF${KF_MAJOR_VERSION}::KIOFileWidgets
    KF${KF_MAJOR_VERSION}::SonnetCore
    KF${KF_MAJOR_VERSION}::SonnetUi
    KF${KF_MAJOR_VERSION}::XmlGui
    KF${KF_MAJOR_VERSION}::I18n
    KF${KF_MAJOR_VERSION}::TextWidgets
    KF${KF_MAJOR_VERSION}::IconThemes
)
if(QT_MAJOR_VERSION STREQUAL "6")
    target_link_libraries(gui KF${KF_MAJOR_VERSION}::IconWidgets)
endif()

if(USE_KHTML)
    target_link_libraries(gui KF${KF_MAJOR_VERSION}::KHtml)
    add_definitions(-DUSE_KHTML=${USE_KHTML})
else()
    target_link_libraries(gui Qt${QT_MAJOR_VERSION}::WebEngineWidgets)
endif()

if(${KSANE_TARGET_NAME}_FOUND)
    target_link_libraries(gui ${KSANE_TARGET_NAME})
endif()
