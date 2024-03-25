########### next target ###############

SET(gui_STAT_SRCS
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

SET(gui_UI
  collectiontemplatedialog.ui
)
qt_wrap_ui(gui_STAT_SRCS ${gui_UI})

add_library(gui STATIC ${gui_STAT_SRCS})

TARGET_LINK_LIBRARIES(gui
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
IF(QT_MAJOR_VERSION STREQUAL "6")
  TARGET_LINK_LIBRARIES(gui KF${KF_MAJOR_VERSION}::IconWidgets)
ENDIF()

IF(USE_KHTML)
  TARGET_LINK_LIBRARIES(gui KF${KF_MAJOR_VERSION}::KHtml)
  ADD_DEFINITIONS(-DUSE_KHTML=${USE_KHTML})
ELSE()
  TARGET_LINK_LIBRARIES(gui Qt${QT_MAJOR_VERSION}::WebEngineWidgets)
ENDIF()

IF( ${KSANE_TARGET_NAME}_FOUND )
  TARGET_LINK_LIBRARIES(gui ${KSANE_TARGET_NAME})
ENDIF( ${KSANE_TARGET_NAME}_FOUND )
