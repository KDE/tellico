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
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::WebEngineWidgets
    KF6::KIOCore
    KF6::KIOFileWidgets
    KF6::SonnetCore
    KF6::SonnetUi
    KF6::XmlGui
    KF6::I18n
    KF6::TextWidgets
    KF6::IconThemes
    KF6::IconWidgets
)

if(KSaneWidgets6_FOUND)
    target_link_libraries(gui KSaneWidgets6)
endif()
